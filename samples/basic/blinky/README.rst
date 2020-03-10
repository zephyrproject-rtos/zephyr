.. _blinky-sample:

Blinky sample
#############

Overview
********

Blinky is a simple application which blinks an LED forever.

The source code shows how to configure GPIO pins as outputs, then turn them on
and off.

.. _blinky-sample-requirements:

Requirements
************

The board must have an LED connected via a GPIO pin. These are called "User
LEDs" on many of Zephyr's :ref:`boards`. The LED must be configured using the
``led0`` :ref:`dt-guide` alias in the :ref:`BOARD.dts file
<devicetree-in-out-files>`.

You will see this error if you try to build Blinky for an unsupported board:

.. code-block:: none

   Unsupported board: led0 devicetree alias is not defined

Building and Running
********************

Build and flash Blinky as follows, changing ``reel_board`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: reel_board
   :goals: build flash
   :compact:

After flashing, the LED starts to blink. Blinky does not print to the console.
