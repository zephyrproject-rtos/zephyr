.. zephyr:code-sample:: is31fl3194
   :name: IS31FL3194 RGB LED
   :relevant-api: led_interface

   Cycle colors on an RGB LED connected to the IS31FL3194 using the LED API.

Overview
********

This sample cycles several colors on an RGB LED forever using the LED API.

Building and Running
********************

This sample can be built and executed on an Arduino Nicla Sense ME, or on
any board where the devicetree has an I2C device node with compatible
:dtcompatible:`issi,is31fl3194` enabled, along with the relevant bus
controller node also being enabled.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/issi_is31fl3194
   :board: arduino_nicla_sense_me
   :goals: build flash
   :compact:

After flashing, the LED starts to switch colors and messages with the current
LED color are printed on the console. If a runtime error occurs, the sample
exits without printing to the console.
