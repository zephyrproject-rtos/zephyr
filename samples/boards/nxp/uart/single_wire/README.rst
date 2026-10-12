.. zephyr:code-sample:: uart-nxp-single-wire
   :name: NXP single-wire UART
   :relevant-api: uart_interface

   Use the single-wire/half-duplex mode of NXP LPUART peripherals.

Overview
********

This sample demonstrates polling communication between two NXP LPUART
peripherals configured for single-wire operation. Every two seconds, LPUART0
sends ``a`` to LPUART1, then LPUART1 sends ``b`` to LPUART0.

The sample uses both UART peripherals on the FRDM-MCXW72. Consequently, it
disables the UART console and does not produce console output.

Hardware Setup
**************

On the FRDM-MCXW72, connect the following UART transmit pins with one wire:

* LPUART0 TX: PTA17 (Arduino D1)
* LPUART1 TX: PTC3 (Arduino A4)

Both pins act as bidirectional data pins while single-wire mode is enabled.

Building and Running
********************

Build and flash the sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/uart/single_wire
   :board: frdm_mcxw72
   :goals: build flash
   :compact:

Use a logic analyzer or oscilloscope on the connected pins to observe ``a``
and ``b`` at 115200 baud every two seconds.
