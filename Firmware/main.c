#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>  // Required for rand() and srand()
#include <string.h>

#include "display.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/structs/timer.h"
#include "hardware/watchdog.h"
#include "music.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "rtc.h"

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
static const uint32_t display_update_interval_ms = 500;  // Update every 500ms

// Global flag to indicate if I2C is active
volatile bool i2c_active = false;

// Watchdog timeout in milliseconds
const uint32_t I2C_WATCHDOG_TIMEOUT_MS = 500;

// Global variable for brightness level (declared elsewhere as uint8_t brightness_level)
extern uint8_t brightness_level;

// New global flag to track if the user has manually set the brightness
volatile bool user_brightness_override = false;

// Define the brightness level for night hours
const uint8_t night_brightness_level = 0b00000001;
// Define the default day brightness level (the one set in the else condition)
const uint8_t default_day_brightness_level = 0b1101000;

// Function to set I2C active flag
void set_i2c_active(bool active) {
  i2c_active = active;
}

typedef void (*AnimationFunction)(int display_index, char previous, char current);

// Updated AnimationType enum
typedef enum {
  ANIM_SLIDE_UP,
  ANIM_PIXELATED,
  ANIM_EXPAND_CONTRACT,
  ANIM_MORPH,
  ANIM_INTERLOCKING_PIECES,
  ANIM_GLITCH_EFFECT,
  ANIM_HEARTBEAT_PULSE,
  ANIM_WATERFALL,
  ANIM_CLOCK_HAND_WIPE,
  ANIM_COUNT  // Keep this as the last element
} AnimationType;

// Updated animations array
AnimationFunction animations[] = {animate_slide_up,
                                  animate_pixelated_transition,
                                  animate_expanding_contracting,
                                  animate_morph,
                                  animate_interlocking_pieces,
                                  animate_glitch_effect,
                                  animate_heartbeat_pulse,
                                  animate_waterfall,
                                  animate_clock_hand_wipe};

// Array of animation names
const char* animation_names[] = {
    "SLIDE_UP",        "PIXELATED",           "EXPAND_CONTRACT",
    "MORPH",           "INTERLOCKING_PIECES", "GLITCH_EFFECT",
    "HEARTBEAT_PULSE", "WATERFALL",           "CLOCK_HAND_WIPE"};

AnimationType selected_animation = ANIM_SLIDE_UP;  // Default animation

void randomize_animation() {
  selected_animation = (AnimationType)(rand() % (ANIM_COUNT));
  printf("Animation set to %s.\n", animation_names[selected_animation]);
}

struct {
  uint8_t set_time : 1;
  uint8_t increment_counter : 1;
  uint8_t decrement_counter : 1;
  uint8_t set_confirm : 1;
} gpio_irq_flags;

// Debounce variables
#define DEBOUNCE_MS 50
uint32_t last_button_press[4] = {0};  // 4 buttons

extern char clock_buffer[7];
extern char* week[];

bool clock_busy = false;
bool clock_set = true;
bool show = true;                // Global variable to track the current mode
void reset_clock_static_vars();  // Function prototype

bool timer_callback(struct repeating_timer* t) {
    // Get the current hour from the clock buffer
    uint8_t current_hour = clock_buffer[2] & 0x3F;

    // Check if it's night time (after 10pm and before 6am)
    bool is_night = (current_hour >= 22 || current_hour <= 6);

    const uint8_t night_brightness_default = 0b00000001;

    if (!user_brightness_override && is_night) {
        if (brightness_level > night_brightness_default) {
            brightness_level = night_brightness_default;
            printf("Night time: Brightness reduced to 0b%08b (%d)\n", brightness_level, brightness_level);
        }
    } else if (!user_brightness_override && !is_night) {
        // We don't explicitly set a day-time default here anymore,
        // assuming display.c initializes it.
        // We might want to restore to a specific day level if the
        // night mode was active and the user hasn't overridden.
        if (brightness_level == night_brightness_default) {
            // Restore to the display.c default if coming out of night mode
            brightness_level = 0b00111111; // Assuming 63 is the display.c default
            printf("Day time: Brightness restored to default 0b%08b (%d)\n", brightness_level, brightness_level);
        }
    }
    return true; // Keep the timer repeating
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

bool update_date() {
  if (clock_set) {
    char* date_string_buffer = malloc(100);
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

// UPDATE CLOCK BIT
bool update_clock() {
  if (!clock_set)
    return false;

  char clock_string_buffer[9];
  clock_read_time();

  uint8_t hour = clock_buffer[2] & 0x3F;
  uint8_t minute = clock_buffer[1] & 0x7F;
  uint8_t second = clock_buffer[0] & 0x7F;

  snprintf(clock_string_buffer, sizeof(clock_string_buffer), "%02x:%02x:%02x",
           hour, minute, second);

  char hour_tens = clock_string_buffer[0];
  char hour_ones = clock_string_buffer[1];
  char minute_tens = clock_string_buffer[3];
  char minute_ones = clock_string_buffer[4];
  char second_tens = clock_string_buffer[6];
  char second_ones = clock_string_buffer[7];

  struct {
    char current;
    char previous;
    int display_index;
  } digits[] = {
      {hour_tens, prev_hour_tens, 0},     {hour_ones, prev_hour_ones, 1},
      {minute_tens, prev_minute_tens, 3}, {minute_ones, prev_minute_ones, 4},
      {second_tens, prev_second_tens, 6}, {second_ones, prev_second_ones, 7}};

  for (int i = 0; i < sizeof(digits) / sizeof(digits[0]); i++) {
    if (digits[i].previous == ' ') {
      set_char(digits[i].display_index, digits[i].current, false);
    } else if (digits[i].current != digits[i].previous) {
      // Simple approach - use the animations function array
      if (selected_animation < ANIM_COUNT) {
        animations[selected_animation](digits[i].display_index,
                                       digits[i].previous, digits[i].current);
      }
    }
  }

  // Update previous values
  prev_hour_tens = hour_tens;
  prev_hour_ones = hour_ones;
  prev_minute_tens = minute_tens;
  prev_minute_ones = minute_ones;
  prev_second_tens = second_tens;
  prev_second_ones = second_ones;

  set_char(2, ':', false);
  set_char(5, ':', false);

  return true;
}

void set_date_and_time() {
  clear_all(false);
  update_display();  // Ensure full clear
  clock_set = true;

  struct {
    const char* prompt;
    int* value;
    int min;
    int max;
  } settings[] = {{"Year:", &(int){23}, 0, 99},  {"Month:", &(int){1}, 1, 12},
                  {"Day:", &(int){1}, 1, 31},    {"Hour:", &(int){0}, 0, 23},
                  {"Minute:", &(int){0}, 0, 59}, {"Second:", &(int){0}, 0, 59}};

  for (int i = 0; i < sizeof(settings) / sizeof(settings[0]); i++) {
    clear_all(false);
    update_display();  // Ensure full clear
    int* value = settings[i].value;

    if (strcmp(settings[i].prompt, "Day:") == 0) {
      int month = *(settings[1].value);
      int year = *(settings[0].value) + 2000;
      int max_days[] = {
          31, (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28,
          31, 30,
          31, 30,
          31, 31,
          30, 31,
          30, 31};
      settings[i].max = max_days[month - 1];
    }

    while (!gpio_irq_flags.set_confirm) {
      if (!gpio_irq_flags.decrement_counter ||
          !gpio_irq_flags.increment_counter) {
        if (gpio_irq_flags.decrement_counter) {
          *value = (*value > settings[i].min) ? *value - 1 : settings[i].max;
          gpio_irq_flags.decrement_counter = 0;
        }
        if (gpio_irq_flags.increment_counter) {
          *value = (*value < settings[i].max) ? *value + 1 : settings[i].min;
          gpio_irq_flags.increment_counter = 0;
        }

        display_string(settings[i].prompt);
        set_char(6, '0' + (*value / 10), false);
        set_char(7, '0' + (*value % 10), true);
      }
    }
    gpio_irq_flags.set_confirm = 0;
    if (i == 4) {  // if minutes were just set.
      clear(3, false);
      clear(4, false);
      update_display();
    }
  }

  clock_set_time(
      int_to_bcd(*(settings[5].value)), int_to_bcd(*(settings[4].value)),
      int_to_bcd(*(settings[3].value)), int_to_bcd(*(settings[2].value)),
      int_to_bcd(*(settings[1].value)), int_to_bcd(*(settings[0].value)));

  clock_read_time();

  // Display full time with separators
  char time_string[9];
  snprintf(time_string, sizeof(time_string), "%02x:%02x:%02x",
           clock_buffer[2] & 0x3F, clock_buffer[1] & 0x7F,
           clock_buffer[0] & 0x7F);
  display_string(time_string);
  set_char(2, ':', false);
  set_char(5, ':', false);
  update_display();
}

void cycle_brightness() {
    user_brightness_override = true; // Set the flag when user changes brightness
    if (brightness_level == 0b00111111) { // Currently at max (63)
        brightness_level = 0b00110000;     // Go to 48
    } else if (brightness_level == 0b00110000) { // Currently at 48
        brightness_level = 0b00011111;     // Go to 31
    } else if (brightness_level == 0b00011111) { // Currently at 31
        brightness_level = 0b00001111;     // Go to 15
    } else if (brightness_level == 0b00001111) { // Currently at 15
        brightness_level = 0b00000001;     // Go to 1
    } else {                                  // Catch-all: If at 1 or something else
        brightness_level = 0b00111111;     // Cycle back to max (63)
    }
    printf("Brightness level set to: 0b%08b (%d)\n", brightness_level, brightness_level);
    // The brightness will be updated in the main loop implicitly
}

void gpio_iqr_handler(uint gpio, uint32_t event) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    int button_index = -1;

    switch (gpio) {
        case 2:
            button_index = 0;
            break;
        case 3:
            button_index = 1;
            break;
        case 4:
            button_index = 2;
            break;
        case 5:
            button_index = 3;
            break;
    }

    if (button_index != -1 &&
        (current_time - last_button_press[button_index]) > DEBOUNCE_MS) {
        last_button_press[button_index] = current_time;

        switch (gpio) {
            case 2:
                gpio_irq_flags.set_time = 1;
                break;
            case 3: // Increment button (now switches animations)
                gpio_irq_flags.increment_counter = 1;
                break;
            case 4: // Third button now cycles brightness
                cycle_brightness();
                break;
            case 5:
                gpio_irq_flags.set_confirm = 1;
                break;
        }
    }
    return;
}

void core1_entry() {
  char serial_buffer[100];
  while (true) {
    if (fgets(serial_buffer, sizeof(serial_buffer), stdin) != NULL) {
      if (strncmp(serial_buffer, "settime", 7) == 0) {
        int year, month, day, hour, minute, second;
        if (sscanf(serial_buffer, "settime %d-%d-%d %d:%d:%d", &year, &month,
                   &day, &hour, &minute, &second) == 6) {
          clock_set_time(int_to_bcd(second), int_to_bcd(minute),
                         int_to_bcd(hour), int_to_bcd(day), int_to_bcd(month),
                         int_to_bcd(year % 100));
          clock_read_time();
          printf("Time set from serial input.\n");
        } else {
          printf("Invalid time format.\n");
        }
      } else if (strncmp(serial_buffer, "reset", 5) == 0) {
        clear_all(true);
        printf("Resetting Pico...\n");
        sleep_ms(100);
        watchdog_enable(1, 1);
        while (1)
          ;
      } else if (strncmp(serial_buffer, "anim ", 5) == 0) {
        int anim_num;
        if (sscanf(serial_buffer, "anim %d", &anim_num) == 1) {
          if (anim_num >= 0 && anim_num < ANIM_COUNT) {
            selected_animation = (AnimationType)anim_num;
            printf("Animation set to %d.\n", anim_num);
          } else {
            printf("Invalid animation number. Must be between 0 and %d.\n",
                   ANIM_COUNT - 1);
          }
        } else {
          printf("Invalid animation command format. Use 'anim <number>'.\n");
        }
      } else {
        printf("Unknown command: %s", serial_buffer);
      }
    }

    if (i2c_active) {
      watchdog_enable(I2C_WATCHDOG_TIMEOUT_MS, 1);
      watchdog_update();
    } else {
      watchdog_enable(1, 0);
    }
  }
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

    gpio_set_irq_enabled_with_callback(2, GPIO_IRQ_EDGE_RISE, true,
                                                       &gpio_iqr_handler);
    gpio_set_irq_enabled(3, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(4, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(5, GPIO_IRQ_EDGE_RISE, true);

    struct repeating_timer timer;
    add_repeating_timer_ms(-60000, timer_callback, NULL, &timer);

    // char startup_message[] = "PICO8LED";
    char startup_message[] = " envclk ";
    int message_length = strlen(startup_message);

    // Seed the random number generator using the current time from the RTC.
    if (clock_set) {
        clock_read_time();
        uint8_t hour = clock_buffer[2] & 0x3F;
        uint8_t minute = clock_buffer[1] & 0x7F;
        uint8_t second = clock_buffer[0] & 0x7F;
        // Calculate the total number of seconds, add some more variation.
        unsigned int seed = (hour * 3600) + (minute * 60) + second;
        seed ^= (seed << 13);
        seed ^= (seed >> 17);
        seed ^= (seed << 5);
        srand(seed);
    } else {
        srand(time(NULL)); // Fallback if the clock isn't set.
    }
    randomize_animation();

    for (int display = 0; display < 8; display++) {
        if (display < message_length) {
            char current_char = ' ';
            char next_char = startup_message[display];
            if (selected_animation < ANIM_COUNT) { // Use selected_animation
                animations[selected_animation](display, current_char, next_char);
            }
        } else {
            set_char(display, ' ', true);
        }
    }

    sleep_ms(1000); // Display the startup message for a while
    for (int display = 0; display < 8; display++) {
        if (display < message_length) {
            char current_char = startup_message[display];
            char next_char = ' '; // Slide to a space
            if (selected_animation < ANIM_COUNT) {
                animations[selected_animation](display, current_char, next_char);
            }
        }
    }

    clear_all(true); // Clear after animation

    bool show = true;
    if (clock_set) {
        // Animate to current time
        char current_time_string[9];
        for (int i = 0; i < 8; i++) {
            clock_read_time();
            sprintf(current_time_string, "%02x:%02x:%02x", clock_buffer[2] & 0x3F,
                    clock_buffer[1] & 0x7F, clock_buffer[0] & 0x7F);
            if (i == 2 || i == 5) {
                set_char(i, ':', false); // Set colon *before* animation
            } else {
                char next_char = current_time_string[i];
                char current_char = ' ';
                if (selected_animation < ANIM_COUNT) {
                    animations[selected_animation](i, current_char, next_char);
                }
            }
        }
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
            gpio_irq_flags.set_confirm = 0;

            if (show) { // Time to Date animation
                char time_string[9];
                char date_string[9];

                // Capture current time
                clock_read_time();
                sprintf(time_string, "%02x:%02x:%02x", clock_buffer[2] & 0x3F,
                        clock_buffer[1] & 0x7F, clock_buffer[0] & 0x7F);

                // Capture current date
                clock_read_time();
                sprintf(date_string, "%02x/%02x/%02x", clock_buffer[4], clock_buffer[5],
                        clock_buffer[6]);

                for (int i = 0; i < 8; i++) {
                    char current_char = ' '; // Or perhaps the current displayed character?
                    char next_char_time = time_string[i];
                    char next_char_date = date_string[i];
                    if (selected_animation < ANIM_COUNT) {
                        animations[selected_animation](i, current_char, next_char_date); // Animate to date
                    }
                }
                sleep_ms(100);
                show = false;
            } else { // Date to Time animation
                char date_string[9];
                char time_string[9];

                // Capture current date
                clock_read_time();
                sprintf(date_string, "%02x/%02x/%02x", clock_buffer[4], clock_buffer[5],
                        clock_buffer[6]);

                // Capture current time
                clock_read_time();
                sprintf(time_string, "%02x:%02x:%02x", clock_buffer[2] & 0x3F,
                        clock_buffer[1] & 0x7F, clock_buffer[0] & 0x7F);

                for (int i = 0; i < 8; i++) {
                    char current_char = ' '; // Or perhaps the current displayed character?
                    char next_char_date = date_string[i];
                    char next_char_time = time_string[i];
                    if (selected_animation < ANIM_COUNT) {
                        animations[selected_animation](i, current_char, next_char_time); // Animate to time
                    }
                }
                sleep_ms(100);
                show = true;
                reset_clock_static_vars();
            }
        }

        if (clock_buffer[6] == 0x00 && clock_buffer[5] == 0x01 &&
            clock_buffer[4] == 0x01) {
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

        if (gpio_irq_flags.increment_counter) {
            selected_animation =
                (AnimationType)((selected_animation + 1) % ANIM_COUNT);
            printf("Animation set to %s.\n",
                   animation_names[selected_animation]); // Print animation name
            gpio_irq_flags.increment_counter = 0;         // Clear flag
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