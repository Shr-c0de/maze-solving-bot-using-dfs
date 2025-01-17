#ifndef SENSORS
#define SENSORS

#include <stdio.h>
#include <vector>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include<cmath>
// pico libraries

// #include "blink.pio.h"
#include "VL53L0X.h"

#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

bool reserved_addr(uint8_t addr);
void i2c_scan();

class Sensor
{
private:
    int xshut[4] = {2, 3, 4, 5}; // left, left front, right front, right
    double sensor[4];
    uint8_t addr[4] = {0x30, 0x31, 0x32, 0x33};
    VL53L0X s[4];
    void reboot(int i);

public:
    bool fixsensor();
    Sensor();
    void init();
    void readings(double *arr);
};

// QMC5883L Constants
#define QMC5883L_ADDR 0x0D
#define CONTROL_REG 0x09
#define DATA_REG 0x00
#define SET_RESET_REG 0x0B

class QMC5883LCompass
{
private:
  uint8_t _ADDR;
  float _magneticDeclinationDegrees = 0.0;
  float _offset[3] = {0.0, 0.0, 0.0};
  float _scale[3] = {1.0, 1.0, 1.0};
  int16_t _vRaw[3] = {0, 0, 0};
  float _vCalibrated[3] = {0.0, 0.0, 0.0};

  void _writeReg(uint8_t reg, uint8_t value);

  void _applyCalibration();

public:
  QMC5883LCompass();

  void init();

  void setMode(uint8_t mode, uint8_t odr, uint8_t rng, uint8_t osr);

  void setMagneticDeclination(int degrees, uint8_t minutes);

  void read();

  int getX();

  int getY();

  int getZ();

  int getAzimuth();
};

#endif