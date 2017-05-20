.. _hexiwear_rotary_led:

Rotary & LEDs
#############

Overview
********
This module demonstrates the Rotary [RGBYO] Click boards, which have a
30 position rotary encoder knob and 16 LEDs.

The encoder knob uses 3 GPIO lines, while the 16 LEDs are connected to a
pair of 74HC595 shift register chips, which are controlled via SPI.

Building and Running
********************

.. code-block:: console

   $ cd samples/boards/hexiwear/rotary-leds
   $ make run


One led "spins" around the circle.   Turning the knob right (or left)
speeds up (or slows down) the spinning effect.   Pressing the button
changes the direction of the spin.


Sample Output
=============

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.7.99 - BUILD: May 16 2017 20:49:22 *****
   Hello World! CONFIG_ARCH=arm
   SPI SPI_0 configured
   Devices initialized
   Pin A configured
   Pin B configured
   Pin BUTTON configured
   Callback added
   Callback B added
   Callback enabled
   Callback B enabled
   button Callback added
   button Callback enabled
   Read encoder: trigger A
   Read encoder: trigger B
     speed:1 deltaT:0 deltaV:1
   Read encoder: trigger A
   Read encoder: trigger B
     speed:5 deltaT:70 deltaV:4
