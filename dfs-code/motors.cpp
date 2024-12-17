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
    }
}

int Motor::calculate_pid_speed(int target, bool is_left, float &error, float &prev_error, float &integral)
{
     int current = is_left ? cA : cB;    
    
    error = target - current;
    integral += error;
    float derivative = error - prev_error;
    prev_error = error;

    int speed = static_cast<int>(kp * error + ki * integral + kd * derivative);
    return speed > 255 ? 255 : (speed < 0 ? 0 : speed);
}

void Motor::set_motor(int motor, int speed, bool forward)
{
    if (motor == 0)
    { // Motor A
        gpio_put(Motor::MOTOR_A_FRONT, forward ? 1 : 0);
        gpio_put(Motor::MOTOR_A_BACK, forward ? 0 : 1);
        pwm_set_gpio_level(Motor::MOTOR_A_PWM, speed);
    }
    else if (motor == 1)
    { // Motor B
        gpio_put(Motor::MOTOR_B_FRONT, forward ? 1 : 0);
        gpio_put(Motor::MOTOR_B_BACK, forward ? 0 : 1);
        pwm_set_gpio_level(Motor::MOTOR_B_PWM, speed);
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

void Motor::move_forward(float units)
{
    reinitvar();
    int steps = static_cast<int>(units * STEPS_PER_UNIT);
    cA = cB = 0;

    gpio_put(MOTOR_A_FRONT, 1);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 1);
    gpio_put(MOTOR_B_BACK, 0);

    
    // pwm_set_gpio_level(MOTOR_A_PWM, 255);
    // pwm_set_gpio_level(MOTOR_B_PWM, 255);
   

    while (cA < steps || cB < steps)
    {
        int left_speed = calculate_pid_speed(steps, true, error[0], prev_error[0], integral[0]);
        int right_speed = calculate_pid_speed(steps, false, error[1], prev_error[1], integral[1]);

        if(abs(steps - cA) < 3) left_speed = 0;
        if(abs(steps - cB) < 3) right_speed = 0;

        set_motor(0, left_speed, true);
        set_motor(1, right_speed, true);

        sleep_ms(10);
    }

    pwm_set_gpio_level(MOTOR_A_PWM, 0);
    pwm_set_gpio_level(MOTOR_B_PWM, 0);

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);

    set_motor(0, 0, true);
    set_motor(1, 0, true);
}

void Motor::turn_left(float units)
{
    reinitvar();
    int steps = static_cast<int>((units * STEPS_PER_UNIT) / 3.2);
    cA = cB = 0;

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 1);
    gpio_put(MOTOR_B_FRONT, 1);
    gpio_put(MOTOR_B_BACK, 0);

    // pwm_set_gpio_level(MOTOR_A_PWM, 255);
    // pwm_set_gpio_level(MOTOR_B_PWM, 255);


    while (cA < steps || cB < steps)
    {
        int left_speed = calculate_pid_speed(steps, true, error[0], prev_error[0], integral[0]);
        int right_speed = calculate_pid_speed(steps, false, error[1], prev_error[1], integral[1]);

        set_motor(1, right_speed, true);
        set_motor(0, left_speed, false);
    }

  pwm_set_gpio_level(MOTOR_A_PWM, 0);
  pwm_set_gpio_level(MOTOR_B_PWM, 0);


    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);

    set_motor(0, 0, true);
    set_motor(1, 0, false);
}

void Motor::turn_right(float units)
{
    reinitvar();

    int steps = static_cast<int>((units * STEPS_PER_UNIT) / 3.2);
    cA = cB = 0;

    gpio_put(MOTOR_A_FRONT, 1);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 1);

    // pwm_set_gpio_level(MOTOR_A_PWM, 255);
    // pwm_set_gpio_level(MOTOR_B_PWM, 255);


    while (cA < steps || cB < steps)
    {
        int left_speed = calculate_pid_speed(steps, true, error[0], prev_error[0], integral[0]);
        int right_speed = calculate_pid_speed(steps, false, error[1], prev_error[1], integral[1]);

        set_motor(0, left_speed, true);
        set_motor(1, right_speed, false);
    }

   pwm_set_gpio_level(MOTOR_A_PWM, 0);
   pwm_set_gpio_level(MOTOR_B_PWM, 0);


    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);

    set_motor(0,0, true);
    set_motor(1, 0, false);
}

void Motor::curved_turn(float radius, float angle, bool is_left_turn)
{
    reinitvar();

    float angle_rad = angle * PI / 180.0;
    float inner_arc = radius * angle_rad;
    float outer_arc = (radius + WHEEL_BASE) * angle_rad;

    int inner_steps = static_cast<int>(inner_arc * STEPS_PER_UNIT);
    int outer_steps = static_cast<int>(outer_arc * STEPS_PER_UNIT);

    cA = cB = 0;

    gpio_put(MOTOR_A_FRONT, 1);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 1);
    gpio_put(MOTOR_B_BACK, 0);

    // pwm_set_gpio_level(MOTOR_A_PWM, 255);
    // pwm_set_gpio_level(MOTOR_B_PWM, 255);

    if (is_left_turn)
    {
        while (cA < inner_steps || cB < outer_steps)
        {
            int inner_speed = calculate_pid_speed(inner_steps, true, error[0], prev_error[0], integral[0]);
            int outer_speed = calculate_pid_speed(outer_steps, false, error[1], prev_error[1], integral[1]);

            set_motor(0, inner_speed, true);
            set_motor(1, outer_speed, true);

            sleep_ms(10);
        }
    }
    else
    {
        while (cA < outer_steps || cB < inner_steps)
        {
            int outer_speed = calculate_pid_speed(outer_steps, cA, error[0], prev_error[0], integral[0]);
            int inner_speed = calculate_pid_speed(inner_steps, cB, error[1], prev_error[1], integral[1]);

            set_motor(0, outer_speed, true);
            set_motor(1, inner_speed, true);

            sleep_ms(10);
        }
    }

    pwm_set_gpio_level(MOTOR_A_PWM, 0);
    pwm_set_gpio_level(MOTOR_B_PWM, 0);

    set_motor(0, 0, true);
    set_motor(1, 0, true);

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);
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
