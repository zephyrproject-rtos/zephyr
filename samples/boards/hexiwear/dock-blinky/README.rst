.. _hexiwear-docking-station-sample:

Docking Station Blinky Application
##################################

Overview
********

This example shows the LED/input lines on the Hexiwear docking station

Requirements
************

A Hexiwear module and the docking station.

The three right-most DIP switches (labelled LED1..3) need to be in the UP
position (LED position).  When these are in the down position, the
corresponding pushbuttons are usable when the GPIO is set for GPIO_DIR_IN.


Building and Running
********************

This samples does not output anything to the console.  It can be built and
flashed to a board as follows:

.. code-block:: console

   $ cd samples/boards/hexiwear/dock-blinky
   $ make
   $ cp outdir/hexiwear_k64/zephyr.bin /daplink


After flashing the image to the board, the three LED's should be cycling.

