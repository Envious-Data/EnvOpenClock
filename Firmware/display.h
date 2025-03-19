#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>  // Add this line
#include <stdio.h>
#include <stdlib.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define I2C_DISPLAY_LINE i2c1
#define I2C_DISPLAY_SDA 26
#define I2C_DISPLAY_SCL 27

#define MODE_ADDR 0b00000000
#define BRIGHTNESS_ADDR 0b00011001
#define UPDATE_ADDR 0b00001100
#define OPTION_ADDR 0b00001101
#define MATRIX_A_ADDR 0b00001110
#define MATRIX_B_ADDR 0b00000001

#define DEFAULT_MODE 0b00011000
#define DEFAULT_OPTIONS 0b00001110

#ifndef count_of
#define count_of(array) (sizeof(array) / sizeof(array[0]))
#endif

extern uint8_t brightness_level;
extern uint8_t addresses[8];
extern uint8_t displays[8][8];

void animate_slide_up(int display, char current_char, char next_char);
void animate_pixelated_transition(int display, char current_char, char next_char);
void animate_expanding_contracting(int display, char current_char, char next_char);
void animate_morph(int display, char current_char, char next_char);

// new animations
void animate_slide_up_with_trail(int display, char current_char, char next_char);
void animate_slide_up_with_scanlines(int display, char current_char, char next_char);
void animate_interlocking_pieces(int display, char current_char, char next_char);
void animate_glitch_effect(int display, char current_char, char next_char);
void animate_heartbeat_pulse(int display, char current_char, char next_char);
void animate_matrix_rain(int display, char current_char, char next_char);
void animate_waterfall(int display, char current_char, char next_char);
void animate_blinds(int display, char current_char, char next_char);
void animate_swirl(int display, char current_char, char next_char);
void animate_bouncing_ball(int display, char current_char, char next_char);
void animate_clock_hand_wipe(int display, char current_char, char next_char);

void convertLeftToRight(const uint8_t *left_matrix, uint8_t *right_matrix);
void pre_generate_alternate_characters();
void set_option(int display, uint8_t address, uint8_t value);
uint8_t *prepend_address(uint8_t address, uint8_t buffer[], int buffer_size);
void update_display();
void set_pixel(int display, uint8_t x, uint8_t y, bool status, bool update);
void set_brightness(uint8_t value);  // Or int value
void clear_all(bool update);
void clear(int display, bool update);
void set_char(int display, char letter, bool update);
void get_char_data(char character, uint8_t *data);
void scroll_display_string(char *string);
void display_string(const char *string);

#endif  // DISPLAY_H