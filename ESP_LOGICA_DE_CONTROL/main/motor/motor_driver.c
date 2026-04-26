// Modulo donde se implementan las funciones para el control de los motores
// de corriente continua mediante señales PWM y control de direccion

#include "robot_config.h"
#include "motor_driver.h"
#include <stdlib.h>

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG_MOTOR = "MOTOR_PCA";

// Función auxiliar para escribir en el PCA9685
esp_err_t pca9685_write(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCA9685_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);
    return ret;
}

void set_pca9685_pwm(uint8_t channel, uint16_t on, uint16_t off) {
    pca9685_write(0x06 + 4 * channel, on & 0xFF);
    pca9685_write(0x07 + 4 * channel, on >> 8);
    pca9685_write(0x08 + 4 * channel, off & 0xFF);
    pca9685_write(0x09 + 4 * channel, off >> 8);
}

void init_motors(){
    // 1. Configurar I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);

    // 2. Inicializar PCA9685 (50Hz para servos)
    pca9685_write(0x00, 0x10); // Sleep mode para configurar frecuencia
    uint8_t prescale = (uint8_t)(round(25000000.0 / (4096.0 * 50.0)) - 1);
    pca9685_write(0xFE, prescale);
    pca9685_write(0x00, 0xA0); // Wake up y auto-incremento

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

    // Configuracion de los pines de direccion y standby
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_DIRECCION_A_MOTOR_A) | (1ULL << PIN_DIRECCION_B_MOTOR_A) |
                        (1ULL << PIN_DIRECCION_A_MOTOR_B) | (1ULL << PIN_DIRECCION_B_MOTOR_B) |
                        (1ULL << PIN_MOTOR_STBY) | (1ULL << PIN_LED),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);

    // Inicializar servos en posición central/reposo inmediatamente
    set_servo_angle(SERVO_ANGULO_BASE + SERVO_OFFSET_CENTRADO);
    set_servo_sensor_angle(ANGULO_SENSOR_CENTRO);
    
    // Asegurar que los motores empiezan detenidos (0V en dirección)
    gpio_set_level(PIN_DIRECCION_A_MOTOR_A, 0);
    gpio_set_level(PIN_DIRECCION_B_MOTOR_A, 0);
    gpio_set_level(PIN_DIRECCION_A_MOTOR_B, 0);
    gpio_set_level(PIN_DIRECCION_B_MOTOR_B, 0);

    set_motor_standby(true); // Iniciar habilitado (STBY en HIGH)
}

void set_motor_speed_right(int speed){
    if (speed > 0) {
        gpio_set_level(PIN_DIRECCION_A_MOTOR_B, 1);
        gpio_set_level(PIN_DIRECCION_B_MOTOR_B, 0);
    } else if (speed < 0) {
        gpio_set_level(PIN_DIRECCION_A_MOTOR_B, 0);
        gpio_set_level(PIN_DIRECCION_B_MOTOR_B, 1);
    } else {
        gpio_set_level(PIN_DIRECCION_A_MOTOR_B, 0);
        gpio_set_level(PIN_DIRECCION_B_MOTOR_B, 0);
    }

    // Log de diagnóstico para verificar el estado real de los pines de dirección
    ESP_LOGI(TAG_MOTOR, "DER [%d]: BIN1(GPIO 25)=%d, BIN2(GPIO 33)=%d, PWMB(GPIO 13)=%d", 
             speed, gpio_get_level(PIN_DIRECCION_A_MOTOR_B), 
             gpio_get_level(PIN_DIRECCION_B_MOTOR_B), abs(speed));

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DER, abs(speed));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DER);
}

void set_motor_speed_left(int speed){
    if (speed > 0) {
        gpio_set_level(PIN_DIRECCION_A_MOTOR_A, 1);
        gpio_set_level(PIN_DIRECCION_B_MOTOR_A, 0);
    } else if (speed < 0) {
        gpio_set_level(PIN_DIRECCION_A_MOTOR_A, 0);
        gpio_set_level(PIN_DIRECCION_B_MOTOR_A, 1);
    } else {
        // Freno/Parada: Ambos en bajo
        gpio_set_level(PIN_DIRECCION_A_MOTOR_A, 0);
        gpio_set_level(PIN_DIRECCION_B_MOTOR_A, 0);
    }

    // Log de diagnóstico para verificar el estado real de los pines de dirección
    ESP_LOGI(TAG_MOTOR, "IZQ [%d]: AIN2(GPIO 27)=%d, AIN1(GPIO 26)=%d, PWMA(GPIO 12)=%d", 
             speed, gpio_get_level(PIN_DIRECCION_A_MOTOR_A), 
             gpio_get_level(PIN_DIRECCION_B_MOTOR_A), abs(speed));

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_IZQ, abs(speed));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_IZQ);
}

void set_servo_angle(int angle){
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    // Mapear ángulo a microsegundos (500us a 2500us según robot_config.h)
    int pulse_width_us = SERVO_PULSO_MIN + (angle * (SERVO_PULSO_MAX - SERVO_PULSO_MIN) / 180);
    
    // El PCA9685 tiene resolución de 12 bits (4096 pasos) para un ciclo de 20ms (50Hz)
    // Valor = (pulso_us * 4096) / 20000
    uint16_t off_val = (uint16_t)(pulse_width_us * 4096 / 20000);
    set_pca9685_pwm(PCA_CHANNEL_DIRECCION, 0, off_val);
}

void set_motor_standby(bool run){
    ESP_LOGI(TAG_MOTOR, "Configurando STBY a: %s", run ? "HIGH (RUN)" : "LOW (OFF)");
    gpio_set_level(PIN_MOTOR_STBY, run ? 1 : 0);
}

void set_servo_sensor_angle(int angle){
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    int pulse_width_us = SERVO_PULSO_MIN + (angle * (SERVO_PULSO_MAX - SERVO_PULSO_MIN) / 180);
    uint16_t off_val = (uint16_t)(pulse_width_us * 4096 / 20000);
    set_pca9685_pwm(PCA_CHANNEL_SENSOR, 0, off_val);
}

void set_led(bool on){
    gpio_set_level(PIN_LED, on ? 1 : 0);
}