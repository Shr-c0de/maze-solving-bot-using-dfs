#ifndef MOTOR
#define MOTOR

#include <stdio.h>
#include<iostream>
#include "pico/stdlib.h"
#include "Sensors.h"
//#include "pico/math.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include <cmath>

#define current_count(is_left, cA, cB) ((is_left) ? (cA) : (cB))
#define RATIO 100
#define WHEEL_DIAMETER 4.3 // cm
#define WHEEL_BASE 14.5
#define PI 3.1415
#define THRESHOLD 5

#define ENCODER_A 14
#define ENCODER_B 15
class Motor
{
private:
    Sensor* s;
    double* distances;

    const int STEPS_PER_UNIT = (RATIO / WHEEL_DIAMETER / PI) * 32.0;

    uint slice_num_a, slice_num_b;

    const double kp = 0.7, ki = 0.2, kd = 0.5;
    double error[2] = {0, 0};
    double prev_error[2] = {0, 0};
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

    int calculate_pid_speed(int target, bool is_left);
    void set_motor(int motor, int pwm);

    static void global_encoder_irq_handler(uint gpio, uint32_t events);
    static void global_encoder_irq_handler_neg(uint gpio, uint32_t events);
    void valcheck(int &left, int &right);

public:
    // a->left, b->right
    Motor(Sensor &s);
    void move_forward(double units);
    void turn(double units, bool direction);
    void curved_turn(double radius, double angle, bool is_left_turn);
};

// void write_register(uint8_t reg, uint8_t value) {
//     uint8_t buffer[2] = {reg, value};
//     i2c_write_blocking(I2C_PORT, HMC5883L_ADDR, buffer, 2, false);
// }

// // Function to read multiple bytes from a register
// void read_register(uint8_t reg, uint8_t *buffer, size_t length) {
//     i2c_write_blocking(I2C_PORT, HMC5883L_ADDR, &reg, 1, true);
//     i2c_read_blocking(I2C_PORT, HMC5883L_ADDR, buffer, length, false);
// }

// int main() {
//     stdio_init_all();

//     // Initialize I2C
//     i2c_init(I2C_PORT, 100 * 1000); // 100 kHz
//     gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
//     gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
//     gpio_pull_up(SDA_PIN);
//     gpio_pull_up(SCL_PIN);

//     // Configure HMC5883L
//     write_register(CONFIG_REG_A, 0x70); // 15Hz, normal measurement mode
//     write_register(CONFIG_REG_B, 0x20); // Gain configuration
//     write_register(MODE_REG, 0x00);     // Continuous measurement mode

//     while (true) {
//         uint8_t data[6];
//         read_register(DATA_REG, data, 6);

//         int16_t x = (data[0] << 8) | data[1];
//         int16_t z = (data[2] << 8) | data[3];
//         int16_t y = (data[4] << 8) | data[5];

//         // Convert raw values to signed integers
//         if (x > 32767) x -= 65536;
//         if (y > 32767) y -= 65536;
//         if (z > 32767) z -= 65536;

//         // Convert to microteslas
//         float xf = x * LSB_TO_UT;
//         float yf = y * LSB_TO_UT;
//         float zf = z * LSB_TO_UT;

//         // Calculate heading in degrees
//         float heading = atan2f(yf, xf) * (180.0 / M_PI);
//         if (heading < 0) heading += 360;

//         // Print results
//         printf("Magnetic field in X: %.2f uT, Y: %.2f uT, Z: %.2f uT, Heading: %.2f°\n", xf, yf, zf, heading);

//         sleep_ms(100);
//     }

//     return 0;
// }

#endif
