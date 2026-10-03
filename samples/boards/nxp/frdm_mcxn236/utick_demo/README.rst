UTICK Demo for FRDM-MCXN236
###########################

Overview
********

This sample demonstrates the MCX N236 Micro-Tick Timer (UTICK) in repeating
interrupt mode. It selects the internal 1 MHz FRO as the UTICK clock and
programs a one-second interval. On every UTICK interrupt, the sample toggles
the board's red LED and prints the interrupt count to the console.

The sample uses the MCUX SDK UTICK HAL. Zephyr's ``IRQ_CONNECT`` routes
``UTICK0_IRQn`` to the SDK's ``UTICK0_DriverIRQHandler``, which clears the
pending UTICK interrupt and invokes the callback registered with
``UTICK_SetTick``. The callback only increments a counter; LED and console
operations run in the main thread.

Building and Running
********************

Build the sample for the FRDM-MCXN236 board:

.. code-block:: console

   west build -b frdm_mcxn236/mcxn236 samples/boards/nxp/frdm_mcxn236/utick_demo

Flash it to the connected board:

.. code-block:: console

   west flash

Open a serial terminal at 115200 baud. The red LED should toggle once per
second, and the console should show output similar to:

.. code-block:: console

   MCXN236 UTICK demo: 1 MHz clock, 1 second repeat interval
   UTICK interrupt 1
   UTICK interrupt 2
   UTICK interrupt 3

Dependencies
************

The sample compiles the MCUX SDK ``fsl_utick.c`` source directly because the
Zephyr tree used for this demo does not provide a UTICK driver Kconfig option.
