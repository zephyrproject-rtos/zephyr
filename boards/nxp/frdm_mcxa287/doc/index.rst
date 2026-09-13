.. SPDX-FileCopyrightText: Copyright 2026 NXP
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: frdm_mcxa287

Overview
********

FRDM-MCXA287 is a compact and scalable development board for rapid prototyping of MCX A287
MCUs. They offer industry standard headers for easy access to the MCUs input/output (I/O),
integrated open-standard serial interfaces, external flash memory and an onboard MCU-Link
debugger.

Hardware
********

- MCX-A287 Arm Cortex-M33 microcontroller running at 200MHz
- 2048KB dual-bank on chip Flash
- 640 KB RAM
- 1x FlexCAN with FD, 1x RGB LED, 2x SW buttons
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header

For more information about the MCX-A287 SoC and FRDM-MCXA287 board, see:

- `MCX-A287 SoC Website`_
- `FRDM-MCXA287 Website`_
- `FRDM-MCXA287 User Guide`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The MCX-A287 SoC has 5 gpio controllers and has pinmux registers which
can be used to configure the functionality of a pin.

+------------+-----------------+----------------------------+
| Name       | Function        | Usage                      |
+============+=================+============================+
| PIO1_8     | UART            | UART RX                    |
+------------+-----------------+----------------------------+
| PIO1_9     | UART            | UART TX                    |
+------------+-----------------+----------------------------+

System Clock
============

The MCX-A287 SoC is configured to use PLL1 running at 200MHz as a source for
the system clock.

Serial Port
===========

The FRDM-MCXA287 SoC has 6 LPUART interfaces for serial communication.
LPUART1 is configured as UART for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

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
firmware was changed. To put the board in ``ISP mode`` to program the firmware,
short jumper JP4.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``ISP mode``
by shortening the jumper JP4.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector of the board.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC, and use the serial terminal of your choice
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
   :board: frdm_mcxa287
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-13072-gcc2e6581a64 ***
   Hello World! frdm_mcxa287/mcxa287

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxa287/mcxa287
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-13072-gcc2e6581a64 ***
   Hello World! frdm_mcxa287/mcxa287

Troubleshooting
===============

.. include:: ../../common/segger-ecc-systemview.rst.inc

.. include:: ../../common/board-footer.rst.inc

.. _MCX-A287 SoC Website:
   https://www.nxp.com/products/MCX-A28X

.. _FRDM-MCXA287 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-MCXA287

.. _FRDM-MCXA287 User Guide:
   https://www.nxp.com/document/guide/getting-started-with-frdm-mcxa287:GS-FRDM-MCXA287
