.. zephyr:board:: pb_rh850u2b_468pin

Overview
********

The RH850/U2B 468pin piggyback board is part of RH850 Evaluation Platform and serves as a simple and
easy to use platform of evaluating the features and performance of Renesas Electronic's 32-bit
RH850/U2B 468pin microcontrollers.

RH850/U2B is a 32-bit single-chip microcontroller with multiple CPU cores, Code Flash, Data Flash,
RAM modules, DMA controllers, A/D converters, timer units and many communication interfaces that
are used in the automotive applications. This microcontroller conforms to the Automotive Safety
Integrity Level (ASIL) that is highly demanded in the recent automotive field (ASIL D level).

The key features of the PB-RH850U2B-468PIN board are categorized in three groups:

**MCU Native Pin Access**

- RH850/U2B MCU in a 468-pin BGA package, with multiple 32-bit RH850G4MH cores
- Code Flash and Data Flash (Code Flash up to 24 MB depending on the device), on-chip RAM
- Native pin access to the MCU signals of the 468-pin package
- Main oscillator (20 MHz, as used by the E2 emulator ``-osc 20`` option)

**System Control and Ecosystem Access**

- 5V external power supply input
- Debug and flash programming via the 46-pin Aurora debug connector (Renesas E2 emulator)
- Serial console through RLIN3 channel 0 (``uart0``, 115200 baud)

**Special Feature Access**

- Automotive communication interfaces of the RH850/U2B: Ethernet, RHSIF, FlexRay, CAN-FD, LIN, SENT and PSI5

Hardware
********
Detailed hardware features for the RH850/U2B MCU group can be found at `RH850 U2B Group User Manual Hardware`_

.. figure:: rh850u2b_468pin_block_diagram.webp
	:width: 442px
	:align: center
	:alt: RH850/U2B MCU group feature

	RH850/U2B Block diagram (Credit: Renesas Electronics Corporation)

Detailed hardware features for the RH850/U2B MCU can be found at `RH850 U2B 468pin User Manual Piggy Board`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``pb_rh850u2b_468pin`` board target configuration can be
built, flashed, and debugged in the usual way. There are many supported soc in
``pb_rh850u2b_468pin`` board such as ``r7f70254xafabg-c`` and ``r7f70254xfabg-c``.
See :ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

**Note:** Currently, the pb_rh850u2b_468pin is built and programmed using the IAR for RH850 toolchain.
Please follow the steps below to program it onto the board:

  1. Download and install IAR toolchain:
     https://www.iar.com/embedded-development-tools/free-trials

  2. Set env variable:

   .. code-block:: console

      export ZEPHYR_TOOLCHAIN_VARIANT=iar
      export IAR_TOOLCHAIN_PATH=<Path/to/your/toolchain>/rh850

  3. Build the Blinky Sample for pb_rh850u2b_468pin

   .. code-block:: console

      cd ~/zephyrproject/zephyr
      west build -p always -b pb_rh850u2b_468pin/@socname samples/basic/blinky

Flashing
========

Program can be flashed to PB_RH850U2B_468PIN via 46-pin Aurora debug connector (e.g, for
using the Renesas standard emulator for RH850/U2B is the E2 emulator)

E2 emulator User's Manual at https://www.renesas.com/en/document/mat/emulation-adapter-rh850u2b-users-manual?r=488796

Driver are available at https://www.renesas.com/en/document/uid/usb-driver-renesas-mcu-toolse2e2-liteie850ie850apg-fp5-v27700for-32-bit-version-windows-os?r=488796

To flash a program to the board:

1. Connect to E2 emulator via 46-pin Aurora debug connector to host PC

2. Make sure 46-pin Aurora debug connector is in default configuration as
described in `RH850 U2B 468pin User Manual Piggy Board`_

3. Execute west command

   .. code-block:: console

      west flash -- --tool-opt="-auth id FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF -osc 20"

Debugging
=========

You can use IAR Embedded Workbench for Renesas RH850 (`IAR Embedded Workbench Download`_) for a visual debug interface

Once downloaded and installed, open IarIdePm and configure the debug project
like so:

* Target Device: RH850 - R7F70254x
* Driver: E2
* Power Supply: 5V
* Program File: <path/to/your/build/zephyr.elf>

**Note:** It's verified that we can debug OK on IAR Embedded Workbench for Renesas RH850 version 3.20.1 so please
use this or later version of IAR Embedded Workbench

References
**********
- `RH850 U2B Website`_
- `RH850 U2B MCU group Website`_

.. _RH850 U2B Website:
   https://www.renesas.com/en/products/rh850-u2b

.. _RH850 U2B MCU group Website:
   https://www.renesas.com/en/document/dst/rh850u2b-datasheet-rev-100

.. _RH850 U2B Group User Manual Hardware:
   https://www.renesas.com/en/document/mah/rh850u2b-hardware-users-manual-rev-120-r01uh0923ej0120?r=1539266

.. _RH850 U2B 468pin User Manual Piggy Board:
   https://www.renesas.com/en/design-resources/boards-kits/y-rh850-u2b-468pin-pb-t1-v1

.. _IAR Embedded Workbench Download:
   https://www.iar.com/embedded-development-tools/free-trials
