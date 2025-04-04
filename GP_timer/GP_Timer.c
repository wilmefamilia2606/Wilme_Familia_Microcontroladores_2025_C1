// TAREA 5
// TAREA 5
// TAREA 5
// TAREA 5

// Librerías utilizadas
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "driver/gpio_etm.h"
#include "esp_Log.h"

// Etiquetas creadas para informar al usuario de ciertos eventos a través de la terminal
static const char *TAG = "example";

// Macros a utilizar para facilitar el entendimiento del código
#define HC_SR84_TRIG_DGPIO 0
#define HC_SR84_ECHO_DGPIO 1

#define EXAMPLE_GPTIMER_RESOLUTION_HZ 1000000 // 1MHz, 1tick = 1us

void app_main(void)
{
    ESP_LOGI(TAG, "configure trig gpio");
    gpio_config_t trig_io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << HC_SR84_TRIG_DGPIO, 
    };
    ESP_ERROR_CHECK(gpio_config(&trig_io_conf));
    // driver low by defalut
    ESP_ERROR_CHECK(gpio_set_level(HC_SR84_TRIG_DGPIO, 0));
    
    ESP_LOGI(TAG, "configure echo gpio");
    gpio_config_t echo_io_conf = {
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_ANYEDGE, // capture signal on both edge
        .pull_up_en = true,  // pull up internally 
        .pin_bit_mask = 1ULL << HC_SR84_ECHO_DGPIO, 
    };
    ESP_ERROR_CHECK(gpio_config(&echo_io_conf));

    ESP_LOGI(TAG, "Create etm event handle for echo gpio");
    esp_etm_event_handle_t gpio_event = NULL;
    gpio_etm_event_config_t gpio_event_config = {
        .edge = GPIO_ETM_EVENT_EDGE_ANY, // capture signal on both edge 
    };
    ESP_ERROR_CHECK(gpio_new_etm_event(&gpio_event_config, &gpio_event));
    ESP_ERROR_CHECK(gpio_etm_event_bind_gpio(gpio_event, HC_SR84_ECHO_DGPIO));
    
    ESP_LOGI(TAG, "create gptimer handle");
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Select the default clock source
        .direction = GPTIMER_COUNT_UP,      // Counting direction is up
        .resolution_hz = EXAMPLE_GPTIMER_RESOLUTION_HZ,   // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    ESP_LOGI(TAG, "Get gptimer etm task handle (capture)");
    esp_etm_task_handle_t gptimer_capture_task = NULL;
    gptimer_etm_task_config_t gptimer_etm_task_conf = {
        .task_type = GPTIMER_ETM_TASK_CAPTURE, 
        };
    
    ESP_ERROR_CHECK(gptimer_new_etm_task(gptimer, &gptimer_etm_task_conf, &gptimer_capture_task));

    ESP_LOGI(TAG, "Create ETM channel");
    esp_etm_channel_handle_t etm_chan = NULL;
    esp_etm_channel_config_t etm_chan_config = {};
    ESP_ERROR_CHECK(esp_etm_new_channel(&etm_chan_config, &etm_chan));

    


    




}
