#include "vision_core.h"
#include "qr_processor.h"
#include "object_processor.h"
#include "camara_driver/camara_driver.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "espcam_config.h"
#include "esp_camera.h"
#include "esp_task_wdt.h"

static const char *TAG = "VISION_CORE";
static vision_mode_t g_current_mode = VISION_MODO_STREAMING;
SemaphoreHandle_t g_cam_mutex = NULL;

void vision_core_set_mode(vision_mode_t mode) {
    if (g_current_mode == mode) return;

    if (xSemaphoreTake(g_cam_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Timeout al tomar mutex para cambiar modo");
        return;
    }

    ESP_LOGI(TAG, "Cambiando modo de %d a %d. Reconfigurando cámara...", g_current_mode, mode);

    // 1. Apagar la cámara y limpiar recursos
    esp_camera_deinit();
    vTaskDelay(pdMS_TO_TICKS(50));

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
        .xclk_freq_hz = (mode == VISION_MODO_TRACKING) ? 10000000 : CAM_XCLK_FREQ, // Reducir a 10MHz para estabilidad RGB
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    // 4. Aplicar configuración específica del modo
    if (mode == VISION_MODO_QR) {
        qr_set_enabled(true);  // Activar lógica de procesamiento QR
        config.pixel_format = PIXFORMAT_GRAYSCALE;
        config.frame_size = FRAMESIZE_QVGA;
        config.fb_count = 2; // Doble buffer para evitar overflow
    } else if (mode == VISION_MODO_TRACKING) {
        qr_set_enabled(false);
        config.pixel_format = PIXFORMAT_GRAYSCALE; // Cambiado a Grayscale para el nuevo modelo
        config.frame_size = FRAMESIZE_QVGA;        // Ampliamos el rango a 320x240
        config.fb_count = 2;                       // Doble buffer para mayor fluidez
    } else {
        qr_set_enabled(false); // Desactivar para ahorrar CPU
        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size = CAM_RESOLUCION;
        config.jpeg_quality = CAM_CALIDAD;
        config.fb_count = CAM_BUFFERS;
    }

    // 5. Reinicializar la cámara
    if (esp_camera_init(&config) != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al reinicializar cámara para modo %d", mode);
    } else {
        // Configuración extra para mejorar el color en Tracking
        sensor_t *s = esp_camera_sensor_get();
        if (s && mode == VISION_MODO_TRACKING) {
            s->set_vflip(s, 1);
            s->set_hmirror(s, 1);
            
            // --- AJUSTES PARA EVITAR IMAGEN QUEMADA ---
            s->set_gain_ctrl(s, 1);      // Habilitar control de ganancia auto
            s->set_exposure_ctrl(s, 1);  // Habilitar control de exposición auto
            s->set_awb_gain(s, 1);       // Auto White Balance
            s->set_brightness(s, -1);    // Bajar un poco el brillo base
            s->set_ae_level(s, 0);       // Nivel de exposición compensada a 0
            
            vTaskDelay(pdMS_TO_TICKS(1000)); // Más tiempo para que el AEC actúe
        }
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
    
    // Esperar un segundo antes de registrar el Watchdog para permitir que el kernel se estabilice
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Suscribir esta tarea al Watchdog ahora que el core está estable
    esp_task_wdt_add(NULL); 

    while (1) {
        // Alimentar al Watchdog al inicio de cada ciclo
        esp_task_wdt_reset();

        // Si estamos en modo STREAMING, esta tarea duerme
        if (g_current_mode == VISION_MODO_STREAMING) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (g_cam_mutex == NULL) {
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
                    object_tracking_process_frame(fb);
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
    
    // Pequeño delay para asegurar que los drivers de sistema y PSRAM estén estables
    vTaskDelay(pdMS_TO_TICKS(100));

    // Crear tarea de procesamiento en Core 1 (APP_CPU)
    xTaskCreatePinnedToCore(
        vision_task,
        "VisionTask",
        28672,  // Reducido a 28KB para mayor estabilidad en el arranque
        NULL,
        5,      // Prioridad
        NULL,
        1       // Core 1 (APP_CPU)
    );
    
    ESP_LOGI(TAG, "Sistema inicializado");
}