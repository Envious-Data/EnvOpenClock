#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "display.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/structs/timer.h"
#include "music.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "rtc.h"
#include "hardware/watchdog.h"

const int song[] = {
    NOTE_E5, 4,  NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4, NOTE_C5,  8,
    NOTE_B4, 8,  NOTE_A4, 4,  NOTE_A4, 8,  NOTE_C5, 8, NOTE_E5,  4,
    NOTE_D5, 8,  NOTE_C5, 8,  NOTE_B4, -4, NOTE_C5, 8, NOTE_D5,  4,
    NOTE_E5, 4,  NOTE_C5, 4,  NOTE_A4, 4,  NOTE_A4, 8, REST,     4,
    NOTE_D5, -4, NOTE_F5, 8,  NOTE_A5, 4,  NOTE_G5, 8, NOTE_F5,  8,
    NOTE_E5, -4, NOTE_C5, 8,  NOTE_E5, 4,  NOTE_D5, 8, NOTE_C5,  8,
    NOTE_B4, 4,  NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4, NOTE_E5,  4,
    NOTE_C5, 4,  NOTE_A4, 4,  NOTE_A4, 4,  REST,    4, NOTE_E5,  4,
    NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4,  NOTE_C5, 8, NOTE_B4,  8,
    NOTE_A4, 4,  NOTE_A4, 8,  NOTE_C5, 8,  NOTE_E5, 4, NOTE_D5,  8,
    NOTE_C5, 8,  NOTE_B4, -4, NOTE_C5, 8,  NOTE_D5, 4, NOTE_E5,  4,
    NOTE_C5, 4,  NOTE_A4, 4,  NOTE_A4, 8,  REST,    4, NOTE_D5,  -4,
    NOTE_F5, 8,  NOTE_A5, 4,  NOTE_G5, 8,  NOTE_F5, 8, NOTE_E5,  -4,
    NOTE_C5, 8,  NOTE_E5, 4,  NOTE_D5, 8,  NOTE_C5, 8, NOTE_B4,  4,
    NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4,  NOTE_E5, 4, NOTE_C5,  4,
    NOTE_A4, 4,  NOTE_A4, 4,  REST,    4,  NOTE_E5, 2, NOTE_C5,  2,
    NOTE_D5, 2,  NOTE_B4, 2,  NOTE_C5, 2,  NOTE_A4, 2, NOTE_GS4, 2,
    NOTE_B4, 4,  REST,    8,  NOTE_E5, 2,  NOTE_C5, 2, NOTE_D5,  2,
    NOTE_B4, 2,  NOTE_C5, 4,  NOTE_E5, 4,  NOTE_A5, 2, NOTE_GS5, 2,
};

static char prev_hour_tens = ' ';
static char prev_hour_ones = ' ';
static char prev_minute_tens = ' ';
static char prev_minute_ones = ' ';
static char prev_second_tens = ' ';
static char prev_second_ones = ' ';

// Variables for display update timing
static uint32_t last_display_update = 0;
static const uint32_t display_update_interval_ms = 500; // Update every 500ms

// Global flag to indicate if I2C is active
volatile bool i2c_active = false;

// Watchdog timeout in milliseconds
const uint32_t I2C_WATCHDOG_TIMEOUT_MS = 500;

// Function to set I2C active flag
void set_i2c_active(bool active) {
    i2c_active = active;
}

struct {
  uint8_t set_time : 1;
  uint8_t increment_counter : 1;
  uint8_t decrement_counter : 1;
  uint8_t set_confirm : 1;
} gpio_irq_flags;

extern char clock_buffer[];
extern char* week[];

bool clock_busy = false;
bool clock_set = true;
bool show = true;                // Global variable to track the current mode
void reset_clock_static_vars();  // Function prototype

bool timer_callback(struct repeating_timer* t) {
  // if after 10pm and before 6am
  if (clock_buffer[2] >= 0x22 || clock_buffer[2] <= 0x06) {
    brightness_level = 0b0000001;
  } else {
    brightness_level = 0b1101000;
  }
}

// Function to check if it's time to update the display
bool should_update_display() {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_display_update >= display_update_interval_ms) {
        last_display_update = current_time;
        return true;
    }
    return false;
}

// ... (Your existing main.c code) ...

bool update_clock() {
    if (clock_set) {
        char *clock_string_buffer = malloc(9);
        if (clock_string_buffer == NULL) {
            return false;
        }

        clock_read_time();

        clock_buffer[0] &= 0x7F;  // sec
        clock_buffer[1] &= 0x7F;  // min
        clock_buffer[2] &= 0x3F;  // hour

        sprintf(clock_string_buffer, "%02x:%02x:%02x", clock_buffer[2],
                clock_buffer[1], clock_buffer[0]);

        // Extract individual digits
        char hour_tens = clock_string_buffer[0];
        char hour_ones = clock_string_buffer[1];
        char minute_tens = clock_string_buffer[3];
        char minute_ones = clock_string_buffer[4];
        char second_tens = clock_string_buffer[6];
        char second_ones = clock_string_buffer[7];

        // Animate digit changes with slide_up
        if (prev_hour_tens == ' ') {
            set_char(0, hour_tens, false);
        } else if (hour_tens != prev_hour_tens) {
            animate_slide_up(0, prev_hour_tens, hour_tens);
        }

        if (prev_hour_ones == ' ') {
            set_char(1, hour_ones, false);
        } else if (hour_ones != prev_hour_ones) {
            animate_slide_up(1, prev_hour_ones, hour_ones);
        }

        if (prev_minute_tens == ' ') {
            set_char(3, minute_tens, false);
        } else if (minute_tens != prev_minute_tens) {
            animate_slide_up(3, prev_minute_tens, minute_tens);
        }

        if (prev_minute_ones == ' ') {
            set_char(4, minute_ones, false);
        } else if (minute_ones != prev_minute_ones) {
            animate_slide_up(4, prev_minute_ones, minute_ones);
        }

        if (prev_second_tens == ' ') {
            set_char(6, second_tens, false);
        } else if (second_tens != prev_second_tens) {
            animate_slide_up(6, prev_second_tens, second_tens);
        }

        if (prev_second_ones == ' ') {
            set_char(7, second_ones, false);
        } else if (second_ones != prev_second_ones) {
            animate_slide_up(7, prev_second_ones, second_ones);
        }

        // Update previous digit values
        prev_hour_tens = hour_tens;
        prev_hour_ones = hour_ones;
        prev_minute_tens = minute_tens;
        prev_minute_ones = minute_ones;
        prev_second_tens = second_tens;
        prev_second_ones = second_ones;

        free(clock_string_buffer);
        set_char(2, ':', false);
        set_char(5, ':', false);
        return true;
    }
    return false;
}

// ... (Your animate_vertical_wipe function from display.c) ...

bool update_date() {
    if (clock_set) {
        char *date_string_buffer = malloc(100);
        if (date_string_buffer == NULL) {
            return false;
        }

        clock_read_time();

        clock_buffer[4] &= 0x7F;  // day
        clock_buffer[5] &= 0x7F;  // month
        clock_buffer[6] &= 0x3F;  // year

        sprintf(date_string_buffer, "%02x/%02x/%02x", clock_buffer[4],
                clock_buffer[5], clock_buffer[6]);
        display_string(date_string_buffer);
        free(date_string_buffer);
        set_char(2, '/', false);
        set_char(5, '/', false);
        return false;
    }
    return false;
}

int calculate_frequency(uint frequency) {
  // 1Hz is once per second
  // Delay is the time in microseconds to sleep
  int delay = (double)100000 / frequency;
  return delay >> 1;
}

// Function to run on core 1 for serial input
void core1_entry() {
    char serial_buffer[100];
    while (true) {
        if (fgets(serial_buffer, sizeof(serial_buffer), stdin) != NULL) {
            // Process serial input here
            if (strncmp(serial_buffer, "settime", 7) == 0) {
                // Parse and set time from serial input
                int year, month, day, hour, minute, second;
                if (sscanf(serial_buffer, "settime %d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
                    clock_set_time(int_to_bcd(second), int_to_bcd(minute), int_to_bcd(hour),
                                    int_to_bcd(day), int_to_bcd(month), int_to_bcd(year % 100));
                    clock_read_time();
                    printf("Time set from serial input.\n");
                } else {
                    printf("Invalid time format.\n");
                }
            } else if (strncmp(serial_buffer, "reset", 5) == 0) {
                // Reset command received
                clear_all(true); // Clear the display
                printf("Resetting Pico...\n");
                sleep_ms(100); // Small delay to allow serial output to complete.
                watchdog_enable(1, 1); // Enable watchdog with a short timeout.
                while(1); // Wait for the watchdog to trigger the reset.
            } else {
                printf("Unknown command: %s", serial_buffer);
            }
        }

        // I2C watchdog logic
        if (i2c_active) {
            // If I2C is active, enable watchdog with I2C timeout
            watchdog_enable(I2C_WATCHDOG_TIMEOUT_MS, 1);
            // reset the watchdog timer.
            watchdog_update();
        } else {
            // If I2C is not active, disable watchdog.
            watchdog_enable(1, 0); // Disable watchdog.
        }
    }
}

void set_date_and_time() {
  clear_all(false);
  clock_set = true;

  // Year
  int year = 23;
  while (!gpio_irq_flags.set_confirm) {
    if (!gpio_irq_flags.decrement_counter ||
        !gpio_irq_flags.increment_counter) {
      if (gpio_irq_flags.decrement_counter) {
        year = year > 0 ? year - 1 : 99;
        gpio_irq_flags.decrement_counter = 0;
      }
      if (gpio_irq_flags.increment_counter) {
        year = year < 99 ? year + 1 : 0;
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
    if (!gpio_irq_flags.decrement_counter ||
        !gpio_irq_flags.increment_counter) {
      if (gpio_irq_flags.decrement_counter) {
        month = month > 1 ? month - 1 : 12;
        gpio_irq_flags.decrement_counter = 0;
      }
      if (gpio_irq_flags.increment_counter) {
        month = month < 12 ? month + 1 : 1;
        gpio_irq_flags.increment_counter = 0;
      }

      display_string("Month:");
      set_char(6, '0' + (month / 10), false);
      set_char(7, '0' + (month % 10), true);
    }
  }
  gpio_irq_flags.set_confirm = 0;

  int max_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int day = 1;
  while (!gpio_irq_flags.set_confirm) {
    if (!gpio_irq_flags.decrement_counter ||
        !gpio_irq_flags.increment_counter) {
      if (gpio_irq_flags.decrement_counter) {
        day = day > 1 ? day - 1 : max_days[month - 1];
        gpio_irq_flags.decrement_counter = 0;
      }
      if (gpio_irq_flags.increment_counter) {
        day = day < max_days[month - 1] ? day + 1 : 1;
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
    if (!gpio_irq_flags.decrement_counter ||
        !gpio_irq_flags.increment_counter) {
      if (gpio_irq_flags.decrement_counter) {
        hour = hour > 0 ? hour - 1 : 23;
        gpio_irq_flags.decrement_counter = 0;
      }
      if (gpio_irq_flags.increment_counter) {
        hour = hour < 23 ? hour + 1 : 0;
        gpio_irq_flags.increment_counter = 0;
      }

      set_char(0, '0' + (hour / 10), false);
      set_char(1, '0' + (hour % 10), true);
    }
  }
  gpio_irq_flags.set_confirm = 0;

  int minute = 0;
  while (!gpio_irq_flags.set_confirm) {
    if (!gpio_irq_flags.decrement_counter ||
        !gpio_irq_flags.increment_counter) {
      if (gpio_irq_flags.decrement_counter) {
        minute = minute > 0 ? minute - 1 : 59;
        gpio_irq_flags.decrement_counter = 0;
      }
      if (gpio_irq_flags.increment_counter) {
        minute = minute < 59 ? minute + 1 : 0;
        gpio_irq_flags.increment_counter = 0;
      }

      set_char(3, '0' + (minute / 10), false);
      set_char(4, '0' + (minute % 10), true);
    }
  }
  gpio_irq_flags.set_confirm = 0;

  int second = 0;
  while (!gpio_irq_flags.set_confirm) {
    if (!gpio_irq_flags.decrement_counter ||
        !gpio_irq_flags.increment_counter) {
      if (gpio_irq_flags.decrement_counter) {
        second = second > 0 ? second - 1 : 59;
        gpio_irq_flags.decrement_counter = 0;
      }
      if (gpio_irq_flags.increment_counter) {
        second = second < 59 ? second + 1 : 0;
        gpio_irq_flags.increment_counter = 0;
      }

      set_char(6, '0' + (second / 10), false);
      set_char(7, '0' + (second % 10), true);
    }
  }
  gpio_irq_flags.set_confirm = 0;

  clock_set_time(int_to_bcd(second), int_to_bcd(minute), int_to_bcd(hour),
                 int_to_bcd(day), int_to_bcd(month), int_to_bcd(year));

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

    sleep_ms(500);

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 0);

    if (i2c_init(I2C_DISPLAY_LINE, 100 * 1000) < 0) {
        fprintf(stderr, "Error initializing I2C.\n");
        return 1;
    }

    gpio_set_function(I2C_DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_set_function(I2C_DISPLAY_SDA, GPIO_FUNC_I2C);

    bi_decl(bi_2pins_with_func(I2C_DISPLAY_SDA, I2C_DISPLAY_SCL, GPIO_FUNC_I2C));

    pre_generate_alternate_characters();

    // Initialize button and buzzer GPIOs
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

    struct repeating_timer timer;
    add_repeating_timer_ms(-60000, timer_callback, NULL, &timer);

    char startup_message[] = "PICO8LED";
    int message_length = strlen(startup_message);

    for (int display = 0; display < 8; display++) {
        if (display < message_length) {
            char current_char = ' ';
            char next_char = startup_message[display];
            animate_slide_up(display, current_char, next_char);
        } else {
            set_char(display, ' ', true);
        }
    }

    sleep_ms(1000); // Display the startup message for a while
    for (int display = 0; display < 8; display++) {
        if (display < message_length) {
            char current_char = startup_message[display];
            char next_char = ' '; // Slide to a space
            animate_slide_up(display, current_char, next_char);
        }
    }

    clear_all(true); // Clear after animation

    bool show = true;
    if (clock_set) {
        // Animate to 00:00:00
        char time_string[] = "00:00:00";
        for (int i = 0; i < 8; i++) {
            if (i == 2 || i == 5) {
                set_char(i, time_string[i], false);
            } else {
                animate_slide_up(i, ' ', time_string[i]);
            }
        }
        sleep_ms(100);

        // Animate to current time
        char current_time_string[9];
        for (int i = 0; i < 8; i++) {
            if (i != 2 && i != 5) {
                // Read current time inside the animation loop
                clock_read_time();
                sprintf(current_time_string, "%02x:%02x:%02x", clock_buffer[2] & 0x3F,
                        clock_buffer[1] & 0x7F, clock_buffer[0] & 0x7F);
                animate_slide_up(i, time_string[i], current_time_string[i]);
            }
        }
        sleep_ms(100);

        // Initialize previous time
        update_clock();
        sleep_ms(100);
    }

    // Launch core 1 for serial input
    multicore_launch_core1(core1_entry);

    while (true) {
        // REMOVED: update_rtc_from_serial();

        if (gpio_irq_flags.set_time) {
            set_date_and_time();
            gpio_irq_flags.set_time = 0;
        } else if (gpio_irq_flags.set_confirm) {
            show = !show;
            gpio_irq_flags.set_confirm = 0;
            if (show) {
                reset_clock_static_vars();
                set_char(2, ':', false);
                set_char(5, ':', false);
                update_clock();
            } else {
                set_char(2, '/', false);
                set_char(5, '/', false);
            }
        }

        if (clock_buffer[6] == 0x00 && clock_buffer[5] == 0x01 && clock_buffer[4] == 0x01) {
            clock_set = false;
            display_string("SET TIME");
			clear_all(true);
            sleep_ms(500);
        }

        if (clock_set) {
            if (should_update_display()) {
                if (show) {
                    set_i2c_active(true); // I2C start
                    if (!update_clock()) {
                        // i2c_bus_recovery(I2C_DISPLAY_LINE); //Removed
                    }
                    set_i2c_active(false); // I2C end
                } else {
                    set_i2c_active(true); // I2C start
                    if (!update_date()) {
                        // i2c_bus_recovery(I2C_DISPLAY_LINE); //Removed
                    }
                    set_i2c_active(false); // I2C end
                }
            }
        }
    }
    return 0;
}

void reset_clock_static_vars() {
  prev_hour_tens = ' ';
  prev_hour_ones = ' ';
  prev_minute_tens = ' ';
  prev_minute_ones = ' ';
  prev_second_tens = ' ';
  prev_second_ones = ' ';
}