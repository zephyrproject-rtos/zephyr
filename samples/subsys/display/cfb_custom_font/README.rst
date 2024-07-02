.. zephyr:code-sample:: cfb-custom-fonts
   :name: CFB Custom fonts
   :relevant-api: compact_framebuffer_subsystem

   Generate and use a custom font.

Overview
********
A simple example showing how to generate Compact Framebuffer (CFB)
subsystem font headers automatically at build time.

This example generates a font with font elements for 6 sided dice from
a PNG image, and then uses the generated header (``cfb_font_dice.h``)
to show the font elements on the display of a supported board.

The source code for this sample application can be found at:
:zephyr_file:`samples/subsys/display/cfb_custom_font`.

Building and Running
********************

There are different configuration files in the cfb_custom_font
directory:

- :file:`prj.conf`
  Generic config file, normally you should use this.

Example building for the reel_board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/cfb_custom_font
   :host-os: unix
   :board: reel_board
   :goals: flash
   :compact:
