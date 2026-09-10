.. zephyr:code-sample:: debugmon
   :name: Debug Monitor

   Configure the Debug Monitor feature on a Cortex-M processor.

Overview
********

The Debug Monitor sample shows a basic configuration of debug monitor feature.


The source code shows how to:

#. Configure registers to enable degugging in debug monitor mode
#. Specify custom interrupt to be executed when entering a breakpoint

.. _debugmon-sample-requirements:

Requirements
************

Your board must:

#. Support Debug Monitor feature (available on Cortex-M processors with the exception of Cortex-M0)
#. Have an LED connected via a GPIO pin (these are called "User LEDs" on many of
   Zephyr's :ref:`boards`).
#. Have the LED configured using the ``led0`` devicetree alias.

Building and Running
********************

Build and flash Debug Monitor as follows, changing ``reel_board`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/debug/debugmon
   :board: reel_board
   :goals: build flash
   :compact:

After flashing the board enters a breakpoint and executes debug monitor exception code.
The LED starts to blink, indicating that even though the processor spins in debug monitor
interrupt, other higher priority interrupts continue to execute.
