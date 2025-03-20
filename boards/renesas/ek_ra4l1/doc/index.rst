.. zephyr:board:: ek_ra4l1

Overview
********

The Renesas RA4L1 group of 32-bit microcontrollers (MCUs) uses the high-performance Arm
Cortex®-M33 core. Share a common set of Renesas peripherals to facilitate design scalability
and efficient platform-based product development.

The MCU in this series incorporates a high-performance Arm Cortex®-M33 core running up to
80 MHz with the following features:

**MCU Native Pin Access**

- R7FA4L1BD4CFP MCU (referred to as RA MCU)
- 80 MHz, Arm® Cortex®-M33 core
- 512 KB Code Flash, 64 KB SRAM
- 100-pin LQFP package

**System Control and Ecosystem Access**

- USB Full Speed Host and Device (USB-C connector)
- Three 5 V input sources

  - USB (Debug, Full Speed)
  - External power supply (using surface mount clamp test points and power input vias)

- Three Debug modes

  - Debug on-board (SWD)
  - Debug in (SWD)
  - Debug out (SWD, SW0 and JTAG)

- User LEDs and buttons

  - Three User LEDs (red, blue, green)
  - Power LED (white) indicating availability of regulated power
  - Debug LED (yellow) indicating the debug connection
  - Two User buttons
  - One Reset button

- Five most popular ecosystems expansions

	- 1 Seeed Grove® system (I3C) connector
	- 1 Seeed Grove® system (I2C/Analog) connector
	- 2 Digilent PmodTM (SPI, UART and I2C) connectors
	- ArduinoTM (Uno R3) connector
	- MikroElektronikaTM mikroBUS connector

- MCU boot configuration jumper

**Special Feature Access**

- 256 Mb (32 MB) External QUAD-SPI Flash
- CAN FD (3-pin header)
- Segment LCD Board Interface (50-pin header)

Hardware
********

Detailed hardware features can be found at:

- RA4L1 MCU: `RA4L1 Group User's Manual Hardware`_
- EK-RA4L1 board: `EK-RA4L1 - User's Manual`_

Debug on-board:

  - Connector Used: USB-C (J10)

+----------------+------+------+--------------------+------+--------------------------------------+
| Debug Modes    | J6   | J6-A | J8                 | J9   | J29                                  |
+================+======+======+====================+======+======================================+
| Debug on-board | Open | Open | Jumper on pins 1-2 | Open | Jumpers on pins 1-2, 3-4, 5-6, 7-8   |
+----------------+------+------+--------------------+------+--------------------------------------+

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Applications for the ``ek_ra4l1`` board configuration can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Flashing
========

Program can be flashed to EK-RA4L1 via the on-board SEGGER J-Link debugger.
SEGGER J-link's drivers are available at https://www.segger.com/downloads/jlink/

To flash the program to board

1. Connect to J-Link OB via USB port to host PC

2. Make sure J-Link OB jumper is in default configuration as describe in `EK-RA4L1 - User's Manual`_

3. Execute west command

  .. code-block:: console

    west flash -r jlink

References
**********
- `EK-RA4L1 Website`_
- `RA4L1 MCU group Website`_

.. _EK-RA4L1 Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ek-ra4l1-evaluation-kit-ra4l1-mcu-group

.. _RA4L1 MCU group Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra4l1-80mhz-arm-cortex-m33-based-low-power-mcu-trustzone-segment-lcd-controller-and-advanced-security

.. _EK-RA4L1 - User's Manual:
   https://www.renesas.com/en/document/mat/ek-ra4l1-v1-users-manual?r=25570359

.. _RA4L1 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/ra4l1-group-users-manual-hardware?r=25568281
