/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

// #define LEDC_CHANNEL     LEDC_CHANNEL_0
/// #define LEDC_TIMER       LEDC_TIMER_0
// #define LEDC_OUTPUT_IO   2 // Pin de salida PWM
// #define LEDC_FREQUENCY   5000 // frecuencia base

/*
extern TaskHandle_t astable_task_handle;
extern void astable_task(void *param);
*/

// GPIO de salida para el LED (NE555 simulado)
#define LED_GPIO GPIO_NUM_2

// Declaración para poder detener la tarea astable si se reinicia el modo
TaskHandle_t astable_task_handle = NULL;

void astable_task(void *param)
{
    double tiempo_alto = ((double *)param)[0];
    double tiempo_bajo = ((double *)param)[1];
    free(param); // liberar memoria pasada

    while (1)
    {
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(tiempo_alto * 1000));

        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(tiempo_bajo * 1000));
    }
}

static const char *REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context
{
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html"))
    {
        type = "text/html";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".js"))
    {
        type = "application/javascript";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".css"))
    {
        type = "text/css";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".png"))
    {
        type = "image/png";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".ico"))
    {
        type = "image/x-icon";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".svg"))
    {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/')
    {
        strlcat(filepath, "/index.html", sizeof(filepath));
    }
    else
    {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1)
    {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do
    {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1)
        {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        }
        else if (read_bytes > 0)
        {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK)
            {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Simple handler for light brightness control */
static esp_err_t light_brightness_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int red = cJSON_GetObjectItem(root, "red")->valueint;
    int green = cJSON_GetObjectItem(root, "green")->valueint;
    int blue = cJSON_GetObjectItem(root, "blue")->valueint;
    ESP_LOGI(REST_TAG, "Light control: red = %d, green = %d, blue = %d", red, green, blue);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

/* Simple handler for getting temperature data */
static esp_err_t temperature_data_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "raw", esp_random() % 20);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t control_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    char *modo = cJSON_GetObjectItem(root, "modo")->valuestring;
    int r1 = cJSON_GetObjectItem(root, "r1")->valueint;
    int r2 = cJSON_GetObjectItem(root, "r2")->valueint;
    double c = cJSON_GetObjectItem(root, "c")->valuedouble;
    char *unidadCapacitancia = cJSON_GetObjectItem(root, "unidadCapacitancia")->valuestring;
    char *accion = cJSON_GetObjectItem(root, "accion")->valuestring;
    ESP_LOGI(REST_TAG, "Control: modo = %s, r1 = %d, r2 = %d, c = %.2f, unidadCapacitancia = %s, accion = %s", modo, r1, r2, c, unidadCapacitancia, accion);

    /*

        // Validaciones básicas de los valores ingresados
        if (r1 <= 0 || (strcmp(modo, "astable") == 0 && r2 <= 0))
        {
            ESP_LOGE(REST_TAG, "Resistencias inválidas: R1 = %d, R2 = %d", r1, r2);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Resistencias deben ser mayores que cero.");
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        if (c <= 0.0)
        {
            ESP_LOGE(REST_TAG, "Capacitancia inválida: C = %.6f", c);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "La capacitancia debe ser mayor que cero.");
            cJSON_Delete(root);
            return ESP_FAIL;
        }


    */

    // MAGIA DE WILME AQUI

    // Convertir el valor de capacitancia según la unidad enviada

    
    if (strcmp(unidadCapacitancia, "1e-6") == 0)
    { // µF
        c *= 1e-6;
    }
    else if (strcmp(unidadCapacitancia, "1e-9") == 0)
    { // nF
        c *= 1e-9;
    }
    else if (strcmp(unidadCapacitancia, "1e-12") == 0)
    { // pF
        c *= 1e-12;
    }
    

    double frecuencia = 0.0;
    double tiempo_alto = 0.0;
    double tiempo_bajo = 0.0;
    double periodo = 0.0;
    double pulso = 0.0;

    if (strcmp(modo, "astable") == 0)
    {
        // Fórmulas para modo Astable (en segundos)
        // T_high = 0.693 * (R1 + R2) * C
        // T_low  = 0.693 * R2 * C
        // T_total = T_high + T_low
        // f = 1 / T_total

        tiempo_alto = 0.693 * (r1 + r2) * c;
        tiempo_bajo = 0.693 * r2 * c;
        periodo = tiempo_alto + tiempo_bajo;
        frecuencia = 1.0 / periodo;

        ESP_LOGI(REST_TAG, "Modo Astable -> T_alto: %.6fs, T_bajo: %.6fs, Periodo: %.6fs, Frecuencia: %.2fHz",
                 tiempo_alto, tiempo_bajo, periodo, frecuencia);
    }
    else if (strcmp(modo, "monostable") == 0)
    {
        // Fórmulas para modo Monostable (en segundos)
        // Tiempo de pulso: T = 1.1 * R1 * C
        pulso = 1.1 * r1 * c;

        ESP_LOGI(REST_TAG, "Modo Monostable -> Tiempo de pulso: %.6fs", pulso);
    }
    else
    {
        ESP_LOGW(REST_TAG, "Modo desconocido recibido: %s", modo);
    }

    // Inicializar el GPIO 2 si no se ha hecho antes
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    // Detener la tarea anterior si estaba corriendo
    if (astable_task_handle != NULL)
    {
        vTaskDelete(astable_task_handle);
        astable_task_handle = NULL;
    }

    if (strcmp(modo, "astable") == 0)
    {
        // Crear parámetros para la tarea
        double *params = malloc(2 * sizeof(double));
        params[0] = tiempo_alto;
        params[1] = tiempo_bajo;

        // Crear tarea astable
        xTaskCreate(astable_task, "astable_task", 2048, params, 5, &astable_task_handle);
    }
    else if (strcmp(modo, "monostable") == 0)
    {
        // Simular un solo pulso en GPIO 2
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(pulso * 1000));
        gpio_set_level(LED_GPIO, 0);
    }

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    /* URI handler for fetching system info */
    httpd_uri_t system_info_get_uri = {
        .uri = "/api/v1/system/info",
        .method = HTTP_GET,
        .handler = system_info_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &system_info_get_uri);

    /* URI handler for fetching temperature data */
    httpd_uri_t temperature_data_get_uri = {
        .uri = "/api/v1/temp/raw",
        .method = HTTP_GET,
        .handler = temperature_data_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &temperature_data_get_uri);

    /* URI handler for light brightness control */
    httpd_uri_t light_brightness_post_uri = {
        .uri = "/api/v1/light/brightness",
        .method = HTTP_POST,
        .handler = light_brightness_post_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &light_brightness_post_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &common_get_uri);

    // MIS RUTAS

    /* URI handler for light brightness control */
    httpd_uri_t control_post_uri = {
        .uri = "/api/v1/control",
        .method = HTTP_POST,
        .handler = control_post_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &control_post_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
