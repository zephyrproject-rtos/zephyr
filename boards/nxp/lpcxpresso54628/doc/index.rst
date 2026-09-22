.. zephyr:board:: lpcxpresso54628

Overview
********

The LPCXpresso54628 development board (OM13098) uses an NXP LPC54628 MCU based
on an Arm Cortex-M4F core.

Hardware
********

- LPC54628 M4F running at up to 220 MHz
- Memory

  - 512KB of on-chip flash memory
  - 200KB of on-chip SRAM (160KB main + 32KB SRAMX + 8KB USB)
- On-board high-speed USB based debug probe (LPC-Link2) with CMSIS-DAP and
  J-Link protocol support
- External debug probe option
- Three user LEDs, target reset, ISP and interrupt/user buttons
- Expansion options based on Arduino UNO and PMOD, plus additional expansion
  port pins
- 8MB SPIFI serial flash, 128Mb SDRAM
- 10/100 Mbps Ethernet, full-speed and high-speed USB, CAN

More information can be found here:

- `LPC546xx SoC Website`_
- `LPC546xx Datasheet`_
- `LPCXpresso54628 Board Website`_
- `LPCXpresso54628 Board User Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The IOCON controller can be used to configure the LPC54628 pins.

+---------+-----------------+----------------------------+
| Name    | Function        | Usage                      |
+=========+=================+============================+
| PIO0_29 | UART            | USART RX (VCOM)            |
+---------+-----------------+----------------------------+
| PIO0_30 | UART            | USART TX (VCOM)            |
+---------+-----------------+----------------------------+
| PIO3_14 | GPIO            | User LED1                  |
+---------+-----------------+----------------------------+
| PIO3_3  | GPIO            | User LED2                  |
+---------+-----------------+----------------------------+
| PIO2_2  | GPIO            | User LED3                  |
+---------+-----------------+----------------------------+

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

LinkServer is the default runner for this board. A debug probe is used for both
flashing and debugging the board. This board is configured by default to use the
on-board :ref:`lpclink2-cmsis-onboard-debug-probe` in the CMSIS-DAP mode. To use
this probe with Zephyr, you need to install the :ref:`linkserver-debug-host-tools`
and make sure they are in your search path. Refer to the detailed overview about
:ref:`application_debugging` for additional information.

Configuring a Console
=====================

Connect a USB cable from your PC to the LPC-Link2 USB connector, and use the
serial terminal of your choice (minicom, putty, etc.) with the following
settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lpcxpresso54628
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0 ***
   Hello World! lpcxpresso54628/lpc54628

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lpcxpresso54628
   :goals: debug

.. include:: ../../common/board-footer.rst.inc

.. _LPC546xx SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/lpc-cortex-m4-mcus/lpc546xx-arm-cortex-m4-based-microcontroller-family:LPC546XX

.. _LPC546xx Datasheet:
   https://www.nxp.com/docs/en/data-sheet/LPC546XX.pdf

.. _LPCXpresso54628 Board Website:
   https://www.nxp.com/products/developer-resources/software-development-tools/developer-resources-/lpcxpresso-boards/lpcxpresso54628-development-board:OM13098

.. _LPCXpresso54628 Board User Manual:
   https://www.nxp.com/webapp/Download?colCode=UM11035
