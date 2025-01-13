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
            right = 100;
    }
    else if (cB - cA > THRESHOLD)
    {
        if (left < 100)
            left = 100;
        if (right > 0)
            right = 0;
    }
}

int Motor::calculate_pid_speed(int inval, bool is_left)
{
    int target = (inval > 0 ? inval : -inval);
    int direction = inval / target;
    int index = !is_left;

    error[index] = (double)((target - current_count(is_left, cA, cB)) / (target / 100.0));

    double derivative = error[index] - prev_error[index];

    int pid_value = (kp * error[index] + kd * derivative + ki * integral[index]) / 60.0;

    // //printf("Target: %d, Current: %d\n", target, current_count(is_left, cA, cB));

    prev_error[index] = error[index];
    integral[index] += error[index];

    pid_value = (pid_value > 100) ? 100 : pid_value;

    int final_speed = (pid_value) * (direction);

    return final_speed;
}

void Motor::set_motor(int motor, int pid)
{
    bool forward = (pid >= 0);

    int speed = abs(pid);

    if (motor == 0)
    { // Motor A
        gpio_put(Motor::MOTOR_A_FRONT, forward);
        gpio_put(Motor::MOTOR_A_BACK, !forward);
        pwm_set_gpio_level(Motor::MOTOR_A_PWM, speed);
    }
    else if (motor == 1)
    { // Motor B
        gpio_put(Motor::MOTOR_B_FRONT, forward);
        gpio_put(Motor::MOTOR_B_BACK, !forward);
        pwm_set_gpio_level(Motor::MOTOR_B_PWM, speed);
    }
}

void Motor::global_encoder_irq_handler(uint gpio, uint32_t events)
{
    // //printf("%d %d\n", cA, cB);

    if (gpio == ENCODER_A)
    {
        // //printf("%d %d\n", cA, cB);
        cA++;
    }
    else if (gpio == ENCODER_B)
    {
        //

        cB++;
    }
}

void Motor::global_encoder_irq_handler_neg(uint gpio, uint32_t events)
{
    // //printf("%d %d\n", cA, cB);

    if (gpio == ENCODER_A)
    {
        // //printf("%d %d\n", cA, cB);
        cA--;
    }
    else if (gpio == ENCODER_B)
    {
        //

        cB--;
    }
}

Motor::Motor(Sensor &sensor)
{   
    s = &sensor;
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

    pwm_set_wrap(slice_num_a, 300);
    pwm_set_wrap(slice_num_b, 300);
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
    // printf("Forward target = %d\n", steps);

    while (cA < steps || cB < steps)
    {
        s->readings(distances);
        int left_pid = calculate_pid_speed(steps, true);
        int right_pid = calculate_pid_speed(steps, false);
        valcheck(left_pid, right_pid);

        // left/right checking code:
        int left = distances[0], right = distances[3];
        if (left > 8 && left < 30 || right < 4)
        {
            right_pid += 15;
            left_pid -= 15;
        }
        if (right > 8 && right < 30 || left < 4)
        {
            right_pid -= 15;
            left_pid += 15;
        }
        // code end
        //printf("PID values: %d %d \n", left_pid, right_pid);
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

void Motor::turn(double units, bool isleft) // 90 degree increments
{
    int steps = units * RATIO * (WHEEL_BASE / WHEEL_DIAMETER) / 4 + isleft * 10;
    reinitvar();
    // printf("%d\n", steps);

    while (cA < steps || cB < steps)
    {
        int left_speed = calculate_pid_speed((2 * isleft - 1) * steps, 1) / 1.5;
        int right_speed = calculate_pid_speed((2 * (!isleft) - 1) * steps, 0) / 1.5;
        valcheck(left_speed, right_speed);

        // printf("Left PID : %d: %d\nRight PID: %d, %d\n\n", left_speed, cA, right_speed, cB);

        set_motor(0, left_speed);
        set_motor(1, right_speed);
    }

    // if (cA > steps + THRESHOLD || cB > steps + THRESHOLD)
    // {
    //     gpio_set_irq_enabled_with_callback(ENCODER_A, GPIO_IRQ_EDGE_RISE, true, &global_encoder_irq_handler_neg);
    //     gpio_set_irq_enabled_with_callback(ENCODER_B, GPIO_IRQ_EDGE_RISE, true, &global_encoder_irq_handler_neg);
    //     while (cA > steps + THRESHOLD || cB > steps + THRESHOLD)
    //     {
    //         int left_speed = calculate_pid_speed((2 * !isleft - 1) * steps, 1);
    //         int right_speed = calculate_pid_speed((2 * (isleft) - 1) * steps, 0);
    //         //printf("speeds: %d\t%d\n", left_speed, right_speed);
    //         set_motor(0, left_speed);
    //         set_motor(1, right_speed);
    //     }
    //     gpio_set_irq_enabled_with_callback(ENCODER_A, GPIO_IRQ_EDGE_RISE, true, &global_encoder_irq_handler);
    //     gpio_set_irq_enabled_with_callback(ENCODER_B, GPIO_IRQ_EDGE_RISE, true, &global_encoder_irq_handler);
    // }

    set_motor(0, 0);
    set_motor(1, 0);

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);
    sleep_ms(500);
}

void Motor::curved_turn(double radius, double angle, bool is_left_turn)
{
    // reinitvar();

    // float angle_rad = angle * PI / 180.0;
    // float inner_arc = radius * angle_rad;
    // float outer_arc = (radius + WHEEL_BASE) * angle_rad;

    // int inner_steps = (inner_arc * STEPS_PER_UNIT);
    // int outer_steps = (outer_arc * STEPS_PER_UNIT);

    // cA = cB = 0;

    // gpio_put(MOTOR_A_FRONT, 1);
    // gpio_put(MOTOR_A_BACK, 0);
    // gpio_put(MOTOR_B_FRONT, 1);
    // gpio_put(MOTOR_B_BACK, 0);

    // if (is_left_turn)
    // {
    //     while (cA < inner_steps || cB < outer_steps)
    //     {
    //         int inner_speed = calculate_pid_speed(inner_steps, true);
    //         int outer_speed = calculate_pid_speed(outer_steps, false);

    //         set_motor(0, inner_speed, true);
    //         set_motor(1, outer_speed, true);

    //         sleep_ms(10);
    //     }
    // }
    // else
    // {
    //     while (cA < outer_steps || cB < inner_steps)
    //     {
    //         int outer_speed = calculate_pid_speed(outer_steps, false);
    //         int inner_speed = calculate_pid_speed(inner_steps, true);

    //         set_motor(0, outer_speed, true);
    //         set_motor(1, inner_speed, true);

    //         sleep_ms(10);
    //     }
    // }

    // pwm_set_gpio_level(MOTOR_A_PWM, 0);
    // pwm_set_gpio_level(MOTOR_B_PWM, 0);

    // set_motor(0, 0, true);
    // set_motor(1, 0, true);

    // gpio_put(MOTOR_A_FRONT, 0);
    // gpio_put(MOTOR_A_BACK, 0);
    // gpio_put(MOTOR_B_FRONT, 0);
    // gpio_put(MOTOR_B_BACK, 0);
}

// int motor_example()
// {
//     Motor motor;

//     motor.move_forward(1.0);
//     sleep_ms(1000);
//     // motor.turn_left(1.0);
//     // sleep_ms(1000);
//     // motor.turn_right(1.0);
//     // sleep_ms(1000);
//     motor.curved_turn(10.0, 90.0, true);
//     sleep_ms(1000);
//     motor.curved_turn(10.0, 90.0, false);

//     return 0;
// }
