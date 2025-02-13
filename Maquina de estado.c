#include <stdio.h>
#include <stdlib.h>

// Definición de los posibles estados de la puerta
#define ESTADO_INICIAL 0
#define ESTADO_ERROR 1
#define ESTADO_ABRIENDO 2
#define ESTADO_CERRANDO 3
#define ESTADO_ABIERTA 4
#define ESTADO_CERRADA 5
#define ESTADO_DETENIDA 6
#define ESTADO_DESCONOCIDO 7

// Definición de valores para sensores y actuadores
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

// Definición de opciones para estados desconocidos y detenidos
#define FDESCONOCIDO_CIERRA 0
#define FDESCONOCIDO_ABRIR 1
#define FDESCONOCIDO_ESPERA 2
#define FDESCONOCIDO_PARPADEAR 3
#define FDETENIDA_SEGUIR 0
#define FDETENIDA_CONTRARIO 1

// Prototipos de funciones para manejar los estados de la puerta
int Func_ESTADO_INICIAL(void);
int Func_ESTADO_ERROR(void);
int Func_ESTADO_ABRIENDO(void);
int Func_ESTADO_CERRANDO(void);
int Func_ESTADO_ABIERTA(void);
int Func_ESTADO_CERRADA(void);
int Func_ESTADO_DETENIDA(void);
int Func_ESTADO_DESCONOCIDO(void);

// Variables globales que manejan la transición de estados
int ESTADO_SIGUIENTE = ESTADO_INICIAL;
int ESTADO_ANTERIOR = ESTADO_INICIAL;
int ESTADO_ACTUAL = ESTADO_INICIAL;

// Estructura que almacena los estados de los sensores y actuadores
struct SYSTEM_IO {
    unsigned int lsc:1; // Sensor de puerta cerrada
    unsigned int lsa:1; // Sensor de puerta abierta
    unsigned int ftc:1; // Fotocelda de seguridad
    unsigned int ma:1; // Motor abriendo
    unsigned int mc:1; // Motor cerrando
    unsigned int lamp:1; // Luz indicadora
    unsigned int buzzer:1; // Alarma sonora
    unsigned int keya:1; // Botón de abrir
    unsigned int keyc:1; // Botón de cerrar
    unsigned int pp:1; // Botón de paro de emergencia
    unsigned int dpsw_DESCONOCIDO:2; // Configuración para estado desconocido
    unsigned int dpsw_DETENIDA:2; // Configuración para estado detenido
} io;

// Estructura de configuración del sistema
struct SYSTEM_CONFIG {
    unsigned int cnt_TCA; // Tiempo de cierre automático en segundos
    unsigned int cnt_RT; // Tiempo máximo de movimiento del motor en segundos
    int FDESCONOCIDO; // Configuración del estado desconocido
    int FDETENIDA; // Configuración del estado detenido
} config;

int main() {
    for(;;) {
        // Evaluación de estados y ejecución de la función correspondiente
        if(ESTADO_SIGUIENTE == ESTADO_INICIAL) {
            ESTADO_SIGUIENTE = Func_ESTADO_INICIAL();
        }
        if(ESTADO_SIGUIENTE == ESTADO_ERROR) {
            ESTADO_SIGUIENTE = Func_ESTADO_ERROR();
        }
        if(ESTADO_SIGUIENTE == ESTADO_ABRIENDO) {
            ESTADO_SIGUIENTE = Func_ESTADO_ABRIENDO();
        }
        if(ESTADO_SIGUIENTE == ESTADO_CERRANDO) {
            ESTADO_SIGUIENTE = Func_ESTADO_CERRANDO();
        }
        if(ESTADO_SIGUIENTE == ESTADO_ABIERTA) {
            ESTADO_SIGUIENTE = Func_ESTADO_ABIERTA();
        }
        if(ESTADO_SIGUIENTE == ESTADO_CERRADA) {
            ESTADO_SIGUIENTE = Func_ESTADO_CERRADA();
        }
        if(ESTADO_SIGUIENTE == ESTADO_DETENIDA) {
            ESTADO_SIGUIENTE = Func_ESTADO_DETENIDA();
        }
        if(ESTADO_SIGUIENTE == ESTADO_DESCONOCIDO) {
            ESTADO_SIGUIENTE = Func_ESTADO_DESCONOCIDO();
        }
    }
    return 0;
}

// Función que gestiona el estado inicial del sistema
int Func_ESTADO_INICIAL(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_INICIAL;
    
    // Se apagan todos los actuadores
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    io.buzzer = BUZZER_OFF;
    
    // Determina el siguiente estado en base a los sensores
    for(;;) {
        if(io.lsc == LM_ACTIVO && io.lsa == LM_ACTIVO) {
            return ESTADO_ERROR; // Error si ambos sensores están activos
        }
        if(io.lsc == LM_INACTIVO && io.lsa == LM_ACTIVO) {
            return ESTADO_ABIERTA; // Puerta detectada como abierta
        }
        if(io.lsc == LM_ACTIVO && io.lsa == LM_INACTIVO) {
            return ESTADO_CERRADA; // Puerta detectada como cerrada
        }
        if(io.lsc == LM_INACTIVO && io.lsa == LM_INACTIVO) {
            return ESTADO_DESCONOCIDO; // No se puede determinar el estado
        }
    }
}

// Función que maneja el estado de error
int Func_ESTADO_ERROR(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ERROR;
    
    // Desactivar motores, encender alarma
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    io.buzzer = BUZZER_ON;
    
    // Espera hasta que la condición de error se corrija
    for(;;) {
        if(io.lsc == LM_INACTIVO && io.lsa == LM_ACTIVO) {
            return ESTADO_ABIERTA;
        }
        if(io.lsc == LM_ACTIVO && io.lsa == LM_INACTIVO) {
            return ESTADO_CERRADA;
        }
        if(io.lsc == LM_INACTIVO && io.lsa == LM_INACTIVO && io.ftc == FTC_INACTIVO) {
            return ESTADO_CERRANDO;
        }
    }
}

// Función que maneja el estado de apertura de la puerta
int Func_ESTADO_ABRIENDO(void) {
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABRIENDO;
    
    // Activar motor de apertura
    io.ma = MOTOR_ON;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_ON;
    io.buzzer = BUZZER_OFF;
    
    // Esperar hasta que la puerta se abra o se presione el botón de paro
    for(;;) {
        if(io.pp == PP_ACTIVO) {
            return ESTADO_DETENIDA;
        }
        if(io.lsa == LM_ACTIVO) {
            return ESTADO_ABIERTA;
        }
    }
}

// Funciones adicionales para los otros estados seguirían el mismo esquema...
