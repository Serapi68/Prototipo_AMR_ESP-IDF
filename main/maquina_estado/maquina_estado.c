#include "maquina_estado.h"
#include "robot_config.h"
#include "kinematic.h"
#include "motor_driver.h"
#include "modo_autonomo/auto_sensores.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "MAQUINA_ESTADO";

// Definición de la cola global
QueueHandle_t g_xbox_queue;

// Variable local para mantener el modo de operación actual
static modo_operacion_t g_current_mode = MODO_MANUAL;

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
            }
            last_button_rb_state = controller_data.boton_rb;

            if (controller_data.boton_lb && !last_button_lb_state) {
                if (g_current_mode == 0) g_current_mode = 2; // Vuelta al final
                else g_current_mode--;
                ESP_LOGI(TAG, "Modo Anterior -> %s", mode_names[g_current_mode]);
                actualizar_movimiento(0, 0); // Detener robot al cambiar de modo
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
                break;
        }
    }
}

void init_maquina_estado(void)
{
    ESP_LOGI(TAG, "Inicializando Maquina de Estados...");

    // Inicializar los módulos de los modos de operación
    auto_sensores_init();

    // 1. Crear la cola para los datos del mando
    g_xbox_queue = xQueueCreate(1, sizeof(xbox_data_t));

    // 2. Crear la tarea de control
    xTaskCreate(control_task, "control_task", STACK_SIZE_MOTORES, NULL, PRIORIDAD_MOTORES, NULL);
}