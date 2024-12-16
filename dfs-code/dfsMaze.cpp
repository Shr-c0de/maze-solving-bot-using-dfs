#include <stdio.h>
#include <vector>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
// pico libraries

#include "blink.pio.h"
#include "VL53L0X.h"
#include "motors.h"
#include "Sensors.h"
// user libraries

#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
// i2c pin definitons

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq)
{
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

int main()
{
    stdio_init_all();
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);
    sleep_ms(3000); // wait for stdio

    // blink, doesnt use cpu

    Sensor S;
    Motor M;
    while (1)
    {
        M.move_forward(1);
        sleep_ms(2000);
        M.turn_left(1);
        sleep_ms(1000);
        M.turn_right(1);
        sleep_ms(1000);
    }

    // i2c_scan();
    // sleep_ms(1000);
    // S.init();
    // printf("Program starts:\n\n");
    // i2c_scan();

    // int arr[4];
    // while (1)
    // {

    //     S.readings(arr);
    //     for (int i = 0; i < 4; i++)
    //     {
    //         printf("%d\n", arr[i]);
    //     }
    //     sleep_ms(1000);
    //     i2c_scan();
    //     M.move_forward(1);
    // }
}