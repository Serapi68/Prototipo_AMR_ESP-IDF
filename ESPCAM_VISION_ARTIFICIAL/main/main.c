#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "camara_driver/camara_driver.h"
#include "wifi_stream/http_stream.h"

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

    // 4. Iniciar Servidor Web
    start_webserver();

    ESP_LOGI(TAG, "Sistema de Visión Iniciado. Esperando conexión...");
}
