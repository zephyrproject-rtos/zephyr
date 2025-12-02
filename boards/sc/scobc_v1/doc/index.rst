.. zephyr:board:: scobc_v1

Overview
********

`Space Cubics`_ OBC Module V1 (SC-OBC V1) is an onboard computer for space
missions that need dependable real-time control with modern edge computing. It
is built around an AMD Versal Adaptive SoC and a Microchip IGLOO2 FPGA, and is
intended for harsh environments found in Earth-orbit and lunar missions.

On the Versal side, SC-OBC V1 leverages the device’s heterogeneous architecture,
general-purpose processors alongside reconfigurable logic, to support on-orbit
workloads such as object detection, image compression/segmentation, and anomaly
detection, while keeping flight-critical tasks under Zephyr RTOS’s predictable
scheduling model.

On the IGLOO2 side, SC-OBC V1 uses the device as the board’s safety processor
(per design): supervising the main system, providing independent watchdog and
fault-management paths, and coordinating safe-mode transitions to improve fault
tolerance.


Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Other hardware features are not currently supported by the port.

System Clock
============

This board provides a 50 MHz reference oscillator for clock generation. The RPU
(Cortex-R5F) runs at 600 MHz by default on this board.

In Zephyr, the "system clock" is the kernel’s tick base. By default it’s 100 Hz
for periodic-tick builds and 10 kHz (10000 Hz) for tickless builds.  See
:kconfig:option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` for more details.


Serial Port
===========

The default Zephyr console is routed to the Versal LPD MIO pins 0 (RXD) and 1
(TXD) and, on the evaluation board, into the USB-to-UART bridge (FTDI) on Port B.
On a Linux host this typically appears as `/dev/ttyUSB1` for the
console.


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
