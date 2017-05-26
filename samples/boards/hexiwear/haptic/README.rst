.. _hexiwear-haptic-sample:

Haptic Feedback
###############

Overview
********

This program cycles the LED through the red, green, blue and white colors,
and then triggers the haptic feedback motor

Use the motor via CONFIG_HAPTIC_FEEDBACK=y in the prj.conf file, and then
use the GPIO device via HAPTIC_MOTOR_NAME and HAPTIC_MOTOR_PIN.

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


