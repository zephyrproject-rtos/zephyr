.. zephyr:board:: ft9001_eval

FT9001 Evaluation Board
#######################

The **FT9001-EVAL** board is a hardware platform designed for evaluating and
debugging the FocalTech FT9001 MCU, a high-performance ARM Cortex-M4F processor
running at up to 160 MHz.

It includes 32 KB of ROM and 256 KB of RAM, divided into 224 KB of standard SRAM
and a distinct 32 KB DBUS-connected block.

Hardware
********

- FocalTech FT9001 SoC (ARM Cortex-M4F compatible)
- 32 KB boot ROM
- 256 KB on-chip SRAM:
  - 224 KB standard SRAM
  - 32 KB DBUS-connected SRAM
- 2 MB external flash memory (SPI interface)
- 2x UART interfaces
- 2x SPI interface
- External GPIO pins with interrupt capability

Supported Features
******************

.. zephyr:board-supported-hw::

Building and Running
********************

Build and flash the :zephyr:code-sample:`hello_world` sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ft9001_eval
   :goals: build flash
   :compact:
