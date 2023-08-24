#include <stdlib.h>
#include <string.h>

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

/**
 * @brief The addresses of the displays in order from left to right.
    They are doubled up since there are 2 displays per chip.
 */
uint8_t addresses[8] = { 0x63, 0x63, 0x60, 0x60, 0x62, 0x62, 0x61, 0x61 };

/**
 * @brief 
 * 
 */
uint8_t displays[8][8] = { 
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },
    { 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 }
};


/**
 * @brief Contains 8x8 bit patterns which correlate to various symbols
    which may need to be displayed on the LTP-305's.

    ASCII Based

    @example
    The symbol "+"
    #Col   1 2 3 4 5
    0b00000000, # Row 1
    0b00000000, # Row 2
    0b00000000, # Row 3
    0b00000000, # Row 4
    0b00000000, # Row 5
    0b00000000, # Row 6
    0b10000000, # Row 7, bit 8 =  decimal place
    0b00000000

    The array { 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00, 0x00, 0x00 }
    0b00001000,
    0b00001000,
    0b00111110,
    0b00001000,
    0b00001000,
    0b00000000,
    0b00000000,
    0b00000000
 */
uint8_t characters[][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } ,  // (space)
    { 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x00 } ,  // !
    { 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 } ,  // "
    { 0x14, 0x7f, 0x14, 0x7f, 0x14, 0x00, 0x00, 0x00 } ,  // #
    { 0x24, 0x2a, 0x7f, 0x2a, 0x12, 0x00, 0x00, 0x00 } ,  // $
    { 0x23, 0x13, 0x08, 0x64, 0x62, 0x00, 0x00, 0x00 } ,  // %
    { 0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x00, 0x00 } ,  // &
    { 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 } ,  // '
    { 0x00, 0x1c, 0x22, 0x41, 0x00, 0x00, 0x00, 0x00 } ,  // (
    { 0x00, 0x41, 0x22, 0x1c, 0x00, 0x00, 0x00, 0x00 } ,  // )
    { 0x08, 0x2a, 0x1c, 0x2a, 0x08, 0x00, 0x00, 0x00 } ,  // *
    { 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00, 0x00, 0x00 } ,  // +
    { 0x00, 0x50, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00 } ,  // ,
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00 } ,  // -
    { 0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00 } ,  // .
    { 0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 0x00, 0x00 } ,  // /
    { 0x3e, 0x51, 0x49, 0x45, 0x3e, 0x00, 0x00, 0x00 } ,  // 0
    { 0x00, 0x42, 0x7f, 0x40, 0x00, 0x00, 0x00, 0x00 } ,  // 1
    { 0x42, 0x61, 0x51, 0x49, 0x46, 0x00, 0x00, 0x00 } ,  // 2
    { 0x21, 0x41, 0x45, 0x4b, 0x31, 0x00, 0x00, 0x00 } ,  // 3
    { 0x18, 0x14, 0x12, 0x7f, 0x10, 0x00, 0x00, 0x00 } ,  // 4
    { 0x27, 0x45, 0x45, 0x45, 0x39, 0x00, 0x00, 0x00 } ,  // 5
    { 0x3c, 0x4a, 0x49, 0x49, 0x30, 0x00, 0x00, 0x00 } ,  // 6
    { 0x01, 0x71, 0x09, 0x05, 0x03, 0x00, 0x00, 0x00 } ,  // 7
    { 0x36, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00, 0x00 } ,  // 8
    { 0x06, 0x49, 0x49, 0x29, 0x1e, 0x00, 0x00, 0x00 } ,  // 9
    { 0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00 } ,  // :
    { 0x00, 0x56, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00 } ,  // ;
    { 0x00, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00 } ,  // <
    { 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00 } ,  // =
    { 0x41, 0x22, 0x14, 0x08, 0x00, 0x00, 0x00, 0x00 } ,  // >
    { 0x02, 0x01, 0x51, 0x09, 0x06, 0x00, 0x00, 0x00 } ,  // ?
    { 0x32, 0x49, 0x79, 0x41, 0x3e, 0x00, 0x00, 0x00 } ,  // @
    { 0x7e, 0x11, 0x11, 0x11, 0x7e, 0x00, 0x00, 0x00 } ,  // A
    { 0x7f, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00, 0x00 } ,  // B
    { 0x3e, 0x41, 0x41, 0x41, 0x22, 0x00, 0x00, 0x00 } ,  // C
    { 0x7f, 0x41, 0x41, 0x22, 0x1c, 0x00, 0x00, 0x00 } ,  // D
    { 0x7f, 0x49, 0x49, 0x49, 0x41, 0x00, 0x00, 0x00 } ,  // E
    { 0x7f, 0x09, 0x09, 0x01, 0x01, 0x00, 0x00, 0x00 } ,  // F
    { 0x3e, 0x41, 0x41, 0x51, 0x32, 0x00, 0x00, 0x00 } ,  // G
    { 0x7f, 0x08, 0x08, 0x08, 0x7f, 0x00, 0x00, 0x00 } ,  // H
    { 0x00, 0x41, 0x7f, 0x41, 0x00, 0x00, 0x00, 0x00 } ,  // I
    { 0x20, 0x40, 0x41, 0x3f, 0x01, 0x00, 0x00, 0x00 } ,  // J
    { 0x7f, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00 } ,  // K
    { 0x7f, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00 } ,  // L
    { 0x7f, 0x02, 0x04, 0x02, 0x7f, 0x00, 0x00, 0x00 } ,  // M
    { 0x7f, 0x04, 0x08, 0x10, 0x7f, 0x00, 0x00, 0x00 } ,  // N
    { 0x3e, 0x41, 0x41, 0x41, 0x3e, 0x00, 0x00, 0x00 } ,  // O
    { 0x7f, 0x09, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00 } ,  // P
    { 0x3e, 0x41, 0x51, 0x21, 0x5e, 0x00, 0x00, 0x00 } ,  // Q
    { 0x7f, 0x09, 0x19, 0x29, 0x46, 0x00, 0x00, 0x00 } ,  // R
    { 0x46, 0x49, 0x49, 0x49, 0x31, 0x00, 0x00, 0x00 } ,  // S
    { 0x01, 0x01, 0x7f, 0x01, 0x01, 0x00, 0x00, 0x00 } ,  // T
    { 0x3f, 0x40, 0x40, 0x40, 0x3f, 0x00, 0x00, 0x00 } ,  // U
    { 0x1f, 0x20, 0x40, 0x20, 0x1f, 0x00, 0x00, 0x00 } ,  // V
    { 0x7f, 0x20, 0x18, 0x20, 0x7f, 0x00, 0x00, 0x00 } ,  // W
    { 0x63, 0x14, 0x08, 0x14, 0x63, 0x00, 0x00, 0x00 } ,  // X
    { 0x03, 0x04, 0x78, 0x04, 0x03, 0x00, 0x00, 0x00 } ,  // Y
    { 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x00, 0x00 } ,  // Z
    { 0x00, 0x00, 0x7f, 0x41, 0x41, 0x00, 0x00, 0x00 } ,  // [
    { 0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x00, 0x00 } ,  // 
    { 0x41, 0x41, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00 } ,  // ]
    { 0x04, 0x02, 0x01, 0x02, 0x04, 0x00, 0x00, 0x00 } ,  // ^
    { 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00 } ,  // _
    { 0x00, 0x01, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00 } ,  // `
    { 0x20, 0x54, 0x54, 0x54, 0x78, 0x00, 0x00, 0x00 } ,  // a
    { 0x7f, 0x48, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00 } ,  // b
    { 0x38, 0x44, 0x44, 0x44, 0x20, 0x00, 0x00, 0x00 } ,  // c
    { 0x38, 0x44, 0x44, 0x48, 0x7f, 0x00, 0x00, 0x00 } ,  // d
    { 0x38, 0x54, 0x54, 0x54, 0x18, 0x00, 0x00, 0x00 } ,  // e
    { 0x08, 0x7e, 0x09, 0x01, 0x02, 0x00, 0x00, 0x00 } ,  // f
    { 0x08, 0x14, 0x54, 0x54, 0x3c, 0x00, 0x00, 0x00 } ,  // g
    { 0x7f, 0x08, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00 } ,  // h
    { 0x00, 0x44, 0x7d, 0x40, 0x00, 0x00, 0x00, 0x00 } ,  // i
    { 0x20, 0x40, 0x44, 0x3d, 0x00, 0x00, 0x00, 0x00 } ,  // j
    { 0x00, 0x7f, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00 } ,  // k
    { 0x00, 0x41, 0x7f, 0x40, 0x00, 0x00, 0x00, 0x00 } ,  // l
    { 0x7c, 0x04, 0x18, 0x04, 0x78, 0x00, 0x00, 0x00 } ,  // m
    { 0x7c, 0x08, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00 } ,  // n
    { 0x38, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00 } ,  // o
    { 0x7c, 0x14, 0x14, 0x14, 0x08, 0x00, 0x00, 0x00 } ,  // p
    { 0x08, 0x14, 0x14, 0x18, 0x7c, 0x00, 0x00, 0x00 } ,  // q
    { 0x7c, 0x08, 0x04, 0x04, 0x08, 0x00, 0x00, 0x00 } ,  // r
    { 0x48, 0x54, 0x54, 0x54, 0x20, 0x00, 0x00, 0x00 } ,  // s
    { 0x04, 0x3f, 0x44, 0x40, 0x20, 0x00, 0x00, 0x00 } ,  // t
    { 0x3c, 0x40, 0x40, 0x20, 0x7c, 0x00, 0x00, 0x00 } ,  // u
    { 0x1c, 0x20, 0x40, 0x20, 0x1c, 0x00, 0x00, 0x00 } ,  // v
    { 0x3c, 0x40, 0x30, 0x40, 0x3c, 0x00, 0x00, 0x00 } ,  // w
    { 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00 } ,  // x
    { 0x0c, 0x50, 0x50, 0x50, 0x3c, 0x00, 0x00, 0x00 } ,  // y
    { 0x44, 0x64, 0x54, 0x4c, 0x44, 0x00, 0x00, 0x00 } ,  // z
    { 0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x00 } ,  // {
    { 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00 } ,  // |
    { 0x00, 0x41, 0x36, 0x08, 0x00, 0x00, 0x00, 0x00 } ,  // }
    { 0x08, 0x08, 0x2a, 0x1c, 0x08, 0x00, 0x00, 0x00 } ,  // ~
};


/**
 * @brief In order for characters to be displayed on the second display they
    will need to be translated into the correct bit pattern. This is since
    the first matrix is represented by the 8 byte buffer in the form

    #Col   1 2 3 4 5
    0b00000000, # Row 1
    0b00000000, # Row 2
    0b00000000, # Row 3
    0b00000000, # Row 4
    0b00000000, # Row 5
    0b00000000, # Row 6
    0b10000000, # Row 7, bit 8 =  decimal place
    0b00000000

    This is compared to the second display which is represented as the first
    but reflected in the line y=x.

    #Row 8 7 6 5 4 3 2 1
    0b01111111, # Col 1, bottom to top
    0b01111111, # Col 2
    0b01111111, # Col 3
    0b01111111, # Col 4
    0b01111111, # Col 5
    0b00000000,
    0b00000000,
    0b01000000  # bit 7, decimal place

    To save on processing time these are precalculated at the start of the 
    program.
 */
uint8_t alternate_characters[count_of(characters)][8];

/**
 * @brief A helper function to add an amount num_chars to of a specific character
    character_to_append to the end of a string. 

    This is used to add whitespace to the end of a scrolling string.
 * 
 * @param original The original string to have letters appended to.
 * @param character_to_append The character to append to the string
 * @param num_chars The number of character_to_append to be appended
 * 
 * @return char* The new string
 */
char *append_chars(const char *original, char character_to_append, size_t num_chars) {
    size_t original_length = strlen(original);
    char *new_string = (char *)malloc(original_length + num_chars + 1); // Don't forget +1 for null terminator

    if (new_string == NULL) {
        return NULL; // Memory allocation failed
    }

    // Copy the original string into the new longer one
    strcpy(new_string, original);
    
    // Append desired amount of characters
    for (size_t i = 0; i < num_chars; i++) {
        new_string[original_length + i] = character_to_append;
    }
    
    new_string[original_length + num_chars] = '\0'; // Null-terminate the string

    return new_string;
}

/**
 * @brief This is another helper function that converts an array of
    uint8_t's from a left matrix, the one defined with columns
    v rows into a right matrix in the form rows v columns.

    Essentially this is a reflection in the line y=x of a 2D matrix.
 * 
 * @param left_matrix The original matrix in the form columns vs rows
 * @param right_matrix The generated matrix in the form rows vs columns
 */
void convertLeftToRight(const uint8_t *left_matrix, uint8_t *right_matrix) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            uint8_t bit_value = (left_matrix[row] >> (col)) & 1;
            right_matrix[col] |= (bit_value << row);
        }
    }
}


/**
 * @brief This MUST be called at the start of the program otherwise the 
    letters for the right matrix will not be generated and will
    not display properly if at all.
 */
void pre_generate_alternate_characters() {
    for (int i = 0; i < count_of(characters); i++)
    {
        convertLeftToRight(characters[i], alternate_characters[i]);
    }   
}


/**
 * @brief Set the option function is used to change a setting on the
    IS31FL3730-QFLS2-TR chips such as brightness or a configuration setting.
 * 
 * @param display The 0 indexed display to update [0,8)
 * @param address The address of the register to update on the IS31FL3730
 * @param value The value to send over i2c to the register
 */
void set_option(int display, uint8_t address, uint8_t value) {
    if (display < 0 || display > 7) return;

    uint8_t buf[2];

    buf[0] = address;
    buf[1] = value;

    i2c_write_timeout_us(I2C_DISPLAY_LINE, addresses[display], buf, count_of(buf), false, 1000);
}

/**
 * @brief This is used when updating the displays, in order to update specific rows
    a certain address must be used before the buffer. This function prepends this 
    address.
 * 
 * @param address The address of the register to be prepended to the start of the buffer 
 * @param buffer The pointer to the current buffer to have an address prepended
 * @param buffer_size The size of the buffer to be prepended
 * 
 * @return uint8_t* The pointer in memory of the new altered buffer
 */
uint8_t* prepend_address(uint8_t address, uint8_t *buffer, int buffer_size) {
    int new_buffer_size = buffer_size + 1;
    
    uint8_t* new_buffer = (uint8_t*)malloc(new_buffer_size * sizeof(uint8_t));

    if (new_buffer == NULL) {
        return NULL; 
    }
    
    new_buffer[0] = address;
    
    for (int i = 0; i < buffer_size; i++) {
        new_buffer[i + 1] = buffer[i];
    }
    
    return new_buffer;
}

/**
 * @brief Used to update all displays all at once. Once called it will push
    what is currently stored in displays to each of the 4 IS31FL3730's
 */
void update_display() {
    for (int i = 0; i < count_of(displays); i++)
    {
        uint8_t mat_addr = i % 2 == 0 ? MATRIX_A_ADDR : MATRIX_B_ADDR;

        uint8_t* pre_mat_buf = prepend_address(mat_addr, displays[i], count_of(displays[i]));
        i2c_write_timeout_us(I2C_DISPLAY_LINE, addresses[i], pre_mat_buf, count_of(displays[i]) + 1, false, 1000);
        free(pre_mat_buf);


        if (i % 2 != 0) {
            set_option(i, MODE_ADDR, DEFAULT_MODE);
            set_option(i, OPTION_ADDR, DEFAULT_OPTIONS);
            set_option(i, BRIGHTNESS_ADDR, 0b00011111);
            set_option(i, UPDATE_ADDR, 0b00000001);
        }
    }
}

/**
 * @brief Set a specific pixel on a display to on or off.
 * 
 * @param display The 0 indexed display to update [0,8)
 * @param x The 0 indexed x value of the pixel to be updated [0, 5)
 * @param y The 0 indexed y value of the pixel to be updated [0, 7)
 * @param status Whether the pixel on the LTP305 should be set to on or off
 * @param update Update the display after changing the pixel
 */
void set_pixel(int display, uint8_t x, uint8_t y, bool status, bool update) {
    if (display < 0 || display > 7) return;

    if (display % 2 == 0) {
        uint8_t tempX = x;
        x = y;
        y = tempX; 
    }
    
    uint8_t changeVal = 1;
    changeVal <<= x;
    
    if (status) {    
        displays[display][y] |= changeVal;
    } else {
        uint8_t value = displays[display][y];
        value = ~value;
        value |= changeVal;
        value = ~value;
        displays[display][y] = value;
    }

    if (update) update_display();
}

/**
 * @brief Clear the buffer and reset all values to 0
 * 
 * @param update Update the screen after clearing all buffers
 */
void clear_all(bool update) {
    for (int i = 0; i < count_of(displays); i++)
    {
        for (int j = 0; j < 8; j++)
        {
            displays[i][j] = 0;
        }
    
    }

    if (update) update_display();
}

/**
 * @brief Clear a specific display by resetting all values to 0
 * 
 * @param display The 0 indexed display to update [0,8)
 * @param update Update the screen after clearing the display buffer
 */
void clear(int display, bool update) {
    if (display < 0 || display > 7) return;
    for (int i = 0; i < 8; i++)
    {
        displays[display][i] = 0;
    }
    

    if (update) update_display();
}

/**
 * @brief Set a specific display to a certain ASCII character;
 * 
 * @param display The 0 indexed display to update [0,8)
 * @param letter The ASCII character to be displayed
 * @param update Update the display after setting the buffer to a character
 */
void set_char(int display, char letter, bool update) {
    if (display < 0 || display > 7) return;

    if (display % 2 == 0) {
        for (int i = 0; i < 8; i++)
        {
            displays[display][i] = characters[((int)letter) - 32][i];
        }   
    } else {
        for (int i = 0; i < 8; i++)
        {
            displays[display][i] = alternate_characters[((int)letter) - 32][i];
        }   
    }

    if (update) update_display();
}

/**
 * @brief Marque a string across the displays with trailing whitespace
 * 
 * @param string The string to be displayed
 */
void scroll_display_string(char *string) {
    string = append_chars(string, ' ', 8);
    int string_length = strlen(string);

    clear_all(false);

    if (string_length > 8) {
        for (int i = 0; i <= string_length - 8; i++) {
            for (int display = 0; display < 8; display++) {
                char char_to_display = string[i + display];
                set_char(display, char_to_display, false);
            }

            update_display();

            if (i == 0) sleep_ms(750);
            sleep_ms(250); 
        }
        
        sleep_ms(2000);
    } else {
        for (int display = 0; display < string_length; display++) {
            set_char(display, string[display], false);
        }
        update_display();
        sleep_ms(2000); 
    }

    free(string);
    clear_all(true);
}

/**
 * @brief Set the screen to display the first 6 letters of an ASCII string.
 * 
 * @param string The string whose first 6 chars should be displayed.
 */
void display_string(char *string) {
    clear_all(false);

    for (int i = 0; i < strlen(string); i++)
    {
        set_char(i, string[i], false);
    }

    update_display();
}