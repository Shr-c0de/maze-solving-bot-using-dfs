#ifndef MOTOR
#define MOTOR

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include <cmath>

#define RATIO 530
#define WHEEL_DIAMETER 4.4
#define WHEEL_BASE 15.0
#define PI 3.1415

extern volatile long cA, cB;

#define ENCODER_A 18
#define ENCODER_B 19
class Motor
{
public:
    const uint MOTOR_A_PWM = 15;
    const uint MOTOR_A_DIR = 14;
    const uint MOTOR_B_PWM = 16;
    const uint MOTOR_B_DIR = 17;

    const int STEPS_PER_UNIT = static_cast<int>((RATIO * 32.0) / (WHEEL_DIAMETER * PI));

    uint slice_num_a, slice_num_b;

    float kp = 1.0, ki = 0.0, kd = 0.0;
    float error_a = 0, prev_error_a = 0, integral_a = 0;
    float error_b = 0, prev_error_b = 0, integral_b = 0;

    int calculate_pid_speed(int target, int current, float &error, float &prev_error, float &integral);
    void set_motor(int motor, int speed, bool forward);

    static void encoder_a_irq_handler(uint gpio, uint32_t events);
    static void encoder_b_irq_handler(uint gpio, uint32_t events);

    static void global_encoder_irq_handler(uint gpio, uint32_t events);
    Motor();
    void init_motor();
    void init_encoders();
    void move_forward(float units);
    void turn_left(float units);
    void turn_right(float units);
    void curved_turn(float radius, float angle, bool is_left_turn);
};
#endif
