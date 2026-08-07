.. zephyr:board:: cpkcor_ra8d1b

Overview
********

The CPKCOR-RA8D1B, based on the Renesas Cortex-M85 architecture RA8D1 chip, offers engineers a convenient evelopment platform for study, evaluation and development.

Key Features

- Arm Cortex-M85
- 480MHz frequency, on-chip 2MB Flash, 1MB SRAM
- 32MB-SDRAM
- 16MB-QSPI Flash
- TF card slot
- USB2.0 high speed host/device controller via. Type-C interface

More information about the board can be found at the `CPKCOR-RA8D1B website`_.

Hardware
********
Detailed Hardware features for the RA8D1 MCU group can be found at `RA8D1 Group User's Manual Hardware`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``cpkcor_ra8d1b`` board can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

**Note:** Only support from SDK v0.16.6 in which GCC for Cortex Arm-M85 was available.
To build for CPKCOR-RA8D1B user need to get and install GNU Arm Embedded toolchain from https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v0.16.6

Flashing
========

Program can be flashed to CPKCOR-RA8D1B via the on-board SEGGER J-Link debugger.

To flash the program to board

1. Connect to J-LINK OB via USB port to host PC

2. Execute west command

	.. code-block:: console

		west flash

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cpkcor_ra8d1b
   :maybe-skip-config:
   :goals: debug

References
**********
.. target-notes::

.. _CPKCOR-RA8D1B Website:
   https://github.com/renesas/cpk_examples/blob/main/cpkcor_ra8d1b

.. _RA8D1 Group User's Manual Hardware:
   https://www.renesas.com/us/en/document/mah/ra8d1-group-users-manual-hardware
