.. _disco-sample:

Disco demo
##########

Overview
********

A simple 'disco' demo. The demo assumes that 2 LEDs are connected to
GPIO outputs of the MCU/board.


Wiring
******

This sample should work on board with multiple built-in LEDs without any
changes, otherwise, the code may need some changes before running on various
board: set PORT, LED0 and LED1 according to the board's GPIO configuration.

Building and Running
*********************

After startup, the program looks up a predefined GPIO device defined by 'PORT',
and configures pins 'LED0' and 'LED1' in output mode.  During each iteration of
the main loop, the state of GPIO lines will be changed so that one of the lines
is in high state, while the other is in low, thus switching the LEDs on and off
in an alternating pattern.

This project does not output to the serial console, but instead causes two LEDs
connected to the GPIO device to blink in an alternating pattern.

The sample can be found here: :zephyr_file:`samples/basic/disco`.
