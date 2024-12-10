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

int main(){
    //alg goes here
}