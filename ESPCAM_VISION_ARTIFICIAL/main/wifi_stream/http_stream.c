#include "http_stream.h"
#include <string.h>
#include "espcam_config.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "esp_camera.h"

static const char *TAG = "HTTP_STREAM";

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// HTML Básico incrustado en el código
// Esto crea una página web simple que carga el stream de video
static const char* INDEX_HTML = 
    "<!doctype html>"
    "<html>"
    "<head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Robot Vision</title>"
    "<style>"
    "body { font-family: Arial, sans-serif; background: #1a1a1a; color: #fff; text-align: center; margin: 0; }"
    "h1 { margin: 10px; }"
    "img { width: 100%; max-width: 640px; height: auto; border: 2px solid #444; }"
    "</style>"
    "</head>"
    "<body>"
    "<h1>Vista en Tiempo Real</h1>"
    "<img src='/stream' id='video-stream'>" // La etiqueta IMG carga la ruta /stream
    "</body>"
    "</html>";


// Handler para el streaming MJPEG
static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");  // Agregado: Evita cache en browser
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGW(TAG, "Salto de frame (buffer ocupado o timeout)");
            vTaskDelay(pdMS_TO_TICKS(10));  // Reducido para retry rápido
            continue;
        }

        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;

        // Enviar boundary
        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res == ESP_OK) {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }

        esp_camera_fb_return(fb);
        fb = NULL;
        _jpg_buf = NULL;

        if(res != ESP_OK){
            ESP_LOGE(TAG, "Error enviando chunk: %d", res);
            break;
        }
        // Sin delay aquí para max rendimiento; el WiFi maneja pacing
    }
    return res;
}


// Handler para la página principal (HTML)
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

// Configuración Wi-Fi
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Cliente conectado al AP. MAC: "MACSTR", AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Cliente desconectado del AP. MAC: "MACSTR", AID=%d", MAC2STR(event->mac), event->aid);
    }
}

void start_wifi(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap(); // Creamos un AP en lugar de una estación

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = 1,
            .password = WIFI_PASS,
            .max_connection = CONEXIONES_MAX,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
    
    ESP_LOGI(TAG, "Punto de Acceso iniciado. SSID: %s", WIFI_SSID);
}

esp_err_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80; // Puerto estándar HTTP para entrar directo con la IP
    httpd_handle_t stream_httpd = NULL;

    // URI para la página web principal
    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    // URI para el stream de video
    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    ESP_LOGI(TAG, "Iniciando servidor web...");
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        ESP_LOGI(TAG, "Servidor Web iniciado correctamente");
        ESP_LOGI(TAG, "Para ver el video, usa este enlace o genera un QR con el: http://192.168.4.1");
        return ESP_OK;
    }
    return ESP_FAIL;
}