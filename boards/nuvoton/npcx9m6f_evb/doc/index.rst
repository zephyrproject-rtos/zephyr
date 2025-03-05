.. zephyr:board:: npcx9m6f_evb

Overview
********

The NPCX9M6F_EVB kit is a development platform to evaluate the
Nuvoton NPCX9 series microcontrollers. This board needs to be mated with
part number NPCX996F.

Hardware
********

- ARM Cortex-M4F Processor
- 256 KB RAM and 64 KB boot ROM
- ADC & GPIO headers
- UART0 and UART1
- FAN PWM interface
- Jtag interface
- Intel Modular Embedded Controller Card (MECC) headers

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Nuvoton to provide the schematic for this board.

System Clock
============

The NPCX9M6F MCU is configured to use the 90Mhz internal oscillator with the
on-chip PLL to generate a resulting EC clock rate of 15 MHz. See Processor clock
control register (chapter 4 in user manual)

Serial Port
===========

UART1 is configured for serial logs.


Programming and Debugging
*************************

This board comes with a Cortex ETM port which facilitates tracing and debugging
using a single physical connection. In addition, it comes with sockets for
JTAG-only sessions.

Flashing
========

If the correct IDC headers are installed, this board supports both J-TAG and
also the ChromiumOS servo.

To flash using Servo V2, μServo, or Servo V4 (CCD), see the
`Chromium EC Flashing Documentation`_ for more information.

To flash with J-TAG, install the drivers for your programmer, for example:
SEGGER J-link's drivers are at https://www.segger.com/downloads/jlink/

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: npcx9m6f_evb
   :maybe-skip-config:
   :goals: build flash

Debugging
=========

Use JTAG/SWD with a J-Link

References
**********
.. target-notes::

.. _Chromium EC Flashing Documentation:
   https://chromium.googlesource.com/chromiumos/platform/ec#Flashing-via-the-servo-debug-board
