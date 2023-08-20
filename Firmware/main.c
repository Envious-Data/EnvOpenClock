#include <stdio.h>
#include "display.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "rtc.h"

extern char clock_buffer[];
extern char *week[];

int main() {
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    i2c_init(I2C_DISPLAY_LINE, 100 * 1000);
    gpio_set_function(I2C_DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_set_function(I2C_DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_DISPLAY_SCL);
    gpio_pull_up(I2C_DISPLAY_SDA);
    
    bi_decl(bi_2pins_with_func(I2C_DISPLAY_SDA, I2C_DISPLAY_SCL, GPIO_FUNC_I2C));
    
    // Must be called otherwise will cause display not to function normally.
    pre_generate_alternate_characters();

    char *clock_string_buffer = malloc(9);


    bool val = false;

    while(true)  
    {  
        set_char(0, 'A', true);
        gpio_put(25, val);
        val = !val;
        set_char(0, 'B', true);
       	clock_read_time();
        set_char(0, 'C', true);

        // Clock has been reset (MUST BE SET)
		if(clock_buffer[6]==0x00 && clock_buffer[5]==0x01 && clock_buffer[4]==0x01)
		{
			clock_set_time(); 
		}
        set_char(0, 'D', true);


		clock_buffer[0] = clock_buffer[0] & 0x7F; //sec
		clock_buffer[1] = clock_buffer[1] & 0x7F; //min
		clock_buffer[2] = clock_buffer[2] & 0x3F; //hour
		clock_buffer[3] = clock_buffer[3] & 0x07; //week
		clock_buffer[4] = clock_buffer[4] & 0x3F; //day
		clock_buffer[5] = clock_buffer[5] & 0x1F; //mouth

        


		//year/month/day
		// sprintf(clock_string_buffer, "20%02x/%02x/%02x  ",clock_buffer[6],clock_buffer[5],clock_buffer[4]);
		//hour:minute/second
        set_char(0, 'E', true);
		sprintf(clock_string_buffer, "%02x:%02x:%02x  ",clock_buffer[2],clock_buffer[1],clock_buffer[0]);
        set_char(0, 'F', true);

		//weekday
		// sprintf(clock_string_buffer, "%s\n",week[(unsigned char)clock_buffer[3]-1]);

        display_string(clock_string_buffer);
        sleep_ms(100);
	}  

    set_char(0, 'Z', true);
}