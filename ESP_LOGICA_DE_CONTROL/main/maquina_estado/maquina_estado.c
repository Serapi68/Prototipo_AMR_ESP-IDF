#include "maquina_estado.h"
#include "robot_config.h"
#include "kinematic.h"
#include "motor_driver.h"
#include "modo_autonomo/auto_sensores.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "comunicacion_vision/uart_vision.h"
#include "modo_autonomo/hc_sr04.h"
#include "esp_timer.h"

static const char *TAG = "MAQUINA_ESTADO";

// Sub-estados para la navegación por visión
typedef enum {
    V_BUSCANDO_QR,
    V_EJECUTANDO_MANIOBRA,
    V_ESPERA_5S,
    V_FINALIZADO,
    V_TRACKING_OBJETO
} vision_nav_state_t;

// Definición de la cola global
QueueHandle_t g_xbox_queue;

// Variable local para mantener el modo de operación actual
static modo_operacion_t g_current_mode = MODO_MANUAL;
static uint8_t g_last_vision_cmd = 0; // Para evitar envíos repetitivos por UART
static vision_nav_state_t v_state = V_BUSCANDO_QR;
static int64_t v_timer_start = 0;
static uint8_t v_active_cmd = 0;

/**
 * @brief Tarea principal de control del robot.
 * Espera datos del mando Xbox y actúa según el modo seleccionado.
 */
void control_task(void *pvParameters) {
    xbox_data_t controller_data = {0};
    uint8_t last_button_lb_state = 0;
    uint8_t last_button_rb_state = 0;
    uint8_t last_button_select_state = 0;
    bool motor_running = true;

    const char* mode_names[] = {"MANUAL", "AUTONOMO_SENSORES", "AUTONOMO_VISION"};
    bool datos_nuevos_mando = false;

    while (1) {
        // --- 0. Recepción y LOG Global de Visión (Cámara -> Control) ---
        // Leemos aquí para que siempre se vacíe la cola y se muestre en el monitor
        uint8_t cmd_rx_global;
        bool hay_comando_vision = false;
        if (g_vision_queue != NULL) {
            hay_comando_vision = (xQueueReceive(g_vision_queue, &cmd_rx_global, 0) == pdTRUE);
        }
        
        if (hay_comando_vision) {
            ESP_LOGI(TAG, ">>> UART CAM -> CONTROL RECIBIDO: 0x%02X", cmd_rx_global);
        }

        // Esperar datos del mando con un timeout. Si no llegan, la tarea se desbloquea igualmente.
        if (xQueueReceive(g_xbox_queue, &controller_data, pdMS_TO_TICKS(20)) == pdPASS) {
            datos_nuevos_mando = true;
        } else {
            datos_nuevos_mando = false;
        }

        // La lógica de cambio de modo solo se ejecuta si hay datos nuevos del mando
        if (datos_nuevos_mando) {
            // --- Lógica de Standby (Select) ---
            if (controller_data.boton_select && !last_button_select_state) {
                motor_running = !motor_running;
                set_motor_standby(motor_running);
                ESP_LOGI(TAG, "Motor Driver STBY: %s", motor_running ? "HIGH (RUN)" : "LOW (STANDBY)");
            }
            last_button_select_state = controller_data.boton_select;

            // --- Lógica de Cambio de Modos (LB / RB) ---
            if (controller_data.boton_rb && !last_button_rb_state) {
                g_current_mode = (g_current_mode + 1) % 3; 
                ESP_LOGI(TAG, "Modo Siguiente -> %s", mode_names[g_current_mode]);
                
                // Resetear estado de Standby al cambiar de modo para evitar confusiones
                motor_running = true;
                set_motor_standby(true);

                actualizar_movimiento(0, 0); // Detener robot al cambiar de modo
                
                // Sincronizar Cámara
                if (g_current_mode == MODO_AUTONOMO_VISION) {
                    uart_vision_send_cmd(CMD_QR_READER);
                    g_last_vision_cmd = CMD_QR_READER;
                    v_state = V_BUSCANDO_QR;
                } else {
                    uart_vision_send_cmd(CMD_STREAM_ONLY);
                    g_last_vision_cmd = CMD_STREAM_ONLY;
                }
            }
            last_button_rb_state = controller_data.boton_rb;

            if (controller_data.boton_lb && !last_button_lb_state) {
                if (g_current_mode == 0) g_current_mode = 2; // Vuelta al final
                else g_current_mode--;
                ESP_LOGI(TAG, "Modo Anterior -> %s", mode_names[g_current_mode]);

                // Resetear estado de Standby al cambiar de modo para evitar confusiones
                motor_running = true;
                set_motor_standby(true);

                actualizar_movimiento(0, 0); // Detener robot al cambiar de modo

                // Limpiar cola de visión al cambiar de modo para evitar comandos residuales
                uint8_t dummy;
                while(xQueueReceive(g_vision_queue, &dummy, 0));
                ESP_LOGD(TAG, "Cola de visión limpiada.");

                // Sincronizar Cámara
                if (g_current_mode == MODO_AUTONOMO_VISION) {
                    uart_vision_send_cmd(CMD_QR_READER);
                    g_last_vision_cmd = CMD_QR_READER;
                    v_state = V_BUSCANDO_QR;
                } else {
                    uart_vision_send_cmd(CMD_STREAM_ONLY);
                    g_last_vision_cmd = CMD_STREAM_ONLY;
                }
            }
            last_button_lb_state = controller_data.boton_lb;
        }

        // --- 1. Medición de distancia de seguridad (Una sola vez por ciclo) ---
        float distancia_seguridad = hc_sr04_get_distance_cm();

        // --- Ejecución del Modo Actual (se ejecuta en cada ciclo de la tarea) ---
        switch (g_current_mode) {
            case MODO_MANUAL:
            {
                float potencia = controller_data.trigger_rt - controller_data.trigger_lt;

                // Freno de seguridad: Solo bloquea el avance (potencia > 0)
                if (distancia_seguridad > 0 && distancia_seguridad < DISTANCIA_MINIMA_CM && potencia > 0) {
                    potencia = 0; 
                }

                actualizar_movimiento(potencia, controller_data.stick_izq_x);
                set_led(controller_data.boton_b); 
            }
            break;

            case MODO_AUTONOMO_SENSORES:
                auto_sensores_run(distancia_seguridad); // Pasar la distancia ya medida
                break;
                
            case MODO_AUTONOMO_VISION:
            {
                // 1. Gestión de sub-modos (Mando -> Cámara)
                if (datos_nuevos_mando) {
                    uint8_t req_cmd = 0;
                    if      (controller_data.boton_a) req_cmd = CMD_STREAM_ONLY;
                    else if (controller_data.boton_b) req_cmd = CMD_QR_READER;
                    else if (controller_data.boton_x) req_cmd = CMD_OBJECT_TRACKER;
                    else if (controller_data.boton_y) req_cmd = CMD_SIGN_READER;

                    if (req_cmd != 0 && req_cmd != g_last_vision_cmd) {
                        uart_vision_send_cmd(req_cmd);
                        g_last_vision_cmd = req_cmd;
                        if (req_cmd == CMD_QR_READER)      v_state = V_BUSCANDO_QR;
                        else if (req_cmd == CMD_OBJECT_TRACKER) v_state = V_TRACKING_OBJETO;
                    }
                }

                // 2. Ejecución de la máquina de estados de navegación (usando el comando leído arriba)
                switch (v_state) {
                        case V_BUSCANDO_QR:
                            if (distancia_seguridad > 0 && distancia_seguridad < DISTANCIA_MINIMA_CM) {
                                actualizar_movimiento(0, 0); // Freno de seguridad en modo visión
                            } else {
                                actualizar_movimiento(0.15, 0); 
                            }
                            set_led(true);
                            
                            if (hay_comando_vision && cmd_rx_global >= VISION_CMD_DERECHA && cmd_rx_global <= VISION_CMD_FIN_TRAYECTO) {
                                ESP_LOGI(TAG, "Iniciando maniobra para comando: 0x%02X", cmd_rx_global);
                                v_active_cmd = cmd_rx_global;
                                v_timer_start = esp_timer_get_time();
                                if      (cmd_rx_global == VISION_CMD_STOP) v_state = V_ESPERA_5S;
                                else if (cmd_rx_global == VISION_CMD_FIN_TRAYECTO) v_state = V_FINALIZADO;
                                else                                               v_state = V_EJECUTANDO_MANIOBRA;
                            }
                            break;

                        case V_TRACKING_OBJETO:
                            // Manejo de comandos de la IA de seguimiento
                            if (hay_comando_vision && cmd_rx_global >= 0x31 && cmd_rx_global <= 0x33) {
                                float v_speed = (cmd_rx_global == VISION_CMD_TRACK_AVANCE) ? 0.20 : 0.0;
                                float v_turn = (cmd_rx_global == VISION_CMD_TRACK_IZQ) ? -0.4 : (cmd_rx_global == VISION_CMD_TRACK_DER ? 0.4 : 0.0);
                                actualizar_movimiento(v_speed, v_turn);
                            }
                            break;

                        case V_EJECUTANDO_MANIOBRA:
                            set_led(false);
                            // Aplicar lógica de dirección + motores
                            if      (v_active_cmd == VISION_CMD_DERECHA)                actualizar_movimiento(0.35, 1.0);
                            else if (v_active_cmd == VISION_CMD_IZQUIERDA)              actualizar_movimiento(0.35, -1.0);
                            else if (v_active_cmd == VISION_CMD_IZQUIERDA_HACIA_ATRAS)  actualizar_movimiento(-0.35, -0.7);
                            else if (v_active_cmd == VISION_CMD_DERECHA_HACIA_ADELANTE) actualizar_movimiento(0.40, 0.5);
                            
                            if ((esp_timer_get_time() - v_timer_start) > 1800000) {
                                v_state = V_BUSCANDO_QR;
                            }
                            break;

                        case V_ESPERA_5S:
                            actualizar_movimiento(0, 0);
                            if ((esp_timer_get_time() - v_timer_start) > 5000000) v_state = V_BUSCANDO_QR;
                            break;

                        case V_FINALIZADO:
                            actualizar_movimiento(0, 0);
                            set_led(false);
                            break;
                    }
            }
                break;
        }
        // Pequeño delay para que FreeRTOS alimente el Watchdog
        vTaskDelay(pdMS_TO_TICKS(1));
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