.. zephyr:code-sample:: auxdisplay_14seg
   :name: Auxiliary digits display
   :relevant-api: auxdisplay_interface

   Output all the supported characters and indicators to an auxiliary 14 segment display.

Overview
********

This sample demonstrates the use of the
:ref:`auxiliary display driver <auxdisplay_api>` for digit/alpha-based
displays, such as 14-segment displays with points and bars.

Building and Running
********************

Note that this sample requires a board with a 14-segment display setup. You can
build your own setup by fly-wiring a 14-segment display to any board you have.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/auxdisplay_14seg
   :board: stm32l476g_disco
   :shield: st_gh08172t
   :goals: build
   :compact:

If successful, the display first lights up all segments (e.g., 8:8:8:8:88=) on
a 6-digit display, shows some strings, lights bars as binary digits and
percentage.
