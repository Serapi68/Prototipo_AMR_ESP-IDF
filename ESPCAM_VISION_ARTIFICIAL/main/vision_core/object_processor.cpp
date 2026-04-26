#include "object_processor.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "vision_core/tracking_config.h"
#include "comunicacion_uart/uart_comm.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include <cstring>  // Para memcpy

static const char *TAG = "OBJ_TRACKER";

// Puntero temporal para que el callback de Edge Impulse acceda al frame actual
static camera_fb_t *current_fb = NULL;

/**
 * Callback de Edge Impulse: Lee directamente de la cámara y empaqueta en 0xRRGGBB
 */
static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    if (!current_fb || !current_fb->buf) {
        return -1;
    }

    // Factores de escala para mapear 96x96 -> 320x240
    // Suponiendo que TRACK_IMG_WIDTH = 320 y TRACK_IMG_HEIGHT = 240
    const float scale_x = 320.0f / EI_CLASSIFIER_INPUT_WIDTH;
    const float scale_y = 240.0f / EI_CLASSIFIER_INPUT_HEIGHT;

    for (size_t i = 0; i < length; i++) {
        // Determinamos qué pixel (x, y) del modelo 96x96 nos están pidiendo
        size_t out_idx = offset + i;
        int x_model = out_idx % EI_CLASSIFIER_INPUT_WIDTH;
        int y_model = out_idx / EI_CLASSIFIER_INPUT_WIDTH;

        // Mapeamos al pixel correspondiente en la imagen de 320x240
        int x_fb = (int)(x_model * scale_x);
        int y_fb = (int)(y_model * scale_y);
        
        uint8_t gray = current_fb->buf[y_fb * 320 + x_fb];
        
        // Lo empaquetamos como si fuera RGB (R=gray, G=gray, B=gray) -> 0xRRGGBB
        uint32_t rgb_val = (gray << 16) | (gray << 8) | gray;
        
        // Edge Impulse espera este entero RGB casteado a float
        out_ptr[i] = (float)rgb_val;
    }
    return 0;
}

extern "C" void object_tracking_process_frame(camera_fb_t *fb) {
    if (!fb) return;
    
    // Validar tamaño: Ahora esperamos QVGA (320x240) = 76,800 bytes
    size_t expected_size = 320 * 240; 
    if (fb->len != expected_size) {
        ESP_LOGE(TAG, "ERROR: FOV Ampliado requiere 320x240. Esperado: %d, Recibido: %d", 
                 expected_size, fb->len);
        return;
    }
    
    // 0. Asignar el frame actual para que 'get_signal_data' lo pueda leer
    current_fb = fb;

    // Log de depuración una vez cada 10 frames para no saturar
    static int frame_count = 0;

    // ==========================================
    // DIAGNÓSTICO VISUAL (Ojo del Robot)
    // ==========================================
    if (frame_count++ % 15 == 0) {
        // Pixel central de 96x96: (48, 48)
        size_t center_idx = (48 * TRACK_IMG_WIDTH + 48);
        uint8_t gray_val = fb->buf[center_idx];
        float intensity = (float)gray_val / 255.0f;
        
        ESP_LOGW("SENSOR_CHECK", "Intensidad Gris Centro: %.2f", intensity);
        
        if (intensity > 0.90f) {
            ESP_LOGE("SENSOR_CHECK", "¡IMAGEN QUEMADA! Demasiada luz.");
        } else if (intensity < 0.10f) {
            ESP_LOGE("SENSOR_CHECK", "¡IMAGEN MUY OSCURA!");
        }
    }
    // ==========================================

    // 1. Preparar la señal para Edge Impulse
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &get_signal_data;

    // 2. Correr la inferencia
    esp_task_wdt_reset();

    int64_t start_time = esp_timer_get_time();
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR ei_error = run_classifier(&signal, &result, false);
    int64_t end_time = esp_timer_get_time();

    // Importante: Limpiamos el puntero por seguridad cuando termine la inferencia
    current_fb = NULL;

    float inference_time = (end_time - start_time) / 1000.0f;  // Convertir a ms
    
    ESP_LOGI(TAG, "Inferencia: %.1f ms. BBoxes: %d | Anomaly: %.2f", 
             inference_time, result.bounding_boxes_count, result.anomaly);

    if (ei_error != EI_IMPULSE_OK) {
        ESP_LOGE(TAG, "Error en inferencia (%d)", ei_error);
        return;
    }

    // DEBUG: Mostrar todas las detecciones incluso las de baja confianza
    // IMPORTANTE: El modelo FOMO filtra por threshold=0.5 antes del postprocessing
    static int debug_frame_count = 0;
    bool show_debug = (debug_frame_count++ % 10 == 0);  // Debug cada 10 frames
    
    if (result.bounding_boxes_count > 0) {
        for (size_t i = 0; i < result.bounding_boxes_count; i++) {
            ESP_LOGI(TAG, "  [%d] Label: %s | Conf: %.3f | X:%d Y:%d W:%d H:%d", 
                     i, result.bounding_boxes[i].label, result.bounding_boxes[i].value,
                     result.bounding_boxes[i].x, result.bounding_boxes[i].y,
                     result.bounding_boxes[i].width, result.bounding_boxes[i].height);
        }
    } else {
        if (show_debug) {
            ESP_LOGW(TAG, "Sin detecciones. Anomaly=%.3f | Buscando configuración FOMO...", 
                     result.anomaly);
        } else {
            ESP_LOGW(TAG, "Sin detecciones en este frame");
        }
    }

    // 3. Analizar resultados de FOMO (Detección de centroides)
    bool found = false;
    uint8_t cmd_to_send = 0;
    float max_conf = 0;
    int best_x = 0;
    static uint8_t last_sent_cmd = 0;

    for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {
        auto bb = result.bounding_boxes[ix];
        if (bb.value >= TRACK_CONFIDENCE_THRESHOLD) {
            if (bb.value > max_conf) {
                max_conf = bb.value;
                // En FOMO, x e y son el centro del objeto detectado
                best_x = bb.x + (bb.width / 2);
                found = true;
            }
        }
    }

    // 4. Lógica de decisión basada en Deadzone
    if (found) {
        // El centro del modelo 96x96 es 48
        const int model_center_x = EI_CLASSIFIER_INPUT_WIDTH / 2;
        
        ESP_LOGI(TAG, ">> SEGUIMIENTO: X=%d | Conf=%.2f %% | Threshold=%.2f", 
                 best_x, max_conf * 100, TRACK_CONFIDENCE_THRESHOLD);

        if (best_x < (model_center_x - (TRACK_DEADZONE_X / 2))) { // Ajuste proporcional de deadzone
            cmd_to_send = CMD_TRACK_IZQ;
        } else if (best_x > (model_center_x + (TRACK_DEADZONE_X / 2))) {
            cmd_to_send = CMD_TRACK_DER;
        } else {
            cmd_to_send = CMD_TRACK_AVANCE;
        }
    } else {
        if (show_debug) {
            ESP_LOGW(TAG, "OBJETO PERDIDO. Búsqueda activa. Threshold=%.2f", TRACK_CONFIDENCE_THRESHOLD);
        } else {
            ESP_LOGW(TAG, "!! OBJETO PERDIDO (Buscando...)");
        }
        cmd_to_send = CMD_TRACK_STOP; // Failsafe: Parar si se pierde
    }

    // 5. Envío UART seguro (Solo si el comando cambió para no saturar)
    if (cmd_to_send != 0 && cmd_to_send != last_sent_cmd) {
        uint8_t packet[3] = {0xAA, cmd_to_send, (uint8_t)~cmd_to_send};
        uart_write_bytes(UART_NUM_1, (const char*)packet, 3);
        last_sent_cmd = cmd_to_send;
    }
}