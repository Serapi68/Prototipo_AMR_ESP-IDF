#ifndef HTTP_STREAM_H
#define HTTP_STREAM_H

#include "esp_err.h"

/**
 * @brief Inicia el punto de acceso WiFi
 */
void start_wifi(void);

/**
 * @brief Inicia el servidor web HTTP para streaming
 * @return ESP_OK si tuvo éxito
 */
esp_err_t start_webserver(void);

#endif // HTTP_STREAM_H