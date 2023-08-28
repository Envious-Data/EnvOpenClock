# Env-OpenClock
 A fancy lil clock using 8x LTP-305s and a Raspberry Pi Pico.


## Information
This clock features a DS3231 but it doesnt have its own onboard battery management meaning its meant to have constant power but there is nothing stopping anyone from soldering something like a pico lipo shim to the back of the PCB. The RTC is mainly for convenience.

There are 4 buttons and a switch, The switch is meant for Summer time adjustment and the 4 buttons are for setting the time or alarm.

## Hardware
I designed the board in KiCad, you can use various plugins to generate production files for your chosen PCB FAB
Some aspects may change but I generally intend to keep the formfactor the same and all the components on one side for cheaper manufacturing.

since the board uses 8x LTP305s you can also display the date instead of the time, the RTC does store the date as well as the time, currently as of writing this the firmware only displays the current time stored on the DS3231 RTC, but the main planned features are day/date/time setting and then goals can expand. Its possible to add text marquee too.


## PCBWAY
[PCBWAY](https://www.pcbway.com/) Kindly sponsored a batch of PCBs of which you can see a video of the board working below.
