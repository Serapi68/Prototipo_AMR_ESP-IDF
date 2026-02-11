#include "hc_sr04.h"
#include "robot_config.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "rom/ets_sys.h"


static const char *TAG = "HC_SR04";

void hc_sr04_init(void) {
    // Configurar el pin de Trigger como salida
    gpio_config_t io_conf_trig = {
        .pin_bit_mask = (1ULL << PIN_HCSR04_TRIG),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf_trig);

    // Configurar el pin de Echo como entrada
    gpio_config_t io_conf_echo = {
        .pin_bit_mask = (1ULL << PIN_HCSR04_ECHO),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf_echo);
    // Asegurarse de que el trigger esté bajo al inicio
    gpio_set_level(PIN_HCSR04_TRIG, 0);
    ESP_LOGI(TAG, "Driver HC-SR04 inicializado.");
}

float hc_sr04_get_distance_cm(void) {
    // 1. Enviar pulso de trigger de 10 microsegundos para iniciar la medición
    gpio_set_level(PIN_HCSR04_TRIG, 0);
    ets_delay_us(2);
    gpio_set_level(PIN_HCSR04_TRIG, 1);
    ets_delay_us(10);
    gpio_set_level(PIN_HCSR04_TRIG, 0);

    // 2. Esperar a que el pin ECHO se ponga en ALTO (inicio del pulso de eco)
    uint64_t start_time = esp_timer_get_time();
    while (gpio_get_level(PIN_HCSR04_ECHO) == 0) {
        if ((esp_timer_get_time() - start_time) > TIMEOUT_ULTRASONICO_US) {
            return -1.0f; // Error: Timeout esperando el inicio del eco
        }
    }

    // 3. Medir el tiempo que el pin ECHO permanece en ALTO
    start_time = esp_timer_get_time();
    while (gpio_get_level(PIN_HCSR04_ECHO) == 1) {
        if ((esp_timer_get_time() - start_time) > TIMEOUT_ULTRASONICO_US) {
            return -2.0f; // Error: Timeout durante la medición del eco (objeto muy lejano o sin objeto)
        }
    }
    uint64_t duration = esp_timer_get_time() - start_time;

    // 4. Calcular la distancia en cm
    // Velocidad del sonido ~343 m/s = 0.0343 cm/µs
    // Distancia = (duración_del_pulso * velocidad_del_sonido) / 2 (porque es un viaje de ida y vuelta)
    float distance = (duration * 0.0343f) / 2.0f;

    return distance;
}
