#ifndef MOTOR
#define MOTOR

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "Sensors.h"
#include <cmath>

#define current_count(is_left, cA, cB) ((is_left) ? (cA) : (cB))
#define RATIO 100
#define WHEEL_DIAMETER 4.3 // cm
#define WHEEL_BASE 14.5
#define PI 3.1415
#define THRESHOLD 5
#define TURN_THRESHOLD 4

#define ENCODER_A 14
#define ENCODER_B 15
class Motor
{
private:
    double *distances;
    mutex_t *mutex;
    const int STEPS_PER_UNIT = (RATIO / WHEEL_DIAMETER / PI) * 32.0;

    uint slice_num_a, slice_num_b;
    absolute_time_t prev_time;

    const double kp = 2, ki = 0.01, kd = 0.2;
    const double ktp = 0.1, kti = 0.01, ktd = 0.2; // for turns

    double error[2] = {0, 0};
    double prev_error[2] = {0, 0};
    double turn_error = 0, prev_turn = 0;
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

    void front_pid_speed(int target);
    void set_motor();

    static void global_encoder_irq_handler(uint gpio, uint32_t events);
    //void turn_pid_speed(int units);

public:
    QMC5883LCompass compass;
    // a->left, b->right
    // Motor();
    Motor(double *arr, mutex_t *mutex);
    void move_forward(double units);
    void turn(double units);
    // void curved_turn(double radius, double angle, bool is_left_turn);
};
#endif
