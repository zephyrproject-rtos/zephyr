.. zephyr:board:: frdm_mcxa346

Overview
********

FRDM-MCXA346 are compact and scalable development boards for rapid prototyping of
MCX A16X MCUs. They offer industry standard headers for easy access to the MCUs I/Os,
integrated open-standard serial interfaces and an on-board MCU-Link debugger.
MCX A Series are high-performance, low-power microcontrollers with MAU,SmartDMA and performance efficiency.

Hardware
********

- MCX-A346 Arm Cortex-M33 microcontroller running at 180 MHz
- 1MB dual-bank on chip Flash
- 256 KB RAM
- 1x FlexCAN with FD, 1x RGB LED, 3x SW buttons
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, SmartDMA/Camera Header, mikroBUS

For more information about the MCX-A346 SoC and FRDM-MCXA346 board, see:

- `MCX-A346 SoC Website`_
- `MCX-A346 Datasheet`_
- `MCX-A346 Reference Manual`_
- `FRDM-MCXA346 Website`_
- `FRDM-MCXA346 User Guide`_
- `FRDM-MCXA346 Board User Manual`_
- `FRDM-MCXA346 Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The MCX-A346 SoC has 6 gpio controllers and has pinmux registers which
can be used to configure the functionality of a pin.

+------------+-----------------+----------------------------+
| Name       | Function        | Usage                      |
+============+=================+============================+
| PIO2_3     | UART            | UART RX                    |
+------------+-----------------+----------------------------+
| PIO2_2     | UART            | UART TX                    |
+------------+-----------------+----------------------------+

System Clock
============

The MCX-A346 SoC is configured to use FRO running at 180MHz as a source for
the system clock.

Serial Port
===========

The FRDM-MCXA346 SoC has 6 LPUART  interfaces for serial communication.
LPUART 2 is configured as UART for the console.

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
10-pin SWD connector (J14) of the board. Additionally, the jumper JP6 must
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
   :board: frdm_mcxa346
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxa346/mcxa346

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxa346/mcxa346
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxa346/mcxa346

Troubleshooting
===============

.. include:: ../../common/segger-ecc-systemview.rst
   :start-after: segger-ecc-systemview

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _MCX-A346 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-a-series-microcontrollers/mcx-a13x-14x-15x-mcus-with-arm-cortex-m33-scalable-device-options-low-power-and-intelligent-peripherals:MCX-A13X-A14X-A15X

.. _MCX-A346 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/MCXAP100M96FS6.pdf

.. _MCX-A346 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=MCXAP100M96FS6RM

.. _FRDM-MCXA346 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/general-purpose-mcus/frdm-development-board-for-mcx-a144-5-6-a154-5-6-mcus:FRDM-MCXA346

.. _FRDM-MCXA346 User Guide:
   https://www.nxp.com/document/guide/getting-started-with-frdm-mcxa346:GS-FRDM-MCXA346

.. _FRDM-MCXA346 Board User Manual:
   https://www.nxp.com/docs/en/user-manual/UM12121.pdf

.. _FRDM-MCXA346 Schematics:
   https://www.nxp.com/webapp/Download?colCode=SPF-90841
