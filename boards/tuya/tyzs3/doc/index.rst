.. zephyr:board:: tyzs3

Overview
********

TYZS3 is a low power-consumption embedded Zigbee module developed by Tuya.
It consists of a highly integrated wireless RF processor chip
(EFR32MG13P732F512GM48) and several peripherals. TYZS3 is embedded with a low
power-consumption 32-bit ARM Cortex-M4 core, 512-KB flash memory, 64-KB RAM, and
rich peripheral resources.

Hardware
********

- EFR32MG13P732F512GM48 Gecko SoC
- CPU core: ARM Cortex®-M4 with FPU
- Flash memory: 512 kB
- RAM: 64 kB
- Transmit power: up to +19 dBm
- Operation frequency: 2.4 GHz
- Crystal for HFXO (38.4 MHz).

For more information about the EFR32MG13 SoC and TYZS3 module:

- `EFR32MG13 Website`_
- `EFR32MG13 Datasheet`_
- `EFR32xG13 Reference Manual`_
- `TYZS3 Module Datasheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The EFR32MG13P SoC is configured to use the 38.4 MHz external oscillator on the
board.

Serial Port
===========

The EFR32MG13P SoC has three USARTs and one Low Energy UARTs (LEUART with 9600
maximum baudrate). USART0 is used for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

This board supports jlink, openocd and pyocd. For pyocd, you have to install the
pack first:

.. code-block:: console

   pyocd pack install efr32mg13p732f512gm48

Solder wires to the pins SWDIO, SWDCLK, nRST, GND and VCC (3.3V). Then connect
them to a power source and a debug adapter of your choice.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: tyzs3
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 9600
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! tyzs3/efr32mg13p732f512gm48


.. _EFR32MG13 Website:
   https://www.silabs.com/wireless/zigbee/efr32mg13-series-1-socs

.. _EFR32MG13 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32mg13-datasheet.pdf

.. _EFR32xG13 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg13-rm.pdf

.. _TYZS3 Module Datasheet:
   https://developer.tuya.com/en/docs/iot/tuya-tyzs3-zigbee-module-datasheet?id=K96efadaercw6
