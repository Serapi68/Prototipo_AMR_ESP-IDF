#ifndef OBJECT_PROCESSOR_H
#define OBJECT_PROCESSOR_H

#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

void object_tracking_process_frame(camera_fb_t *fb);

#ifdef __cplusplus
}
#endif

#endif