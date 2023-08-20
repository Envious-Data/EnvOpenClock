

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define CLOCK_ADDR 0x68

#define CLOCK_I2C_LINE	i2c1
#define CLOCK_I2C_SCL		27
#define CLOCK_I2C_SDA		26

void clock_set_time();
void clock_read_time();
