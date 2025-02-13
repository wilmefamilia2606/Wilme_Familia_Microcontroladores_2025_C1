#include <stdio.h>
#include <stdlib.h>

// Definición de estados del sistema
enum ESTADO {
    ESTADO_INICIAL,
    ESTADO_ERROR,
    ESTADO_ABRIENDO,
    ESTADO_CERRANDO,
    ESTADO_ABIERTA,
    ESTADO_CERRADA,
    ESTADO_DETENIDA,
    ESTADO_DESCONOCIDO
};

// Definición de señales de sensores y actuadores
#define LM_ACTIVO 1
#define LM_INACTIVO 0
#define PP_ACTIVO 1
#define PP_INACTIVO 0
#define FTC_ACTIVO 1
#define FTC_INACTIVO 0
#define KEY_ACTIVO 1
#define KEY_INACTIVO 0
#define MOTOR_ON 1
#define MOTOR_OFF 0
#define LAMP_ON 1
#define LAMP_OFF 0
#define BUZZER_ON 1
#define BUZZER_OFF 0

// Definición de configuraciones para estados desconocidos y detenidos
#define FDESCONOCIDO_CIERRA 0
#define FDESCONOCIDO_ABRIR 1
#define FDESCONOCIDO_ESPERA 2
#define FDESCONOCIDO_PARPADEAR 3
#define FDETENIDA_SEGUIR 0
#define FDETENIDA_CONTRARIO 1

// Variables globales de estado
int ESTADO_SIGUIENTE = ESTADO_INICIAL;
int ESTADO_ANTERIOR = ESTADO_INICIAL;
int ESTADO_ACTUAL = ESTADO_INICIAL;

// Estructura para manejar entradas y salidas del sistema
struct SYSTEM_IO {
    unsigned int lsc:1; // Sensor de límite puerta cerrada
    unsigned int lsa:1; // Sensor de límite puerta abierta
    unsigned int ftc:1; // Fotocelda de seguridad
    unsigned int ma:1;  // Motor abrir
    unsigned int mc:1;  // Motor cerrar
    unsigned int lamp:1;// Luz indicadora
    unsigned int buzzer:1;
    unsigned int keya:1;// Llave de apertura
    unsigned int keyc:1;// Llave de cierre
    unsigned int pp:1;  // Botón de apertura manual
    unsigned int dpsw_DESCONOCIDO:2; // Configuración desde estado desconocido
    unsigned int dpsw_DETENIDA:2;    // Configuración desde estado detenida
} io;

// Estructura de configuración del sistema
struct SYSTEM_CONFIG {
    unsigned int cnt_TCA; // Tiempo de cierre automático
    unsigned int cnt_RT;  // Tiempo máximo de movimiento del motor
    int FDESCONOCIDO;
    int FDETENIDA;
} config;

// Prototipos de funciones de estado
int Func_ESTADO_INICIAL(void);
int Func_ESTADO_ERROR(void);
int Func_ESTADO_ABRIENDO(void);
int Func_ESTADO_CERRANDO(void);
int Func_ESTADO_ABIERTA(void);
int Func_ESTADO_CERRADA(void);
int Func_ESTADO_DETENIDA(void);
int Func_ESTADO_DESCONOCIDO(void);

int main() {
    // Bucle principal de la máquina de estados
    while (1) {
        switch (ESTADO_SIGUIENTE) {
            case ESTADO_INICIAL:
                ESTADO_SIGUIENTE = Func_ESTADO_INICIAL();
                break;
            case ESTADO_ERROR:
                ESTADO_SIGUIENTE = Func_ESTADO_ERROR();
                break;
            case ESTADO_ABRIENDO:
                ESTADO_SIGUIENTE = Func_ESTADO_ABRIENDO();
                break;
            case ESTADO_CERRANDO:
                ESTADO_SIGUIENTE = Func_ESTADO_CERRANDO();
                break;
            case ESTADO_ABIERTA:
                ESTADO_SIGUIENTE = Func_ESTADO_ABIERTA();
                break;
            case ESTADO_CERRADA:
                ESTADO_SIGUIENTE = Func_ESTADO_CERRADA();
                break;
            case ESTADO_DETENIDA:
                ESTADO_SIGUIENTE = Func_ESTADO_DETENIDA();
                break;
            case ESTADO_DESCONOCIDO:
                ESTADO_SIGUIENTE = Func_ESTADO_DESCONOCIDO();
                break;
        }
    }
    return 0;
}

// Implementación de cada función de estado
int Func_ESTADO_INICIAL(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_INICIAL;

    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    io.buzzer = BUZZER_OFF;

    while (1) {
        if (io.lsc == LM_ACTIVO && io.lsa == LM_ACTIVO) return ESTADO_ERROR;
        if (io.lsc == LM_INACTIVO && io.lsa == LM_ACTIVO) return ESTADO_ABIERTA;
        if (io.lsc == LM_ACTIVO && io.lsa == LM_INACTIVO) return ESTADO_CERRADA;
        if (io.lsc == LM_INACTIVO && io.lsa == LM_INACTIVO) return ESTADO_DESCONOCIDO;
    }
}

int Func_ESTADO_ERROR(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ERROR;
    io.buzzer = BUZZER_ON;
    while (1) {
        if (io.lsc == LM_INACTIVO && io.lsa == LM_ACTIVO) return ESTADO_ABIERTA;
        if (io.lsc == LM_ACTIVO && io.lsa == LM_INACTIVO) return ESTADO_CERRADA;
        if (io.lsc == LM_INACTIVO && io.lsa == LM_INACTIVO && io.ftc == FTC_INACTIVO) return ESTADO_CERRANDO;
    }
}

int Func_ESTADO_ABRIENDO(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABRIENDO;
    io.ma = MOTOR_ON;
    while (1) {
        if (io.pp == PP_ACTIVO) return ESTADO_DETENIDA;
        if (io.lsa == LM_ACTIVO) return ESTADO_ABIERTA;
    }
}

int Func_ESTADO_CERRANDO(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_CERRANDO;
    io.mc = MOTOR_ON;
    while (1) {
        if (io.pp == PP_ACTIVO) return ESTADO_DETENIDA;
        if (io.lsc == LM_ACTIVO) return ESTADO_CERRADA;
    }
}

int Func_ESTADO_ABIERTA(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABIERTA;
    while (1) {
        if (io.keyc == KEY_ACTIVO || io.pp == PP_ACTIVO) return ESTADO_CERRANDO;
    }
}

int Func_ESTADO_CERRADA(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_CERRADA;
    while (1) {
        if (io.keya == KEY_ACTIVO || io.pp == PP_ACTIVO) return ESTADO_ABRIENDO;
    }
}

int Func_ESTADO_DETENIDA(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_DETENIDA;
    while (1) {
        if (io.pp == PP_ACTIVO) return (config.FDETENIDA == FDETENIDA_SEGUIR) ? ESTADO_ANTERIOR : (ESTADO_ANTERIOR == ESTADO_ABRIENDO ? ESTADO_CERRANDO : ESTADO_ABRIENDO);
    }
}

int Func_ESTADO_DESCONOCIDO(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_DESCONOCIDO;
    while (1) {
        if (config.FDESCONOCIDO == FDESCONOCIDO_CIERRA) return ESTADO_CERRANDO;
        if (config.FDESCONOCIDO == FDESCONOCIDO_ABRIR) return ESTADO_ABRIENDO;
    }
}
