.. _hexiwear-blinky-sample:

Blinky Application
##################

Overview
********

This Blinky example shows how to use the three different colors in the RGB
LED available in the Hexiwear module.

Requirements
************

A Hexiwear module

Building and Running
********************

This samples does not output anything to the console.  It can be built and
flashed to a board as follows:

.. code-block:: console

   $ cd samples/boards/hexiwear/blinky
   $ make 
   $ cp outdir/hexiwear_k64/zephyr.bin /daplink


After flashing the image to the board, the user LED on the board should start to
blink in a repeating color sequence (red, green, blue, white).
