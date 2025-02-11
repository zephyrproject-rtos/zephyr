.. zephyr:code-sample:: lp3943
   :name: LP3943 RGBW LED
   :relevant-api: led_interface

   Control up to 16 RGBW LEDs connected to an LP3943 driver chip.

Overview
********

This sample controls 16 LEDs connected to an LP3943 driver, in
a continuous pattern of turning them on one at a time (at a one
second interval) until they're all on, and then turning them off in
reverse order.

Requirements
************

The :ref:`96b_neonkey` board has an LP3943 driver and 16 LEDs on board,
so we'll use this board for our example.

Building and Running
********************

Build the application for the :ref:`96b_neonkey` board, which has an
LP3943 driver included.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_lp3943
   :board: 96b_neonkey
   :goals: build
   :compact:

For flashing the application, refer to the Flashing section of the
:ref:`96b_neonkey` board documentation.

When you connect to the board's serial console, you should see the
following output in addition to the LED pattern:

.. code-block:: none

   ***** BOOTING ZEPHYR OS v1.11.99 *****
   [general] [INF] main: Found LED device LP3943
   [general] [INF] main: Displaying the pattern

References
**********

- LP3943: http://www.ti.com/lit/ds/snvs256d/snvs256d.pdf
