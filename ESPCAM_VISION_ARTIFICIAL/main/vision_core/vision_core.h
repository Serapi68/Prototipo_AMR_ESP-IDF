#ifndef VISION_CORE_H
#define VISION_CORE_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t g_cam_mutex; // Declaración del semáforo para sincronización
//Modos de operacion

typedef enum {
    VISION_MODO_IDLE = 0,
    VISION_MODO_QR,
    VISION_MODO_TRACKING,
    VISION_MODO_SENALES,
} vision_mode_t;
//Inicializa el sistema de vision
void vision_core_init();

//Cambia el modo de operacion del sistema de vision
void vision_core_set_mode(vision_mode_t mode);

#endif