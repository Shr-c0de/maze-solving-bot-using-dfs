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

int Motor::calculate_pid_speed(int target, bool is_left)
{
   int index = !is_left;


int current_count = is_left ? cA : cB;
error[index] = (target - current_count) / (float)target; 
float derivative = error[index] - prev_error[index];
prev_error[index] = error[index];
integral[index] += error[index]; 


float pid_value = (kp * error[index] + kd * derivative + ki * integral[index]) * 255;




int deviation = cA - cB;
if (abs(deviation) >= THRESHOLD) {
    if (is_left && deviation > 0) { 
        pid_value *= 0.85; 
    } else if (!is_left && deviation < 0) { 
        pid_value *= 0.85; 
    }
}

return pid_value;
}

void Motor::set_motor(int motor, int pid_output, bool forward)
{

    bool actual_direction = pid_output >= 0 ? forward : !forward;
    int speed = abs(pid_output);

    
    if (speed < 50)
        speed = 50;
    if (speed > 255)
        speed = 255;

    
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
    //printf("%d\n", gpio);
    if (gpio == ENCODER_A)
    {
        //printf("%d %d\n", cA, cB);
        cA++;
    }
    else if (gpio == ENCODER_B)
    {
        //printf("%d %d\n", cA, cB);

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
    int steps = (units * STEPS_PER_UNIT);

   
    // pwm_set_gpio_level(MOTOR_A_PWM, 255);
    // pwm_set_gpio_level(MOTOR_B_PWM, 255);
    
    reinitvar();
        printf("Forward target = %d\n", steps);
    while (cA < steps || cB < steps)
    {
        int left_pid = calculate_pid_speed(steps, true);
        int right_pid = calculate_pid_speed(steps, false);

      

         int deviation = abs(cA - cB);

        if (deviation >= THRESHOLD * 2) { 
        printf("Motor synchronization issue detected! Stopping motors.\n");
            
            set_motor(0, 0, true);
            set_motor(1, 0, true);

            return; 
        }

        set_motor(0, left_pid, true);
        set_motor(1, right_pid, true);

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

void Motor::turn(float units, int direction) // 90 degree increments
{

   int steps = units * RATIO * WHEEL_BASE / WHEEL_DIAMETER / 4;
    printf("turn target = %d\n", steps);

    reinitvar();
    
    switch (direction)
    {
    case 0:
        gpio_put(MOTOR_A_FRONT, 0);
        gpio_put(MOTOR_A_BACK, 1);
        gpio_put(MOTOR_B_FRONT, 1);
        gpio_put(MOTOR_B_BACK, 0);
        break;
    case 1:
        gpio_put(MOTOR_A_FRONT, 1);
        gpio_put(MOTOR_A_BACK, 0);
        gpio_put(MOTOR_B_FRONT, 0);
        gpio_put(MOTOR_B_BACK, 1);
    }

   

    while (cA < steps && cB < steps)
    {
        int left_speed = calculate_pid_speed(steps, 1);
        set_motor(0, left_speed, direction);

        int right_speed = calculate_pid_speed(steps, 0);
        set_motor(1, right_speed, !direction);
    }

    pwm_set_gpio_level(MOTOR_A_PWM, 0);
    pwm_set_gpio_level(MOTOR_B_PWM, 0);

    //     while (cA < steps || cB < steps)
    //     {
    //         int left_speed = calculate_pid_speed(steps, true);
    //         int right_speed = calculate_pid_speed(steps, false);
    //         set_motor(0, left_speed, true);
    //         set_motor(1, right_speed, false);
    //     }

    gpio_put(MOTOR_A_FRONT, 0);
    gpio_put(MOTOR_A_BACK, 0);
    gpio_put(MOTOR_B_FRONT, 0);
    gpio_put(MOTOR_B_BACK, 0);
}

// void Motor::turn_right(float units)
// {
//     reinitvar();
//     int steps = RATIO * WHEEL_BASE / WHEEL_DIAMETER / 4;
//     cA = cB = 0;
//     // pwm_set_gpio_level(MOTOR_A_PWM, 255);
//     // pwm_set_gpio_level(MOTOR_B_PWM, 255);
//     pwm_set_gpio_level(MOTOR_A_PWM, 0);
//     pwm_set_gpio_level(MOTOR_B_PWM, 0);
//     gpio_put(MOTOR_A_FRONT, 0);
//     gpio_put(MOTOR_A_BACK, 0);
//     gpio_put(MOTOR_B_FRONT, 0);
//     gpio_put(MOTOR_B_BACK, 0);
//     set_motor(0, 0, true);
//     set_motor(1, 0, false);
// }

void Motor::curved_turn(float radius, float angle, bool is_left_turn)
{
    reinitvar();

    float angle_rad = angle * PI / 180.0;
    float inner_arc = radius * angle_rad;
    float outer_arc = (radius + WHEEL_BASE) * angle_rad;

    int inner_steps = (inner_arc * STEPS_PER_UNIT);
    int outer_steps = (outer_arc * STEPS_PER_UNIT);

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
            int inner_speed = calculate_pid_speed(inner_steps, true);
            int outer_speed = calculate_pid_speed(outer_steps, false);

            set_motor(0, inner_speed, true);
            set_motor(1, outer_speed, true);

            sleep_ms(10);
        }
    }
    else
    {
        while (cA < outer_steps || cB < inner_steps)
        {
            int outer_speed = calculate_pid_speed(outer_steps, false);
            int inner_speed = calculate_pid_speed(inner_steps, true);

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
    // motor.turn_left(1.0);
    // sleep_ms(1000);
    // motor.turn_right(1.0);
    // sleep_ms(1000);
    motor.curved_turn(10.0, 90.0, true);
    sleep_ms(1000);
    motor.curved_turn(10.0, 90.0, false);

    return 0;
}
