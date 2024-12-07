#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "VL53L0X.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#include "blink.pio.h"

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

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    // PIO Blinking example
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);

    VL53L0X s1;
    s1.init();
    s1.setTimeout(500);
    s1.setMeasurementTimingBudget(70000);
    s1.setSignalRateLimit(0.01);
    s1.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 18);

    while (true)
    {
        printf("dist = %d\n", s1.readRangeSingleMillimeters());
        sleep_ms(1000);
    }
}
