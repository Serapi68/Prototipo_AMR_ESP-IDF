#ifndef QR_PROCESSOR_H
#define QR_PROCESSOR_H

#include "esp_camera.h"

/**
 * @brief Inicializa el procesador de códigos QR
 */
void qr_init(void);

/**
 * @brief Procesa un frame en busca de códigos QR
 * @param fb Framebuffer a procesar
 */
void qr_process_frame(camera_fb_t *fb);

#endif // QR_PROCESSOR_H