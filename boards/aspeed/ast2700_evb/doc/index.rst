.. Copyright (c) 2026 ASPEED Technology Inc.
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: ast2700_evb

Overview
********

The AST2700 evaluation board target ``ast2700_evb/ast2700/bootmcu`` supports
the AST2700 BootMCU in a console-only configuration.

Hardware
********

- lowRISC Ibex RISC-V core (RV32IMC with Zicsr and Zifencei), running in machine mode
- 244 KB of SRAM for instructions and data
- 64-bit BootMCU timer at 1 MHz
- UART12 console

Supported Features
==================

.. zephyr:board-supported-hw::

Serial Port
===========

UART12 is configured for serial logs. The default serial setup is 115200 8N1.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

No flash or debug runner is supported for the BootMCU.

Flashing
========

The BootMCU executes from its SRAM at ``0x14b80000`` and does not execute in place
from flash. At power-on the BootMCU firmware is loaded from the boot image
in SPI flash into SRAM and started. Build the application as usual, then package
:file:`build/zephyr/zephyr.bin` as the BootMCU firmware of the AST2700 boot image
with the ASPEED boot image tools, and program the image into the SPI flash.

For example, build the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :board: ast2700_evb/ast2700/bootmcu
   :goals: build
