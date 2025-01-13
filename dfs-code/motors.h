#ifndef MOTOR
#define MOTOR

#include <stdio.h>
#include<iostream>
#include "pico/stdlib.h"
#include "Sensors.h"
//#include "pico/math.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include <cmath>

#define current_count(is_left, cA, cB) ((is_left) ? (cA) : (cB))
#define RATIO 100
#define WHEEL_DIAMETER 4.3 // cm
#define WHEEL_BASE 14.5
#define PI 3.1415
#define THRESHOLD 5

#define ENCODER_A 14
#define ENCODER_B 15
class Motor
{
private:
    Sensor* s;
    double* distances;

    const int STEPS_PER_UNIT = (RATIO / WHEEL_DIAMETER / PI) * 32.0;

    uint slice_num_a, slice_num_b;

    const double kp = 0.7, ki = 0.2, kd = 0.5;
    double error[2] = {0, 0};
    double prev_error[2] = {0, 0};
    double integral[2] = {0, 0};
    int speed[2] = {0, 0};

    const uint MOTOR_A_PWM = 16;
    const uint MOTOR_A_FRONT = 20;
    const uint MOTOR_A_BACK = 18;

    const uint MOTOR_B_PWM = 17;
    const uint MOTOR_B_FRONT = 21;
    const uint MOTOR_B_BACK = 19;

    void init_encoders();
    void init_motor();
    void reinitvar();

    int calculate_pid_speed(int target, bool is_left);
    void set_motor(int motor, int pwm);

    static void global_encoder_irq_handler(uint gpio, uint32_t events);
    static void global_encoder_irq_handler_neg(uint gpio, uint32_t events);
    void valcheck(int &left, int &right);

public:
    // a->left, b->right
    Motor(Sensor &s);
    void move_forward(double units);
    void turn(double units, bool direction);
    void curved_turn(double radius, double angle, bool is_left_turn);
};

#endif
