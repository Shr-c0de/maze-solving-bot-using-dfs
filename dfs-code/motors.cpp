#include "motors.h"


int Motor::calculate_pid_speed(int target, int current, float &error, float &prev_error, float &integral)
{
    error = target - current;
    integral += error;
    float derivative = error - prev_error;
    prev_error = error;

    int speed = static_cast<int>(kp * error + ki * integral + kd * derivative);
    return speed > 255 ? 255 : (speed < 0 ? 0 : speed);
}

void Motor::set_motor(int motor, int speed, bool forward)
{
     if (motor == 0) { // Motor A
        gpio_put(FORWARD_A, forward ? 1 : 0);
        gpio_put(BACKWARD_A, forward ? 0 : 1);
        pwm_set_gpio_level(MOTOR_ENABLE_A, speed);
    } else if (motor == 1) { // Motor B
        gpio_put(FORWARD_B, forward ? 1 : 0);
        gpio_put(BACKWARD_B, forward ? 0 : 1);
        pwm_set_gpio_level(MOTOR_ENABLE_B, speed);
    }
}

 void Motor::global_encoder_irq_handler(uint gpio, uint32_t events) {
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


    gpio_init(FORWARD_A);
    gpio_init(BACKWARD_A);
    gpio_init(FORWARD_B);
    gpio_init(BACKWARD_B);
    gpio_set_dir(FORWARD_A, GPIO_OUT);
    gpio_set_dir(BACKWARD_A, GPIO_OUT);
    gpio_set_dir(FORWARD_B, GPIO_OUT);
    gpio_set_dir(BACKWARD_B, GPIO_OUT);

    slice_num_a = pwm_gpio_to_slice_num(MOTOR_A_PWM);
    slice_num_b = pwm_gpio_to_slice_num(MOTOR_B_PWM);

    pwm_set_wrap(slice_num_a, 255);
    pwm_set_wrap(slice_num_b, 255);
    pwm_set_enabled(slice_num_a, true);
    pwm_set_enabled(slice_num_b, true);
}

void Motor::init_encoders()
{
    gpio_set_irq_enabled_with_callback(ENCODER_A, GPIO_IRQ_EDGE_RISE, true, &encoder_a_irq_handler);
    gpio_set_irq_enabled_with_callback(ENCODER_B, GPIO_IRQ_EDGE_RISE, true, &encoder_b_irq_handler);
}

void Motor::move_forward(float units)
{
    int steps = static_cast<int>(units * STEPS_PER_UNIT);
    cA = cB = 0;

    gpio_put(FORWARD_A, 1);
    gpio_put(BACKWARD_A, 0);
    gpio_put(FORWARD_B, 1);
    gpio_put(BACKWARD_B, 0);

    gpio_put(MOTOR_ENABLE_A, 1);
    gpio_put(MOTOR_ENABLE_B, 1);


    while (cA < steps && cB < steps)
    {
        int left_speed = calculate_pid_speed(steps, cA, error_a, prev_error_a, integral_a);
        int right_speed = calculate_pid_speed(steps, cB, error_b, prev_error_b, integral_b);

        set_motor(0, left_speed, true);
        set_motor(1, right_speed, true);

        sleep_ms(10);
    }

    gpio_put(MOTOR_ENABLE_A, 0);
    gpio_put(MOTOR_ENABLE_B, 0);
}

void Motor::move_backward(float units) {
    int steps = static_cast<int> (units * STEPS_PER_UNIT);

    cA = 0;
    cB = 0;

    gpio_put(FORWARD_A, 0);
    gpio_put(BACKWARD_A, 1);
    gpio_put(FORWARD_B, 0);
    gpio_put(BACKWARD_B, 1);

    gpio_put(MOTOR_ENABLE_A, 1);
    gpio_put(MOTOR_ENABLE_B, 1);

    while (cA < steps && cB < steps) {
        int left_speed = calculate_pid_speed(steps, cA, error_a, prev_error_a, integral_a);
        int right_speed = calculate_pid_speed(steps, cB, error_b, prev_error_b, integral_b);

        set_motor(0, false, speed_a); 
        set_motor(1, false, speed_b); 
    }

    gpio_put(MOTOR_ENABLE_A, 0);
    gpio_put(MOTOR_ENABLE_B, 0);
}

void Motor::turn_left(float units)
{
    int steps = static_cast<int>((units * STEPS_PER_UNIT) / 3.2);
    cA = cB = 0;

    gpio_put(FORWARD_A, 0);
    gpio_put(BACKWARD_A, 1);
    gpio_put(FORWARD_B, 1);
    gpio_put(BACKWARD_B, 0);

    gpio_put(MOTOR_ENABLE_A, 1);
    gpio_put(MOTOR_ENABLE_B, 1);

    while (cA < steps || cB < steps)
    {
        int left_speed = calculate_pid_speed(steps, cA, error_a, prev_error_a, integral_a);
        int right_speed = calculate_pid_speed(steps, cB, error_b, prev_error_b, integral_b);

        set_motor(1, right_speed, true);
        set_motor(0, left_speed, false);
    }

    gpio_put(MOTOR_ENABLE_A, 0);
    gpio_put(MOTOR_ENABLE_B, 0);
}

void Motor::turn_right(float units)
{
    int steps = static_cast<int>((units * STEPS_PER_UNIT) / 3.2);
    cA = cB = 0;

    while (cA < steps || cB < steps)
    {
        int left_speed = calculate_pid_speed(steps, cA, error_a, prev_error_a, integral_a);
        int right_speed = calculate_pid_speed(steps, cB, error_b, prev_error_b, integral_b);

        set_motor(0, left_speed, true);
        set_motor(1, right_speed, false);
    }

    
    gpio_put(MOTOR_ENABLE_A, 0);
    gpio_put(MOTOR_ENABLE_B, 0);
}

void Motor::curved_turn(float radius, float angle, bool is_left_turn)
{
    float angle_rad = angle * PI / 180.0;
    float inner_arc = radius * angle_rad;
    float outer_arc = (radius + WHEEL_BASE) * angle_rad;

    int inner_steps = static_cast<int>(inner_arc * STEPS_PER_UNIT);
    int outer_steps = static_cast<int>(outer_arc * STEPS_PER_UNIT);

    cA = cB = 0;

    if (is_left_turn)
    {
        while (cA < inner_steps || cB < outer_steps)
        {
            int inner_speed = calculate_pid_speed(inner_steps, cA, error_a, prev_error_a, integral_a);
            int outer_speed = calculate_pid_speed(outer_steps, cB, error_b, prev_error_b, integral_b);

            set_motor(0, inner_speed, true);
            set_motor(1, outer_speed, true);

            sleep_ms(10);
        }
    }
    else
    {
        while (cA < outer_steps || cB < inner_steps)
        {
            int outer_speed = calculate_pid_speed(outer_steps, cA, error_a, prev_error_a, integral_a);
            int inner_speed = calculate_pid_speed(inner_steps, cB, error_b, prev_error_b, integral_b);

            set_motor(0, outer_speed, true);
            set_motor(1, inner_speed, true);

            sleep_ms(10);
        }
    }

    set_motor(0, 0, true);
    set_motor(1, 0, true);
}


int motor_example()
{
    Motor motor;

    motor.move_forward(1.0);
    sleep_ms(1000);
    motor.turn_left(1.0);
    sleep_ms(1000);
    motor.turn_right(1.0);
    sleep_ms(1000);
    motor.curved_turn(10.0, 90.0, true);
    sleep_ms(1000);
    motor.curved_turn(10.0, 90.0, false);

    return 0;
}
