#include "Sensors.h"

bool reserved_addr(uint8_t addr)
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

bool Sensor::fixsensor()
{
    uint address;
    uint8_t rxdata;
    for (int i = 0; i < 4; i++)
    {
        if (i2c_read_blocking(i2c_default, addr[i], &rxdata, 1, false) < 0)
        {
            printf("Error found\nreinit called\n");
            reboot(i);
        }
    }
    return 1;
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
// i2c checkers

// if sensors give bad reading, we can reboot individually
void Sensor::reboot(int i)
{
    gpio_init(xshut[i]);
    gpio_set_dir(xshut[i], GPIO_OUT);
    gpio_put(xshut[i], 0);
    printf("Sensor %d setup\n", i + 1);
    gpio_put(xshut[i], 1);
    sleep_ms(50);
    VL53L0X tmp;
    tmp.init();
    tmp.setTimeout(500);
    tmp.setMeasurementTimingBudget(70000);
    tmp.setSignalRateLimit(0.01);
    tmp.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 18);
    tmp.setAddress(addr[i]);
    s[i] = tmp;
    sleep_ms(50);
}

// initialises all sensors, switches them all off.
Sensor::Sensor()
{
#ifndef i2c_begin
#define i2c_begin
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
 
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
#endif
    // switching off all sensors, so we can initialise them one-by-one in init()
    for (int i = 0; i < 4; i++)
    {
        gpio_init(xshut[i]);
        gpio_set_dir(xshut[i], GPIO_OUT);
        gpio_put(xshut[i], 0);
    }
    sleep_ms(100); // to stabilise
}

// starts the sensors, gives each different addresses,
void Sensor::init()
{
    for (int i = 0; i < 4; i++)
    {
        printf("Sensor %d setup\n", i + 1);
        gpio_put(xshut[i], 1);
        sleep_ms(50);
        VL53L0X tmp;
        tmp.init();
        tmp.setTimeout(500);
        tmp.setMeasurementTimingBudget(70000);
        tmp.setSignalRateLimit(0.01);
        tmp.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 18);
        tmp.setAddress(addr[i]);
        s[i] = tmp;
        sleep_ms(50);
    }
    printf("Ready to go!");
}

// get readings
void Sensor::readings(int *arr)
{
    uint8_t rxdata;
    if (i2c_read_blocking(i2c_default, 0x29, &rxdata, 1, false) >= 0)
        fixsensor();
    for (int i = 0; i < 4; i++)
    {
        arr[i] = s[i].readRangeSingleMillimeters();
    }
}

int sensor_example()
{

    stdio_init_all();
    sleep_ms(3000);

    // blink, doesnt use cpu
    // PIO pio = pio0;
    // uint offset = pio_add_program(pio, &blink_program);
    // blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);

    Sensor S;
    i2c_scan();
    sleep_ms(1000);
    S.init();
    printf("Program starts:\n\n");
    i2c_scan();

    int arr[4];
    while (1)
    {

        S.readings(arr);
        for (int i = 0; i < 4; i++)
        {
            printf("%d\n", arr[i]);
        }
        sleep_ms(1000);
        i2c_scan();
    }
}
