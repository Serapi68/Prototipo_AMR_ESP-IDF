
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_event.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_hidh.h"
#include "esp_hid_gap.h"
#include "xbox_config.h"
#include "robot_config.h"
#include "maquina_estado.h"
#include "motor_driver.h"

static xbox_data_t g_mando_conectado = {0}; // Estructura global para almacenar el estado del mando

static const char *TAG = "XBOX_HANDLER";


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

        //Normalizar los rangos de los sticks y gatillos a -1.0 a 1.0 o 0.0 a 1.0
        // Restamos 32768 para centrar en 0, dividimos por 32768 para rango -1.0 a 1.0
        g_mando_conectado.stick_izq_x = ((float)left_stick_x - 32768.0f) / 32768.0f;
        g_mando_conectado.trigger_lt = (float)(raw_lt - XBOX_TRIGGER_MIN) / (XBOX_TRIGGER_MAX - XBOX_TRIGGER_MIN); // Normalizado a 0.0 a 1.0
        g_mando_conectado.trigger_rt = (float)(raw_rt - XBOX_TRIGGER_MIN) / (XBOX_TRIGGER_MAX - XBOX_TRIGGER_MIN); // Normalizado a 0.0 a 1.0
        
        //Botones 
        g_mando_conectado.boton_a = (d[XBOX_BYTE_BUTTONS_1] & XBOX_MASK_A) ? 1 : 0; // Botón A
        g_mando_conectado.boton_b = (d[XBOX_BYTE_BUTTONS_1] & XBOX_MASK_B) ? 1 : 0; // Botón B
        g_mando_conectado.boton_lb = (d[XBOX_BYTE_BUTTONS_1] & XBOX_MASK_LB) ? 1 : 0; // Botón LB
        g_mando_conectado.boton_rb = (d[XBOX_BYTE_BUTTONS_1] & XBOX_MASK_RB) ? 1 : 0; // Botón RB

        // Enviar los datos del mando a la tarea de control a través de la cola.
        // Usamos xQueueOverwrite para reemplazar siempre el valor anterior, ya que solo nos interesa el último estado.
        if (g_xbox_queue != NULL) {
            xQueueOverwrite(g_xbox_queue, &g_mando_conectado);
        }
        break;

    case ESP_HIDH_CLOSE_EVENT:
        ESP_LOGW(TAG, "MANDO DESCONECTADO");
        // Cuando el mando se desconecta, enviamos un estado "cero" para que la tarea de control detenga el robot.
        memset(&g_mando_conectado, 0, sizeof(xbox_data_t));
        xQueueOverwrite(g_xbox_queue, &g_mando_conectado);
        set_led(true); // Encender LED para indicar desconexión
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
        // Log para depuración: Ver qué dispositivos se han encontrado
        ESP_LOGI(TAG, "Evaluando dispositivo: %s (RSSI: %d)", (r->name ? r->name : "SIN NOMBRE"), r->rssi);

        // Usamos strstr para buscar "Xbox" en el nombre, es más flexible que strcmp exacto.
        // También verificamos que r->name no sea NULL.
        if (r->transport == ESP_HID_TRANSPORT_BLE && r->name != NULL && strstr(r->name, "Xbox") != NULL) {
            ESP_LOGI(TAG, "¡Mando Xbox Encontrado!: %s", r->name);
            // Abrir conexión con el dispositivo encontrado
            dev = esp_hidh_dev_open(r->bda, r->transport, r->ble.addr_type);
            break;
        }
        r = r->next;
    }

    // Liberar memoria de los resultados del escaneo
    esp_hid_scan_results_free(results);
}
