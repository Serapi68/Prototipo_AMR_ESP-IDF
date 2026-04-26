#ifndef UART_VISION_H
#define UART_VISION_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Comandos enviados A la ESP32-CAM (Control -> Camara)
#define CMD_STREAM_ONLY    0x10
#define CMD_QR_READER      0x20
#define CMD_OBJECT_TRACKER 0x30
#define CMD_SIGN_READER    0x40

// Comandos recibidos DE la ESP32-CAM (Camara -> Control)
#define VISION_CMD_DERECHA                0x11
#define VISION_CMD_IZQUIERDA              0x12
#define VISION_CMD_IZQUIERDA_HACIA_ATRAS  0x13
#define VISION_CMD_DERECHA_HACIA_ADELANTE 0x14
#define VISION_CMD_STOP                   0x15
#define VISION_CMD_FIN_TRAYECTO           0x16

// Comandos específicos para Tracking (Ajuste fino)
#define VISION_CMD_TRACK_IZQ              0x31
#define VISION_CMD_TRACK_DER              0x32
#define VISION_CMD_TRACK_AVANCE           0x33

// Cola global para recibir comandos de visión
extern QueueHandle_t g_vision_queue;

// Inicializa el UART y la tarea de recepción
void uart_vision_init(void);

// Envía un comando a la cámara
void uart_vision_send_cmd(uint8_t cmd);

#endif