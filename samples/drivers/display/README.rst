.. _display-sample:

Display Sample
##############

Overview
********

This sample will draw some basic rectangles onto the display.
The rectangle colors and positions are chosen so that you can check the
orientation of the LCD and correct RGB bit order. The rectangles are drawn
in clockwise order, from top left corner: red, green, blue, grey. The shade of
grey changes from black through to white. If the grey looks too green or red
at any point or the order of the corners is not as described above then the LCD
may be endian swapped.

Building and Running
********************

As this is a generic sample it should work with any display supported by Zephyr.

Below is an example on how to build for a :ref:`nrf52840dk_nrf52840` board with a
:ref:`adafruit_2_8_tft_touch_v2`.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: nrf52840dk_nrf52840
   :goals: build
   :shield: adafruit_2_8_tft_touch_v2
   :compact:

For testing purpose without the need of any hardware, the :ref:`native_posix`
board is also supported and can be built as follows;

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: native_posix
   :goals: build
   :compact:

List of Arduino-based display shields
*************************************

- :ref:`adafruit_2_8_tft_touch_v2`
- :ref:`ssd1306_128_shield`
- :ref:`st7789v_generic`
- :ref:`waveshare_e_paper_raw_panel_shield`
