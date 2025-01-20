#include "motors.h"

#ifndef ENC_PIN
#define ENC_PIN
volatile long cA = 0;
volatile long cB = 0;
#endif
int constrain(double speed, int max, int min)
{
    if (speed > max)
    {
        speed = max;
    }
    else if (speed < min)
    {
        speed = min;
    }
    return speed;
}
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

void Motor::set_motor()
{
    // Motor A
    int forward1 = (speed[0] > 0 ? 1 : 0);
    gpio_put(Motor::MOTOR_A_FRONT, forward1);
    gpio_put(Motor::MOTOR_A_BACK, !forward1);
    pwm_set_gpio_level(Motor::MOTOR_A_PWM, abs(speed[0]));

    // Motor B
    int forward2 = (speed[1] > 0 ? 1 : 0);
    gpio_put(Motor::MOTOR_B_FRONT, forward2);
    gpio_put(Motor::MOTOR_B_BACK, !forward2);
    pwm_set_gpio_level(Motor::MOTOR_B_PWM, abs(speed[1]));
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

void Motor::front_pid_speed(int target)
{
    absolute_time_t current_time = get_absolute_time();

    error[0] = (target - cA) / (target / 100);
    error[1] = (target - cB) / (target / 100);

    double derivative0 = ((error[0] - prev_error[0]) / (current_time / 10000 - prev_time / 10000));
    double derivative1 = ((error[1] - prev_error[1]) / (current_time / 10000 - prev_time / 10000));

    speed[0] = (kp * error[0] + kd * derivative0 + ki * integral[0]) / 10;
    speed[1] = (kp * error[1] + kd * derivative1 + ki * integral[1]) / 10;

    prev_error[0] = error[0];
    prev_error[1] = error[1];

    integral[0] += error[0];
    integral[1] += error[1];

    prev_time = current_time;
}

void Motor::move_forward(double units)
{
    int steps = (units * 700);

    reinitvar();
    printf("Forward target = %d\n", steps);

    while (cA < steps || cB < steps)
    {
        // front_pid_speed(steps);
        // std::cout << speed[0] << " " << speed[1] << std::endl;
        uint32_t k;
        // mutex_enter_blocking(mutex);
        int left_dist = distances[0], right_dist = distances[3];
        int fL = distances[1], fR = distances[2];

        speed[0] = 90;
        speed[1] = 90;

        if (cA > cB + THRESHOLD)
        {
            speed[0] -= 70;
        }
        else if (cB > cA + THRESHOLD)
        {
            speed[1] -= 70;
        }

        if ((left_dist < 8) || (right_dist > 11 && right_dist < 31))
        {
            speed[1] -= 30;
            speed[0] += 30;
        }
        else if ((right_dist < 8) || (left_dist > 11 && left_dist < 31))
        {
            speed[1] += 30;
            speed[0] -= 30;
        }
        if (fL < 20 || fR < 20)
        {
            speed[0] = 0;
            speed[1] = 0;
            set_motor();
            break;
        }
        set_motor();
    }

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);
    sleep_ms(500);
}

// void Motor::turn_PID(int target, int azimuth)
// {
//     absolute_time_t current_time = get_absolute_time();
//     turn_error = (target - azimuth); // func
//     double derivative = ((turn_error - prev_turn) / (current_time / 10000 - prev_time / 10000));
//     speed[0] = (kp * error[0] + kd * derivative + ki * integral[0]) / 2;
//     if (cA > cB + THRESHOLD)
//     {
//         speed[0] = 0;
//     }
//     else if (cB > cA + THRESHOLD)
//     {
//         speed[1] = 0;
//     }
//     prev_error[0] = error[0];
//     prev_error[1] = error[1];
//     integral[0] += error[0];
//     integral[1] += error[1];
//     prev_time = current_time;
// }
// void Motor::turn(double degree)
// {
//     int target = compass.getAzimuth() + degree;
//     reinitvar();
//     int azimuth;
//     int facing = compass.getAzimuth();
//     while (abs(compass.getAzimuth() - target) > 3)
//     {
//         compass.read();
//         azimuth = compass.getAzimuth();
//         turn_PID(target, azimuth);
//         set_motor();
//     }
//     gpio_put(MOTOR_A_FRONT, 0);
//     gpio_put(MOTOR_A_BACK, 0);
//     gpio_put(MOTOR_B_FRONT, 0);
//     gpio_put(MOTOR_B_BACK, 0);
// }

void Motor::turn(double degree) // 0 is clockwise right, 1 is anticlockwise left
{
    int target_angle = 190, direction = degree > 0 ? 1 : -1;
    reinitvar();

    while (cA < target_angle || cB < target_angle)
    {
        speed[0] = 60 * direction;
        speed[1] = 60 * (-direction);

        if (cA > cB + THRESHOLD)
        {
            speed[0] = 0;
        }
        else if (cB > cA + THRESHOLD)
        {
            speed[1] = 0;
        }
        set_motor();

        // sleep_ms(20);
    }

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);
    sleep_ms(500);
}

// void Motor::turn_pid_speed(int target_angle)
// {
//     absolute_time_t current_time = get_absolute_time();

//     turn_error = target_angle - compass.getAzimuth();

//     if (turn_error > 180)
//         turn_error -= 360;
//     else if (turn_error < -180)
//         turn_error += 360;

//     double delta_time = absolute_time_diff_us(prev_time, current_time) / 1000000.0;
//     if(delta_time < 0.0001)
//     {
//         delta_time = 0.0001;
//     }

//     double derivative = (turn_error - prev_turn) / delta_time;

//     if(abs(turn_error) > TURN_THRESHOLD)
//     {
//     integral[0] += turn_error * delta_time;
//     integral[0] = constrain(integral[0], -1000, 1000);
//     }

//     double turn_speed = (ktp * turn_error) + (ktd * derivative) + (kti * integral[0]);

//     turn_speed = constrain(turn_speed, -150, 150);

//      if(abs(turn_speed) <10) turn_speed = (turn_speed > 0) ? 10 : -10;

//     speed[0] = turn_speed;
//     speed[1] = -turn_speed;

//     prev_turn = turn_error;
//     prev_time = current_time;
// }

// void Motor::turn(double degree)
// {
//     int initial_azimuth = compass.getAzimuth();
//     int target_angle = initial_azimuth + degree;

//     if (target_angle >= 360)
//         target_angle -= 360;
//     else if (target_angle < 0)
//         target_angle += 360;

//     reinitvar();

//     while (true)
//     {
//         compass.read();

//         int current_azimuth = compass.getAzimuth();
//         int error = target_angle - current_azimuth;

//         if (error > 180)
//             error -= 360;
//         else if (error < -180)
//             error += 360;

//         if (abs(error) <= TURN_THRESHOLD)
//             break;

//         turn_pid_speed(target_angle);

//         set_motor();

//         sleep_ms(1000);
//     }

//     gpio_put(MOTOR_A_FRONT, 0);
//     gpio_put(MOTOR_A_BACK, 0);
//     gpio_put(MOTOR_B_FRONT, 0);
//     gpio_put(MOTOR_B_BACK, 0);
// }
