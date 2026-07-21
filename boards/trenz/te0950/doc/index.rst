.. zephyr:board:: te0950

Overview
********

The `Trenz Electronic TE0950-02`_ is a powerful adaptive SoC evaluation board, equipped with an AMD
Versal™ AI (Edge) device. Furthermore, the board is equipped with up to 8GB DDR4 SDRAM, 128 MByte
SPI Flash and an eMMC for configuration and data storage as well as powerful switching power
supplies for all required voltages. Inputs and outputs are provided by robust, flexible and
cost-effective high-speed connectors.


Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

This board provides a 33.333 MHz reference oscillator for clock generation. The RPU (Cortex-R5F)
runs at 600 MHz by default on this board.

In Zephyr, the "system clock" is the kernel’s tick base. By default it’s 100 Hz
for periodic-tick builds and 10 kHz (10000 Hz) for tickless builds.  See
:kconfig:option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` for more details.


Serial Port
===========

The TE0950-02 has a UART port via the FTDI FT2232H USB interface. The same USB port can also be used
for JTAG. Please refer to the board schematics for more details.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Here is an example for building and flashing the :zephyr:code-sample:`hello_world` application
for the board:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: te0950
   :goals: flash
   :flash-args: --pdi /path/to/your.pdi

After flashing, you should see message similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0 ***
   Hello World! te0950/versal_rpu


References
**********

.. target-notes::

.. _Trenz Electronic TE0950-02:
   http://trenz.org/te0950-info
