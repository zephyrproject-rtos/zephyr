.. zephyr:board:: frdm_mcxa366

Overview
********

FRDM-MCXA366 are compact and scalable development boards for rapid prototyping of
MCX A27X MCUs. They offer industry standard headers for easy access to the MCUs I/Os,
integrated open-standard serial interfaces and an on-board MCU-Link debugger.
MCX A Series are high-performance, low-power microcontrollers with MAU,SmartDMA and performance efficiency.

Hardware
********

- MCX-A366 Arm Cortex-M33 microcontroller running at 180 MHz
- 1MB dual-bank on chip Flash
- 256 KB RAM
- 1x FlexCAN with FD, 1x RGB LED, 3x SW buttons
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, SmartDMA/Camera Header, mikroBUS

For more information about the MCX-A366 SoC and FRDM-MCXA366 board, see:

- `MCX-A366 SoC Website`_
- `MCX-A366 Datasheet`_
- `MCX-A366 Reference Manual`_
- `FRDM-MCXA366 Website`_
- `FRDM-MCXA366 User Guide`_
- `FRDM-MCXA366 Board User Manual`_
- `FRDM-MCXA366 Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The MCX-A366 SoC has 6 gpio controllers and has pinmux registers which
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

The MCX-A366 SoC is configured to use FRO running at 180MHz as a source for
the system clock.

Serial Port
===========

The FRDM-MCXA366 SoC has 6 LPUART  interfaces for serial communication.
LPUART 2 is configured as UART for the console.

Programming and Debugging
*************************

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

Programming and Debugging
=========================

.. zephyr:board-supported-runners::

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxa366
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxa366/mcxa366

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxa366/mcxa366
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxa366/mcxa366

Troubleshooting
===============

.. include:: ../../common/segger-ecc-systemview.rst.inc

.. include:: ../../common/board-footer.rst.inc

.. _MCX-A366 SoC Website:
   https://www.nxp.com/products/MCX-A345-A346

.. _MCX-A366 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/MCXAP144M240F60.pdf

.. _MCX-A366 Reference Manual:
   https://www.nxp.com/docs/en/reference-manual/MCXAP144M240F60RM.pdf

.. _FRDM-MCXA366 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-MCXA346

.. _FRDM-MCXA366 User Guide:
   https://www.nxp.com/document/guide/getting-started-with-frdm-mcxa346:GS-FRDM-MCXA346

.. _FRDM-MCXA366 Board User Manual:
   https://www.nxp.com/docs/en/user-manual/UM12348.pdf

.. _FRDM-MCXA366 Schematics:
   https://www.nxp.com/webapp/Download?colCode=FRDM-MCXA346-DESIGN-FILES
