#ifndef CAMARA_DRIVER_H
#define CAMARA_DRIVER_H

#include "esp_err.h"

/**
 * @brief Inicializa la cámara OV2640 con configuración QVGA.
 * @return ESP_OK si tiene éxito.
 */
esp_err_t init_camera(void);

#endif