.. _zscilib-mtx-mult-sample:

Matrix Multiply
###############

Overview
********

This sample application shows how to multiply two matrices together using
zscilib. The results will be sent to the console for verification.

Requirements
************

Depending on the device you are testing with, you may need to enable or
disable the ``CONFIG_STDOUT_CONSOLE`` flag.

It should be set to ``y`` when using the emulator, and to ``n`` when running on
real hardware.

Building and Running
********************

To run this sample in qemu, run the following command which will compile the
application for the ``mps2_an521``, run it on the emulator, and output
the result to the console:

.. zephyr-app-commands::
   :zephyr-app: samples/zscilib/mtx_mult
   :board: mps2_an521
   :goals: run

To run the application on real HW, typically outputting the results to the
serial port, you can try a variant of the following, adjusting ``<board>``
as appropriate:

.. zephyr-app-commands::
   :zephyr-app: samples/zscilib/mtx_mult
   :board: <board>
   :goals: flash
   :compact:
