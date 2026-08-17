.. zephyr:board:: frdm_imxrt1152

Overview
********

The FRDM-IMXRT1152 board is a design and evaluation platform based on the
NXP i.MX RT1152 crossover MCU, featuring a single Cortex-M7 core running
up to 800 MHz, 512KB of FlexRAM and 1MB of dedicated OCRAM.

Hardware
********

- MIMXRT1152XHM8B MCU

  - 800MHz Cortex-M7
  - 512KB FlexRAM (configurable as ITCM/DTCM/OCRAM)
  - 1MB dedicated OCRAM

- Memory

  - 512 Mbit QSPI Flash (W25Q512NW)
  - 2x 256 Mbit HyperRAM (W959D8NFYA5) on FlexSPI2

- Connectivity

  - MCU-Link debug adapter with VCOM serial port
  - 10/100/1000 Mbit ENET and ENET QOS PHYs
  - USB 2.0 high speed
  - FRDM/Arduino compatible expansion headers

Supported Features
==================

.. zephyr:board-supported-hw::

Serial Port
===========

The i.MX RT1152 SoC has 9 LPUART interfaces. LPUART1 is configured as the
debug console, routed to the MCU-Link virtual COM port.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board
has an :ref:`mcu-link-cmsis-onboard-debug-probe`.

Configuring a Console
=====================

Regardless of your choice in debug probe, we will use the MCU-Link
serial device for a console once the board is flashed.

Use the following settings with your serial terminal of choice (minicom,
putty, etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imxrt1152/mimxrt1152
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.x.x *****
   Hello World! frdm_imxrt1152/mimxrt1152

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imxrt1152/mimxrt1152
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.x.x *****
   Hello World! frdm_imxrt1152/mimxrt1152

.. include:: ../../common/board-footer.rst.inc
