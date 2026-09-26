.. zephyr:board:: npck6m9k_evb

Overview
********

The NPCK6M9K_EVB kit is a development platform to evaluate the
Nuvoton NPCK6 series microcontrollers. This board is designed to provide
a range of peripherals and interfaces for development and testing. It needs
to be mated with part number NPCK6M9K.

Hardware
********

- ARM Cortex-M33F Processor
- 640 KB RAM and 64 KB boot ROM
- GPIO headers
- UART0 and UART1
- JTAG interface

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The NPCK6M9K MCU is configured to use the 90 MHz internal oscillator with the
on-chip PLL to generate a resulting EC clock rate of 15 MHz. See Processor clock
control register (chapter 4 in user manual)

Serial Port
===========

UART1 is configured for serial logs.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

This board comes with a Cortex ETM port which facilitates tracing and debugging
using a single physical connection.  In addition, it comes with sockets for
JTAG only sessions.

Flashing
========

Build the application as usual for the ``npck6m9k_evb`` board.

Debugging
=========

Use JTAG/SWD with a J-Link.

References
**********

.. target-notes::
