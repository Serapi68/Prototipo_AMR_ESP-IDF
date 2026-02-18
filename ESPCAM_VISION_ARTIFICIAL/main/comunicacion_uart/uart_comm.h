#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdint.h>

/**
 * @brief Inicializa el control por UART para recibir comandos
 */
void uart_control_init(void);

/**
 * @brief Envía un byte por UART (para respuestas de QR)
 * @param data Byte a enviar
 */
void uart_send_byte(uint8_t data);

#endif // UART_COMM_H