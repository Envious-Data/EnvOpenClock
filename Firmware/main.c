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

    if (i2c_init(I2C_DISPLAY_LINE, 400 * 1000) < 0) {
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
    char *clock_string_buffer = malloc(9);

    bool val = true;

    while(true)  
    {
        // // clock_read_time();
        
        // // // Clock has been reset (MUST BE SET)
		// // if(clock_buffer[6]==0x00 && clock_buffer[5]==0x01 && clock_buffer[4]==0x01)
		// // {
		// // 	clock_set_time(); 
		// // }

        // // clock_buffer[0] &= 0x7F; //sec
        // // clock_buffer[1] &= 0x7F; //min
        // // clock_buffer[2] &= 0x3F; //hour

		// // //year/month/day
		// // // sprintf(clock_string_buffer, "20%02x/%02x/%02x  ",clock_buffer[6],clock_buffer[5],clock_buffer[4]);
		// // //hour:minute/second
        // // // set_char(0, 'E', true);
		// // sprintf(clock_string_buffer, "%02x:%02x:%02x",clock_buffer[2],clock_buffer[1],clock_buffer[0]);
		// // //weekday
		// // // sprintf(clock_string_buffer, "%s\n",week[(unsigned char)clock_buffer[3]-1]);

		// // // sprintf(clock_string_buffer, "%02x:%02x:%02x",clock_buffer[0],clock_buffer[0],clock_buffer[0]);
        // // display_string(clock_string_buffer);

        // for (int i = 4; i < 5; i++)
        // {
        //     for (int x = 0; x < 5; x++)
        //     {
        //         for (int y = 0; y < 7; y++)
        //         {
        //             set_pixel(i, x, y, val, true);
        //         }                
        //     }
        // }
        // val = !val;

        update_clock();
        sleep_ms(1000);        
	}  

    return 0;
}
