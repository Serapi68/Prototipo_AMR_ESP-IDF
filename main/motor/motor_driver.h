#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

void init_motors();
void set_motor_speed_left(int speed);
void set_motor_speed_right(int speed);
void set_servo_angle(int angle);

#endif // MOTOR_DRIVER_H