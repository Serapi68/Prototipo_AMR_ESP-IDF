#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "esp_log.h"

//mis modulos
#include "robot_config.h"
#include "xbox_handler.h"
#include "motor_driver.h"
#include "kinematic.h"
#include "maquina_estado.h"
#include "modo_autonomo/hc_sr04.h"

static const char *TAG = "MAIN_APP";

/* ================= APP MAIN ================= */
void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando Sistema AMR");

    //Inicializar NVS (necesario para Bluetooth)
    ESP_ERROR_CHECK(nvs_flash_init());
    
    //Inicializar motores (PWM)
    ESP_LOGI(TAG, "Inicializando motores");
    init_motors();

    //Inicializar sensor ultrasónico
    ESP_LOGI(TAG, "Inicializando sensor HC-SR04");
    hc_sr04_init();

    //Inicializar Mando Xbox 
    init_xbox();

    // Inicializar la Máquina de Estados (Crea cola y tarea de control)
    init_maquina_estado();

    ESP_LOGI(TAG, "Sistema listo. Controlar el robot con el mando Xbox.");
}