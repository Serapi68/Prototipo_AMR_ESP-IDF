// Funciones matematicas para cinemática del robot de veloccidad diferencial
#include "robot_config.h"
#include "motor_driver.h"
#include <math.h>
#include "esp_log.h"

//Constantes fisicas estan en el archivo de configuracion robot_config.h

//Funcion de actualizacion de la posicion del robot

void actualizar_movimiento(float potencia, float direccion_stick) {
    
    static const char *TAG_KIN = "KINEMATIC";

    // 1. Zona muerta (si el stick se mueve apenas un poco, lo ignoramos)
    if (fabs(direccion_stick) < XBOX_DEADZONE) direccion_stick = 0;
    if (fabs(potencia) < 0.05) potencia = 0;

    // 2. Control del Servo
    // Usamos el offset y el limite definidos en config para evitar forzar el mecanismo
    int angulo_servo = SERVO_ANGULO_BASE + SERVO_OFFSET_CENTRADO + (int)(direccion_stick * SERVO_ANGULO_MAX_GIRO); 
    set_servo_angle(angulo_servo);

    // 3. Diferencial Electrónico
    int pwm_izq = 0;
    int pwm_der = 0;

    if (direccion_stick == 0) {
        // Marcha recta: misma potencia a ambos motores
        int pwm_val = (int)(potencia * 255); // Convertir 0.0-1.0 a 0-255 bits (8 bits)
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

        int pwm_int = (int)(v_interior * 255);
        int pwm_ext = (int)(v_exterior * 255);

        if (direccion_stick > 0) { // Girar a la derecha
            pwm_der = pwm_int;
            pwm_izq = pwm_ext;
        } else { // Girar a la izquierda
            pwm_izq = pwm_int;
            pwm_der = pwm_ext;
        }

    }

    // Limitar PWM al rango de 8 bits (0-255) para evitar desbordamientos si la normalización falla
    if (pwm_izq > 255)  pwm_izq = 255;
    if (pwm_izq < -255) pwm_izq = -255;
    if (pwm_der > 255)  pwm_der = 255;
    if (pwm_der < -255) pwm_der = -255;

    // 4. Suavizado de aceleración (Soft Start) para evitar picos de corriente (Brownouts)
    static int current_pwm_izq = 0;
    static int current_pwm_der = 0;
    const int ramp_step_up = 25;    // Aceleración suave
    const int ramp_step_down = 100; // Frenado agresivo para evitar choques

    // Rampa para motor izquierdo
    if (pwm_izq > current_pwm_izq) current_pwm_izq += (pwm_izq - current_pwm_izq > ramp_step_up) ? ramp_step_up : (pwm_izq - current_pwm_izq);
    else if (pwm_izq < current_pwm_izq) current_pwm_izq -= (current_pwm_izq - pwm_izq > ramp_step_down) ? ramp_step_down : (current_pwm_izq - pwm_izq);

    // Rampa para motor derecho
    if (pwm_der > current_pwm_der) current_pwm_der += (pwm_der - current_pwm_der > ramp_step_up) ? ramp_step_up : (pwm_der - current_pwm_der);
    else if (pwm_der < current_pwm_der) current_pwm_der -= (current_pwm_der - pwm_der > ramp_step_down) ? ramp_step_down : (current_pwm_der - pwm_der);

    // Aplicar los valores suavizados
    pwm_izq = current_pwm_izq;
    pwm_der = current_pwm_der;

    ESP_LOGI(TAG_KIN, "Motores -> L: %d, R: %d (Stick: %.2f, Pot: %.2f)", pwm_izq, pwm_der, direccion_stick, potencia);
    set_motor_speed_left(pwm_izq);
    set_motor_speed_right(pwm_der);

}
