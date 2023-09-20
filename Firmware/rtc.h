

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define CLOCK_ADDR 0x68

#define CLOCK_I2C_LINE	i2c1
#define CLOCK_I2C_SCL		27
#define CLOCK_I2C_SDA		26

unsigned char int_to_bcd(int n);
void clock_set_time(unsigned char second, unsigned char minute, unsigned char hour, unsigned char day, unsigned char week, unsigned char year);
void clock_read_time();
