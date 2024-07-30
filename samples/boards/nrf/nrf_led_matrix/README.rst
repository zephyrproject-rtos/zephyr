.. _nrf_led_matrix_sample:

nRF LED matrix sample
#####################

Overview
********

This is a simple application intended to present the nRF LED matrix display
driver in action and to serve as a test ensuring that this driver is buildable
for both the :ref:`bbc_microbit_v2` and :ref:`bbc_microbit` boards.

Requirements
************

The sample has been tested on the bbc_microbit_v2 and bbc_microbit boards,
but it could be ported to any board with an nRF SoC and the proper number
of GPIOs available for driving a LED matrix. To do it, one needs to add an
overlay file with the corresponding ``"nordic,nrf-led-matrix"`` compatible
node.

Building and Running
********************

The code can be found in :zephyr_file:`samples/boards/nrf/nrf_led_matrix`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf/nrf_led_matrix
   :board: bbc_microbit_v2
   :goals: build flash
   :compact:
