.. _rgb-led-sample:

PWM: RGB LED
############

Overview
********

This is a sample app which drives a RGB LED using PWM.

There are three LEDs of single color in a RGB LED. The
three LEDs will be driven by a pwm port each. The pulse
width for each pwm port will change from zero to period.
So, the brightness for each LED will change from dark to
max brightness in 11 steps. The three "for" loops (one
for each LED) will generate 1331 combinations and so,
1331 different colors. The whole process will repeat for
ever.

Wiring
******

Arduino 101
===========

You will need to connect the LED pins to PWM0, PWM1 and PWM2
on Arduino 101 via the shield. Depending on what kind of RGB
LED you are using, please connect the common cathode to the
ground or the common anode to Vcc. You need current limiting
resistor for each of the single color LEDs.

The sample app requires three PWM ports. So, it can not work
on Quark D2000 platform.

Hexiwear K64
============
No special board setup is necessary because there is an on-board RGB LED
connected to the K64 PWM.

Building and Running
********************

This samples does not output anything to the console.  It can be built and
flashed to a board as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/rgb_led
   :board: arduino_101
   :goals: build flash
   :compact:
