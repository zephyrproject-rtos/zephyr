.. _led_rgb:

LED RGB sample application
##########################

Overview
********

This sample demonstrates the led_rgb driver. The first "rgb-leds" compatible
device instance found in DT is used. For each LEDs attached to this device
(child nodes) the same sequence is executed.
The LED API functions are used to control the LEDs.

Sequence
============

For each RGB LEDs (one after the other):

- Turning on
- Turning off
- Increasing brightness gradually with different colors
- Blinking
- Turning off

Building and Running
********************

This sample can be built and executed on all the boards with LEDs connected.
The LEDs must be correctly described in the DTS. Minimum one "rgb-leds" compatible
device node must be described in the DTS combining 3 single LEDs into one RGB LED child node.
