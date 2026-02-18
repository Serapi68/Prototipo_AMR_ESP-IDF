#include "vision_core.h"
#include "qr_processor.h"
#include "camara_driver/camara_driver.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "espcam_config.h"
#include "esp_camera.h"

static const char *TAG = "VISION_CORE";
static vision_mode_t g_current_mode = VISION_MODO_STREAMING;
SemaphoreHandle_t g_cam_mutex = NULL;

// ... (mantén tus includes)

void vision_core_set_mode(vision_mode_t mode) {
    if (g_current_mode == mode) return;

    if (xSemaphoreTake(g_cam_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Timeout al tomar mutex para cambiar modo");
        return;
    }

    ESP_LOGI(TAG, "Cambiando modo de %d a %d. Reconfigurando cámara...", g_current_mode, mode);

    // 1. Apagar la cámara y limpiar recursos
    esp_camera_deinit();
    gpio_uninstall_isr_service();
    gpio_reset_pin(CAM_PIN_XCLK);

    // 2. Power Cycle del sensor para un reinicio limpio
    gpio_set_direction(CAM_PIN_PWDN, GPIO_MODE_OUTPUT);
    gpio_set_level(CAM_PIN_PWDN, 1); // Apagar
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(CAM_PIN_PWDN, 0); // Encender
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Crear nueva configuración
    camera_config_t config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        .xclk_freq_hz = CAM_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    // 4. Aplicar configuración específica del modo
    if (mode == VISION_MODO_QR) {
        config.pixel_format = PIXFORMAT_GRAYSCALE;
        config.frame_size = FRAMESIZE_QVGA;
        config.fb_count = 2; // Doble buffer para evitar overflow
    } else { // Streaming y otros modos
        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size = CAM_RESOLUCION;
        config.jpeg_quality = CAM_CALIDAD;
        config.fb_count = CAM_BUFFERS;
    }

    // 5. Reinicializar la cámara
    if (esp_camera_init(&config) != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al reinicializar cámara para modo %d", mode);
    }

    // 6. Actualizar estado y liberar mutex
    g_current_mode = mode;
    xSemaphoreGive(g_cam_mutex);
}

// ... (resto del código SIN CAMBIOS)

vision_mode_t vision_core_get_mode(void) {
    return g_current_mode;
}

static void vision_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea iniciada en Core %d", xPortGetCoreID());
    
    // Esperar a que WiFi y otros sistemas inicien
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1) {
        // Si estamos en modo STREAMING, esta tarea duerme
        if (g_current_mode == VISION_MODO_STREAMING) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Tomar mutex con timeout
        if (xSemaphoreTake(g_cam_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            ESP_LOGW(TAG, "Timeout esperando mutex");
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // Capturar frame
        camera_fb_t *fb = camara_capture();
        
        if (fb) {
            // Procesar según el modo
            switch (g_current_mode) {
                case VISION_MODO_QR:
                    qr_process_frame(fb);
                    break;
                    
                case VISION_MODO_TRACKING:
                    // TODO: Implementar seguimiento de objetos
                    ESP_LOGD(TAG, "Modo tracking aún no implementado");
                    break;
                    
                case VISION_MODO_SENALES:
                    // TODO: Implementar detección de señales
                    ESP_LOGD(TAG, "Modo señales aún no implementado");
                    break;
                    
                default:
                    break;
            }
            
            // Devolver frame
            camara_return_fb(fb);
        } else {
            ESP_LOGW(TAG, "No se pudo capturar frame");
        }

        // Liberar mutex
        xSemaphoreGive(g_cam_mutex);
        
        // Delay para no saturar CPU (ajustable según FPS deseado)
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vision_core_init(void) {
    // Crear mutex para sincronización
    g_cam_mutex = xSemaphoreCreateMutex();
    if (g_cam_mutex == NULL) {
        ESP_LOGE(TAG, "Fallo al crear mutex");
        return;
    }
    
    // Crear tarea de procesamiento en Core 1
    xTaskCreatePinnedToCore(
        vision_task,
        "VisionTask",
        20480,  // Stack size aumentado a 20KB para seguridad
        NULL,
        5,      // Prioridad
        NULL,
        1       // Core 1 (APP_CPU)
    );
    
    ESP_LOGI(TAG, "Sistema inicializado");
}