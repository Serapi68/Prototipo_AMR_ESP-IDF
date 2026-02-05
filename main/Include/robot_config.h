// Sección donde se definiran los pines utilizados en el proyecto 
// Se definira constantes fisicas del prototipo
#include <stdint.h>

#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

/*=========CONFIGURACION DE PINES===========*/
#define PIN_PWM_MOTOR_A  12               //Salida PWM
#define PIN_DIRECCION_A_MOTOR_A  27       //Salida digital
#define PIN_DIRECCION_B_MOTOR_A  26      //Salida digital
#define PIN_PWM_MOTOR_B  13             //Salida PWM
#define PIN_DIRECCION_A_MOTOR_B  25     //Salida digital
#define PIN_DIRECCION_B_MOTOR_B  33     //Salida digital
#define PIN_SERVO_DIRECCION 14         //Salida PWM
#define PIN_HCSR04_TRIG 32          //Salida digital
#define PIN_HCSR04_ECHO 35         //Entrada analogica

//Puede tener cambios
#define LES_STATUS 2

/*=========CONSTANTES FISICAS===========*/
//Constantes fisicas del prototipo

#define DISTANCIA_EJE_MOTORES_CM  0.20f    //Distancia entre los ejes de los motores en cm
#define RADIO_GIRO_CM  0.30f               //Radio de giro del robot en cm
#define ANCHO_VIA_CM 0.15f                 //Ancho de la via del robot en cm

//limites de velocidad
#define VELOCIDAD_MAX  100.0f           //Velocidad maxima en
#define VELOCIDAD_MIN  20.0f            //Velocidad minima en cm/s
#define VELOCIDAD_GIRO 60.0f            //Velocidad de giro en cm/s

/*====================Configuracion PWM=================*/

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_FRECUENCIA         200
#define LEDC_RESOLUCION         LEDC_TIMER_10_BIT  // 10 bits = 0-1023

// Canales PWM para los motores
#define LEDC_CHANNEL_IZQ        LEDC_CHANNEL_0
#define LEDC_CHANNEL_DER        LEDC_CHANNEL_1

// Parámetros del PWM para servomotor
#define SERVO_TIMER             LEDC_TIMER_1
#define SERVO_CHANNEL           LEDC_CHANNEL_2
#define SERVO_FRECUENCIA        50        // 50 Hz (estándar para servos)
#define SERVO_RESOLUCION        LEDC_TIMER_14_BIT  // 14 bits para precisión

// Límites del servo 
#define SERVO_PULSO_MIN         500      // ~1ms (ángulo mínimo)
#define SERVO_PULSO_MAX         2500      // ~2ms (ángulo máximo)
#define SERVO_PULSO_CENTRO      1500      // ~1.5ms (centro)

/* ========== CONFIGURACIÓN DEL MANDO XBOX ========== */

// Rangos de los ejes analógicos (valores típicos HID)
#define XBOX_STICK_MIN          -32768    // Valor mínimo del stick
#define XBOX_STICK_MAX          32767     // Valor máximo del stick
#define XBOX_TRIGGER_MIN        0         // Gatillo sin presionar
#define XBOX_TRIGGER_MAX        1023      // Gatillo totalmente presionado

// Zona muerta 
#define XBOX_DEADZONE           0.15f     // 15% de zona muerta

/* ========== CONFIGURACIÓN DEL SENSOR ULTRASÓNICO ========== */

#define DISTANCIA_MINIMA_CM     5         // Distancia de detección (5cm)
#define TIMEOUT_ULTRASONICO_US  30000     // Timeout de 30ms


/* ========== ESTADOS DEL ROBOT ========== */

typedef enum {
    MODO_MANUAL = 0,           // Control total por mando Xbox
    MODO_AUTONOMO_SENSORES,    // Evasión automática con HC-SR04
    MODO_AUTONOMO_VISION       // Control con visión artificial
} modo_operacion_t;

/* ========== ESTRUCTURA DE DATOS DEL MANDO ========== */

typedef struct {
    float trigger_rt;          // Gatillo derecho (0.0 a 1.0) - Avanzar
    float trigger_lt;          // Gatillo izquierdo (0.0 a 1.0) - Retroceder
    float stick_izq_x;         // Stick izquierdo eje X (-1.0 a 1.0) - Dirección
    float stick_izq_y;         // Stick izquierdo eje Y (reservado)
    uint8_t boton_a;           // Botón A - Cambio de modo
    uint8_t boton_b;           // Botón B - Emergencia
} xbox_data_t;

/* ========== ESTRUCTURA DE VELOCIDADES CALCULADAS ========== */

typedef struct {
    float velocidad_izq;       // Velocidad motor izquierdo (0.0 a 100.0)
    float velocidad_der;       // Velocidad motor derecho (0.0 a 100.0)
    float angulo_servo;        // Ángulo del servo (-1.0 a 1.0)
    int8_t direccion_izq;      // Dirección: 1=adelante, -1=atrás, 0=parado
    int8_t direccion_der;      // Dirección: 1=adelante, -1=atrás, 0=parado
} velocidades_motor_t;

/* ========== PRIORIDADES DE TAREAS FREERTOS ========== */

#define PRIORIDAD_XBOX          10        // Máxima prioridad (baja latencia)
#define PRIORIDAD_MOTORES       8         // Alta prioridad (control)
#define PRIORIDAD_SENSORES      5         // Media prioridad (lectura)
#define PRIORIDAD_VISION        3         // Baja prioridad (procesamiento pesado)

/* ========== TAMAÑOS DE STACK ========== */

#define STACK_SIZE_XBOX         4096
#define STACK_SIZE_MOTORES      2048
#define STACK_SIZE_SENSORES     2048
#define STACK_SIZE_VISION       8192


#endif // ROBOT_CONFIG_H