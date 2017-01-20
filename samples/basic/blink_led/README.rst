PWM: Blink LED
##############

Overview
********

This is a sample app which blinks a LED using PWM.

The LED will start at a blinking frequency of 0.5Hz. Every 4 seconds,
the blinking frequency will double. When the blinking frequency
reaches 500Hz, the blinking frequency will be halved every 4 seconds
until the blinking frequency reaches 0.5Hz. This completes a whole
blinking cycle. From now on, the LED will repeat the blinking cycle
for ever.

Wiring
******

Arduino 101 and Quark D2000 CRB
===============================
You will need to connect the LED to ground and PWM0 via the shield.
You may need a current limiting resistor. See your LED datasheet.

Nucleo_F401RE and Nucleo_L476RG
===============================
Connect PWM2(PA0) to LED

Nucleo_F103RB
=============
Connect PWM1(PA8) to LED

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the arduino_101 board:

.. code-block:: console

   $ cd samples/basic/blink_led
   $ make BOARD=arduino_101
   $ make BOARD=arduino_101 flash

After flashing the image to the board, the user LED on the board should start to
blinking as discussed in overview

