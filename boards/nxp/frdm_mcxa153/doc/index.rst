.. zephyr:board:: frdm_mcxa153

Overview
********

FRDM-MCXA153 are compact and scalable development boards for rapid prototyping of
MCX A14x and A15x MCUs. They offer industry standard headers for easy access
to the MCU's I/Os, integrated open-standard serial interfaces, external flash
memory and an on-board MCU-Link debugger. Additional tools like NXP's Expansion
Board Hub for add-on boards and the Application Code Hub for software examples
are available through the MCUXpresso Developer Experience.

Hardware
********

- MCX-A153 Arm Cortex-M33 microcontroller running at 96 MHz
- 128KB dual-bank on chip Flash
- 32 KB RAM
- USB full-speed with on-chip FS PHY. USB Type-C connectors
- 1x I3C
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, mikroBUS

For more information about the MCX-A153 SoC and FRDM-MCXA153 board, see:

- `MCX-A153 SoC Website`_
- `MCX-A153 Datasheet`_
- `MCX-A153 Reference Manual`_
- `FRDM-MCXA153 Website`_
- `FRDM-MCXA153 User Guide`_
- `FRDM-MCXA153 Board User Manual`_
- `FRDM-MCXA153 Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The MCX-A153 SoC has 4 gpio controllers and has pinmux registers which
can be used to configure the functionality of a pin.

+------------+-----------------+----------------------------+
| Name       | Function        | Usage                      |
+============+=================+============================+
| PIO0_2     | UART            | UART RX                    |
+------------+-----------------+----------------------------+
| PIO0_3     | UART            | UART TX                    |
+------------+-----------------+----------------------------+

System Clock
============

The MCX-A153 SoC is configured to use FRO running at 96MHz as a source for
the system clock.
The MCX-A153 uses OS timer as the kernel timer.

Serial Port
===========

The FRDM-MCXA153 SoC has 3 LPUART interfaces for serial communication.
LPUART 0 is configured as UART for the console.

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
short jumper JP8.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``ISP mode``
by shorting the jumper JP8.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J18) of the board. Additionally, the jumper JP20 must
be shorted.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J15, and use the serial terminal of your choice
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
   :board: frdm_mcxa153
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxa3/mcxa153

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxa153/mcxa153
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxa153/mcxa153

Troubleshooting
===============

.. include:: ../../common/segger-ecc-systemview.rst
   :start-after: segger-ecc-systemview

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _MCX-A153 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-a-series-microcontrollers/mcx-a13x-14x-15x-mcus-with-arm-cortex-m33-scalable-device-options-low-power-and-intelligent-peripherals:MCX-A13X-A14X-A15X

.. _MCX-A153 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/MCXAP64M96FS3.pdf

.. _MCX-A153 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=MCXAP64M96FS3RM

.. _FRDM-MCXA153 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-MCXA153

.. _FRDM-MCXA153 User Guide:
   https://www.nxp.com/document/guide/getting-started-with-frdm-mcxa153:GS-FRDM-MCXAXX

.. _FRDM-MCXA153 Board User Manual:
   https://www.nxp.com/docs/en/user-manual/UM12012.pdf

.. _FRDM-MCXA153 Schematics:
   https://www.nxp.com/webapp/Download?colCode=SPF-90829_A1
