.. zephyr:board:: ra8d1_vision_board

Overview
********

The RA8D1-VISION-BOARD, based on the Renesas Cortex-M85 architecture RA8D1 chip, offers
engineers a flexible and comprehensive development platform, empowering them to explore the realm of
machine vision more deeply.

Key Features

- Arm Cortex-M85
- 480MHz frequency, on-chip 2Mb Flash, 1Mb SRAM
- 32Mb-SDRAM; 8Mb-QSPI Flash
- MIPI-DSI; RGB666; 8bit Camera
- On-board DAP-LINK debugger with CMSIS-DAP
- Raspberry Pi Interface

More information about the board can be found at the `RA8D1-VISION-BOARD website`_.

Hardware
********
Detailed Hardware features for the RA8D1 MCU group can be found at `RA8D1 Group User's Manual Hardware`_

Supported Features
==================

.. zephyr:board-supported-hw::

Default Zephyr Peripheral Mapping:
----------------------------------

The RA8D1-VISION-BOARD board features a On-board CMSIS-DAP debugger/programmer. Board is configured as follows:

- UART9 TX/RX : P209/P208 (CMSIS-DAP Virtual Port Com)
- LED0 : P102
- LED1 : P106
- LED2 : PA07
- USER BUTTON : P907

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ra8d1_vision_board`` board can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

**Note:** Only support from SDK v0.16.6 in which GCC for Cortex Arm-M85 was available.
To build for RA8D1-VISION-BOARD user need to get and install GNU Arm Embedded toolchain from https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v0.16.6

Flashing
========

Program can be flashed to RA8D1-VISION-BOARD via the on-board DAP-LINK debugger.

Linux users: to fix the permission issue, simply add the following udev rule for the
CMSIS-DAP interface:

.. code-block:: console

   $ echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0416", ATTR{idProduct}=="7687", MODE:="666"' > /etc/udev/rules.d/50-cmsis-dap.rules

To flash the program to board

1. Connect to DAP-LINK via USB port to host PC

2. Execute west command

	.. code-block:: console

		west flash

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ra8d1_vision_board
   :maybe-skip-config:
   :goals: debug

References
**********
.. target-notes::

.. _RA8D1-VISION-BOARD Website:
   https://github.com/RT-Thread-Studio/sdk-bsp-ra8d1-vision-board

.. _RA8D1 Group User's Manual Hardware:
   https://www.renesas.com/us/en/document/mah/ra8d1-group-users-manual-hardware
