.. Copyright (c) 2026 Advanced Micro Devices, Inc.
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: zynqmp_rpu

Overview
********

This board configuration targets the Cortex-R5 real-time processing unit (RPU) on
AMD Zynq UltraScale+ MPSoC. R5 DDR starts at ``0x00400000``, GIC for the RPU is
at ``0xf9000000``, and PS UART0 is at ``0xff000000``.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

The default console is UART0 (typical Linux name ``ttyPS0``).

Memories
--------

* ``sram0`` is at ``0x0400_0000`` (64 MiB).
* QSPI linear flash is mapped at ``0xc0000000`` (512 MiB).

Known limitations
=================

* Dual-redundant lock-step R5 operation is not targeted by this board file.
* Only the first R5 core is supported.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

QEMU
====

The board uses the ``arm-generic-fdt`` QEMU machine with a ZCU102 hardware DTB
compiled from ``zynqmp_rpu-qemu.dts`` at build time. The Zephyr ELF is loaded on
``cpu-num=4`` (the R5 cluster); MMIO pokes at ``0xff5e023c`` and ``0xff9a0000``
release the R5 from reset.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: zynqmp_rpu
   :goals: run
   :compact:

Hardware (XSDB)
===============

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: zynqmp_rpu
   :goals: build

References
**********

1. AMD Zynq UltraScale+ Device Technical Reference Manual (UG1085)
