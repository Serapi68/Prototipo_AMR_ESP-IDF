#include "vision_core.h"
#include <string.h>
#include "esp_log.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "img_converters.h"
#include "freertos/semphr.h"

#if __has_include("quirc.h")
#include "quirc.h"
#define SOPORTE_QR 1
#else
#define SOPORTE_QR 0
#endif

SemaphoreHandle_t g_cam_mutex = NULL;
static const char *TAG = "VISION_CORE";
static vision_mode_t g_current_mode = VISION_MODO_IDLE;

// Variables estáticas GLOBALES para Quirc (no dentro de la función)
#if SOPORTE_QR
static struct quirc *q = NULL;
static uint8_t *rgb_buf = NULL;
static size_t rgb_buf_len = 0;
static int quirc_w = 0, quirc_h = 0;
#endif

void vision_core_set_mode(vision_mode_t mode) {
    g_current_mode = mode;
    ESP_LOGI(TAG, "Modo de visión cambiado a: %d", mode);
}

#if SOPORTE_QR
static void inicializar_quirc(int width, int height) {
    if (q != NULL) {
        quirc_destroy(q);
        q = NULL;
    }
    if (rgb_buf != NULL) {
        heap_caps_free(rgb_buf);
        rgb_buf = NULL;
        rgb_buf_len = 0;
    }
    
    q = quirc_new();
    if (!q) {
        ESP_LOGE(TAG, "No se pudo crear objeto Quirc");
        return;
    }
    
    if (quirc_resize(q, width, height) < 0) {
        ESP_LOGE(TAG, "Fallo al reservar memoria en Quirc");
        quirc_destroy(q);
        q = NULL;
        return;
    }
    
    // Obtener dimensiones reales de quirc
    quirc_w = width;
    quirc_h = height;
    quirc_end(q);
    
    // Reservar buffer RGB
    size_t rgb_len = width * height * 3;
    rgb_buf = (uint8_t *)heap_caps_malloc(rgb_len, MALLOC_CAP_SPIRAM);
    if (!rgb_buf) {
        // Intentar en DRAM si PSRAM falla
        rgb_buf = (uint8_t *)malloc(rgb_len);
    }
    rgb_buf_len = rgb_len;
    
    ESP_LOGI(TAG, "Quirc inicializado: %dx%d (buffer: %d bytes)", 
             quirc_w, quirc_h, rgb_buf_len);
}

static void proceso_qr(camera_fb_t *fb) {
    if (fb->len < 100 || fb->width == 0 || fb->height == 0) {
        return;
    }
    
    // Inicializar Quirc si es necesario
    if (q == NULL) {
        inicializar_quirc(fb->width, fb->height);
        if (q == NULL) return;
    }
    
    // Verificar que las dimensiones coincidan
    if (quirc_w != fb->width || quirc_h != fb->height) {
        ESP_LOGW(TAG, "Dimensiones cambiaron, reinicializando Quirc");
        inicializar_quirc(fb->width, fb->height);
        if (q == NULL) return;
    }
    
    // Obtener buffer de Quirc
    uint8_t *image = quirc_begin(q, &quirc_w, &quirc_h);
    if (!image) {
        ESP_LOGE(TAG, "quirc_begin falló");
        return;
    }

    if(rgb_buf == NULL){
        ESP_LOGE(TAG, "Buffer RGB no disponible");
        return;
    }
    
    // Convertir JPEG a RGB
    bool conversion_ok = fmt2rgb888(fb->buf, fb->len, fb->format, rgb_buf);
    
    // **CORRECCIÓN**: Ceder CPU después de la pesada decodificación JPEG para evitar WDT.
    vTaskDelay(1);

    if (conversion_ok && rgb_buf != NULL) {
        // Convertir a escala de grises con límites seguros
        int pixels = quirc_w * quirc_h;
        int max_pixels = rgb_buf_len / 3;
        if (pixels > max_pixels) pixels = max_pixels;
        
        // Bucle de conversión a escala de grises
        for (int i = 0; i < pixels; i++) {
            uint32_t base = i * 3;
            uint8_t r = rgb_buf[base];
            uint8_t g = rgb_buf[base + 1];
            uint8_t b = rgb_buf[base + 2];
            image[i] = (r * 30 + g * 59 + b * 11) / 100;

            // **CORRECCIÓN CRÍTICA**: Ceder CPU para evitar Watchdog Timeout
            // Este bucle es muy intensivo. Cedemos el control cada 2048 píxeles.
            if ((i > 0) && (i % 2048 == 0)) {
                vTaskDelay(1); // Cede el control por 1 tick del sistema operativo
            }
        }
    } else {
        ESP_LOGW(TAG, "Conversión fallida, limpiando buffer");
        memset(image, 0, quirc_w * quirc_h);
    }
    
    // Finalizar carga de imagen
    quirc_end(q);
    
    // Procesar resultados
    int count = quirc_count(q);
    if (count > 0) {
        struct quirc_code code;
        // No alojar 'data' en el stack, es demasiado grande.
        // Usamos malloc para alojarlo en el heap y evitar el Stack Overflow.
        struct quirc_data *data = (struct quirc_data *)malloc(sizeof(struct quirc_data));
        if (!data) {
            ESP_LOGE(TAG, "Fallo al alojar memoria para datos del QR");
            return;
        }
        
        for (int i = 0; i < count && i < 3; i++) {  // Máximo 3 QRs
            quirc_extract(q, i, &code);
            if (!quirc_decode(&code, data)) {
                char payload[256];
                int len = data->payload_len < 255 ? data->payload_len : 255;
                memcpy(payload, data->payload, len);
                payload[len] = '\0';
                
                ESP_LOGI(TAG, "QR [%d]: %s", i, payload);
                
                // Comandos
                if (strcmp(payload, "ADELANTE") == 0) {
                    ESP_LOGI(TAG, "COMANDO: AVANZAR");
                    //uart_send("uint1_t0x010")
                } else if (strcmp(payload, "ATRAS") == 0) {
                    ESP_LOGI(TAG, "COMANDO: RETROCEDER");
                } else if (strcmp(payload, "IZQUIERDA") == 0) {
                    ESP_LOGI(TAG, "COMANDO: IZQUIERDA");
                } else if (strcmp(payload, "DERECHA") == 0) {
                    ESP_LOGI(TAG, "COMANDO: DERECHA");
                }
            }

            // **CORRECCIÓN**: Ceder CPU en cada iteración para procesar múltiples QRs sin bloquear.
            vTaskDelay(1);
        }

        free(data); // Liberar la memoria del heap.
    }
}
#else
static void proceso_qr(camera_fb_t *fb) {
    static int aviso = 0;
    if (aviso++ % 100 == 0) {
        ESP_LOGW(TAG, "Librería quirc no disponible");
    }
}
#endif

static void vision_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea de visión iniciada (Core %d)", xPortGetCoreID());
    
    // Pequeño delay para dejar que WiFi inicie completamente
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    while (1) {
        if (g_current_mode == VISION_MODO_IDLE) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        
        // Obtener frame CON mutex mantenido durante todo el procesamiento
        if (xSemaphoreTake(g_cam_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        camera_fb_t *fb = esp_camera_fb_get();
        
        if (fb != NULL) {
            // Procesar MIENTRAS tenemos el mutex
            switch (g_current_mode) {
                case VISION_MODO_QR:
                    proceso_qr(fb);
                    break;
                case VISION_MODO_TRACKING:
                    break;
                case VISION_MODO_SENALES:
                    break;
                default:
                    break;
            }
            
            // Liberar frame ANTES de liberar mutex
            esp_camera_fb_return(fb);
        }
        
        // Ahora liberar mutex
        xSemaphoreGive(g_cam_mutex);
        
        // Delay para no saturar CPU (ajustable según FPS deseado)
        vTaskDelay(pdMS_TO_TICKS(100));  // Procesar a ~5-10 FPS, más responsivo
    }
}

void vision_core_init(void) {
    g_cam_mutex = xSemaphoreCreateMutex();
    if (g_cam_mutex == NULL) {
        ESP_LOGE(TAG, "Fallo al crear mutex");
        return;
    }
    
    // Crear tarea en Core 0 (APP_CPU) con stack generoso
    xTaskCreatePinnedToCore(
        vision_task,
        "VisionTask",
        16384,  // ← Aumentado de 12288 a 16384
        NULL,
        3,      // Prioridad media 
        NULL,
        1      // Core 1
    );
    
    vision_core_set_mode(VISION_MODO_QR);  // ← Iniciar en modo QR
    ESP_LOGI(TAG, "Sistema de visión inicializado");
}