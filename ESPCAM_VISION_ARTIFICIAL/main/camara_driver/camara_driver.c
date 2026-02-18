#include "camara_driver.h"
#include "espcam_config.h"
#include "esp_log.h"

static const char *TAG = "CAMERA_DRIVER";

esp_err_t init_camera(void) {
    camera_config_t camera_config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        .xclk_freq_hz = CAM_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = CAM_RESOLUCION,
        .jpeg_quality = CAM_CALIDAD,
        .fb_count = CAM_BUFFERS,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY
    };

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al inicializar cámara: 0x%x", err);
        return err;
    }
    
    ESP_LOGI(TAG, "Cámara inicializada correctamente");
    return ESP_OK;
}

esp_err_t camera_set_format_grayscale_qvga(void) {
    sensor_t *sensor = esp_camera_sensor_get();
    if (!sensor) return ESP_FAIL;
    
    sensor->set_pixformat(sensor, PIXFORMAT_GRAYSCALE);
    sensor->set_framesize(sensor, FRAMESIZE_QVGA);
    
    // Limpiar buffers antiguos
    vTaskDelay(pdMS_TO_TICKS(50));
    for (int i = 0; i < 3; i++) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    ESP_LOGI(TAG, "Formato cambiado a GRAYSCALE 320x240");
    return ESP_OK;
}

esp_err_t camera_set_format_jpeg_vga(void) {
    sensor_t *sensor = esp_camera_sensor_get();
    if (!sensor) return ESP_FAIL;
    
    sensor->set_pixformat(sensor, PIXFORMAT_JPEG);
    sensor->set_framesize(sensor, CAM_RESOLUCION);
    
    // Limpiar buffers antiguos
    vTaskDelay(pdMS_TO_TICKS(50));
    for (int i = 0; i < 3; i++) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    ESP_LOGI(TAG, "Formato cambiado a JPEG VGA");
    return ESP_OK;
}

camera_fb_t* camara_capture(void) {
    return esp_camera_fb_get();
}

void camara_return_fb(camera_fb_t * fb) {
    esp_camera_fb_return(fb);
}