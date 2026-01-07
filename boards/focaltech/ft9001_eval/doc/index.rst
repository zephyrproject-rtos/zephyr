:orphan:

.. zephyr:board:: ft9001_eval

Overview
********

The FT9001-EVAL board is an evaluation platform for the FocalTech FT9001 MCU
(Arm Cortex-M4F, up to 160 MHz).

Hardware
********

- FocalTech FT9001 SoC (ARM Cortex-M4F compatible)
- 32 KB boot ROM
- 256 KB on-chip SRAM:

  - 224 KB standard SRAM
  - 32 KB DBUS-connected SRAM

- 2 MB external flash memory (SPI interface)
- 2x UART interfaces
- 2x SPI interfaces
- External GPIO pins with interrupt capability

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Build and flash the :zephyr:code-sample:`hello_world` sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ft9001_eval
   :goals: build flash
   :compact:
