// Modulo donde se implementan las funciones para el control de los motores
// de corriente continua mediante señales PWM y control de direccion

#include "robot_config.h"

#include "driver/ledc.h"
#include "driver/gpio.h"

void init_motors(){
    // Configuracion del timer para los motores
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_RESOLUCION,
        .freq_hz          = LEDC_FRECUENCIA,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Configuracion del canal para el motor izquierdo
    ledc_channel_config_t ledc_channel_izq = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_IZQ,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_PWM_MOTOR_A,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel_izq);

    // Configuracion del canal para el motor derecho
    ledc_channel_config_t ledc_channel_der = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_DER,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_PWM_MOTOR_B,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel_der);

    ledc_timer_config_t ledc_timer_servo = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = SERVO_TIMER,
        .duty_resolution  = SERVO_RESOLUCION,
        .freq_hz          = SERVO_FRECUENCIA,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer_servo);

    ledc_channel_config_t ledc_channel_servo = {
        .speed_mode     = LEDC_MODE,
        .channel        = SERVO_CHANNEL,
        .timer_sel      = SERVO_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_SERVO_DIRECCION,
        .duty           = 0,
        .hpoint         = 0
    }; 
    ledc_channel_config(&ledc_channel_servo);

    // Configuracion de los pines de direccion como salidas
    gpio_set_direction(PIN_DIRECCION_A_MOTOR_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_DIRECCION_B_MOTOR_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_DIRECCION_A_MOTOR_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_DIRECCION_B_MOTOR_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
}

void set_motor_speed_right(int speed){
    if (speed > 0) {
        gpio_set_level(PIN_DIRECCION_A_MOTOR_B, 1);
        gpio_set_level(PIN_DIRECCION_B_MOTOR_B,0);

    }else{
        gpio_set_level(PIN_DIRECCION_A_MOTOR_B, 0);
        gpio_set_level(PIN_DIRECCION_B_MOTOR_B, 1);
    }
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DER, abs(speed));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DER);
}

void set_motor_speed_left(int speed){
    if (speed > 0) {
        gpio_set_level(PIN_DIRECCION_A_MOTOR_A, 1);
        gpio_set_level(PIN_DIRECCION_B_MOTOR_A, 0);
    }else{

        gpio_set_level(PIN_DIRECCION_A_MOTOR_A, 0);
        gpio_set_level(PIN_DIRECCION_B_MOTOR_A, 1);
    }
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_IZQ, abs(speed));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_IZQ);
}

void set_servo_angle(int angle){

  
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    // Calcular el ancho de pulso en microsegundos (1000-2000 µs)
    int pulse_width_us = SERVO_PULSO_MIN + (angle * (SERVO_PULSO_MAX - SERVO_PULSO_MIN) / 180);
    
    // Convertir microsegundos a duty cycle
    // Fórmula: duty = (pulse_width_us * frecuencia * 2^resolución) / 1000000
    // Para 50Hz y 14 bits: duty = (pulse_width_us * 50 * 16384) / 1000000
    int duty = (pulse_width_us * SERVO_FRECUENCIA * (1 << SERVO_RESOLUCION)) / 1000000;
    
    ledc_set_duty(LEDC_MODE, SERVO_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, SERVO_CHANNEL);
    
}

void set_led(bool on){
    gpio_set_level(PIN_LED, on ? 1 : 0);
}