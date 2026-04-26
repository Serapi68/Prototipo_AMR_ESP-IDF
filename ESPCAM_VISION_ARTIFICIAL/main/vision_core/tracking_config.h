#ifndef TRACKING_CONFIG_H
#define TRACKING_CONFIG_H

// Parámetros de Inferencia
// CRÍTICO: Debe coincidir con threshold en model_variables.h (ei_fill_result_fomo_i8_config_944718_3.threshold = 0.5)
// El modelo FOMO filtra detecciones por debajo de 0.5 ANTES del postprocessing
#define TRACK_CONFIDENCE_THRESHOLD  0.50f  // Sincronizado con model threshold

// Geometría de la imagen - AHORA CAPTURAMOS DIRECTAMENTE A 96x96
// Coincide exactamente con EI_CLASSIFIER_INPUT_WIDTH/HEIGHT
// Esto EVITA downsampling pobre sin interpolación
#define TRACK_IMG_WIDTH            320
#define TRACK_IMG_HEIGHT           240

// Lógica de Centrado
#define TRACK_CENTER_X             (TRACK_IMG_WIDTH / 2)
#define TRACK_DEADZONE_X           35  // Aumentado para mayor estabilidad en el avance

// Comandos UART para Tracking
#define CMD_TRACK_IZQ              0x31
#define CMD_TRACK_DER              0x32
#define CMD_TRACK_AVANCE           0x33
#define CMD_TRACK_STOP             0x15

#endif