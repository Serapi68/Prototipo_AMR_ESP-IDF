#ifndef UART_VISION_H
#define UART_VISION_H

/**
 * @brief Inicializa el puerto UART para la comunicación con el módulo de visión.
 */
void init_uart_vision(void);

/**
 * @brief Inicia una tarea de prueba que envía mensajes y loguea lo recibido.
 */
void start_vision_test(void);

#endif // UART_VISION_H