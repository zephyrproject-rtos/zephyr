Blinky Application
##################

Overview
********

The Blinky example shows how to configure GPIO pins as outputs which can also be
used to drive LEDs on the hardware usually delivered as "User LEDs" on many of
the supported boards in Zephyr.

Requirements
************

The demo assumes that an LED is connected to one of GPIO lines. The
sample code is configured to work on boards with user defined buttons and that
have defined the LED0\_* variables in :file:`board.h`.

The :file:`board.h` must define the following variables:

- LED0_GPIO_PORT
- LED0_GPIO_PIN


Building and Running
********************

This samples does not output anything to the console.  It can be built and
flashed to a board as follows:

.. code-block:: console

   $ cd samples/basic/blinky
   $ make BOARD=arduino_101
   $ make BOARD=arduino_101 flash

After flashing the image to the board, the user LED on the board should start to
blink.
