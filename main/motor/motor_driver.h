#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

void init_motors();
void set_motor_speed(int speed);
void set_servo_angle(int angle);
void test_servo_raw();
#endif // MOTOR_DRIVER_H