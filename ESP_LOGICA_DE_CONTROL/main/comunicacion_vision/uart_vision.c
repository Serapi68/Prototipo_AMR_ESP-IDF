#include "uart_vision.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "robot_config.h" // Para obtener UART_TX_PIN y UART_RX_PIN

static const char *TAG = "UART_VISION";

#define UART_PORT_NUM      UART_NUM_2
#define BUF_SIZE           1024

// Definición de la cola global
QueueHandle_t g_vision_queue = NULL;

// Tarea de recepción: Lee del UART y envía a la cola
static void uart_rx_task(void *arg) {
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    uint8_t rx_state = 0; // 0: Esperando Header, 1: Esperando CMD, 2: Esperando CRC
    uint8_t temp_cmd = 0;
    
    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, pdMS_TO_TICKS(100));
        
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                uint8_t byte = data[i];

                // Sincronización robusta: Si vemos el header, reiniciamos el estado
                if (byte == 0xAA) {
                    rx_state = 1;
                    continue;
                } else if (rx_state == 1) {
                    temp_cmd = byte;
                    rx_state = 2;
                } else if (rx_state == 2) {
                    // Validar Checksum: CMD + CRC debe ser 0xFF
                    if ((uint8_t)(temp_cmd + byte) == 0xFF) {
                        ESP_LOGI(TAG, ">> PAQUETE VÁLIDO RECIBIDO: 0x%02X", temp_cmd);
                        if (g_vision_queue != NULL) {
                            if (xQueueSend(g_vision_queue, &temp_cmd, 0) != pdPASS) {
                                ESP_LOGW(TAG, "Cola de visión llena, descartando 0x%02X", temp_cmd);
                            }
                        }
                    } else {
                        ESP_LOGW(TAG, ">> ERROR CRC: Cmd=0x%02X, Crc=0x%02X (Suma=0x%02X)", 
                                 temp_cmd, byte, (uint8_t)(temp_cmd + byte));
                    }
                    rx_state = 0;
                }
            }
        }
    }
    free(data);
    vTaskDelete(NULL);
}

void uart_vision_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // Instalación del driver
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Crear la cola de mensajes (capacidad para 10 comandos)
    g_vision_queue = xQueueCreate(10, sizeof(uint8_t));

    // Crear la tarea de recepción
    xTaskCreate(uart_rx_task, "uart_vision_rx", 4096, NULL, 10, NULL);
    ESP_LOGI(TAG, "UART Vision inicializado (TX:%d, RX:%d)", UART_TX_PIN, UART_RX_PIN);
}

void uart_vision_send_cmd(uint8_t cmd) {
    uint8_t packet[3] = {0xAA, cmd, (uint8_t)~cmd};
    uart_write_bytes(UART_PORT_NUM, (const char*)packet, 3);
    ESP_LOGI(TAG, "Comando enviado a CAM: 0x%02X", cmd);
}