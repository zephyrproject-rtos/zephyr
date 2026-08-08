.. _heltec_t114_v2_display:

Heltec T114 v2 display shield
#############################

Overview
********

The 1.14-inch ST7789V TFT is an optional module for the Heltec T114 v2.
The base board keeps its power and backlight disabled. Select this shield to
power the module when building a display application.

Programming
***********

Add ``--shield heltec_t114_v2_display`` to the build command. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: heltec_t114_v2/nrf52840/uf2
   :shield: heltec_t114_v2_display
   :goals: build

Landscape orientation
*********************

To use a 240 by 135 pixel landscape orientation, add the following snippet to
the application overlay and build with the same shield:

.. code-block:: dts

   &tft_display {
           width = <240>;
           height = <135>;
           x-offset = <40>;
           y-offset = <53>;
           mdac = <0x60>;
   };
