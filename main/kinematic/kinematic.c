// Funciones matematicas para cinemática del robot de veloccidad diferencial
#include "robot_config.h"
#include "motor_driver.h"
#include "xbox_handler.h"
#include <math.h>
#include "esp_log.h"

//Constantes fisicas estan en el archivo de configuracion robot_config.h

static const char *TAG = "KINEMATIC";

//Funcion de actualizacion de la posicion del robot

void actualizar_movimiento(float potencia, float direccion_stick) {
    
    // 1. Zona muerta (si el stick se mueve apenas un poco, lo ignoramos)
    if (fabs(direccion_stick) < XBOX_DEADZONE) direccion_stick = 0;
    if (fabs(potencia) < 0.05) potencia = 0;

    // 2. Control del Servo
    // Usamos el offset y el limite definidos en config para evitar forzar el mecanismo
    int angulo_servo = 90 + SERVO_OFFSET_CENTRADO + (int)(direccion_stick * SERVO_ANGULO_MAX_GIRO); 
    set_servo_angle(angulo_servo);

    // 3. Diferencial Electrónico
    int pwm_izq = 0;
    int pwm_der = 0;

    if (direccion_stick == 0) {
        // Marcha recta: misma potencia a ambos motores
        int pwm_val = (int)(potencia * 1023); // Convertir 0.0-1.0 a 0-1023 bits
        pwm_izq = pwm_val;
        pwm_der = pwm_val;
    } else {
        // Calculamos radio de giro R basado en el ángulo real del servo
        // Clamp del ángulo mínimo para evitar R infinito o inestable
        float angulo_rad = fabs(direccion_stick) * (SERVO_ANGULO_MAX_GIRO * M_PI / 180.0f); 
        if (angulo_rad < 0.01f) angulo_rad = 0.05f; 

        float R = DISTANCIA_EJE_MOTORES_CM / tan(angulo_rad);

        // Calculamos velocidades para cada rueda usando diferencial electrónico
        // Factor de corrección: Si R es muy grande, el término es cercano a 0.
        // añadimos un multiplicador para que el efecto de giro sea más pronunciado (ajustable)
        float factor_diferencial = (ANCHO_VIA_CM / (2 * R)) * EFECTO_GIRO;
        
        // Limitamos el factor para que no invierta el motor (opcional, por seguridad)
        if (factor_diferencial > 0.9f) factor_diferencial = 0.9f;

        float v_interior = potencia * (1.0f - factor_diferencial);
        float v_exterior = potencia * (1.0f + factor_diferencial);
        
        // Asegurar que no excedemos el rango -1.0 a 1.0 para permitir marcha atras
        if (v_exterior > 1.0f) v_exterior = 1.0f;
        if (v_exterior < -1.0f) v_exterior = -1.0f;
        if (v_interior > 1.0f) v_interior = 1.0f;
        if (v_interior < -1.0f) v_interior = -1.0f;

        int pwm_int = (int)(v_interior * 1023);
        int pwm_ext = (int)(v_exterior * 1023);

        if (direccion_stick > 0) { // Girar a la derecha
            pwm_der = pwm_int;
            pwm_izq = pwm_ext;
        } else { // Girar a la izquierda
            pwm_izq = pwm_int;
            pwm_der = pwm_ext;
        }

    }

    // Aplicar velocidades
    set_motor_speed_left(pwm_izq);
    set_motor_speed_right(pwm_der);

    

    // 4. Monitor Serial 
    // Usamos un contador estático para no saturar el log 
    static int log_counter = 0;
    if (log_counter++ > 20) {
        ESP_LOGI(TAG, "Stick: %.2f | Servo: %d | Pot: %.2f | PWM L: %d | PWM R: %d", 
                 direccion_stick, angulo_servo, potencia, pwm_izq, pwm_der);
        log_counter = 0;
    }
}