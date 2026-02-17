#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "camara_driver/camara_driver.h"
#include "wifi_stream/http_stream.h"
#include "vision_core/vision_core.h"

static const char *TAG = "ESP_CAM_TEST";

void app_main(void)
{
    // 1. Inicializar NVS (Necesario para Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Inicializar Cámara
    if(init_camera() != ESP_OK){
        ESP_LOGE(TAG, "Error fatal: No se pudo iniciar la cámara");
        return;
    }

    // 3. Iniciar Wi-Fi
    start_wifi(); 

    // 4. Iniciar vision_core (tarea de procesamiento)
    vision_core_init();
    ESP_LOGI(TAG, "Sistema de Visión Iniciado. Esperando conexión...");
    
    // 5. Iniciar servidor web (después de iniciar visión_core para evitar bloqueos)
    start_webserver();
}

