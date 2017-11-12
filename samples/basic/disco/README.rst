.. _disco-sample:

Disco demo
##########

Overview
********

A simple 'disco' demo. The demo assumes that 2 LEDs are connected to
GPIO outputs of the MCU/board.


Wiring
******

The code may need some work before running on another board: set PORT,
LED1 and LED2 according to the board's GPIO configuration.

Nucleo-64 F103RB/F401RE boards
==============================

Connect two LEDs to PB5 and PB8 pins. PB5 is mapped to the
Arduino's D4 pin and PB8 to Arduino's D15. For more details about
these boards see:

- https://developer.mbed.org/platforms/ST-Nucleo-F103RB/
- https://developer.mbed.org/platforms/ST-Nucleo-F401RE/

Arduino 101 (x86)
=================

Connect two LEDs to D4 (IO4) and D7 (IO7) pins. The schematics for the Arduino
101 board is available at:

https://www.arduino.cc/en/uploads/Main/Arduino101-REV4Schematic.pdf

For Arduino 101's pinmux mapping in Zephyr, see: :file:`boards/x86/arduino_101/pinmux.c`

Modify the src/main.c file and set:

.. code-block:: c

   #define PORT	CONFIG_GPIO_QMSI_0_NAME
   /* GPIO_19 is Arduino's D4 */
   #define LED1	19
   /* GPIO_20 is Arduino's D7 */
   #define LED2	20

Building and Running
*********************

After startup, the program looks up a predefined GPIO device defined by 'PORT',
and configures pins 'LED1' and 'LED2' in output mode.  During each iteration of
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

Arduino 101
============

.. zephyr-app-commands::
   :zephyr-app: samples/basic/disco
   :board: arduino_101
   :goals: build
   :compact:
