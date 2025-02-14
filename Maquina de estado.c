#include <stdio.h>
#include <stdlib.h>

// Definición de los posibles estados de la puerta
#define ESTADO_INICIAL 0
#define ESTADO_ERROR 1
#define ESTADO_ABRIENDO 2
#define ESTADO_CERRANDO 3
#define ESTADO_ABIERTO 4
#define ESTADO_CERRADO 5
#define ESTADO_DETENIDA 6
#define ESTADO_DESCONOCIDO 7

// Definición de valores para sensores y actuadores
#define LM_ACTIVO 1
#define LM_NOACTIVO 0
#define PP_ACTIVO 1
#define PP_INACTIVO 0
#define FTC_ACTIVO 1
#define FTC_INACTIVO 0
#define KEY_ACTIVO 1
#define KEY_NOACTIVO 0
#define MOTOR_ON 1
#define MOTOR_OFF 0
#define LAMP_ON 1
#define LAMP_OFF 0
#define BUZZER_ON 1
#define BUZZER_OFF 0
#define TIME_CA 120

// Definición de opciones para estados desconocidos y detenidos
#define DESCONOCIDO_ABRIERTO 1
#define DESCONOCIDO_CERRADO 0
#define DESCONOCIDO_PARPADEAR 3
#define DESCONOCIDO_ESPERA 2

// definicion de estados.
int ESTADO_SIGUIENTE = ESTADO_INICIAL;
int ESTADO_ANTERIOR = ESTADO_INICIAL;
int ESTADO_ACTUAL = ESTADO_INICIAL;

/*dip swich_fd (dpsw_fd)
1-1 de desconocido a cerrar.
1-0 de desconocido a esperar comando.
0-1 de desconocido a abrir.
0-0 de deconocido a parpadear.

dpsw_fpp

0 - seguir en el estado antes de detener
1 - estado contrario de donde estaba antes

si estaba cerrando entonces empieza a abrir.

*/

// Estructura que almacena los estados de los sensores y actuadores
struct SYSTEM_IO
{
    unsigned int lsc : 1;              // Sensor de puerta cerrada
    unsigned int lsa : 1;              // Sensor de puerta abierta
    unsigned int ftc : 1;              // Fotocelda de seguridad
    unsigned int ma : 1;               // Motor abriendo
    unsigned int mc : 1;               // Motor cerrando
    unsigned int lamp : 1;             // Luz indicadora
    unsigned int buzzer : 1;           // Alarma sonora
    unsigned int keya : 1;             // Botón de abrir
    unsigned int keyc : 1;             // Botón de cerrar
    unsigned int pp : 1;               // Botón de paro de emergencia
    unsigned int dpsw_DESCONOCIDO : 2; // Configuración para estado desconocido
    unsigned int dpsw_DETENIDA : 2;    // Configuración para estado detenido
} io;

// Estructura de configuración del sistema
struct SYSTEM_CONFIG
{
    unsigned int cntTCA; // Tiempo de cierre automático en segundos
    unsigned int cntRT;  // Tiempo máximo de movimiento del motor en segundos
    int FDESCONOCIDO;    // Configuración del estado desconocido
    int FDETENIDA;       // Configuración del estado detenido
} config;

int main()
{
    printf("Iniciando Maquina de estado.\n");

    switch (ESTADO_ACTUAL)
    {
    case ESTADO_INICIAL:
        ESTADO_SIGUIENTE = Func_ESTADO_INICIAL();
        break;
    case ESTADO_ERROR:
        ESTADO_SIGUIENTE = Func_ESTADO_ERROR();
        break;
    case ESTADO_ABRIENDO:
        ESTADO_SIGUIENTE = Func_ESTADO_ABRIENDO();
        break;
    case ESTADO_ABIERTO:
        ESTADO_SIGUIENTE = Func_ESTADO_ABIERTO();
        break;
    case ESTADO_CERRANDO:
        ESTADO_SIGUIENTE = Func_ESTADO_CERRANDO();
        break;
    case ESTADO_CERRADO:
        ESTADO_SIGUIENTE = Func_ESTADO_CERRADO();
        break;
    case ESTADO_DETENIDA:
        ESTADO_SIGUIENTE = Func_ESTADO_DETENIDA();
        break;
    case ESTADO_DESCONOCIDO:
        ESTADO_SIGUIENTE = Func_ESTADO_DESCONOCIDO();
        break;
    default:
        fprintf(stderr, "Error: ESTADO_ACTUAL desconocido.\n");
        break;
    }
    printf("Terminando Maquina de estado.\n");
}
int Func_ESTADO_INICIAL(void)
{
    // inicializacion de estado.
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_INICIAL;
    // inicializacion de estado.
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    io.buzzer = BUZZER_ON;

    // ciclo de estado
    for (;;)
    {
        // detectar error en limit SW.
        if (io.lsa == LM_ACTIVO && io.lsc == LM_ACTIVO)
        {
            return ESTADO_ERROR;
        }
        if (io.lsa == LM_NOACTIVO && io.lsc == LM_NOACTIVO)
        {
            return ESTADO_DESCONOCIDO;
        }
        if (io.lsa == LM_NOACTIVO && io.lsc == LM_ACTIVO)
        {
            return ESTADO_CERRADO;
        }
        if (io.lsa == LM_ACTIVO && io.lsc == LM_NOACTIVO)
        {
            return ESTADO_ABIERTO;
        }
    }
}
int Func_ESTADO_ERROR(void)
{
    // inicializacion de estado.
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ERROR;
    // inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    io.buzzer = BUZZER_OFF;

    // ciclo de estado
    for (;;)
    {
        if (io.lsa == LM_NOACTIVO && io.lsc == LM_ACTIVO)
        {
            return ESTADO_CERRADO;
        }

        if (io.lsc == LM_NOACTIVO && io.lsa == LM_ACTIVO)
        {
            return ESTADO_ABIERTO;
        }
        if (io.ftc == FTC_INACTIVO && io.lsc == LM_NOACTIVO && io.lsa == LM_NOACTIVO)
        {
            return ESTADO_CERRANDO;
        }

        io.buzzer = BUZZER_ON;
    }
}
int Func_ESTADO_ABRIENDO(void)
{
    // inicializacion de estado.
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABRIENDO;

    // inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    io.buzzer = BUZZER_OFF;

    // ciclo de estado
    for (;;)
    {

        // si se presiona el paro de emergencia cuando la puerta se esta abriendo el sistema cambia a el estado detenida.
        if (io.pp == PP_ACTIVO)
        {
            return ESTADO_DETENIDA;
        }

        // cuando la puerta abre completamente cambia a estado abierto.
        if (io.lsa == LM_ACTIVO)
        {
            return ESTADO_ABIERTO;
        }
        
        // se enciende el motor en el sentido de abrir la puerta.
        io.ma = MOTOR_ON;
    }
}
int Func_ESTADO_CERRANDO(void)
{
    // inicializacion de estado.
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_CERRANDO;
    // inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    io.buzzer = BUZZER_OFF;

    // ciclo de estado
    for (;;)
    {

        // si se presiona el paro de emergencia cuando la puerta se esta cerrando el sistema cambia a el estado detenida.
        if (io.pp == PP_ACTIVO)
        {
            return ESTADO_DETENIDA;
        }

        // cuando la puerta cierra completamente cambia a estado abierto.
        if (io.lsa == LM_ACTIVO)
        {
            return ESTADO_CERRADO;
        }

        // se enciende el motor en el sentido de cerrar la puerta.
        io.mc = MOTOR_ON;
    }
}
int Func_ESTADO_ABIERTO(void)
{
    // inicializacion de estado.
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABIERTO;

    // inicializacion de estado.
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    config.cntTCA = 0;

    // ciclo de estado
    for (;;)
    {
        // si la puerta esta abierta basta con precionar el boton de cerrar en la llave y el pp para Cerrar el porton.
        if (config.cntTCA >= TIME_CA)
        {
            return ESTADO_CERRANDO;
        }

        if (io.pp == PP_ACTIVO || io.keyc == KEY_ACTIVO)
        {
            return ESTADO_CERRANDO;
        }
    }
}
int Func_ESTADO_CERRADO(void)
{
    // inicializacion de estado.
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_CERRADO;
    // inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    io.buzzer = BUZZER_OFF;

    // ciclo de estado
    for (;;)
    {

        // si la puerta esta cerrada basta con precionar el boton de abrir en la llave y el pp para Abrir el porton.
        if (io.keya == KEY_ACTIVO || io.keya == KEY_ACTIVO)
        {
            return ESTADO_ABRIENDO;
        }
    }
}
int Func_ESTADO_DETENIDA(void)
{
    // inicializacion de estado.
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_DETENIDA;
}

int Func_ESTADO_DESCONOCIDO(void)
{
    // inicializacion de estado.
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_DESCONOCIDO;
    // inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    io.buzzer = BUZZER_OFF;

    // ciclo de estado
    for (;;)
    {

        if (config.FDESCONOCIDO == DESCONOCIDO_CERRADO)
        {
            // 0-0 cerrar
            return ESTADO_CERRANDO;
        }

        if (config.FDESCONOCIDO == DESCONOCIDO_ABRIERTO)
        {
            // 0-1 abrir
            return ESTADO_ABRIENDO;
        }

        if (config.FDESCONOCIDO == DESCONOCIDO_ESPERA)
        {
            // 1-0 en espera de la llave remota
            if (io.keyc == KEY_ACTIVO)
            {
                return ESTADO_CERRANDO;
            }

            if (io.keya == KEY_ACTIVO)
            {
                return ESTADO_ABRIENDO;
            }
        }

        if (config.FDESCONOCIDO == DESCONOCIDO_PARPADEAR)
        {
            if (io.pp == PP_ACTIVO)
            {
                // 1-1 parpadeando
                return ESTADO_CERRANDO;
            }
        }
    }
}
