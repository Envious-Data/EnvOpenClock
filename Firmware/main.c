#include <stdio.h>
#include "display.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "rtc.h"

extern char clock_buffer[];
extern char *week[];
bool update = true;

bool update_clock() {
    if (update) {
        char *clock_string_buffer = malloc(9);
        if (clock_string_buffer == NULL) {
            return false;
        }
        
        clock_read_time();

        clock_buffer[0] &= 0x7F; //sec
        clock_buffer[1] &= 0x7F; //min
        clock_buffer[2] &= 0x3F; //hour

        sprintf(clock_string_buffer, "%02x:%02x:%02x", clock_buffer[2], clock_buffer[1], clock_buffer[0]);
        display_string(clock_string_buffer);    
        free(clock_string_buffer);

        return true;
    }
}

int main() {
    stdio_init_all();

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    if (i2c_init(I2C_DISPLAY_LINE, 100 * 1000) < 0) {
        fprintf(stderr, "Error initializing I2C.\n");
        return 1;
    }

    gpio_set_function(I2C_DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_set_function(I2C_DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_DISPLAY_SCL);
    gpio_pull_up(I2C_DISPLAY_SDA);
    
    bi_decl(bi_2pins_with_func(I2C_DISPLAY_SDA, I2C_DISPLAY_SCL, GPIO_FUNC_I2C));
    
    // Must be called otherwise it will cause the display not to function normally.
    pre_generate_alternate_characters();
    
    clock_read_time();

    // Clock has been reset (MUST BE SET)
    if (clock_buffer[6] == 0x00 && clock_buffer[5] == 0x01 && clock_buffer[4] == 0x01) {
        clock_set_time(); 
    }

    // struct repeating_timer timer;
    // if (add_repeating_timer_ms(-1000, update_clock, NULL, &timer) < 0) {
    //     fprintf(stderr, "Error adding repeating timer.\n");
    //     return 1;
    // }

    while (true) {
        char *clock_string_buffer = malloc(9);
        if (clock_string_buffer == NULL) {
            return false;
        }
        
        clock_read_time();

        clock_buffer[0] &= 0x7F; //sec
        clock_buffer[1] &= 0x7F; //min
        clock_buffer[2] &= 0x3F; //hour

        sprintf(clock_string_buffer, "%02x:%02x:%02x", clock_buffer[2], clock_buffer[1], clock_buffer[0]);
        display_string(clock_string_buffer);    
        free(clock_string_buffer);

        return true;
    }  

    return 0;
}
