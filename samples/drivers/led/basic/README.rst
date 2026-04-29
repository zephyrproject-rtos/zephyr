.. zephyr:code-sample:: led-basic
   :name: Basic LED driver sample
   :relevant-api: led_interface

   Control LED driver using the LED API.

Overview
********

This sample allows to run any LED driver compliant with the LED driver API. It
uses only standard API. The device labeled as "leds" in the device tree is used.
Each LED associated with this device is turned on one by one. After all LEDs are
turned on, if the write_channels function is implemented, it is used to blink
all LEDs simultaneously. Following this, each LED is turned off one by one.
After all LEDs are turned off, the pattern repeats.

Building and Running
********************

This sample can be built and executed on boards with an LED driver compliant
with the LED driver API. The device tree must include a device labeled as "leds"
to represent the LED driver. Each LED associated with this device should be
properly defined to allow sequential control as demonstrated in this sample.
