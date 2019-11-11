.. _st7789v-sample:

ST7789V Display driver
######################

Overview
********
This sample will draw some basic rectangles onto the display.
The rectangle colors and positions are chosen so that you can check the
orientation of the LCD and correct RGB bit order. The rectangles are drawn
in clockwise order, from top left corner: Red, Green, Blue, grey. The shade of
grey changes from black through to white. (if the grey looks too green or red
at any point then the LCD may be endian swapped).

Note: The display driver rotates the display so that the 'natural' LCD
orientation is effectively 270 degrees clockwise of the default display
controller orientation.

Building and Running
********************

The sample uses the :ref:`st7789v_generic` and the pin assignments on a
:ref:`nrf52_pca10040` are as follows:

+-------------------+-------------+
| | NRF52 PCA10040  | | LCD module|
| | Pin             | | signal    |
+===================+=============+
| P1.15 (D13)       | SPI_SCK     |
+-------------------+-------------+
| P1.14 (D12)       | SPI_MISO    |
+-------------------+-------------+
| P1.13 (D11)       | SPI_MOSI    |
+-------------------+-------------+
| P1.12 (D10)       | CS          |
+-------------------+-------------+
| P1.11 (D9)        | DATA/CMD    |
+-------------------+-------------+
| P1.10 (D8)        | RESET       |
+-------------------+-------------+

You might need to alter these according to your specific board/LCD configuration.

For :ref:`nrf52_pca10040`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/display/st7789v
   :board: nrf52_pca10040
   :shield: st7789v_generic
   :goals: build
   :compact:

See :ref:`nrf52_pca10040` on how to flash the build.


References
**********

- `ST7789V datasheet`_

.. _Manufacturer site: https://www.sitronix.com.tw/en/product/Driver/mobile_display.html
.. _ST7789V datasheet: https://www.crystalfontz.com/controllers/Sitronix/ST7789V/
