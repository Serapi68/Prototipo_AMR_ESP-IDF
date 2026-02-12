#ifndef MAQUINA_ESTADO_H
#define MAQUINA_ESTADO_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Cola global para recibir datos del mando.
 * Definida en maquina_estado.c, utilizada por xbox_handler.c
 */
extern QueueHandle_t g_xbox_queue;

/**
 * @brief Inicializa la máquina de estados.
 * Crea la cola de mensajes y lanza la tarea de control (control_task).
 */
void init_maquina_estado(void);

#endif // MAQUINA_ESTADO_H