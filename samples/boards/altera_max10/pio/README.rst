.. _altera_max10_pio-sample:

Altera Nios-II PIO sample
###############################

Overview
********
This sample application demonstrates the usage of Nios-II
Parallel Input/Output (PIO) soft IP by controlling the LEDs
on an Altera MAX10 board.

Requirements
************

This application uses an Altera MAX10 development board for the demo.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/boards/altera_max10/pio
   :board: altera_max10
   :goals: build
   :compact:

Sample Output
=============

nios2 core output
-----------------

.. code-block:: console

  Turning off all LEDs
  Turn On LED[0]
  Turn On LED[1]
  Turn On LED[2]
  Turn On LED[3]
  Turning on all LEDs
