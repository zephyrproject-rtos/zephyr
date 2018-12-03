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

For example, on the following boards with additional LEDs, follow the
instructions below:

Nucleo-64 F103RB/F401RE boards
==============================

Connect two LEDs to PB5 and PB8 pins. PB5 is mapped to the
Arduino's D4 pin and PB8 to Arduino's D15. For more details about
these boards see:

- https://developer.mbed.org/platforms/ST-Nucleo-F103RB/
- https://developer.mbed.org/platforms/ST-Nucleo-F401RE/


Building and Running
*********************

After startup, the program looks up a predefined GPIO device defined by 'PORT',
and configures pins 'LED0' and 'LED1' in output mode.  During each iteration of
the main loop, the state of GPIO lines will be changed so that one of the lines
is in high state, while the other is in low, thus switching the LEDs on and off
in an alternating pattern.

This project does not output to the serial console, but instead causes two LEDs
connected to the GPIO device to blink in an alternating pattern.

The sample can be found here: :file:`samples/basic/disco`.

Nucleo F103RB
=============

.. zephyr-app-commands::
   :zephyr-app: samples/basic/disco
   :board: nucleo_f103rb
   :goals: build
   :compact:

Nucleo F401RE
=============

.. zephyr-app-commands::
   :zephyr-app: samples/basic/disco
   :board: nucleo_f401re
   :goals: build
   :compact:

reel Board
==========

.. zephyr-app-commands::
   :zephyr-app: samples/basic/disco
   :board: reel_board
   :goals: build
   :compact:
