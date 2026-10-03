..
   SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
   SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: m5stack_coremp135

Overview
********

The M5Stack CoreMP135 is an all-in-one miniature computer built around the
STM32MP135DAE7 processor. It combines a single-core Arm Cortex-A7 CPU running at
up to 1 GHz with 512 MiB of DDR3L SDRAM and a 2-inch IPS touch screen. The board
also provides two Ethernet interfaces, USB host and device connectors, CAN FD
(Controller Area Network Flexible Data-Rate), RS-485, a microSD card slot, and
an AXP2101 power-management integrated circuit.

Zephyr currently supports the ``m5stack_coremp135/stm32mp135dxx/fsbl`` target.

FSBL variant
============

The ``fsbl`` variant starts Zephyr directly from boot ROM, without an
intervening bootloader. The boot ROM loads the Zephyr application into the
128 KiB SYSRAM, and the application executes from there. The application image
loaded by boot ROM must therefore fit within 128 KiB.

During Zephyr startup, the platform initialization required by this variant is
performed before ``main()`` runs, including configuration of the required
system clocks and initialization of external DDR memory. Additional memory,
including SRAM and initialized external DDR, is available at runtime.

The build also creates ``zephyr.stm32`` with the STM32 image header required by
the boot ROM.

More information about the board is available from the
`M5Stack CoreMP135 documentation`_.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The Zephyr console uses the A and B terminals of the orange PWR485 port on
the left side of the device. Connect an RS-485 adapter configured for 115200 baud
to view the console output.


Programming and Debugging
*************************

The FSBL target uses the STM32MP13 boot ROM's USB DFU (Device Firmware Upgrade)
interface for flashing.

.. zephyr:board-supported-runners::

Running Hello World from SYSRAM
===============================

At startup, the processor's boot ROM searches the microSD card for a bootable
image. If it does not find one, it exposes the USB DFU interface. The boot ROM
loads the FSBL into SYSRAM and starts it from there, so the complete image must
fit within 128 KiB.

Build :zephyr:code-sample:`hello_world` for the FSBL target:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: m5stack_coremp135/stm32mp135dxx/fsbl
   :goals: build

The build creates ``build/zephyr/zephyr.stm32`` with the unsigned, 512-byte
STM32 image header version 2.0 expected by the STM32MP13 boot ROM. Install
``dfu-util`` on the host before flashing; STM32CubeProgrammer is not required.

Remove the microSD card, reset the CoreMP135, and connect its USB device port to
the host. The board appears as a USB DFU device ``0483:df11`` and exposes the
``@FSBL`` target as alternate setting 0. Upload and start the image with:

.. code-block:: console

   west flash

The runner downloads ``zephyr.stm32`` and then sends a separate DFU detach
request to start it. Zephyr configures the clocks, initializes DDR, and prints
the following output through the RS-485 console:

.. code-block:: console

   *** Booting Zephyr OS build <version> ***
   Hello World! m5stack_coremp135/stm32mp135dxx/fsbl

References
**********

.. target-notes::

.. _M5Stack CoreMP135 documentation:
   https://docs.m5stack.com/en/core/M5CoreMP135
