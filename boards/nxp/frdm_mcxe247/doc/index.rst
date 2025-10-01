.. zephyr:board:: frdm_mcxe247

Overview
********

FRDM-MCXE247 is compact and scalable development board for rapid
prototyping of MCX E245/246/247 MCUs. It offers industry standard
headers for easy access to the MCU's I/Os, integrated
open-standard serial interfaces, external flash memory and
an on-board MCU-Link debugger.

Hardware
********

- MCXE247VLQ Arm Cortex-M4F 32-bit CPU
- Supports up to 112 MHz frequency (HSRUN mode)
- LQFP144 package
- 2 MB on chip flash memory
- 64 kB FlexNVM for data flash memory and EEPROM emulation
- 256 kB SRAM
- 4 kB of FlexRAM for use as SRAM or EEPROM emulation
- 2x ADC, 1x CMP, TRGMUX
- 2x LPI2C, 3xLPSPI, 1x FlexIO, 3x LPUART, 2x SAI, QSPI, 3xFlexCAN, IEEE1588 100 Mbps ETH MAC
- 8x FlexTimer, Internal and external WDOG, 1x LPIT, 1x LPTMR, 2x PDB, RTC
- NMH1000 magnetic switch
- FXLS8974CF accelerometer
- RGB user LED
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, mikroBUS, Pmod

For more information about the MCXE247 SoC and FRDM-MCXE247 board, see
these references:

- `MCX E24x Series Microcontrollers Website`_
- `FRDM-MCXE247 Website`_
- `FRDM-MCXE247 Quick Reference Guide`_
- `FRDM-MCXE247 Getting Started`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The MCX E247 SoC has five pairs of pinmux/gpio controllers (PORTA/GPIOA,
PORTB/GPIOB, PORTC/GPIOC, PORTD/GPIOD, and PORTE/GPIOE).

+-------+-------------+---------------------------+
| Name  | Function    | Usage                     |
+=======+=============+===========================+
| PTC13 | GPIO        | Red LED                   |
+-------+-------------+---------------------------+
| PTB11 | GPIO        | Green LED                 |
+-------+-------------+---------------------------+
| PTC12 | GPIO        | Blue LED                  |
+-------+-------------+---------------------------+
| PTD17 | LPUART2_RX  | UART Console              |
+-------+-------------+---------------------------+
| PTE12 | LPUART2_TX  | UART Console              |
+-------+-------------+---------------------------+
| PTA5  | RESET       | RESET Button SW1          |
+-------+-------------+---------------------------+
| PTA9  | GPIO        | User button SW2           |
+-------+-------------+---------------------------+
| PTC10 | GPIO        | User button SW3           |
+-------+-------------+---------------------------+
| PTA2  | I2C0_SDA    | I2C sensor                |
+-------+-------------+---------------------------+
| PTA3  | I2C0_SCL    | I2C sensor                |
+-------+-------------+---------------------------+

System Clock
============

The MCXE247 SoC is configured to use SPLL running at 80 MHz (RUN mode) as
a system clock source.

Serial Port
===========

The MCX E247 LPUART2 is used for the console.

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
short jumper JP2.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``DFU mode``
by shortening the jumper JP2.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J10) of the board.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J13, and use the serial terminal of your choice
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
   :board: frdm_mcxe247
   :goals: flash

Open a serial terminal, reset the board (press the SW1 button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-5194-ge7d44ce91cb3 ***
   Hello World! frdm_mcxe247/mcxe247

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxe247
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-5194-ge7d44ce91cb3 ***
   Hello World! frdm_mcxe247/mcxe247

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _MCX E24x Series Microcontrollers Website:
   https://www.nxp.com/products/MCX-E24X

.. _FRDM-MCXE247 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-MCXE247

.. _FRDM-MCXE247 Quick Reference Guide:
   https://www.nxp.com/docs/en/quick-reference-guide/MCXE247QSG.pdf

.. _FRDM-MCXE247 Getting Started:
   https://www.nxp.com/document/guide/getting-started-with-the-frdm-mcxe247-board:GS-FRDM-MCXE247
