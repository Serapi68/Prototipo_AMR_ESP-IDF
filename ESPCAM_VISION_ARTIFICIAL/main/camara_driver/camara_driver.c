#include "camara_driver.h"
#include "espcam_config.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "esp_heap_caps.h"

static const char *TAG = "CAMERA_DRIVER";

esp_err_t init_camera(void) 
{
    camera_config_t config = {0};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.xclk_freq_hz = CAM_XCLK_FREQ;
    config.pixel_format = PIXFORMAT_JPEG;  

    // Ajustar calidad y tamaño
    // Usamos heap_caps_get_total_size para verificar si la PSRAM está activa y disponible
    if(heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0){
        config.frame_size = CAM_RESOLUCION; 
        config.jpeg_quality = CAM_CALIDAD;  
        config.fb_count = CAM_BUFFERS;       
        config.grab_mode = CAMERA_GRAB_LATEST;  // Sobrescribe viejos
    } else {
        ESP_LOGW(TAG, "PSRAM no detectada. Usando configuración mínima.");
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 30; // Calidad media/baja para RAM interna
        config.fb_count = 1;
    }

    // Inicializar cámara
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al iniciar la cámara 0x%x", err);
        return err;
    }

    // Ajustes adicionales del sensor (para evitar imagen negra)
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 none
        s->set_whitebal(s, 1);       // Auto white balance
        s->set_awb_gain(s, 1);       // Auto gain
        s->set_exposure_ctrl(s, 1);  // Auto exposure
        s->set_aec_value(s, 300);    // Exposure value
        s->set_gain_ctrl(s, 1);      // Auto gain control
    }

    ESP_LOGI(TAG, "Cámara inicializada correctamente");
    return ESP_OK;
}