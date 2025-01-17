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

    // if (cA - cB > THRESHOLD)
    // {
    //     if (left > 0)
    //         left = 0;
    //     if (right < 100)
    //         right *= 0.7;
    // }
    // else if (cB - cA > THRESHOLD)
    // {
    //     if (left < 100)
    //         left *= 0.7;
    //     if (right > 0)
    //         right = 0;
    // }
}

void Motor::front_pid_speed(int target)
{
    absolute_time_t current_time = get_absolute_time();

    error[0] = (target - cA) / (target / 100);
    error[1] = (target - cB) / (target / 100);

    double derivative0 = ((error[0] - prev_error[0]) / (current_time / 10000 - prev_time / 10000));
    double derivative1 = ((error[1] - prev_error[1]) / (current_time / 10000 - prev_time / 10000));

    speed[0] = (kp * error[0] + kd * derivative0 + ki * integral[0])/2;
    speed[1] = (kp * error[1] + kd * derivative1 + ki * integral[1])/2;

    if (cA > cB + THRESHOLD)
    {
        speed[0] = 0;
    }
    else if (cB > cA + THRESHOLD)
    {
        speed[1] = 0;
    }

    prev_error[0] = error[0];
    prev_error[1] = error[1];

    integral[0] += error[0];
    integral[1] += error[1];

    prev_time = current_time;
}

void Motor::set_motor()
{
    // Motor A
    int forward1 = abs(speed[0]);
    gpio_put(Motor::MOTOR_A_FRONT, forward1);
    gpio_put(Motor::MOTOR_A_BACK, !forward1);
    pwm_set_gpio_level(Motor::MOTOR_A_PWM, speed[0]);

    // Motor B
    int forward2 = abs(speed[1]);
    gpio_put(Motor::MOTOR_B_FRONT, forward2);
    gpio_put(Motor::MOTOR_B_BACK, !forward2);
    pwm_set_gpio_level(Motor::MOTOR_B_PWM, speed[1]);
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

Motor::Motor(double *arr, mutex_t *mut)
{
    this->mutex = mutex;
    this->distances = arr;
    compass.init();
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
    int steps = (units * STEPS_PER_UNIT);

    reinitvar();
    printf("Forward target = %d\n", steps);

    while (cA < steps || cB < steps)
    {
        front_pid_speed(steps);
        std::cout << speed[0] << " " << speed[1] << std::endl;
        uint32_t k;

        int left_dist = distances[0], right_dist = distances[3];
        int fL = distances[1], fR = distances[2];
        // std::cout << "move_f: " << left_dist << " " << fL << " " << fR << " " << right_dist << std::endl;
        std::cout << "move_f speed: " << speed[0] << " " << speed[1] << std::endl;

        if (fL < 8 || fR < 8)
        {
            speed[0] = 0;
            speed[1] = 0;
            set_motor();
            return;
        }

        if (left_dist < 6)
        {
            speed[1] /= 2;
        }
        else if (right_dist < 6)
        {
            speed[0] /= 2;
        }
        set_motor();

        sleep_ms(20);
    }

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);
}

void Motor::turn(double units) // 90 degree increments
{
    int target = compass.getAzimuth() + units;
    if (target > 360)
        target -= 360;
    reinitvar();

    int facing = compass.getAzimuth();

    // set_motor(0, 0);
    // set_motor(1, 0);

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);
    printf("Turn::cA %d\ncB %d\n\n", cA, cB);
}
