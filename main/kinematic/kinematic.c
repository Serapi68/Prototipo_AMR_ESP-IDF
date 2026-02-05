// Funciones matematicas para cinemática del robot de veloccidad diferencial
#include "robot_config.h"
#include "motor_driver.h"
#include <math.h>

//Constantes fisicas estan en el archivo de configuracion robot_config.h

//Funcion de actualizacion de la posicion del robot

void actualizar_movimiento (float potencia, float direccion_stick){

    //Control del Servo de direccion
    // Ajustar el rango de ángulo (ejemplo: -1.0 a 1.0 mapea a 0 a 180 grados)
    int angulo_servo = 90 + (direccion_stick * 90); 
    set_servo_angle(angulo_servo);

    //Calculo de diferencial electrico para motores
    if(fabs(direccion_stick) < 0.05){
        set_motor_speed_left(potencia); // Movimiento recto
        set_motor_speed_right(potencia);
    }else{
        //Calculamos el radio de giro basado en la dirección del stick
        float angulo_rad = fabs(direccion_stick) * (M_PI / 2.0); // Convertir a radianes
        float R = DISTANCIA_EJE_MOTORES_CM / tan(angulo_rad);  // Radio de giro

        //Calculo de velocidades para cada motor
        float v_interior = potencia * (1 - (ANCHO_VIA_CM * tan(angulo_rad) / (2 * R))); // Velocidad del motor interior
        float v_exterior = potencia * (1 + (ANCHO_VIA_CM * tan(angulo_rad) / (2 * R))); // Velocidad del motor exterior

        //Asignar velocidades a los motores
        if(direccion_stick > 0){
            set_motor_speed_right((int)v_interior); // Giro a la derecha
            set_motor_speed_left((int)v_exterior);
        }else{
            set_motor_speed_right((int)v_exterior); // Giro a la izquierda
            set_motor_speed_left((int)v_interior);
        }
    }
}