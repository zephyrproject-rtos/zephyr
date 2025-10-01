.. zephyr:board:: mcxw23_evk

Overview
********

The MCXW23 EVK base board, combined with a MCXW23 Radio Daughter Module (RDM),
is an evaluation environment supporting NXP’s MCXW23 wireless processor.
The EVK supports IoT applications and is intended for prototyping, demos,
software development and measurements (power consumption and RF).
The MCXW23 EVK constitutes multiple debugging mechanisms, therefore aiding
easier application development.

Hardware
********

- MCXW23 Arm® Cortex®-M33 microcontroller running at up to 32 MHz
- 1 MB flash and 128 KB SRAM on-chip
- 40HVQFN package
- On-board MCU-Link debugger with CMSIS-DAP
- LED
- Reset, ISP, wake, and user buttons for easy testing of software functionality
- NXP FXLS8974CF accelerometer
- NXP FXPQ3115BVT1 pressure sensor
- TMP117 temperature sensor
- Arduino Header, mikroBUS

For more information about the MCXW236 SoC and MCXW23-EVK board, see:

- `MCXW23 SoC Website`_
- `MCXW23 Datasheet`_
- `MCXW23 Reference Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The MCXW23 SoC has IOCON registers, which can be used to configure
the functionality of a pin.

+---------+-----------------+----------------------------+
| Name    | Function        | Usage                      |
+=========+=================+============================+
| PIO0_20 | GPIO            | ISP SW2                    |
+---------+-----------------+----------------------------+
| PIO0_2  | USART           | USART RX                   |
+---------+-----------------+----------------------------+
| PIO0_3  | USART           | USART TX                   |
+---------+-----------------+----------------------------+
| PIO0_19 | GPIO            | RED LED                    |
+---------+-----------------+----------------------------+
| PIO0_18 | GPIO            | USR SW4                    |
+---------+-----------------+----------------------------+
| PIO0_21 | GPIO            | Wakeup SW3                 |
+---------+-----------------+----------------------------+
| PIO0_14 | I2C             | I2C SCL                    |
+---------+-----------------+----------------------------+
| PIO0_13 | I2C             | I2C SDA                    |
+---------+-----------------+----------------------------+
| PIO0_1  | GPIO            | FXLS8974CF INT1            |
+---------+-----------------+----------------------------+
| PIO0_1  | GPIO            | TMP117 INT1                |
+---------+-----------------+----------------------------+
| PIO0_1  | GPIO            | FXPQ3115BVT1 INT1          |
+---------+-----------------+----------------------------+

System Clock
============

The MCXW23 SoC is configured to use FRO running at 32 MHz as a system clock source.

Serial Port
===========

The MCXW23 SoC has 3 FLEXCOMM interfaces for serial
communication. One is configured as USART for the console, one is
configured for I2C, and the other one is not used.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application`
and :ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the MCU-Link CMSIS-DAP Onboard Debug Probe.

Using LinkServer
----------------

Linkserver is the default runner for this board, and supports the factory
default MCU-Link firmware. Follow the instructions in
:ref:`mcu-link-cmsis-onboard-debug-probe` to reprogram the default MCU-Link
firmware. This only needs to be done if the default onboard debug circuit
firmware was changed. To put the board in ``DFU mode`` to program the firmware,
short jumper J32.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``DFU mode``
by shortening the jumper J32.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J11) of the board. Additionally, the jumper JP30 must
be shortened.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J33, and use the serial terminal of your choice
(minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mcxw23_evk
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-2105-g9da1d56da9e7 ***
   Hello World! mcxw23_evk/mcxw236

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mcxw23_evk
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-2105-g9da1d56da9e7 ***
   Hello World! mcxw23_evk/mcxw236

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _MCXW23 SoC Website:
   https://www.nxp.com/products/MCX-W23

.. _MCXW23 Datasheet:
    https://www.nxp.com/docs/en/data-sheet/MCXW23.pdf

.. _MCXW23 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=MCXW23RM
