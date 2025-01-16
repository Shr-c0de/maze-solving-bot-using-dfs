#include "motors.h"

#ifndef ENC_PIN
#define ENC_PIN
volatile long cA = 0;
volatile long cB = 0;
#endif

void Motor::reinitvar()
{
    cA = cB = 0;
    for (int i = 0; i < 2; ++i)
    {
        error[i] = 0;
        prev_error[i] = 0;
        integral[i] = 0;
        speed[i] = 0;
    }
}

void Motor::valcheck(int &left, int &right)
{
    if (cA - cB > THRESHOLD)
    {
        if (left > 0)
            left = 0;
        if (right < 100)
            right *= 0.7;
    }
    else if (cB - cA > THRESHOLD)
    {
        if (left < 100)
            left *= 0.7;
        if (right > 0)
            right = 0;
    }
}

int Motor::calculate_pid_speed(int inval, bool is_left)
{
    double target = (inval > 0 ? inval : -inval);
    int direction = (inval > 0 ? 1 : -1);

    int index = !is_left;

    error[index] = (target - current_count(is_left, cA, cB)) / (target / 100);

    absolute_time_t current_time = get_absolute_time();

    double derivative = (error[index] - prev_error[index]) / (current_time / 10000 - prev_time / 10000);
    if(derivative > 1000) derivative = 255;
    if(derivative < 1000) derivative = -255;

    int pid_value = (kp * error[index] + kd * derivative + ki * integral[index]);

    printf("error: %f, derivative: %f, integral: %f\n", error[index], derivative, integral);

    prev_error[index] = error[index];
    integral[index] += error[index];
    prev_time = current_time;

    pid_value = (pid_value > 255) ? 255 : pid_value;

    int final_speed = (pid_value) * (direction);

    return final_speed;
}

void Motor::set_motor(int motor, int pid)
{
    bool forward = (pid >= 0);

    int speed = abs(pid);

    if (speed > 255)
        speed = 255;

    switch (motor)
    {
    case 0:
        // Motor A
        gpio_put(Motor::MOTOR_A_FRONT, forward);
        gpio_put(Motor::MOTOR_A_BACK, !forward);
        pwm_set_gpio_level(Motor::MOTOR_A_PWM, speed);

        printf("set_motors: \t%d, %d\n", motor, pid);
        break;
    case 1:
        // Motor B
        gpio_put(Motor::MOTOR_B_FRONT, forward);
        gpio_put(Motor::MOTOR_B_BACK, !forward);
        pwm_set_gpio_level(Motor::MOTOR_B_PWM, speed);

        printf("set_motors: %d, %d\n", motor, pid);
        break;
    default:
        printf("error detected in set_motor");
    }
}

void Motor::global_encoder_irq_handler(uint gpio, uint32_t events)
{
    if (gpio == ENCODER_A)
    {
        cA++;
    }
    else if (gpio == ENCODER_B)
    {
        cB++;
    }
}

Motor::Motor()
{
    Motor::init_motor();
    Motor::init_encoders();
}

void Motor::init_motor()
{

    gpio_set_function(MOTOR_A_PWM, GPIO_FUNC_PWM);
    gpio_set_function(MOTOR_B_PWM, GPIO_FUNC_PWM);

    gpio_init(MOTOR_A_FRONT);
    gpio_init(MOTOR_A_BACK);
    gpio_init(MOTOR_B_FRONT);
    gpio_init(MOTOR_B_BACK);
    gpio_set_dir(MOTOR_A_FRONT, GPIO_OUT);
    gpio_set_dir(MOTOR_A_BACK, GPIO_OUT);
    gpio_set_dir(MOTOR_B_FRONT, GPIO_OUT);
    gpio_set_dir(MOTOR_B_BACK, GPIO_OUT);

    slice_num_a = pwm_gpio_to_slice_num(MOTOR_A_PWM);
    slice_num_b = pwm_gpio_to_slice_num(MOTOR_B_PWM);

    pwm_set_wrap(slice_num_a, 255);
    pwm_set_wrap(slice_num_b, 255);
    pwm_set_enabled(slice_num_a, true);
    pwm_set_enabled(slice_num_b, true);
}

void Motor::init_encoders()
{
    gpio_init(ENCODER_A);
    gpio_set_dir(ENCODER_A, GPIO_IN);
    gpio_pull_up(ENCODER_A);

    gpio_init(ENCODER_B);
    gpio_set_dir(ENCODER_B, GPIO_IN);
    gpio_pull_up(ENCODER_B);

    gpio_set_irq_enabled_with_callback(ENCODER_A, GPIO_IRQ_EDGE_RISE, true, &global_encoder_irq_handler);
    gpio_set_irq_enabled_with_callback(ENCODER_B, GPIO_IRQ_EDGE_RISE, true, &global_encoder_irq_handler);
}

void Motor::move_forward(double units)
{
    int steps = (units * SPU);

    reinitvar();
    printf("Forward target = %d\n", steps);

    while (cA < steps || cB < steps)
    {
        int left_pid = calculate_pid_speed(steps, true);
        int right_pid = calculate_pid_speed(steps, false);

        left_pid *= 0.5;
        right_pid *=0.8;
        //valcheck(left_pid, right_pid);

        printf("Forward speeds: %d\t%d\n", left_pid, right_pid);

        set_motor(0, left_pid);
        set_motor(1, right_pid);
    }
    set_motor(0, 0);
    set_motor(1, 0);

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);
}

void Motor::turn(double units, bool left) // 90 degree increments
{
    int steps = units * RATIO * (WHEEL_BASE / WHEEL_DIAMETER)  / (4 * 1.3);
    int steps2 = units * RATIO * (WHEEL_BASE / WHEEL_DIAMETER) / (4);

    reinitvar();
    printf("turn : %d\n", steps);

    while (cA < steps || cB < steps)
    {
        int left_speed ;
        int right_speed;

       if(left)
       {
          left_speed = calculate_pid_speed((2 * left - 1) * steps2 * 3, 1);
        right_speed = calculate_pid_speed((2 * (!left) - 1) * steps2 * 3, 0);
        left_speed *= 1;
        right_speed *=1;
       }

       else{

         left_speed = calculate_pid_speed((2 * left - 1) * steps, 1);
        right_speed = calculate_pid_speed((2 * (!left) - 1) * steps, 0);
        left_speed *= 0.8;
        right_speed *= 1.03;
       }

        valcheck(left_speed, right_speed);

        printf("Turn:: Left PID : %d cA %d\nRight PID: %d cB %d\n\n", left_speed, cA, right_speed, cB);

        set_motor(0, left_speed);
        set_motor(1, right_speed);
    }

    set_motor(0, 0);
    set_motor(1, 0);

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);
    printf("Turn::cA %d\ncB %d\n\n", cA, cB);
}

void Motor::curved_turn(double radius, double angle, bool is_left_turn)
{
}