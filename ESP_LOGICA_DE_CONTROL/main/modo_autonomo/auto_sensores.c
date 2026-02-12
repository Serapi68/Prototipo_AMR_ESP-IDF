#include "auto_sensores.h"
#include "robot_config.h"
#include "hc_sr04.h"
#include "motor_driver.h"
#include "kinematic.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const char *TAG = "AUTO_SENSORES";

// Estados internos para la lógica de evasión de obstáculos
typedef enum {
    AVANZANDO,
    DETENIDO,
    INICIAR_ESCANEO_DERECHA,
    ESPERANDO_SERVO_DERECHA,
    INICIAR_ESCANEO_IZQUIERDA,
    ESPERANDO_SERVO_IZQUIERDA,
    DECIDIR_GIRO,
    RETROCEDER,
    GIRANDO
} estado_autonomo_t;

static estado_autonomo_t estado_actual = AVANZANDO;
static int64_t tiempo_inicio_maniobra = 0; // Para controlar tiempos de servo, retroceso y giro
static float dist_izq_guardada = 0;
static float dist_der_guardada = 0;

void auto_sensores_init(void) {
    // Al iniciar el modo, empezamos en el estado de avanzar
    estado_actual = AVANZANDO;
    ESP_LOGI(TAG, "Módulo autónomo por sensores inicializado.");
}

void auto_sensores_run(void) {
    float dist_frontal;
    static int proxima_direccion = 0; // 1 para derecha, -1 para izquierda

    switch (estado_actual) {
        case AVANZANDO:
            // 1. Poner servo sensor al frente y avanzar
            set_servo_sensor_angle(ANGULO_SENSOR_CENTRO);
            actualizar_movimiento(VELOCIDAD_AUTONOMO, 0); // Avanza a velocidad constante

            // 2. Medir distancia frontal
            dist_frontal = hc_sr04_get_distance_cm();

            // 3. Comprobar si hay obstáculo
            if (dist_frontal > 0 && dist_frontal < DISTANCIA_MINIMA_CM) {
                ESP_LOGI(TAG, "¡Obstáculo a %.1f cm! Deteniendo.", dist_frontal);
                actualizar_movimiento(0, 0); // Detener motores
                estado_actual = DETENIDO;
            }
            break;

        case DETENIDO:
            // Pequeña pausa para estabilizar antes de escanear
            vTaskDelay(pdMS_TO_TICKS(TIEMPO_PAUSA_DETENIDO_MS));
            ESP_LOGI(TAG, "Escaneando entorno...");
            estado_actual = INICIAR_ESCANEO_DERECHA;
            break;

        case INICIAR_ESCANEO_DERECHA:
            set_servo_sensor_angle(ANGULO_SENSOR_DERECHA);
            tiempo_inicio_maniobra = esp_timer_get_time(); // Iniciar temporizador para el servo
            estado_actual = ESPERANDO_SERVO_DERECHA;
            break;

        case ESPERANDO_SERVO_DERECHA:
            // Esperar un tiempo no bloqueante para que el servo llegue
            if ((esp_timer_get_time() - tiempo_inicio_maniobra) / 1000 > TIEMPO_ESPERA_SERVO_MS) {
                dist_der_guardada = hc_sr04_get_distance_cm();
                estado_actual = INICIAR_ESCANEO_IZQUIERDA;
            }
            break;

        case INICIAR_ESCANEO_IZQUIERDA:
            set_servo_sensor_angle(ANGULO_SENSOR_IZQUIERDA);
            tiempo_inicio_maniobra = esp_timer_get_time(); // Reiniciar temporizador para el servo
            estado_actual = ESPERANDO_SERVO_IZQUIERDA;
            break;

        case ESPERANDO_SERVO_IZQUIERDA:
            // Esperar un tiempo no bloqueante para que el servo llegue
            if ((esp_timer_get_time() - tiempo_inicio_maniobra) / 1000 > TIEMPO_ESPERA_SERVO_MS) {
                dist_izq_guardada = hc_sr04_get_distance_cm();
                estado_actual = DECIDIR_GIRO;
            }
            break;
        
        case DECIDIR_GIRO:
            ESP_LOGI(TAG, "Distancia Derecha: %.1f cm, Izquierda: %.1f cm", dist_der_guardada, dist_izq_guardada);
            // Decidir dirección
            proxima_direccion = (dist_izq_guardada > dist_der_guardada) ? -1 : 1;
            ESP_LOGI(TAG, "Decisión: Girar a la %s.", (proxima_direccion == -1) ? "IZQUIERDA" : "DERECHA");
            
            estado_actual = RETROCEDER;
            tiempo_inicio_maniobra = esp_timer_get_time(); // Iniciar temporizador para la maniobra de retroceso
            break;

        case RETROCEDER:
            actualizar_movimiento(VELOCIDAD_RETROCESO, 0); // Retroceder a un 40% de potencia
            
            // Esperar un tiempo fijo para retroceder
            if ((esp_timer_get_time() - tiempo_inicio_maniobra) / 1000 > TIEMPO_RETROCESO_MS) {
                ESP_LOGI(TAG, "Retroceso completado. Girando...");
                estado_actual = GIRANDO;
                tiempo_inicio_maniobra = esp_timer_get_time(); // Reiniciar temporizador
            }
            break;

        case GIRANDO:
            // Girar en la dirección decidida
            actualizar_movimiento(VELOCIDAD_AUTONOMO, proxima_direccion); // Avanza y gira

            // Esperar un tiempo fijo para girar
            if ((esp_timer_get_time() - tiempo_inicio_maniobra) / 1000 > TIEMPO_GIRO_MS) {
                ESP_LOGI(TAG, "Giro completado. Volviendo a avanzar.");
                estado_actual = AVANZANDO;
            }
            break;
    }
}