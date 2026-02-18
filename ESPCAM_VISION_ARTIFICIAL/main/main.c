#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "camara_driver/camara_driver.h"
#include "wifi_stream/http_stream.h"
#include "vision_core/vision_core.h"
#include "comunicacion_uart/uart_comm.h"

static const char *TAG = "MAIN";

void app_main(void) {
    // 1. Inicializar NVS (necesario para WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Inicializar cámara
    ESP_LOGI(TAG, "Inicializando cámara...");
    if (init_camera() != ESP_OK) {
        ESP_LOGE(TAG, "Fallo crítico al inicializar cámara");
        return;
    }

    // 3. Iniciar WiFi
    ESP_LOGI(TAG, "Iniciando WiFi...");
    start_wifi();

    // 4. Iniciar sistema de visión
    ESP_LOGI(TAG, "Iniciando sistema de visión...");
    vision_core_init();

    // 5. Iniciar servidor web
    ESP_LOGI(TAG, "Iniciando servidor web...");
    start_webserver();

    // 6. Iniciar control por UART
    ESP_LOGI(TAG, "Iniciando control por UART...");
    uart_control_init();

    ESP_LOGI(TAG, "Sistema completo iniciado correctamente");
}