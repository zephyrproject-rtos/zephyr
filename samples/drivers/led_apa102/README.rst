.. _led_apa102_sample:

APA102 Sample Application
#########################

Overview
********

This sample application demonstrates basic usage of the APA102 LED
strip driver, for controlling LED strips using APA102, Adafruit DotStar,
and compatible driver chips.

Requirements
************

.. _Dotstar product from AdaFruit: https://www.adafruit.com/category/885
.. _74AHCT125: https://cdn-shop.adafruit.com/datasheets/74AHC125.pdf

- LED strip using APA102 or compatible, such as the any `Dotstar product
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
   pin of the first APA102 IC in the strip.
#. Connect the SCLK pin of your board's SPI master to the clock input
   pin of the first APA102 IC in the strip.
#. Connect the 5V power supply pin to the 5V input of the LED strip.

Building and Running
********************

The sample application is located at ``samples/drivers/led_apa102/``
in the Zephyr source tree.

Configure For Your Board
========================

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
   use for this demo is enabled. See ``boards/nucleo_l432kc.conf`` for
   an example.

   #. Configure your board's dts overlay. See ``nucleo_l432kc.overlay``
   for an example.

#. Set the number of LEDs in your strip in the application sources.
   This is determined by the macro ``STRIP_NUM_LEDS`` in the file
   ``src/main.c``.

Then build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_apa102
   :board: <board>
   :goals: flash
   :compact:

Refer to your :ref:`board's documentation <boards>` for alternative
flash instructions if your board doesn't support the ``flash`` target.

When you connect to your board's serial console, you should see the
following output:

.. code-block:: none

   ***** BOOTING ZEPHYR OS zephyr-v1.13.XX *****
   [general] [INF] main: Found LED strip device APA102
