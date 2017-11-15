.. _led_ws2812_sample:

WS2812 Sample Application
#########################

Overview
********

This sample application demonstrates basic usage of the WS2812 LED
strip driver, for controlling LED strips using WS2812, WS2812b,
SK6812, and compatible driver chips.

Requirements
************

.. _NeoPixel Ring 12 from AdaFruit: https://www.adafruit.com/product/1643
.. _74AHCT125: https://cdn-shop.adafruit.com/datasheets/74AHC125.pdf

- LED strip using WS2812 or compatible, such as the `NeoPixel Ring 12
  from AdaFruit`_.

- Zephyr board with SPI master driver. SPI communications must use 5V
  signaling, which may require a level translator, such as the
  `74AHCT125`_.

- 5V power supply.

Wiring
******

#. Ensure your Zephyr board, the 5V power supply, and the LED strip
   share a common ground.
#. Connect the MOSI pin of your board's SPI master to the data input
   pin of the first WS2812 IC in the strip.
#. Connect the 5V power supply pin to the 5V input of the LED strip.

Building and Running
********************

.. _blog post on WS2812 timing: https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/

The sample application is located at ``samples/drivers/led_ws2812/``
in the Zephyr source tree.

Configure For Your LED Strip
----------------------------

The first thing you need to do is make sure that the driver is
configured to match the particular LED chips you're using. For
example, some chips have a "green, red, blue" color ordering on their
data pin, while others have "red, green, blue, white". Check your
chip's datasheet for your configuration (though given the number of
competing implementations and knock-offs, some trial and error may be
necessary).

To make sure the driver is set up properly for your chips, check the
values of the following configuration options:

- :option:`CONFIG_WS2812_RED_ORDER`
- :option:`CONFIG_WS2812_GRN_ORDER`
- :option:`CONFIG_WS2812_BLU_ORDER`
- :option:`CONFIG_WS2812_WHT_ORDER` (available if your chips have a white
  channel by setting :option:`CONFIG_WS2812_HAS_WHITE_CHANNEL` to ``y``).

Refer to their help strings for details.

Configure For Your Board
------------------------

Now check if your board is already supported, by looking for a file
named ``boards/YOUR_BOARD_NAME.conf`` in the application directory.

If your board isn't supported yet, you'll need to configure the
application as follows.

#. Configure your board's SPI master in a configuration file under
   ``boards/`` in the sample directory.

   To provide additional configuration for some particular board,
   create a ``boards/YOUR_BOARD_NAME.conf`` file in the application
   directory. It will be merged into the application configuration.

   In this file, you must ensure that the SPI peripheral you want to
   use for this demo is enabled, and that its name is "ws2812_spi".
   See ``boards/96b_carbon.conf`` for an example, and refer to your
   board's configuration options to set up your desired SPI master.

#. Set the number of LEDs in your strip in the application sources.
   This is determined by the macro ``STRIP_NUM_LEDS`` in the file
   ``src/main.c``. The value in the file was chosen to work with the
   AdaFruit NeoPixel Ring 12.

#. Make sure that the SPI timing will generate pulses of the
   appropriate widths. This involves setting the configuration options
   :option:`CONFIG_WS2812_STRIP_SPI_BAUD_RATE`,
   :option:`CONFIG_WS2812_STRIP_ONE_FRAME`, and
   :option:`CONFIG_WS2812_STRIP_ZERO_FRAME`. Refer to the Kconfig help for
   those options, as well as this `blog post on WS2812 timing`_, for
   more details.

Then build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_ws2812
   :board: <board>
   :goals: flash
   :compact:

Refer to your :ref:`board's documentation <boards>` for alternative
flash instructions if your board doesn't support the ``flash`` target.

When you connect to your board's serial console, you should see the
following output:

.. code-block:: none

   ***** BOOTING ZEPHYR OS v1.9.99 *****
   [general] [INF] main: Found SPI device ws2812_spi
   [general] [INF] main: Found LED strip device ws2812_strip
   [general] [INF] main: Displaying pattern on strip

References
**********

- `RGB LED strips: an overview <http://nut-bolt.nl/2012/rgb-led-strips/>`_
- `74AHCT125 datasheet
  <https://cdn-shop.adafruit.com/datasheets/74AHC125.pdf>`_
- An excellent `blog post on WS2812 timing`_.
