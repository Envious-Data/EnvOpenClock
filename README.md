# Env-OpenClock
 A fancy lil clock using 8x LTP-305s and a Raspberry Pi Pico.


## Information
This clock features a DS3231 but it doesnt have its own onboard battery management meaning its meant to have constant power but there is nothing stopping anyone from soldering something like a pico lipo shim to the back of the PCB. The RTC is mainly for convenience.

There are 4 buttons and a switch, The switch is meant for Summer time adjustment and the 4 buttons are for setting the time or alarm.

## Implemented Features
- Time Setting
- Date Mode

## Planned software features
(Some ideas, may not get implemented)
- Animated text to show time has been set?
- Animated Transition to menus?
- A menu system
- Alarm system
- Text Marquee modes

## Planned Hardware features
In the current version of the board there is a a DS3231 w/ CR1220 battery, Raspberry pi pico, 4x tactile buttons, 4x IS31FL37s, a small piezo speaker and amp and obviously the 8x LTP305 sockets.

We'd like to change this to have easier to press buttons, a larger battery holder, the speaker may not get re-implemented, we'll see when we decide to move toward finalizing the design and preparing for order. Keep in mind that TIL305s have been discontinued for a long time and LTP305s are now discontinued and hard to get so we'll substitute our own "DIY305" which is a dip14 sized PCB with standard headers and 0402 leds but we have made a 0201 version but may not use it due to cost.



## Hardware
I designed the board in KiCad, you can use various plugins to generate production files for your chosen PCB FAB
Some aspects may change but I generally intend to keep the formfactor the same and all the components on one side for cheaper manufacturing.

since the board uses 8x LTP305s you can also display the date instead of the time, the RTC does store the date as well as the time, currently as of writing this the firmware only displays the current time stored on the DS3231 RTC, but the main planned features are day/date/time setting and then goals can expand. Its possible to add text marquee too.


## PCBWAY
[![PCBWAY](https://4.bp.blogspot.com/-sn_1frB-tto/W_eevs6kyzI/AAAAAAAANhE/ZPlkvH6ysTAMuBJlbtYsSxkC28xaRrZugCLcBGAs/s1600/PCBWay%2BTlogo.png)](http://pcbway.com)

[PCBWAY](http://pcbway.com) Kindly sponsored a batch of PCBs of which you can see a video of the board working below

https://github.com/Envious-Data/Env-OpenClock/assets/5001879/fdac6cdb-803a-443c-b91d-f5f40f54c28f
