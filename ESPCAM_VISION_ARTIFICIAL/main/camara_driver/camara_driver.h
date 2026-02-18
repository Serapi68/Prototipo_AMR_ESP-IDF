#ifndef CAMARA_DRIVER_H
#define CAMARA_DRIVER_H

#include "esp_camera.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t init_camera(void);
esp_err_t camera_set_format_grayscale_qvga(void);
esp_err_t camera_set_format_jpeg_vga(void);

/**
 * @brief Captura un frame de la cámara (wrapper de esp_camera_fb_get)
 * @return Puntero al framebuffer o NULL si falla
 */
camera_fb_t* camara_capture(void);

/**
 * @brief Devuelve el frame a la cámara (wrapper de esp_camera_fb_return)
 * @param fb Puntero al framebuffer
 */
void camara_return_fb(camera_fb_t * fb);

#endif