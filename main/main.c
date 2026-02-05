#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"

#include "esp_hidh.h"
#include "esp_hid_gap.h"

static const char *TAG = "XBOX_MAP";

static uint8_t last_report[64] = {0};

/* ================= HID CALLBACK ================= */
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
        ESP_LOGI(TAG, "Nombre: %s",
                 esp_hidh_dev_name_get(param->open.dev));
        break;

    case ESP_HIDH_INPUT_EVENT:
        for (int i = 0; i < param->input.length; i++) {
            if (param->input.data[i] != last_report[i]) {
                printf("Byte %02d : %3d -> %3d\n",
                       i, last_report[i], param->input.data[i]);
                last_report[i] = param->input.data[i];
            }
        }
        break;

    case ESP_HIDH_CLOSE_EVENT:
        ESP_LOGW(TAG, "MANDO DESCONECTADO");
        break;

    default:
        break;
    }
}

/* ================= APP MAIN ================= */
void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando mapeo Xbox BLE");

    /* NVS */
    ESP_ERROR_CHECK(nvs_flash_init());

    /* HID GAP (BLE) */
    ESP_ERROR_CHECK(esp_hid_gap_init(HIDH_BLE_MODE));  // Modo BLE

    /* Registrar callback GATTC para HID BLE */
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler));

    /* HID Host */
    esp_hidh_config_t config = {
        .callback = hidh_callback,
        .event_stack_size = 4096,
        .callback_arg = NULL,
    };
    ESP_ERROR_CHECK(esp_hidh_init(&config));

    ESP_LOGI(TAG, "Escaneando dispositivos BLE HID...");
    
    // Escanear por 10 segundos
    size_t results_len = 0;
    esp_hid_scan_result_t *results = NULL;
    esp_hid_scan(10, &results_len, &results);

    // Buscar el controlador Xbox (se anuncia como "Xbox Wireless Controller")
    esp_hid_scan_result_t *r = results;
    esp_hidh_dev_t *dev = NULL;
    while (r) {
        if (r->transport == ESP_HID_TRANSPORT_BLE && strcmp(r->name, "Xbox Wireless Controller") == 0) {
            ESP_LOGI(TAG, "Encontrado: %s", r->name);
            dev = esp_hidh_dev_open(r->bda, r->transport, r->ble.addr_type);
            break;
        }
        r = r->next;
    }

    // Liberar resultados del escaneo
    esp_hid_scan_results_free(results);

    if (dev == NULL) {
        ESP_LOGE(TAG, "No se encontró el mando Xbox. Asegúrate de que esté en modo emparejamiento.");
    } else {
        ESP_LOGI(TAG, "Pon el mando Xbox en modo emparejamiento si no conecta...");
    }

    // El callback ahora manejará las entradas una vez conectado
}