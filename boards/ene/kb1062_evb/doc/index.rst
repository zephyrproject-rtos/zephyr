.. zephyr:board:: kb1062_evb

Overview
********

The KB1062_EVB kit is a development platform to evaluate the
ENE KB106X series microcontrollers. This board needs to be mated with
part number KB1062.

Hardware
********

- ARM Cortex-M3 Processor
- 256KB Flash and 64KB RAM
- ADC & GPIO headers
- SER serial port
- FAN PWM interface
- ENE Debug interface

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The KB106x MCU is configured to use the 48Mhz internal oscillator with the
on-chip DPLL to generate a resulting EC clock rate of 48MHz/24MHz
See Processor clock control register (refer 5.1 General Configuration)

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

If the correct headers are installed, this board supports SWD Debug Interface.

To flash with SWD, install the drivers for your programmer, for example:
SEGGER J-link's drivers are at https://www.segger.com/downloads/jlink/

Debugging
=========

Use SWD with a J-Link

References
==========

.. target-notes::
