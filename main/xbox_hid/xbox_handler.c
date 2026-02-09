
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_hidh.h"
#include "esp_hid_gap.h"
#include "xbox_config.h"
#include "robot_config.h"
#include "kinematic.h"
#include "motor_driver.h"

static xbox_data_t g_mando_conectado = {0}; // Estructura global para almacenar el estado del mando

static const char *TAG = "XBOX_HANDLER";

// Variable estática para guardar el último reporte y comparar cambios (temporal)
static uint8_t last_report[64] = {0};

/* ================= HID CALLBACK ================= */
// Esta función maneja los eventos del stack HID (conexión, entrada de datos, desconexión)
static void hidh_callback(void *arg,
                          esp_event_base_t base,
                          int32_t id,
                          void *event_data)
{
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *param = event_data;

    switch (event) {

    case ESP_HIDH_OPEN_EVENT:
        ESP_LOGI(TAG, "MANDO XBOX CONECTADO");
        ESP_LOGI(TAG, "Nombre: %s", esp_hidh_dev_name_get(param->open.dev));
        break;

    case ESP_HIDH_INPUT_EVENT:
  
        uint8_t *d = param->input.data;
        //Aplicar offsets y escalados a los datos del mando
        // Interpretamos como uint16_t (0..65535) donde el centro es aprox 32768
        uint16_t left_stick_x = (d[XBOX_BYTE_LX + 1] << 8) | d[XBOX_BYTE_LX]; 
        uint16_t raw_lt = (d[XBOX_BYTE_LT + 1] << 8) | d[XBOX_BYTE_LT]; // Trigger izquierdo
        uint16_t raw_rt = (d[XBOX_BYTE_RT + 1] << 8) | d[XBOX_BYTE_RT]; // Trigger derecho

        // DEBUG: Descomenta esto si el stick hace cosas raras para ver el valor crudo
        // static int log_raw = 0;
        // if (log_raw++ > 20) {
        //    ESP_LOGI(TAG, "Raw Stick X: %u (Centro ideal: 32768)", left_stick_x);
        //    log_raw = 0;
        // }

        //Normalizar los rangos de los sticks y gatillos a -1.0 a 1.0 o 0.0 a 1.0
        // Restamos 32768 para centrar en 0, dividimos por 32768 para rango -1.0 a 1.0
        g_mando_conectado.stick_izq_x = ((float)left_stick_x - 32768.0f) / 32768.0f;
        g_mando_conectado.trigger_lt = (float)(raw_lt - XBOX_TRIGGER_MIN) / (XBOX_TRIGGER_MAX - XBOX_TRIGGER_MIN); // Normalizado a 0.0 a 1.0
        g_mando_conectado.trigger_rt = (float)(raw_rt - XBOX_TRIGGER_MIN) / (XBOX_TRIGGER_MAX - XBOX_TRIGGER_MIN); // Normalizado a 0.0 a 1.0
        
        //Botonos A y B (bit 0 y bit 1 del byte de botones)
        g_mando_conectado.boton_a = (d[XBOX_BYTE_BUTTONS_1] & XBOX_MASK_A) ? 1 : 0; // Botón A
        g_mando_conectado.boton_b = (d[XBOX_BYTE_BUTTONS_1] & XBOX_MASK_B) ? 1 : 0; // Botón B

        //Calcular potencia neta para el robot (gatillo derecho - gatillo izquierdo)
        float potencia = (g_mando_conectado.trigger_rt - g_mando_conectado.trigger_lt);

        //Actualizar movimiento del robot con la función de cinemática
        actualizar_movimiento(potencia, g_mando_conectado.stick_izq_x);
        set_led(g_mando_conectado.boton_b);
        break;

    case ESP_HIDH_CLOSE_EVENT:
        ESP_LOGW(TAG, "MANDO DESCONECTADO");
        // Aquí se podría implementar lógica de reconexión o parada de emergencia
        break;

    default:
        break;
    }
}

/* ================= FUNCION DE INICIALIZACION ================= */
void init_xbox(void)
{
    ESP_LOGI(TAG, "Inicializando modulo Xbox BLE...");

    /* HID GAP (BLE) - Inicialización del stack */
    ESP_ERROR_CHECK(esp_hid_gap_init(HIDH_BLE_MODE));

    /* Registrar callback GATTC para HID BLE (necesario para el stack HID de ESP-IDF) */
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler));

    /* Configuración del HID Host */
    esp_hidh_config_t config = {
        .callback = hidh_callback,
        .event_stack_size = 4096,
        .callback_arg = NULL,
    };
    ESP_ERROR_CHECK(esp_hidh_init(&config));

    ESP_LOGI(TAG, "Escaneando dispositivos BLE HID...");
    
    // Escanear por 10 segundos buscando dispositivos
    size_t results_len = 0;
    esp_hid_scan_result_t *results = NULL;
    esp_hid_scan(10, &results_len, &results);

    // Buscar específicamente el controlador Xbox
    esp_hid_scan_result_t *r = results;
    esp_hidh_dev_t *dev = NULL;
    while (r) {
        if (r->transport == ESP_HID_TRANSPORT_BLE && strcmp(r->name, "Xbox Wireless Controller") == 0) {
            ESP_LOGI(TAG, "Encontrado: %s", r->name);
            // Abrir conexión con el dispositivo encontrado
            dev = esp_hidh_dev_open(r->bda, r->transport, r->ble.addr_type);
            break;
        }
        r = r->next;
    }

    // Liberar memoria de los resultados del escaneo
    esp_hid_scan_results_free(results);
}
