#ifndef MOTOR
#define MOTOR

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include <cmath>

#define RATIO 551
#define WHEEL_DIAMETER 4.3 //cm
#define WHEEL_BASE 16.0
#define PI 3.1415
#define THRESHOLD 10

#define ENCODER_A 14
#define ENCODER_B 15
class Motor
{
public:
    //a->left, b->right
    const uint MOTOR_A_PWM = 16;
    const uint MOTOR_A_FRONT = 18;
    const uint MOTOR_A_BACK = 20;

    const uint MOTOR_B_PWM = 17;
    const uint MOTOR_B_FRONT = 19;
    const uint MOTOR_B_BACK = 21;

    const int STEPS_PER_UNIT = (RATIO / WHEEL_DIAMETER / PI) * 32.0;

    uint slice_num_a, slice_num_b;

    const double kp = 0.6, ki = 0.06, kd = 1;
    double error[2] = {0,0};
    double prev_error[2] = {0,0};
    double integral[2] = {0,0};
    int speed[2] = {0, 0};

    int calculate_pid_speed(int target, bool is_left);
    void set_motor(int motor, int speed, bool forward);

    static void global_encoder_irq_handler(uint gpio, uint32_t events);
    Motor();
    void init_motor();
    void reinitvar();
    void init_encoders();
    void move_forward(float units);
    void turn(float units, int direction);
    void curved_turn(float radius, float angle, bool is_left_turn);
};
#endif
