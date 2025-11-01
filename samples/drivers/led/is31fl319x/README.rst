.. zephyr:code-sample:: is31fl319x
   :name: IS31FL319x RGB LED
   :relevant-api: led_interface

   Cycle colors on an RGB LED connected to the IS31FL3194 or IS31FL3197
   using the LED API.

Overview
********

This sample first looks through the color table to map which channels
map to Red, Green and Blue.

It then shows the mapping, but setting one color on and fading it
off. First Red, then Green and finally Blue.

It then forever cycles through several colors on an RGB LED. It uses a
helper function, that maps the RGB colors into the correct order
for the actual hardware LED, as some hardware LEDs will be defined in
RGB order whereas others may be in another order such as BGR.
Once mapped it calls off to the led library to display the color.

Building and Running
********************

This sample can be built and executed on an Arduino Nicla Sense ME, or
Arduino Giga with an Arduino Giga Display shield, or on
any board where the devicetree has an I2C device node with compatible
:dtcompatible:`issi,is31fl3194` or :dtcompatible:`issi,is31fl3197`
with the relevant bus controller node also being enabled.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/is31fl319x
   :board: arduino_nicla_sense_me
   :goals: build flash
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/is31fl319x
   :board: arduino_giga_r1//m7
   :shield: arduino_giga_display_shield
   :goals: build flash
   :compact:

After flashing, the LED starts to switch colors and messages with the current
LED color are printed on the console. If a runtime error occurs, the sample
exits without printing to the console.
