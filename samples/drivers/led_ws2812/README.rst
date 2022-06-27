.. _led_ws2812_sample:

WS2812 Sample Application
#########################

Overview
********

This sample application demonstrates basic usage of the WS2812 LED
strip driver, for controlling LED strips using WS2812, WS2812b,
SK6812, Everlight B1414 and compatible driver chips.

Requirements
************

.. _NeoPixel Ring 12 from AdaFruit: https://www.adafruit.com/product/1643
.. _74AHCT125: https://cdn-shop.adafruit.com/datasheets/74AHC125.pdf

- LED strip using WS2812 or compatible, such as the `NeoPixel Ring 12
  from AdaFruit`_.

- Note that 5V communications may require a level translator, such as the
  `74AHCT125`_.

- LED power strip supply. It's fine to power the LED strip off of your board's
  IO voltage level even if that's below 5V; the LEDs will simply be dimmer in
  this case.

Wiring
******

#. Ensure your Zephyr board, and the LED strip share a common ground.
#. Connect the LED strip control pin (either SPI MOSI or GPIO) from your board
   to the data input pin of the first WS2812 IC in the strip.
#. Power the LED strip at an I/O level compatible with the control pin signals.

Building and Running
********************

.. _blog post on WS2812 timing: https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/

This sample's source directory is :zephyr_file:`samples/drivers/led_ws2812/`.

To make sure the sample is set up properly for building, you must:

- select the correct WS2812 driver backend for your SoC. This currently should
  be :kconfig:option:`CONFIG_WS2812_STRIP_SPI` unless you are using an nRF51 SoC, in
  which case it will be :kconfig:option:`CONFIG_WS2812_STRIP_GPIO`.

- create a ``led-strip`` :ref:`devicetree alias <dt-alias-chosen>`, which
  refers to a node in your :ref:`devicetree <dt-guide>` with a
  ``worldsemi,ws2812-spi`` or ``worldsemi,ws2812-gpio`` compatible. The node
  must be properly configured for the driver backend (SPI or GPIO) and daisy
  chain length (number of WS2812 chips).

For example devicetree configurations for each compatible, see
:zephyr_file:`samples/drivers/led_ws2812/boards/nrf52dk_nrf52832.overlay` and
:zephyr_file:`samples/drivers/led_ws2812/boards/nrf51dk_nrf51422.overlay`.

Some boards are already supported out of the box; see the :file:`boards`
directory for this sample for details.

Then build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_ws2812
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

- `RGB LED strips: an overview <http://nut-bolt.nl/2012/rgb-led-strips/>`_
- `74AHCT125 datasheet
  <https://cdn-shop.adafruit.com/datasheets/74AHC125.pdf>`_
- An excellent `blog post on WS2812 timing`_.
