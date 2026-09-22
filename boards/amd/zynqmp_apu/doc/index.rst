.. Copyright (c) 2026 Advanced Micro Devices, Inc.
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: zynqmp_apu

Overview
********

This board targets the Cortex-A53 application processing unit (APU) on AMD
Zynq UltraScale+ MPSoC devices. GIC-400 for the APU cluster is at
``0xf9010000``, high DDR at ``0x8_0000_0000``, and PS UART0 at ``0xff000000``.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

The default console is UART0 (typical Linux name ``ttyPS0``).

Memories
--------

* ``sram0`` uses the low DDR window (2 GiB from ``0x0``).

Known limitations
=================

* Only CPU0 is supported by this configuration.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build (example):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: zynqmp_apu
   :goals: build

Arm Trusted Firmware (TF-A)
===========================

When ``CONFIG_BUILD_WITH_TFA`` is enabled (default for this board), the build
also produces TF-A ``bl31.elf`` under
``build/tfa/zynqmp/<release|debug>/bl31/``. On hardware, Zephyr runs as BL33
after platform firmware (bitstream, FSBL, and PMUFW from the PDI) and TF-A have
initialized the PS and provide PSCI via SMC.

AMD ZynqMP boards do not boot from ``fip.bin``. TF-A is built with
``PRELOADED_BL33_BASE`` set to the Zephyr load address from devicetree, and the
XSDB runner loads the bitstream, FSBL, ``bl31.elf``, and Zephyr ELF separately.
When ``CONFIG_BUILD_WITH_TFA`` is enabled, ``west flash`` passes the built
``bl31.elf`` path automatically via ``--bl31``.

References
**********

1. AMD Zynq UltraScale+ Device Technical Reference Manual (UG1085)
