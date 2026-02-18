#ifndef VISION_CORE_H
#define VISION_CORE_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef enum {
    VISION_MODO_STREAMING = 0,  // Streaming de video (Manual/Auto por sensores)
    VISION_MODO_QR = 1,         // Lector de códigos QR
    VISION_MODO_TRACKING = 2,   // Seguidor de objetos
    VISION_MODO_SENALES = 3     // Lector de señales de tránsito
} vision_mode_t;

// Mutex global para sincronización de acceso a la cámara
extern SemaphoreHandle_t g_cam_mutex;

/**
 * @brief Inicializa el sistema de visión
 */
void vision_core_init(void);

/**
 * @brief Cambia el modo de operación del sistema de visión
 * @param mode Modo objetivo
 */
void vision_core_set_mode(vision_mode_t mode);

/**
 * @brief Obtiene el modo actual de operación
 * @return Modo actual
 */
vision_mode_t vision_core_get_mode(void);

#endif // VISION_CORE_H