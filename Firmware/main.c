#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

#include "display.h"
#include "rtc.h"
#include "music.h"

const int song[] = {
  NOTE_E5, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_C5,8,  NOTE_B4,8,
  NOTE_A4, 4,  NOTE_A4,8,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, -4,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,8,  REST, 4,

  NOTE_D5, -4,  NOTE_F5,8,  NOTE_A5,4,  NOTE_G5,8,  NOTE_F5,8,
  NOTE_E5, -4,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,4, REST, 4,

  NOTE_E5, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_C5,8,  NOTE_B4,8,
  NOTE_A4, 4,  NOTE_A4,8,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, -4,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,8, REST, 4,

  NOTE_D5, -4,  NOTE_F5,8,  NOTE_A5,4,  NOTE_G5,8,  NOTE_F5,8,
  NOTE_E5, -4,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,4, REST, 4,
  

  NOTE_E5,2,  NOTE_C5,2,
  NOTE_D5,2,   NOTE_B4,2,
  NOTE_C5,2,   NOTE_A4,2,
  NOTE_GS4,2,  NOTE_B4,4,  REST,8, 
  NOTE_E5,2,   NOTE_C5,2,
  NOTE_D5,2,   NOTE_B4,2,
  NOTE_C5,4,   NOTE_E5,4,  NOTE_A5,2,
  NOTE_GS5,2,
  
};

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

int calculate_frequency(uint frequency) {
    // 1Hz is once per second
    // Delay is the time in microseconds to sleep
    int delay = (double)100000 / frequency;
    return delay >> 1;
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
    char *clock_string_buffer = malloc(9);

    gpio_init(28);
    gpio_set_dir(28, GPIO_OUT);

    // // Beats per minute
    // int tempo = 120;
    // // Duration of a beat in microseconds
    // // Times by 4 so that 1 quater becomes 1 beat ima forget what i mean by this but oh well
    // long whole_note = ((60 * 1000 * 1000) * 4) / tempo;
    
    // for (int i = 0; i < count_of(song); i+=2)
    // {
    //     int note = song[i];
    //     int duration = song[i + 1];

    //     if (note == 0) {
    //         sleep_us(whole_note / duration);
    //         printf("Resting for %d\n", duration);
    //         continue;
    //     }
        
    //     printf("Playing note %d\n", note);

    //     int frequency_pause_duration = calculate_frequency(note);
        
    //     int beat_loop_count = ((whole_note / duration) / (frequency_pause_duration << 1));
    //     beat_loop_count *= beat_loop_count < 0 ? -1.5 : 1;


    //     for (double j = 0; j < beat_loop_count * 0.9; j++)
    //     {
    //         gpio_put(28, true);
    //         sleep_us(frequency_pause_duration);
    //         gpio_put(28, false);
    //         sleep_us(frequency_pause_duration);
    //     }

    //     double rest_time = frequency_pause_duration;
    //     rest_time *= 2;
    //     sleep_us(rest_time * (beat_loop_count * 0.1));
    // }

    bool value = true;

    while(true)  
    {        
        // Clock has been reset (MUST BE SET)
		if(clock_buffer[6]==0x00 && clock_buffer[5]==0x01 && clock_buffer[4]==0x01)
		{
			clock_set_time(); 
		}

        for (int n = 0; n < 8; n++)
        {
            for (int x = 0; x < 5; x++)
            {
                for (int y = 0; y < 7; y++)
                {
               
                    set_pixel(n, x, y, value, true);
                }
            }
        }

        value = !value;
    
        // update_clock();
        // sleep_ms(1000);        
	}  

    return 0;
}
