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
    // s[i].writeReg(0x00, 0x00);
    s[i].stopContinuous();
    gpio_put(xshut[i], 0);
    printf("Sensor %d setup Address : %d\n", i + 1, s[i].getAddress());
    sleep_ms(50);
    gpio_put(xshut[i], 1);
    sleep_ms(10);
    VL53L0X tmp;
    tmp.init();
    tmp.setTimeout(500);
    tmp.setMeasurementTimingBudget(200000);
    tmp.setSignalRateLimit(0.1);
    tmp.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 18);
    tmp.setAddress(addr[i]);
    tmp.startContinuous(100);
    s[i] = tmp;
    sleep_ms(50);
}
// initialises all sensors, switches them all off.
Sensor::Sensor()
{
    i2c_init(I2C_PORT, 100 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // switching off all sensors, so we can initialise them one-by-one in init()
    for (int i = 0; i < 4; i++)
    {
        gpio_init(xshut[i]);
        gpio_set_dir(xshut[i], GPIO_OUT);
        gpio_put(xshut[i], 0);
        gpio_pull_up(xshut[i]);
    }
    sleep_ms(100); // to stabilise
    init();
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
        tmp.setMeasurementTimingBudget(200000);
        tmp.setSignalRateLimit(0.1);
        tmp.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 18);
        tmp.setAddress(addr[i]);
        sleep_ms(10);
        tmp.startContinuous(100);
        s[i] = tmp;
    }
    printf("Ready to go!");
}

// get readings
void Sensor::readings(double *arr)
{
    // uint8_t rxdata;
    // if (i2c_read_blocking(i2c_default, 0x29, &rxdata, 1, false) >= 0)
    //     fixsensor();

    int count = 0;
    for (int i = 0; i < 4; i++)
    {
        arr[i] = s[i].readRangeContinuousMillimeters() / 10.0;

        if (count > 10)
        {
            while (1)
            {
                printf("read error::\nsensor::reading - %f, %f, %f, %f\n", arr[0], arr[1], arr[2], arr[3]);
                i2c_scan();
                sleep_ms(2000);
            }
        }
        if (arr[i] == 6553.5)
        {
            // reboot(i);
            // count++;
            i--;
        }
    }
    printf("sensor::reading - %f, %f, %f, %f\n", arr[0], arr[1], arr[2], arr[3]);
}

// int sensor_example()
// {

//     stdio_init_all();
//     sleep_ms(3000);

//     // blink, doesnt use cpu
//     // PIO pio = pio0;
//     // uint offset = pio_add_program(pio, &blink_program);
//     // blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);

//     Sensor S;
//     i2c_scan();
//     sleep_ms(1000);
//     S.init();
//     printf("Program starts:\n\n");
//     i2c_scan();

//     double arr[4];
//     while (1)
//     {

//         S.readings(arr);
//         for (int i = 0; i < 4; i++)
//         {
//             printf("%s- %d\n", sensor_example[i], arr[i]);
//         }
//         sleep_ms(1000);
//         i2c_scan();
//     }
// }

///////////////////////////////// compass module

void QMC5883LCompass::_writeReg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    i2c_write_blocking(i2c_default, _ADDR, data, 2, false);
}

void QMC5883LCompass::_applyCalibration()
{
    for (int i = 0; i < 3; i++)
    {
        _vCalibrated[i] = (_vRaw[i] - _offset[i]) * _scale[i];
    }
}

QMC5883LCompass::QMC5883LCompass()
{
    _ADDR = QMC5883L_ADDR;
}

void QMC5883LCompass::init()
{
    _writeReg(SET_RESET_REG, 0x01);  // Set Reset Register
    setMode(0x01, 0x0C, 0x10, 0x00); // Default mode configuration
}

void QMC5883LCompass::setMode(uint8_t mode, uint8_t odr, uint8_t rng, uint8_t osr)
{
    _writeReg(CONTROL_REG, mode | odr | rng | osr);
}

void QMC5883LCompass::setMagneticDeclination(int degrees, uint8_t minutes)
{
    _magneticDeclinationDegrees = degrees + minutes / 60.0f;
}

void QMC5883LCompass::read()
{
    uint8_t data[6];
    uint8_t reg = DATA_REG;
    i2c_write_blocking(i2c_default, _ADDR, &reg, 1, true);
    i2c_read_blocking(i2c_default, _ADDR, data, 6, false);

    _vRaw[0] = (int16_t)(data[1] << 8 | data[0]);
    _vRaw[1] = (int16_t)(data[3] << 8 | data[2]);
    _vRaw[2] = (int16_t)(data[5] << 8 | data[4]);

    _applyCalibration();
}

int QMC5883LCompass::getX()
{
    return (int)_vCalibrated[0];
}

int QMC5883LCompass::getY()
{
    return (int)_vCalibrated[1];
}

int QMC5883LCompass::getZ()
{
    return (int)_vCalibrated[2];
}

int QMC5883LCompass::getAzimuth()
{
    float heading = atan2(_vCalibrated[1], _vCalibrated[0]) * 180.0 / M_PI;
    heading += _magneticDeclinationDegrees;
    if (heading < 0)
        heading += 360;
    if (heading >= 360)
        heading -= 360;
    return (int)heading;
}
