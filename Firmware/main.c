#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/structs/timer.h"

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

struct {
    uint8_t set_time            : 1;
    uint8_t increment_counter   : 1;
    uint8_t decrement_counter   : 1;
    uint8_t set_confirm         : 1;
} gpio_irq_flags;

extern char clock_buffer[];
extern char *week[];

bool clock_busy = false;
bool clock_set = true;

void timer_callback(struct repeating_timer *t) {
    // if after 10pm and before 6am
    if (clock_buffer[2] >= 0x22 || clock_buffer[2] <= 0x06) {
        brightness_level = 0b0000001;
    } else {
        brightness_level =  0b1101000 ;
    }   
}


bool update_clock() {
    if (clock_set) {
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



bool update_date() {    
    char *date_string_buffer = malloc(100);
    if (date_string_buffer == NULL) {
        return false;
    }
    
    clock_read_time();

    clock_buffer[4] &= 0x7F; //day
    clock_buffer[5] &= 0x7F; //month
    clock_buffer[6] &= 0x3F; //year

    sprintf(date_string_buffer, "%02x/%02x/%02x", clock_buffer[4], clock_buffer[5], clock_buffer[6]);
    display_string(date_string_buffer);    
    free(date_string_buffer);

    return true;    
}

int calculate_frequency(uint frequency) {
    // 1Hz is once per second
    // Delay is the time in microseconds to sleep
    int delay = (double)100000 / frequency;
    return delay >> 1;
}

void set_date_and_time() {
    clear_all(false);
    clock_set = true;

    // Year
    int year = 23;
    while (!gpio_irq_flags.set_confirm) {
        if (!gpio_irq_flags.decrement_counter || !gpio_irq_flags.increment_counter) {
            if (gpio_irq_flags.decrement_counter)  {
                year = year > 0 ? year - 1 : 99;
                gpio_irq_flags.decrement_counter = 0;
            }
            if (gpio_irq_flags.increment_counter) {
                year = year < 99 ? year + 1: 0;
                gpio_irq_flags.increment_counter = 0;      
            }

            display_string("Year:");
            set_char(6, '0' + (year / 10), false);
            set_char(7, '0' + (year % 10), true);
        }
    }

    gpio_irq_flags.set_confirm = 0;

    // month
    int month = 1;
    while (!gpio_irq_flags.set_confirm) {
        if (!gpio_irq_flags.decrement_counter || !gpio_irq_flags.increment_counter) {
            if (gpio_irq_flags.decrement_counter)  {
                month = month > 1 ? month - 1 : 12;
                gpio_irq_flags.decrement_counter = 0;
            }
            if (gpio_irq_flags.increment_counter) {
                month = month < 12 ? month + 1: 1;
                gpio_irq_flags.increment_counter = 0;      
            }

            display_string("Month:");
            set_char(6, '0' + (month / 10), false);
            set_char(7, '0' + (month % 10), true);
        }
    }
    gpio_irq_flags.set_confirm = 0;

    int max_days[] = {31, 28, 31, 30,31, 30, 31, 31, 30, 31, 30, 31};
    int day = 1;
    while (!gpio_irq_flags.set_confirm) {
        if (!gpio_irq_flags.decrement_counter || !gpio_irq_flags.increment_counter) {
            if (gpio_irq_flags.decrement_counter)  {
                day = day > 1 ? day - 1 : max_days[month - 1];
                gpio_irq_flags.decrement_counter = 0;
            }
            if (gpio_irq_flags.increment_counter) {
                day = day < max_days[month - 1] ? day + 1: 1;
                gpio_irq_flags.increment_counter = 0;      
            }

            display_string("Day:");
            set_char(6, '0' + (day / 10), false);
            set_char(7, '0' + (day % 10), true);
        }
    }
    gpio_irq_flags.set_confirm = 0;

    display_string("00:00:00");
    int hour = 0;
    while (!gpio_irq_flags.set_confirm) {
        if (!gpio_irq_flags.decrement_counter || !gpio_irq_flags.increment_counter) {
            if (gpio_irq_flags.decrement_counter)  {
                hour = hour > 0 ? hour - 1 : 23;
                gpio_irq_flags.decrement_counter = 0;
            }
            if (gpio_irq_flags.increment_counter) {
                hour = hour < 23 ? hour + 1: 0;
                gpio_irq_flags.increment_counter = 0;      
            }

            set_char(0, '0' + (hour / 10), false);
            set_char(1, '0' + (hour % 10), true);
        }
    }
    gpio_irq_flags.set_confirm = 0;

    int minute = 0;
    while (!gpio_irq_flags.set_confirm) {
        if (!gpio_irq_flags.decrement_counter || !gpio_irq_flags.increment_counter) {
            if (gpio_irq_flags.decrement_counter)  {
                minute = minute > 0 ? minute - 1 : 59;
                gpio_irq_flags.decrement_counter = 0;
            }
            if (gpio_irq_flags.increment_counter) {
                minute = minute < 59 ? minute + 1: 0;
                gpio_irq_flags.increment_counter = 0;      
            }

            set_char(3, '0' + (minute / 10), false);
            set_char(4, '0' + (minute % 10), true);
        }
    }
    gpio_irq_flags.set_confirm = 0;

    int second = 0;
    while (!gpio_irq_flags.set_confirm) {
        if (!gpio_irq_flags.decrement_counter || !gpio_irq_flags.increment_counter) {
            if (gpio_irq_flags.decrement_counter)  {
                second = second > 0 ? second - 1 : 59;
                gpio_irq_flags.decrement_counter = 0;
            }
            if (gpio_irq_flags.increment_counter) {
                second = second < 59 ? second + 1: 0;
                gpio_irq_flags.increment_counter = 0;      
            }

            set_char(6, '0' + (second / 10), false);
            set_char(7, '0' + (second % 10), true);
        }
    }
    gpio_irq_flags.set_confirm = 0;

    clock_set_time(
        int_to_bcd(second),
        int_to_bcd(minute),
        int_to_bcd(hour),
        int_to_bcd(day),
        int_to_bcd(month),
        int_to_bcd(year)
    );

    clock_read_time();
    return;
}

void gpio_iqr_handler(uint gpio, uint32_t event) {
    switch (gpio) {
        case 2:
            gpio_irq_flags.set_time = 1;
            break;
        case 3:
            gpio_irq_flags.increment_counter = 1;
            break;
        case 4:
            gpio_irq_flags.decrement_counter = 1;
            break;
        case 5: 
            gpio_irq_flags.set_confirm = 1;
            break;
    }

    return;
}

    int main() {
    stdio_init_all();
    printf("start\n");

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    if (i2c_init(I2C_DISPLAY_LINE, 400 * 1000) < 0) {
        fprintf(stderr, "Error initializing I2C.\n");
        return 1;
    }

    gpio_set_function(I2C_DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_set_function(I2C_DISPLAY_SDA, GPIO_FUNC_I2C);
    
    bi_decl(bi_2pins_with_func(I2C_DISPLAY_SDA, I2C_DISPLAY_SCL, GPIO_FUNC_I2C));
    
    // Must be called otherwise it will cause the display not to function normally.
    pre_generate_alternate_characters();
    char *clock_string_buffer = malloc(9);

    gpio_init(28);
    gpio_init(2);
    gpio_init(3);
    gpio_init(4);
    gpio_init(5);
    gpio_set_dir(28, GPIO_OUT);
    gpio_set_dir(2, GPIO_IN);
    gpio_set_dir(3, GPIO_IN);
    gpio_set_dir(4, GPIO_IN);
    gpio_set_dir(5, GPIO_IN);

    gpio_pull_up(2);
    gpio_pull_up(3);
    gpio_pull_up(4);
    gpio_pull_up(5);

    gpio_set_irq_enabled_with_callback(2, GPIO_IRQ_EDGE_RISE, true, &gpio_iqr_handler);
    gpio_set_irq_enabled(3, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(4, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(5, GPIO_IRQ_EDGE_RISE, true);

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

    struct repeating_timer timer;
    add_repeating_timer_ms(-60000, timer_callback, NULL, &timer);

   bool show = true;

    while(true)  
    {        
        if (gpio_irq_flags.set_time) {
            set_date_and_time();
            gpio_irq_flags.set_time = 0;
        } else if (gpio_irq_flags.set_confirm)  {
            show = !show;
            gpio_irq_flags.set_confirm = 0;
        }

        // Clock has been reset (MUST BE SET)
		if(clock_buffer[6]==0x00 && clock_buffer[5]==0x01 && clock_buffer[4]==0x01)
		{
            clock_set = false;
			display_string("SET TIME");
            sleep_ms(500);
            clear_all(true);
            sleep_ms(500);
		}
    
        if (clock_set)
        {
            if (show) update_clock();
            else update_date();

        }
	}  

    return 0;
}
