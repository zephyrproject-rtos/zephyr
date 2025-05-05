.. zephyr:board:: ek_ra8d1

Overview
********

The EK-RA8D1 is an Evaluation Kit for Renesas RA8D1 MCU Group which are the industry’s first 32-bit
graphics-enabled MCUs based on the Arm Cortex-M85 (CM85) core, delivering breakthrough performance
of over 3000 Coremark points at 480 MHz and superior graphics capabilities that enable high-resolution
displays and Vision AI applications.

The key features of the EK-RA8D1 board are categorized in three groups as follow:

**MCU Native Pin Access**

- 480MHz Arm Cortex-M85 based RA8D1 MCU in 224 pins, BGA package
- Native pin acces througgh 2 x 50-pin, and 2 x 40-pin male headers
- MCU current measurement points for precision current consumption measurement
- Multiple clock sources - RA8D1 MCU oscillator and sub-clock oscillator crystals,
  providing precision 20.000MHz and 32,768 Hz refeence clocks.
  Additional low precision clocks are available internal to the RA8D1 MCU

**System Control and Ecosystem Access**

- USB Full Speed Host and Device (micro-AB connector)
- Four 5V input sources

  - USB (Debug, Full Speed, High Speed)
  - External power supply (using surface mount clamp test points and power input vias)

- Three Debug modes

  - Debug on-board (SWD)
  - Debug in (ETM, SWD and JTAG)
  - Debug out (SWD)

- User LEDs and buttons

  - Three User LEDs (red, blue, green)
  - Power LED (white) indicating availability of regulated power
  - Debug LED (yellow) indicating the debug connection
  - Two User buttons
  - One Reset button

- Five most popular ecosystems expansions

  - Two Seeed Grove system (I2C/I3C) connectors
  - One SparkFun Qwiic connector
  - Two Digilent Pmod (SPI, UART and I2C/I3C) connectors
  - Arduino (Uno R3) connector
  - MikroElektronika mikroBUS connector

- MCU boot configuration jumper

**Special Feature Access**

- Ethernet (RJ45 RMII interface)
- USB High Speed Host and Device (micro-AB connector)
- 512 Mb (64 MB) External Octo-SPI Flash (present in the MCU Native Pin Access area of the EK-RA8D1 board)
- CAN FD (3-pin header)

Hardware
********
Detailed Hardware features for the RA8D1 MCU group can be found at `RA8D1 Group User's Manual Hardware`_

.. figure:: ra8d1_block_diagram.png
	:width: 442px
	:align: center
	:alt: RA8D1 MCU group feature

	RA8D1 Block diagram (Credit: Renesas Electronics Corporation)

Detailed Hardware features for the EK-RA8D1 MCU can be found at `EK-RA8D1 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

.. note::

   - For using Ethernet on RA8D1 board please set switch SW1 as following configuration:

     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
     | SW1-1 PMOD1 | SW1-2 TRACE | SW1-3 CAMERA | SW1-4 ETHA | SW1-5 ETHB | SW1-6 GLCD | SW1-7 SDRAM | SW1-8 I3C |
     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
     |     OFF     |      OFF    |      OFF     |     OFF    |     ON     |      OFF   |      OFF    |     OFF   |
     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+

   - For using SDHC channel 1 on RA8D1 board please set switch SW1 as following configuration:

     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
     | SW1-1 PMOD1 | SW1-2 TRACE | SW1-3 CAMERA | SW1-4 ETHA | SW1-5 ETHB | SW1-6 GLCD | SW1-7 SDRAM | SW1-8 I3C |
     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
     |     OFF     |      OFF    |      OFF     |     OFF    |     OFF    |      OFF   |      OFF    |     OFF   |
     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+

   - For using MIPI Graphics Expansion Port (J58) on RA8D1 board please set switch SW1 as following configuration:

     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
     | SW1-1 PMOD1 | SW1-2 TRACE | SW1-3 CAMERA | SW1-4 ETHA | SW1-5 ETHB | SW1-6 GLCD | SW1-7 SDRAM | SW1-8 I3C |
     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
     |     OFF     |     OFF     |      OFF     |     OFF    |     OFF    |     ON     |     ON      |    OFF    |
     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+

   - For using the Parallel Graphics Expansion Port (J57) with the Graphics Expansion Board supplied as part of the kit,
     please set switch SW1 as following configuration:

     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
     | SW1-1 PMOD1 | SW1-2 TRACE | SW1-3 CAMERA | SW1-4 ETHA | SW1-5 ETHB | SW1-6 GLCD | SW1-7 SDRAM | SW1-8 I3C |
     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
     |     OFF     |     OFF     |      OFF     |     OFF    |     OFF    |     ON     |     ON      |    OFF    |
     +-------------+-------------+--------------+------------+------------+------------+-------------+-----------+

.. warning::

   Do not enable SW1-4 and SW1-5 together

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ek_ra8d1`` board configuration can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

**Note:** Only support from SDK v0.16.6 in which GCC for Cortex Arm-M85 was available.
To build for EK-RA8M1 user need to get and install GNU Arm Embedded toolchain from https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v0.16.6

Flashing
========

Program can be flashed to EK-RA8D1 via the on-board SEGGER J-Link debugger.
SEGGER J-link's drivers are available at https://www.segger.com/downloads/jlink/

To flash the program to board

1. Connect to J-Link OB via USB port to host PC

2. Make sure J-Link OB jumper is in default configuration as describe in `EK-RA8D1 - User's Manual`_

3. Execute west command

	.. code-block:: console

		west flash -r jlink

Debugging
=========

You can use Segger Ozone (`Segger Ozone Download`_) for a visual debug interface

Once downloaded and installed, open Segger Ozone and configure the debug project
like so:

* Target Device: R7FA8D1BH
* Target Interface: SWD
* Target Interface Speed: 4 MHz
* Host Interface: USB
* Program File: <path/to/your/build/zephyr.elf>

**Note:** It's verified that debug is OK on Segger Ozone v3.30d so please use this or later
version of Segger Ozone

References
**********
- `EK-RA8D1 Website`_
- `RA8D1 MCU group Website`_

.. _EK-RA8D1 Website:
   https://www.renesas.com/us/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ek-ra8d1-evaluation-kit-ra8d1-mcu-group

.. _RA8D1 MCU group Website:
   https://www.renesas.com/us/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra8d1-480-mhz-arm-cortex-m85-based-graphics-microcontroller-helium-and-trustzone

.. _EK-RA8D1 - User's Manual:
   https://www.renesas.com/us/en/document/mat/ek-ra8d1-v1-user-manual

.. _RA8D1 Group User's Manual Hardware:
   https://www.renesas.com/us/en/document/mah/ra8d1-group-users-manual-hardware

.. _Segger Ozone Download:
   https://www.segger.com/downloads/jlink#Ozone
