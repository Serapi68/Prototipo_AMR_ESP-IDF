#ifndef XBOX_HANDLER_H
#define XBOX_HANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Inicializa la pila Bluetooth, configura el perfil HID y busca el mando Xbox.
 *        Esta función es bloqueante durante el escaneo inicial.
 */
void init_xbox(void);

#endif 
