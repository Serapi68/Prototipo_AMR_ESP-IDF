#include "uart_comm.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "vision_core/vision_core.h"

// Configuración UART
#define UART_PORT_NUM      UART_NUM_2
#define UART_RX_PIN        14
#define UART_TX_PIN        15
#define RX_BUF_SIZE        128

static const char *TAG = "UART_CONTROL";

// Protocolo de comandos
#define CMD_STREAM_ONLY    0x10  // Botón A: Solo streaming
#define CMD_QR_READER      0x20  // Botón B: Lector QR
#define CMD_OBJECT_TRACKER 0x30  // Botón X: Seguidor de objetos
#define CMD_SIGN_READER    0x40  // Botón Y: Lector de señales

static void uart_rx_task(void *arg) {
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE + 1);
    ESP_LOGI(TAG, "Tarea de recepción UART iniciada");

    while (1) {
        const int rxBytes = uart_read_bytes(UART_PORT_NUM, data, RX_BUF_SIZE, pdMS_TO_TICKS(100));
        
        if (rxBytes > 0) {
            uint8_t cmd = data[0];
            ESP_LOGI(TAG, "Comando UART recibido: 0x%02X", cmd);

            switch (cmd) {
                case CMD_STREAM_ONLY:
                    ESP_LOGI(TAG, "→ Cambiando a modo STREAMING");
                    vision_core_set_mode(VISION_MODO_STREAMING);
                    break;
                    
                case CMD_QR_READER:
                    ESP_LOGI(TAG, "→ Cambiando a modo LECTOR QR");
                    vision_core_set_mode(VISION_MODO_QR);
                    break;
                    
                case CMD_OBJECT_TRACKER:
                    ESP_LOGI(TAG, "→ Cambiando a modo SEGUIDOR DE OBJETOS");
                    vision_core_set_mode(VISION_MODO_TRACKING);
                    break;
                    
                case CMD_SIGN_READER:
                    ESP_LOGI(TAG, "→ Cambiando a modo LECTOR DE SEÑALES");
                    vision_core_set_mode(VISION_MODO_SENALES);
                    break;
                    
                default:
                    ESP_LOGW(TAG, "Comando UART desconocido: 0x%02X", cmd);
                    break;
            }
        }
    }
    
    free(data);
}

void uart_control_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, 
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(uart_rx_task, "uart_rx_task", 8192, NULL, 10, NULL);
    ESP_LOGI(TAG, "Control por UART inicializado en pines TX:%d, RX:%d", 
             UART_TX_PIN, UART_RX_PIN);
}

void uart_send_byte(uint8_t data) {
    uart_write_bytes(UART_PORT_NUM, (const char*)&data, 1);
}