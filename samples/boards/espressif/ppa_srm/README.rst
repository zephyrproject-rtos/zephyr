.. zephyr:code-sample:: esp32-ppa-srm
   :name: Pixel processing accelerator

   Scale, rotate and mirror an image with the pixel processing accelerator.

Overview
********

This sample drives the pixel processing accelerator directly, without a
graphics library, so its scale, rotate and mirror operations can be checked
on screen.

A source image of four coloured quadrants with a white diagonal is generated
in memory and drawn on the left of the display. It is then transformed
repeatedly, one step every three seconds, and each result is drawn on the
right: identity, two times scale, rotation by 90 and 180 degrees, and a
horizontal mirror.

Requirements
************

A board with a pixel processing accelerator and a display, and enough
external RAM to hold the source and destination images.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ppa_srm
   :board: esp32p4x_function_ev_board/esp32p4/hpcore
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   PPA SRM: source is red/green/blue/yellow quadrants + white diagonal
   step 0: 1x identity -> out 128x128
   step 1: 2x scale -> out 256x256
   step 2: 1x rotate 90 -> out 128x128
   step 3: 1x rotate 180 -> out 128x128
   step 4: 1x mirror-x -> out 128x128
