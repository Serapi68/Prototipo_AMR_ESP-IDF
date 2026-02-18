#include "maquina_estado.h"
#include "robot_config.h"
#include "kinematic.h"
#include "motor_driver.h"
#include "modo_autonomo/auto_sensores.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "comunicacion_vision/uart_vision.h"

static const char *TAG = "MAQUINA_ESTADO";

// Definición de la cola global
QueueHandle_t g_xbox_queue;

// Variable local para mantener el modo de operación actual
static modo_operacion_t g_current_mode = MODO_MANUAL;
static uint8_t g_last_vision_cmd = 0; // Para evitar envíos repetitivos por UART

/**
 * @brief Tarea principal de control del robot.
 * Espera datos del mando Xbox y actúa según el modo seleccionado.
 */
void control_task(void *pvParameters)
{
    xbox_data_t controller_data;
    uint8_t last_button_lb_state = 0;
    uint8_t last_button_rb_state = 0;

    const char* mode_names[] = {"MANUAL", "AUTONOMO_SENSORES", "AUTONOMO_VISION"};
    bool datos_nuevos_mando = false;

    while (1) {
        // Esperar datos del mando con un timeout. Si no llegan, la tarea se desbloquea igualmente.
        if (xQueueReceive(g_xbox_queue, &controller_data, pdMS_TO_TICKS(20)) == pdPASS) {
            datos_nuevos_mando = true;
        } else {
            datos_nuevos_mando = false;
        }

        // La lógica de cambio de modo solo se ejecuta si hay datos nuevos del mando
        if (datos_nuevos_mando) {
            // --- Lógica de Cambio de Modos (LB / RB) ---
            if (controller_data.boton_rb && !last_button_rb_state) {
                g_current_mode = (g_current_mode + 1) % 3; 
                ESP_LOGI(TAG, "Modo Siguiente -> %s", mode_names[g_current_mode]);
                actualizar_movimiento(0, 0); // Detener robot al cambiar de modo
                
                // Si NO estamos en modo visión, asegurar que la cámara esté en Streaming
                if (g_current_mode != MODO_AUTONOMO_VISION) {
                    uart_vision_send_cmd(CMD_STREAM_ONLY);
                    g_last_vision_cmd = CMD_STREAM_ONLY; // Actualizar estado para evitar desincronización
                }
            }
            last_button_rb_state = controller_data.boton_rb;

            if (controller_data.boton_lb && !last_button_lb_state) {
                if (g_current_mode == 0) g_current_mode = 2; // Vuelta al final
                else g_current_mode--;
                ESP_LOGI(TAG, "Modo Anterior -> %s", mode_names[g_current_mode]);
                actualizar_movimiento(0, 0); // Detener robot al cambiar de modo

                // Si NO estamos en modo visión, asegurar que la cámara esté en Streaming
                if (g_current_mode != MODO_AUTONOMO_VISION) {
                    uart_vision_send_cmd(CMD_STREAM_ONLY);
                    g_last_vision_cmd = CMD_STREAM_ONLY; // Actualizar estado
                }
            }
            last_button_lb_state = controller_data.boton_lb;
        }

        // --- Ejecución del Modo Actual (se ejecuta en cada ciclo de la tarea) ---
        switch (g_current_mode) {
            case MODO_MANUAL:
                // Solo actualiza el movimiento si hay datos nuevos del mando
                if (datos_nuevos_mando) {
                    float potencia = controller_data.trigger_rt - controller_data.trigger_lt;
                    actualizar_movimiento(potencia, controller_data.stick_izq_x);
                    set_led(controller_data.boton_b); 
                }
                break;

            case MODO_AUTONOMO_SENSORES:
                auto_sensores_run(); // Ejecutar un ciclo de la lógica autónoma
                break;
                
            case MODO_AUTONOMO_VISION:
                actualizar_movimiento(0, 0); // Placeholder: Detener el robot (a implementar)
                
                // --- LÓGICA DE RECEPCIÓN DE COMANDOS DE VISIÓN ---
                uint8_t cmd_recibido;
                // Revisamos si hay algo en la cola (sin bloquear, wait=0)
                if (g_vision_queue != NULL && xQueueReceive(g_vision_queue, &cmd_recibido, 0) == pdTRUE) {
                    ESP_LOGI(TAG, "Procesando comando de visión: 0x%02X", cmd_recibido);
                    
                    // Aquí implementaremos la lógica de movimiento más adelante
                    switch(cmd_recibido) {
                        case VISION_CMD_ADELANTE:
                            ESP_LOGI(TAG, "ACCION: Mover ADELANTE");
                            break;
                        case VISION_CMD_ATRAS:
                            ESP_LOGI(TAG, "ACCION: Mover ATRAS");
                            break;
                        case VISION_CMD_IZQUIERDA:
                            ESP_LOGI(TAG, "ACCION: Mover IZQUIERDA");
                            break;
                        case VISION_CMD_DERECHA:
                            ESP_LOGI(TAG, "ACCION: Mover DERECHA");
                            break;
                    }
                }

                if (datos_nuevos_mando) {
                    uint8_t cmd = 0;
                    if (controller_data.boton_a) cmd = CMD_STREAM_ONLY;
                    else if (controller_data.boton_b) cmd = CMD_QR_READER;
                    else if (controller_data.boton_x) cmd = CMD_OBJECT_TRACKER;
                    else if (controller_data.boton_y) cmd = CMD_SIGN_READER;

                    // Enviar solo si se presionó un botón y es diferente al último enviado
                    // para no saturar el puerto UART
                    if (cmd != 0 && cmd != g_last_vision_cmd) {
                        uart_vision_send_cmd(cmd);
                        g_last_vision_cmd = cmd;
                    }
                }
                break;
        }
    }
}

void init_maquina_estado(void)
{
    ESP_LOGI(TAG, "Inicializando Maquina de Estados...");

    // Inicializar los módulos de los modos de operación
    auto_sensores_init();

    // Inicializar UART para visión
    uart_vision_init();

    // 1. Crear la cola para los datos del mando
    g_xbox_queue = xQueueCreate(1, sizeof(xbox_data_t));

    // 2. Crear la tarea de control
    xTaskCreate(control_task, "control_task", STACK_SIZE_MOTORES, NULL, PRIORIDAD_MOTORES, NULL);
}