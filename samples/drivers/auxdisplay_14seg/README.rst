.. zephyr:code-sample:: auxdisplay_14seg
   :name: Auxiliary digits display
   :relevant-api: auxdisplay_interface

   Output increasing numbers to an auxiliary display.

Overview
********

This sample demonstrates the use of the
:ref:`auxiliary display driver <auxdisplay_api>` for digit-based displays, such
as 14-segment displays with points and bars.

Building and Running
********************

Note that this sample requires a board with a 14-segment display setup. You can
build your own setup by fly-wiring a 14-segment display to any board you have.

A sample overlay is provided for the ``stm32l476g_disco`` target. See the overlay file
:zephyr_file:`samples/drivers/auxdisplay_14seg/boards/stm32l476g_disco.overlay` for a
demonstration.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/auxdisplay_14seg
   :host-os: unix
   :board: stm32l476g_disco
   :goals: build
   :compact:

If successful, the display first lights up all segments (e.g., 8:8:8:8:88=) on a
6-digit display, shows some strings, lights bars as binary digits and percentage.

File and folders
****************

ORI boards/st/stm32l476g_disco/stm32l476g_disco.dts

MOD drivers/auxdisplay/CMakeLists.txt
MOD drivers/auxdisplay/Kconfig
ADD drivers/auxdisplay/Kconfig.gh08172t
ADD drivers/auxdisplay/auxdisplay_gh08172t.c

ADD dts/bindings/auxdisplay/st,gh08172t.yaml

ADD samples/drivers/auxdisplay_14seg/boards/stm32l476g_disco.overlay
ADD samples/drivers/auxdisplay_14seg/src/main.c
ADD samples/drivers/auxdisplay_14seg/CMakeLists.txt
ADD samples/drivers/auxdisplay_14seg/prj.conf
ADD samples/drivers/auxdisplay_14seg/README.rst
ADD samples/drivers/auxdisplay_14seg/tests.yaml

Open points
***********

- clarify how to split this driver in a common 14seg LCD interface and a specific GH08172T driver
- implement auxdisplay_display_on and off
- implement auxdisplay_display_position_set and get
- snow demo with segments
