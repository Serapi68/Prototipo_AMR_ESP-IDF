#include "driver/uart.h"
#include "robot_config.h"
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_vision.h"

static const char *TAG = "UART_VISION";
static const int RX_BUF_SIZE = 1024;

void init_uart_vision(){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    
    ESP_LOGI(TAG, "UART Vision Inicializado (TX: %d, RX: %d)", UART_TX_PIN, UART_RX_PIN);
}

static void vision_test_task(void *arg)
{
    uint8_t *data = (uint8_t *) malloc(RX_BUF_SIZE + 1);
    while (1) {
        // 1. Leer datos entrantes (RX)
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0; // Null-terminate para imprimir como string
            ESP_LOGI(TAG, "Recibido: '%s'", data);
        }

        // 2. Enviar datos de prueba (TX) cada 2 segundos aprox
        static int counter = 0;
        if (counter++ % 20 == 0) { // 20 * 100ms = 2000ms
            const char *msg = "PING desde Control ESP32\n";
            uart_write_bytes(UART_NUM_1, msg, strlen(msg));
        }
    }
    free(data);
}

void start_vision_test(void) {
    xTaskCreate(vision_test_task, "vision_test_task", 4096, NULL, 5, NULL);
}
