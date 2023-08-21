cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(env_open_clock)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(clock
    main.c
    rtc.c
    display.c
)   

# pull in common dependencies
target_link_libraries(clock pico_stdlib hardware_i2c hardware_pwm hardware_clocks)

# create map/bin/hex file etc.
pico_add_extra_outputs(clock)
