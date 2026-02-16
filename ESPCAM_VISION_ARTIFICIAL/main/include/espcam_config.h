#ifndef ESPCAM_CONFIG_H
#define ESPCAM_CONFIG_H

// --- CREDENCIALES WI-FI ---
#define WIFI_SSID      "ROBOT_VISION_AP"  // Nombre de la red que creará el robot
#define WIFI_PASS      "123456789"        // Contraseña para conectarte a él
#define CONEXIONES_MAX 4                   // Máximo de dispositivos que pueden conectarse al robot
// --- PINES CÁMARA 
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1 // Software Reset
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

// --- CONFIGURACIÓN DE IMAGEN (CALIDAD) ---
// Resoluciones disponibles: FRAMESIZE_QVGA (320x240), FRAMESIZE_VGA (640x480), FRAMESIZE_SVGA (800x600), etc.
#define CAM_RESOLUCION      FRAMESIZE_QVGA   

// Calidad JPEG: 0-63 (Menor número = Mayor calidad). 
// 10-15: Alta calidad (bueno para visión). 63: Muy mala calidad.
#define CAM_CALIDAD         12              

// Buffers: Más buffers = video más fluido pero consume más RAM.
#define CAM_BUFFERS         2               

// Frecuencia del reloj XCLK (Hz).
// 20000000 (20MHz) es estándar. Si falla la inicialización (Error 0x106), prueba con 15000000 (15MHz).
#define CAM_XCLK_FREQ       18000000

#endif 
