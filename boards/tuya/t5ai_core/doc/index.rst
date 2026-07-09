.. zephyr:board:: t5ai_core

Overview
********

T5AI Core is a Tuya board based on the Beken BK7258 SoC. The BK7258
contains an Arm Cortex-M33 CPU and on-chip flash and SRAM.

Hardware
********

- Beken BK7258 SoC
- Arm Cortex-M33 CPU
- 128 KiB SRAM
- On-chip flash
- UART1 console on GPIO0 TX and GPIO1 RX

Supported Features
==================

.. zephyr:board-supported-hw::

Serial Console
==============

UART1 is used as the default console at 115200 baud, 8 data bits, no parity,
and 1 stop bit.

Programming and Debugging
*************************

T5AI Core requires a first-stage bootloader outside of Zephyr. This board
support does not provide a bootloader or a flashing runner.

Build the Hello World sample with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: t5ai_core/bk7258
   :goals: build
