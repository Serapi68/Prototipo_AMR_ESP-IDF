#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

//mis modulos
#include "robot_config.h"
#include "xbox_handler.h"
#include "motor_driver.h"
#include "kinematic.h"

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

    //Inicializar Mando Xbox 
    init_xbox();
    ESP_LOGI(TAG, "Sistema listo. Controlar el robot con el mando Xbox.");

    while(1){
        // Aquí se podrían implementar tareas periódicas, como lectura de sensores o actualización de estado
        vTaskDelay(pdMS_TO_TICKS(1000)); // Esperar 1 segundo (ajustable según necesidades)
    }
}