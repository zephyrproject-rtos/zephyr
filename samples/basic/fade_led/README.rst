PWM: Fade LED
#############

Overview
********

This is a sample app which fades a LED using PWM.

The LED will start from dark and increases its
brightness gradually for 10 seconds. Then, the
brightness reduces gradually for 10 seconds and
finally the LED becomes dark again. The LED will
repeat this cycle for ever.

Wiring
******

Arduino 101 and Quark D2000 CRB
===============================
You will need to connect the LED to ground and PWM0 via
the shield. You may need a current limiting resistor. See
your LED datasheet.

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

   $ cd samples/basic/fade_led
   $ make BOARD=arduino_101
   $ make BOARD=arduino_101 flash
