#include "uart_comm.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "proceso_qr/qr_processor.h"
#include "vision_core/vision_core.h"

static const char *TAG = "UART_COMM_CAM";
#define UART_PORT_NUM UART_NUM_1 // Ajustar según tu conexión física
#define BUF_SIZE 1024

void uart_rx_task(void *arg) {
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    uint8_t rx_state = 0; 
    uint8_t temp_cmd = 0;

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, pdMS_TO_TICKS(50));
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                uint8_t byte = data[i];

                // Protocolo: [0xAA][CMD][CRC]
                if (rx_state == 0 && byte == 0xAA) {
                    rx_state = 1;
                } else if (rx_state == 1) {
                    temp_cmd = byte;
                    rx_state = 2;
                } else if (rx_state == 2) {
                    if ((uint8_t)(temp_cmd + byte) == 0xFF) {
                        ESP_LOGI(TAG, "Comando recibido correctamente: 0x%02X", temp_cmd);
                        
                        // Control de modos de la cámara
                        if (temp_cmd == 0x10) { // CMD_STREAM_ONLY
                            vision_core_set_mode(VISION_MODO_STREAMING);
                            ESP_LOGI(TAG, "Modo: Solo Streaming");
                        } 
                        else if (temp_cmd == 0x20) { // CMD_QR_READER
                            vision_core_set_mode(VISION_MODO_QR);
                            ESP_LOGI(TAG, "Modo: Lector QR Activo");
                        }
                        else if (temp_cmd == 0x30) { // CMD_OBJECT_TRACKER
                            vision_core_set_mode(VISION_MODO_TRACKING);
                            ESP_LOGI(TAG, "Modo: Seguimiento de Objetos");
                        }
                        else if (temp_cmd == 0x40) { // CMD_SIGN_READER
                            vision_core_set_mode(VISION_MODO_SENALES);
                            ESP_LOGI(TAG, "Modo: Lector de Señales");
                        }
                    } else {
                        ESP_LOGW(TAG, "Error de Checksum en comando recibido");
                    }
                    rx_state = 0;
                }
            }
        }
    }
    free(data);
    vTaskDelete(NULL);
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

    // Instalación del driver
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    
    // Configuración de pines para UART1 (TX:14, RX:15)
    // Verifica que estos sean los pines físicos que conectaste a la otra placa
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, 14, 15, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 10, NULL);
    ESP_LOGI(TAG, "UART de control (RX/TX) inicializado correctamente");
}

void uart_send_byte(uint8_t byte) {
    uart_write_bytes(UART_PORT_NUM, (const char*)&byte, 1);
}