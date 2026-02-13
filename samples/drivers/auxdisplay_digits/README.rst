.. zephyr:code-sample:: auxdisplay_digits
   :name: Auxiliary digits display
   :relevant-api: auxdisplay_interface

   Output increasing numbers to an auxiliary display.

Overview
********

This sample demonstrates the use of the
:ref:`auxiliary display driver <auxdisplay_api>` for digit-based displays, such
as 7-segment displays.

Building and Running
********************

Note that this sample requires a board with a 7-segment display setup. You can
build your own setup by fly-wiring a 7-segment display to any board you have.

A sample overlay is provided for the ``native_sim`` target. See the overlay file
:zephyr_file:`samples/drivers/auxdisplay_digits/boards/native_sim.overlay` for a
demonstration.

A sample overlay is provided for the ``frdm_mcxc444`` target. See the overlay file
:zephyr_file:`samples/drivers/auxdisplay_digits/boards/frdm_mcxc444.overlay` for a
demonstration.

A sample overlay is provided for the ``frdm_mcxa266`` target. See the overlay file
:zephyr_file:`samples/drivers/auxdisplay_digits/boards/frdm_mcxa266.overlay` for a
demonstration.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/auxdisplay_digits
   :host-os: unix
   :board: native_sim
   :goals: build
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/auxdisplay_digits
   :host-os: unix
   :board: frdm_mcxc444
   :goals: build
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/auxdisplay_digits
   :host-os: unix
   :board: frdm_mcxa266
   :goals: build
   :compact:

If successful, the display first lights up all segments (e.g., 8.8.8. on a
3-digit display), blinks once, sequentially lights up each digit from left to
right, and then counts up from 0 to the maximum number that can be displayed.

For frdm_mcxc444 and frdm_mcxa266, they will have 4 digits / 6 digits display,
with allowed '.' and ':', in its given position
