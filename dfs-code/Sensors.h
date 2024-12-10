#ifndef SENSORS
#define SENSORS

#include <stdio.h>
#include <vector>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
// pico libraries

//#include "blink.pio.h"
#include "VL53L0X.h"

#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

bool reserved_addr(uint8_t addr);
void i2c_scan();

class Sensor
{
private:
    int xshut[4] = {2, 3, 4, 5};
    uint8_t addr[4] = {0x30, 0x31, 0x32, 0x33};
    VL53L0X s[4];
    void reboot(int i);

public:
    Sensor();
    void init();
    void readings(int *arr);
};

#endif