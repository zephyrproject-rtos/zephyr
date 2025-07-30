.. zephyr:board:: frdm_mcxa156

Overview
********

FRDM-MCXA156 are compact and scalable development boards for rapid prototyping of
MCX A144/5/6 A154/5/6 MCUs. They offer industry standard headers for easy access
to the MCU's I/Os, integrated open-standard serial interfaces, external flash
memory and an on-board MCU-Link debugger. Additional tools like our Expansion
Board Hub for add-on boards and the Application Code Hub for software examples
are available through the MCUXpresso Developer Experience.

Hardware
********

- MCX-A156 Arm Cortex-M33 microcontroller running at 96 MHz
- 1MB dual-bank on chip Flash
- 128 KB RAM
- USB full-speed with on-chip FS PHY. USB Type-C connectors
- 1x FlexCAN with FD, 1x I3Cs
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, FlexIO/LCD Header, SmartDMA/Camera Header, mikroBUS

For more information about the MCX-A156 SoC and FRDM-MCXA156 board, see:

- `MCX-A156 SoC Website`_
- `MCX-A156 Datasheet`_
- `MCX-A156 Reference Manual`_
- `FRDM-MCXA156 Website`_
- `FRDM-MCXA156 User Guide`_
- `FRDM-MCXA156 Board User Manual`_
- `FRDM-MCXA156 Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The MCX-A156 SoC has 5 gpio controllers and has pinmux registers which
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

The MCX-A156 SoC is configured to use FRO running at 96MHz as a source for
the system clock.
The MCX-A156 uses OS timer as the kernel timer.

Serial Port
===========

The FRDM-MCXA156 SoC has 5 LPUART  interfaces for serial communication.
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
firmware was changed. To put the board in ``DFU mode`` to program the firmware,
short jumper JP5.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``DFU mode``
by shortening the jumper JP5.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J24) of the board. Additionally, the jumper JP7 must
be shortened.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J21, and use the serial terminal of your choice
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
   :board: frdm_mcxa156
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxa156/mcxa156

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxa156/mcxa156
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxa156/mcxa156

Troubleshooting
===============

.. include:: ../../common/segger-ecc-systemview.rst
   :start-after: segger-ecc-systemview

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _MCX-A156 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-a-series-microcontrollers/mcx-a13x-14x-15x-mcus-with-arm-cortex-m33-scalable-device-options-low-power-and-intelligent-peripherals:MCX-A13X-A14X-A15X

.. _MCX-A156 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/MCXAP100M96FS6.pdf

.. _MCX-A156 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=MCXAP100M96FS6RM

.. _FRDM-MCXA156 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/general-purpose-mcus/frdm-development-board-for-mcx-a144-5-6-a154-5-6-mcus:FRDM-MCXA156

.. _FRDM-MCXA156 User Guide:
   https://www.nxp.com/document/guide/getting-started-with-frdm-mcxa156:GS-FRDM-MCXA156

.. _FRDM-MCXA156 Board User Manual:
   https://www.nxp.com/docs/en/user-manual/UM12121.pdf

.. _FRDM-MCXA156 Schematics:
   https://www.nxp.com/webapp/Download?colCode=SPF-90841
