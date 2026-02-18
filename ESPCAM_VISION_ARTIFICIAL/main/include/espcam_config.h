#ifndef ESPCAM_CONFIG_H
#define ESPCAM_CONFIG_H

// === CREDENCIALES WI-FI ===
#define WIFI_SSID      "ROBOT_VISION_AP"
#define WIFI_PASS      "123456789"
#define CONEXIONES_MAX 4

// === PINES CÁMARA (AI-Thinker ESP32-CAM) ===
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0      5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

// === CONFIGURACIÓN DE IMAGEN ===
#define CAM_RESOLUCION  FRAMESIZE_VGA  // 640x480 para streaming
#define CAM_CALIDAD     12             // 0-63 (menor = mejor)
#define CAM_BUFFERS     2              // Número de buffers
#define CAM_XCLK_FREQ   15000000       // 15MHz a 20MHz (estable)

#endif // ESPCAM_CONFIG_H