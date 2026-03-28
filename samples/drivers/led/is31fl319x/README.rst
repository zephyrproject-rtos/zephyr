.. zephyr:code-sample:: is31fl319x
   :name: IS31FL319x RGB LED
   :relevant-api: led_interface

   Cycle colors on an RGB LED connected to the IS31FL3194 or IS31FL3197
   using the LED API.

Overview
********

This sample looks through the info for each LED that is defined on
the device, and prints out information about it including
which colors are defined. It will output each of these colors and
fade them down to off.

After all of the LED indexes have been enumerated, it will then
fade all of the defined LEDs by using the led_write_channels API.

The sample will then check to see if this device contains an LED
with a Red, Green and a Blue color defined. If so it will
fade these colors, in the order Red, Green and Blue, and then it
will cylcle through and display several different colors.

It uses a helper function, that maps the RGB colors into the correct order
for the actual hardware LED, as some hardware LEDs will be defined in
RGB order whereas others may be in another order such as BGR.
Once mapped it calls off to the led library to display the color.

For those LEDs with only one color defined, it will use the
led_set brightness API and cycle up to 100% and back down to
zero percent.  It will do this fade in and out, the number of
times that matches the index of this LED within the device.

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
