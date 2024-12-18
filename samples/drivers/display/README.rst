.. zephyr:code-sample:: display
   :name: Display
   :relevant-api: display_interface

   Draw basic rectangles on a display device.

Overview
********

This sample will draw some basic rectangles onto the display.
The rectangle colors and positions are chosen so that you can check the
orientation of the LCD and correct RGB bit order. The rectangles are drawn
in clockwise order, from top left corner: red, green, blue, grey. The shade of
grey changes from black through to white. If the grey looks too green or red
at any point or the order of the corners is not as described above then the LCD
may be endian swapped.

On displays with the :c:enumerator:`SCREEN_INFO_X_ALIGNMENT_WIDTH` capability,
such as those using the :dtcompatible:`sharp,ls0xx` driver, it is only possible
to draw full lines at a time. On these displays, the rectangles described above
will be replaced with bars that take up the entire width of the display. Only
the green and grey bar will be visible.

On monochrome displays, the rectangles (or bars) will all be some shade of grey.

On displays with 1 bit per pixel, the greyscale animation of the bottom
rectangle (or bar) will appear as flickering between black and white.

Building and Running
********************

As this is a generic sample it should work with any display supported by Zephyr.

Below is an example on how to build for a :ref:`nrf52840dk_nrf52840` board with a
:ref:`adafruit_2_8_tft_touch_v2`.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: nrf52840dk/nrf52840
   :goals: build
   :shield: adafruit_2_8_tft_touch_v2
   :compact:

For testing purpose without the need of any hardware, the :ref:`native_sim <native_sim>`
board is also supported and can be built as follows;

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: native_sim
   :goals: build
   :compact:

List of Arduino-based display shields
*************************************

- :ref:`adafruit_2_8_tft_touch_v2`
- :ref:`ssd1306_128_shield`
- :ref:`st7789v_generic`
- :ref:`waveshare_epaper`
