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
    //s[i].writeReg(0x00, 0x00);
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
            //reboot(i);
            //count++;
            i--;
        }
    }
    printf("sensor::reading - %f, %f, %f, %f\n", arr[0], arr[1], arr[2], arr[3]);
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

    double arr[4];
    while (1)
    {

        S.readings(arr);
        for (int i = 0; i < 4; i++)
        {
            printf("%s- %d\n", sensor_example[i], arr[i]);
        }
        sleep_ms(1000);
        i2c_scan();
    }
}


void write_register(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, HMC5883L_ADDR, buffer, 2, false);
}

// Function to read multiple bytes from a register
void read_register(uint8_t reg, uint8_t *buffer, size_t length) {
    i2c_write_blocking(I2C_PORT, HMC5883L_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, HMC5883L_ADDR, buffer, length, false);
}

int main() {
    stdio_init_all();

    // Initialize I2C
    i2c_init(I2C_PORT, 100 * 1000); // 100 kHz
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Configure HMC5883L
    write_register(CONFIG_REG_A, 0x70); // 15Hz, normal measurement mode
    write_register(CONFIG_REG_B, 0x20); // Gain configuration
    write_register(MODE_REG, 0x00);     // Continuous measurement mode

    while (true) {
        uint8_t data[6];
        read_register(DATA_REG, data, 6);

        int16_t x = (data[0] << 8) | data[1];
        int16_t z = (data[2] << 8) | data[3];
        int16_t y = (data[4] << 8) | data[5];

        // Convert raw values to signed integers
        if (x > 32767) x -= 65536;
        if (y > 32767) y -= 65536;
        if (z > 32767) z -= 65536;

        // Convert to microteslas
        float xf = x * LSB_TO_UT;
        float yf = y * LSB_TO_UT;
        float zf = z * LSB_TO_UT;

        // Calculate heading in degrees
        float heading = atan2f(yf, xf) * (180.0 / M_PI);
        if (heading < 0) heading += 360;

        // Print results
        printf("Magnetic field in X: %.2f uT, Y: %.2f uT, Z: %.2f uT, Heading: %.2f°\n", xf, yf, zf, heading);

        sleep_ms(100);
    }

    return 0;
}
