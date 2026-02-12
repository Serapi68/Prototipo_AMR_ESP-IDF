#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "ESP_CAM_TEST";

// Configuración UART para ESP32-CAM
// Asegúrate de conectar:
// CAM TX (14) -> CONTROL RX
// CAM RX (15) -> CONTROL TX
// GND -> GND
#define UART_NUM            UART_NUM_1
#define UART_TX_PIN         14
#define UART_RX_PIN         15
#define UART_BAUD_RATE      115200
#define RX_BUF_SIZE         1024

void init_uart(void) {
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    // Instalamos el driver y configuramos pines
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "UART Inicializado en pines TX=%d, RX=%d", UART_TX_PIN, UART_RX_PIN);
}

void rx_task(void *arg) {
    uint8_t *data = (uint8_t *) malloc(RX_BUF_SIZE + 1);
    while (1) {
        // Leer datos del UART
        const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0; // Null-terminate
            ESP_LOGI(TAG, "Recibido del Control: '%s'", data);

            // Responder al ESP32 de Control
            const char *msg = "PONG desde ESP32-CAM\n";
            uart_write_bytes(UART_NUM, msg, strlen(msg));
        }
    }
    free(data);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando Prueba de Comunicación UART...");
    init_uart();
    xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, 10, NULL);
}
