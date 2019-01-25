.. _blink-led-sample:

PWM: Blink LED
##############

Overview
********

This is a sample app which blinks a LED using PWM.

The LED will start at a blinking frequency of 1 Hz. Every 4 seconds,
the blinking frequency will double. When the blinking frequency
reaches 64 Hz, the blinking frequency will be halved every 4 seconds
until the blinking frequency reaches 1 Hz. This completes a whole
blinking cycle. This faster-then-slower LED blinking cycle repeats forever.

Wiring
******

Nucleo_F401RE, Nucleo_L476RG, STM32F4_DISCOVERY, Nucleo_F302R8
==============================================================
Connect PWM2(PA0) to LED

Nucleo_F103RB
=============
Connect PWM1(PA8) to LED

Nucleo_L496ZG
=============
No special board setup is necessary because there are three on-board LEDs (red,
green, blue) connected to the Nucleo's PWM.

Hexiwear K64
============
No special board setup is necessary because there is an on-board RGB LED
connected to the K64 PWM.

nrf52840_pca10056
=================
No special board setup is necessary because there is an on-board LED connected.

Arduino 101 and Quark D2000 CRB
===============================
You will need to connect the LED to ground and PWM0 via the shield.
You may need a current limiting resistor. See your LED datasheet.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the arduino_101 board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blink_led
   :board: nrf52840_pca10056
   :goals: build flash
   :compact:

After flashing the image to the board, the user LED on the board should start to
blinking as discussed in overview

