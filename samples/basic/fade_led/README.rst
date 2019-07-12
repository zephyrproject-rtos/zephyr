.. _fade-led-sample:

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

Nucleo_F401RE, Nucleo_L476RG
============================
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

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the nrf52840_pca10056 board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/fade_led
   :board: nrf52840_pca10056
   :goals: build flash
   :compact:
