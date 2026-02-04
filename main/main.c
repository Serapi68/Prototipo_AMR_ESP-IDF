
//librerias estandar
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

// Librerias propias
#include "motor_driver.h"


//He dejado la frecuencia en 200 Hz para probar los motores con menos ruido audible, y tienen mayor torque funcionan mejor.
void app_main(void)
{
    init_motors();
    
    const int speeds[] = {
        0, 200, 300, 400, 500, 600, 700, 800, 900, 1000,
        900, 800, 700, 600, 500, 400, 300, 200, 0,
        -200, -300, -400, -500, -600, -700, -800, -900, -1000,
        -900, -800, -700, -600, -500, -400, -300, -200, 0
    };

    const int delays_ms[] = {
        5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 2000,
        5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000,
        5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 2000,
        5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000
    };
    
    int steps = sizeof(speeds) / sizeof(speeds[0]);
    int repeticion = 5;

    while(repeticion>0){
        set_servo_angle(90); // Centrar el servo inicialmente
        printf("MOTOR_TEST: Moviendo servo a 90 grados.\n");
        vTaskDelay(pdMS_TO_TICKS(5000)); // Esperar 2 segundos
        set_servo_angle(0); // Mover a 0 grados
        printf("MOTOR_TEST: Moviendo servo a 0 grados.\n");
        vTaskDelay(pdMS_TO_TICKS(5000)); // Esperar 2 segundos
        set_servo_angle(180); // Mover a 180 grados
        printf("MOTOR_TEST: Moviendo servo a 180 grados.\n");
        vTaskDelay(pdMS_TO_TICKS(5000)); // Esperar 2 segundos
        repeticion--;
    }


    for (int i = 0; i < steps; i++) {
        set_motor_speed(speeds[i]);
        printf("MOTOR_TEST: %d\n", speeds[i]);
        vTaskDelay(pdMS_TO_TICKS(delays_ms[i]));
    }

    printf("MOTOR_TEST: Test de motores completado.\n");
}

