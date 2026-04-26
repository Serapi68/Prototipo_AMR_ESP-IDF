#include "qr_processor.h"
#include "esp_log.h"
#include <string.h>
#include "comunicacion_uart/uart_comm.h"
#include "driver/uart.h"

#if __has_include("quirc.h")
#include "quirc.h"
#define SOPORTE_QR 1
#else
#define SOPORTE_QR 0
#endif

static const char *TAG = "QR_PROC";

static bool g_qr_enabled = false; // Flag de control

#if SOPORTE_QR
static struct quirc *q = NULL;

void qr_set_enabled(bool enabled) {
    g_qr_enabled = enabled;
}

void qr_init(void) {
    if (q) {
        ESP_LOGW(TAG, "Quirc ya estaba inicializado");
        return;
    }
    
    q = quirc_new();
    if (!q) {
        ESP_LOGE(TAG, "No se pudo crear objeto Quirc");
        return;
    }
    
    if (quirc_resize(q, 320, 240) < 0) {
        ESP_LOGE(TAG, "Fallo al reservar memoria en Quirc");
        quirc_destroy(q);
        q = NULL;
        return;
    }
    
    ESP_LOGI(TAG, "Quirc inicializado correctamente (320x240)");
}

void qr_process_frame(camera_fb_t *fb) {
    // Si el modo QR no está activo, salir inmediatamente
    if (!g_qr_enabled) {
        return;
    }

    if (!fb || fb->len < 100) {
        ESP_LOGW(TAG, "Frame inválido o muy pequeño");
        return;
    }
    
    //  CRÍTICO: Verificar que realmente sea GRAYSCALE
    if (fb->format != PIXFORMAT_GRAYSCALE) {
        ESP_LOGW(TAG, "Frame no es GRAYSCALE (formato=%d), omitiendo", fb->format);
        return;
    }
    
    // Inicializar Quirc si es necesario
    if (!q) {
        qr_init();
    }
    if (!q) {
        ESP_LOGE(TAG, "Quirc no disponible");
        return;
    }

    int w, h;
    uint8_t *image = quirc_begin(q, &w, &h);
    if (!image) {
        ESP_LOGE(TAG, "quirc_begin falló");
        return;
    }
    
    //  Validar dimensiones
    size_t expected_size = (size_t)(w * h);
    if (fb->len < expected_size) {
        ESP_LOGW(TAG, "Frame muy pequeño: %d < %zu", fb->len, expected_size);
        quirc_end(q);
        return;
    }
    
    // Copia directa del buffer GRAYSCALE
    memcpy(image, fb->buf, expected_size);
    quirc_end(q);

    // Buscar códigos QR
    int count = quirc_count(q);
    if (count > 0) {
        ESP_LOGI(TAG, "✓ Detectados %d código(s) QR", count);
    }
    
    // Procesar hasta 3 QRs
    for (int i = 0; i < count && i < 3; i++) {
        struct quirc_code code;
        // Usar malloc para evitar desbordamiento de pila (stack overflow)
        struct quirc_data *data = (struct quirc_data *)malloc(sizeof(struct quirc_data));
        if (!data) {
            ESP_LOGE(TAG, "Fallo al reservar memoria para datos QR");
            continue;
        }
        
        quirc_extract(q, i, &code);
        
        quirc_decode_error_t err = quirc_decode(&code, data);
        if (err == 0) {
            // Copiar payload de forma segura
            char payload[256];
            int len = (data->payload_len < 255) ? data->payload_len : 255;
            memcpy(payload, data->payload, len);
            payload[len] = '\0';

            //Eliminar saltos de línea o retornos de carro que rompen la comparación
            for (int k = 0; k < len; k++) {
                if (payload[k] == '\r' || payload[k] == '\n') {
                    payload[k] = '\0';
                    break; // Cortamos el string en el primer salto de línea
                }
            }
            
            ESP_LOGI(TAG, "QR[%d]: \"%s\"", i, payload);
            
            // Heurística de proximidad: Calcular área aproximada del QR
            int min_x = 320, max_x = 0, min_y = 240, max_y = 0;
            for (int c = 0; c < 4; c++) {
                if (code.corners[c].x < min_x) min_x = code.corners[c].x;
                if (code.corners[c].x > max_x) max_x = code.corners[c].x;
                if (code.corners[c].y < min_y) min_y = code.corners[c].y;
                if (code.corners[c].y > max_y) max_y = code.corners[c].y;
            }
            int area = (max_x - min_x) * (max_y - min_y);

            // Solo procesar si el QR está lo suficientemente cerca (ajustar 3500 según pruebas)
            if (area > 3500) {
                uint8_t cmd = 0;
                if      (strcmp(payload, "DERECHA") == 0)                cmd = 0x11;
                else if (strcmp(payload, "IZQUIERDA") == 0)              cmd = 0x12;
                else if (strcmp(payload, "IZQUIERDA_HACIA_ATRAS") == 0)  cmd = 0x13;
                else if (strcmp(payload, "DERECHA_HACIA_ADELANTE") == 0) cmd = 0x14;
                else if (strcmp(payload, "STOP") == 0)                   cmd = 0x15;
                else if (strcmp(payload, "FIN_TRAYECTO") == 0)           cmd = 0x16;

                if (cmd != 0) {
                    ESP_LOGI(TAG, "Enviando Comando Seguro: 0x%02X (Area: %d)", cmd, area);
                    // Enviar paquete completo de una vez
                    uint8_t packet[3] = {0xAA, cmd, (uint8_t)~cmd};
                    uart_write_bytes(UART_NUM_1, (const char*)packet, 3);
                }
                else {
                    ESP_LOGI(TAG, "Código QR leído: %s (Sin acción)", payload);
                }
            } else {
                ESP_LOGD(TAG, "QR detectado pero muy lejos (Area: %d)", area);
            }
        }
        
        free(data); // Liberar memoria
    }
}

#else
// Si quirc no está disponible
void qr_init(void) {
    ESP_LOGW(TAG, "Librería Quirc no está disponible");
}

void qr_process_frame(camera_fb_t *fb) {
    static int aviso = 0;
    if (aviso++ % 100 == 0) {
        ESP_LOGW(TAG, "Librería quirc no disponible - no se pueden procesar QR");
    }
}
#endif