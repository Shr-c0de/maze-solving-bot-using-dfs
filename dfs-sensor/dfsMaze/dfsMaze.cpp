#include <stdio.h>
#include <vector>
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

bool reserved_addr(uint8_t addr)
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}
void i2c_scan()
{
    printf("  00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
    for (int addr = 0; addr < (1 << 7); ++addr)
    {
        if (addr % 16 == 0)
        {
            printf("%02x ", addr);
        }
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_blocking(i2c_default, addr, &rxdata, 1, false);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");
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

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);

    int xshut[2] = {4, 5};
    for (int i = 0; i < 2; i++)
    {
        gpio_init(xshut[i]);
        gpio_set_dir(xshut[i], GPIO_OUT);
        gpio_put(xshut[i], 0);
    }
    sleep_ms(5000);
    printf("Program starts:\n\n");
    i2c_scan();

    VL53L0X sensor[2];
    uint8_t addr[2] = {0x30, 0x40};

    for (int i = 0; i < 2; i++)
    {
        printf("Sensor %d setup\n", i);
        gpio_put(xshut[i], 1);
        sleep_ms(50);
        VL53L0X tmp;
        tmp.init();
        tmp.setTimeout(500);
        tmp.setMeasurementTimingBudget(70000);
        tmp.setSignalRateLimit(0.01);
        tmp.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 18);
        tmp.setAddress(addr[i]);
        sensor[i] = tmp;
        i2c_scan();
        sleep_ms(500);
    }

    while (1)
    {
        printf("sensor1 value = %d\n sensor2 value = %d\n\n",
               sensor[0].readRangeSingleMillimeters(), sensor[1].readRangeSingleMillimeters());
        sleep_ms(1000);
    }
}
