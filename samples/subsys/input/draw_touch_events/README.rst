.. zephyr:code-sample:: draw_touch_events
   :name: Draw touch events
   :relevant-api: input_events display_interface

   Visualize touch events on a display.

Overview
********
This sample will draw a small plus in the last touched coordinates, that way you can check
if the touch screen works for a board, examine its parameters such as inverted/swapped axes.

Building and Running
********************
This is a generic sample and it should work with any board with both a display controller
and a touch controller supported by Zephyr (provided ``/chosen node`` ``zephyr,display`` property
and ``touch`` alias are set in devicetree).
Below is an example on how to build the sample for :zephyr:board:`stm32f746g_disco`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/input/draw_touch_events
   :board: stm32f746g_disco
   :goals: build
   :compact:

For testing purposes without the need of any hardware, the :ref:`native_sim <native_sim>`
board is also supported and can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/input/draw_touch_events
   :board: native_sim
   :goals: build
   :compact:
