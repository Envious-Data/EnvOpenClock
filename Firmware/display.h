#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_DISPLAY_LINE    i2c1 
#define I2C_DISPLAY_SDA     26
#define I2C_DISPLAY_SCL     27

#define MODE_ADDR       0b00000000
#define BRIGHTNESS_ADDR 0b00011001
#define UPDATE_ADDR     0b00001100
#define OPTION_ADDR     0b00001101
#define MATRIX_A_ADDR   0b00001110
#define MATRIX_B_ADDR   0b00000001

#define DEFAULT_MODE    0b00011000
#define DEFAULT_OPTIONS 0b00001110

extern uint8_t brightness_level;
extern uint8_t addresses[8];
extern uint8_t displays[8][8];

// Add a function prototype for the vertical wipe animation
void animate_slide_up(int display, char current_char, char next_char);

void convertLeftToRight(const uint8_t *left_matrix, uint8_t *right_matrix);
void pre_generate_alternate_characters();
void set_option(int display, uint8_t address, uint8_t value);
uint8_t* prepend_address(uint8_t address, uint8_t buffer[], int buffer_size);
void update_display();
void set_pixel(int display, uint8_t x, uint8_t y, bool status, bool update);
void set_brightness(double value);
void clear_all(bool update);
void clear(int display, bool update);
void set_char(int display, char letter, bool update);
void scroll_display_string(char *string);
void display_string(char *string);

