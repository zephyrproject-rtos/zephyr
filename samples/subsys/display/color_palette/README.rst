.. zephyr:code-sample:: color-palette
   :name: Color Palette Display
   :relevant-api: display_interface

   Display vertical color bands using indexed color palette.

Overview
********

This sample demonstrates the use of indexed color formats (such as I_4) with
a color palette. The sample divides the display into vertical bands, with each
band filled using a different palette color index.

Indexed color formats use a lookup table (color palette) to map small index
values to full color values. This allows for significant memory savings when
the number of colors needed is limited. For example, the I_4 format uses only
4 bits per pixel (16 colors), compared to 32 bits for ARGB8888.

The sample:

* Reads the color palette from the display capabilities
* Counts the number of valid (non-null) palette entries
* Divides the display width evenly into vertical bands
* Fills each band with the corresponding palette color index
* Displays the result on screen

Requirements
************

This sample requires a display that supports indexed color formats and has a
color palette defined in devicetree using the ``zephyr,panel-color-palette``
compatible.

Boards with integrated displays that support color palettes include:

- :zephyr:board:`inkplate_6color`

For simulation, the sample can also run on :zephyr:board:`native_sim <native_sim>`
with the SDL display driver configured for I_4 pixel format.

Building and Running
********************

Example building for :zephyr:board:`native_sim <native_sim>`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/color_palette
   :board: native_sim/native/64
   :goals: build run

This will open an SDL window showing 16 vertical color bands, one for each
color in the palette (ranging from black through various colors to white).

Example building for :zephyr:board:`inkplate_6color`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/color_palette
   :board: inkplate_6color/esp32/procpu
   :goals: build flash
