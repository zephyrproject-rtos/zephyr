.. zephyr:board:: scobc_v1

Overview
********

The `Space Cubics`_ OBC Module V1 (SC-OBC Module V1) is an onboard computer
for space missions that require reliable real-time control and edge
processing. It is built around an AMD Versal Adaptive SoC and a Microchip
IGLOO2 FPGA, and is designed for harsh environments such as Earth orbit and
lunar missions.

On the Versal side, the SC-OBC V1 leverages the device’s heterogeneous
architecture, combining general-purpose processors, programmable logic, and a
vector processor, to run on-orbit workloads such as object detection, image
compression/segmentation, and high-speed signal processing, while keeping
flight-critical tasks under Zephyr RTOS’s predictable scheduling model.

On the IGLOO2 side, the SC-OBC V1 uses the device as the board’s safety
processor, supervising the main system, providing independent watchdog and
fault-management paths, and coordinating safe-mode transitions to improve fault
tolerance.


Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

This board provides a 50 MHz reference oscillator for clock generation. The RPU
(Cortex-R5F) runs at 600 MHz by default on this board.

In Zephyr, the "system clock" is the kernel’s tick base. By default it’s 100 Hz
for periodic-tick builds and 10 kHz (10000 Hz) for tickless builds.  See
:kconfig:option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` for more details.


Serial Port
===========

The SC-OBC Module V1 has multiple UART ports. The primary ports are routed to
the FT4232H on the evaluation board, so when you connect the board to your PC
with a USB Type-C cable, your development machine will enumerate them.

- Port A is used for JTAG.
- Port B connects to UART0 via LPD MIO pin 0 (RXD) and pin 1 (TXD) on Bank 502.
- Port C connects to UART1 via LPD MIO pin 4 (TXD) and pin 5 (RXD) on Bank 502.
  For Zephyr on Versal RPU, UART1 is selected as the default console in
  :file:`scobc_v1_versal_rpu.dts`. On a Linux host, the console typically
  appears as something like ``/dev/ttyUSB2`` (device index may vary).
- Port D connects to IGLOO2.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Here is an example for building and flashing the :zephyr:code-sample:`hello_world` application
for the board:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: scobc_v1
   :goals: flash
   :flash-args: --pdi /path/to/your.pdi

After flashing, you should see message similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0 ***
   Hello World! scobc_v1/versal_rpu


References
**********

.. target-notes::

.. _Space Cubics:
   https://spacecubics.com/
