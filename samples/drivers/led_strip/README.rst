.. zephyr:code-sample:: led-strip
   :name: LED strip sample
   :relevant-api: led_strip_interface

   Control an LED strip example.

Overview
********

This sample application demonstrates basic usage of the LED strip.

Requirements
************

Zephyr supports various LED strip chips. For example,

- WS2812, such as the `NeoPixel(WS2812 compatible) LED Strip from AdaFruit`_.
- APA102, such as the `Dotstar(APA102 compatible) LED Strip from AdaFruit`_.
- LPD8806, such as the `LPD8806 LED Strip from AdaFruit`_.

- Power supply. These LED strips usually require a 5V supply.

- If the LED strip connects to the SPI bus, SPI communications usually use 5V
  signaling, which may require a level translator, such as the
  `74AHCT125 datasheet`_.

.. _NeoPixel(WS2812 compatible) LED Strip from AdaFruit: https://www.adafruit.com/product/3919
.. _Dotstar(APA102 compatible) LED Strip from AdaFruit: https://www.adafruit.com/product/2242
.. _LPD8806 LED Strip from AdaFruit: https://www.adafruit.com/product/1948
.. _74AHCT125 datasheet: https://cdn-shop.adafruit.com/datasheets/74AHC125.pdf

Wiring
******

APA020 and LPD880x
==================

#. Ensure your Zephyr board, the 5V power supply, and the LED strip
   share a common ground.
#. Connect the MOSI pin of your board's SPI master to the data input
   pin of the first IC in the strip.
#. Connect the SCLK pin of your board's SPI master to the clock input
   pin of the first IC in the strip.
#. Connect the 5V power supply pin to the 5V input of the LED strip.

WS2812
======

#. Ensure your Zephyr board, and the LED strip share a common ground.
#. Connect the LED strip control pin (either I2S SDOUT, SPI MOSI or GPIO) from
   your board to the data input pin of the first WS2812 IC in the strip.
#. Power the LED strip at an I/O level compatible with the control pin signals.

Note about thingy52
-------------------

The thingy52 has integrated NMOS transistors, that can be used instead of a level shifter.
The I2S driver supports inverting the output to suit this scheme, using the ``out-active-low`` dts
property. See the overlay file
:zephyr_file:`samples/drivers/led_strip/boards/thingy52_nrf52832.overlay` for more detail.

Building and Running
********************

The sample updates the LED strip periodically. The update frequency can be
modified by changing the :kconfig:option:`CONFIG_SAMPLE_LED_UPDATE_DELAY`.

If there is no chain-length property in the devicetree node, you need to set
the number of LEDs in the :kconfig:option:`CONFIG_SAMPLE_LED_STRIP_LENGTH` option.

Then build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_strip
   :board: <board>
   :goals: flash
   :compact:

When you connect to your board's serial console, you should see the
following output:

.. code-block:: none

   ***** Booting Zephyr OS build v2.1.0-rc1-191-gd2466cdaf045 *****
   [00:00:00.005,920] <inf> main: Found LED strip device WS2812
   [00:00:00.005,950] <inf> main: Displaying pattern on strip

References
**********

- `WS2812 datasheet`_
- `LPD8806 datasheet`_
- `APA102C datasheet`_
- `74AHCT125 datasheet`_
- `RGB LED strips: an overview`_
- An excellent `blog post on WS2812 timing`_.

.. _WS2812 datasheet: https://cdn-shop.adafruit.com/datasheets/WS2812.pdf
.. _LPD8806 datasheet: https://cdn-shop.adafruit.com/datasheets/lpd8806+english.pdf
.. _APA102C datasheet: https://cdn-shop.adafruit.com/product-files/2477/APA102C-iPixelLED.pdf
.. _blog post on WS2812 timing: https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
.. _RGB LED strips\: an overview: http://nut-bolt.nl/2012/rgb-led-strips/
