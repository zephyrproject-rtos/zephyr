.. zephyr:board:: fpb_ra4e1

Overview
********

The Renesas RA4E1 Group delivers up to 100 MHz of CPU performance using an Arm® Cortex®-M33 core
with 512 KB code flash memory, 8 KB of data flash memory, and 128 KB of SRAM. RA4E1 MCUs
offer leading-performance. The RA4E1 Group offers a wide set of peripherals, including
USB 2.0 Full-Speed, Quad SPI, and advanced analog.

The MCU in this series incorporates a high-performance Arm Cortex®-M33 core running up to
100 MHz with the following features:

**MCU Native Pin Access**

- R7FA4E10D2CFM MCU (referred to as RA MCU)
- 100 MHz, Arm® Cortex®-M33 core
- 512 KB Code Flash, 8 KB Data Flash, 128 KB SRAM
- 64 pins, LQFP package
- Native pin access through 2 x 50-pin male headers (not fitted)
- MCU current measurement point for precision current consumption measurement
- Multiple clock sources - Low-precision (~1%) clocks are available internal to the RA MCU.
  RA MCU oscillator and sub-clock oscillator crystals, providing precision 24.000 MHz (not fitted)
  and 32,768 Hz reference clocks are also available

**System Control and Ecosystem Access**

- Two 5 V input sources

   - USB (Debug, Full Speed)
   - External power supply (using 2-pin header) (not fitted)

- Built-in SEGGER J-Link Emulator On-Board programmer/debugger (SWD)

- User LEDs and buttons

   - Two User LEDs (green)
   - Power LED (green) (not fitted) indicating availability of regulated power
   - Debug/power LED (yellow) indicating power and the debug connection
   -  One User button
   - One Reset button

- Two popular ecosystem expansions

   - Two Digilent PmodTM (SPI, UART) connectors (not fitted)
   - ArduinoTM (Uno R3) connectors

- MCU boot configuration jumper (not fitted)

Hardware
********

Detailed hardware features can be found at:

- RA4E1 MCU: `RA4E1 Group User's Manual Hardware`_
- FPB-RA4E1 board: `FPB-RA4E1 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``fpb_ra4e1`` board can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Flashing
========

Program can be flashed to FPB-RA4E1 via the on-board SEGGER J-Link debugger.
SEGGER J-link's drivers are available at https://www.segger.com/downloads/jlink/

To flash the program to board

1. Connect to J-Link OB via USB port to host PC

2. Make sure J-Link OB jumper is in default configuration as describe in `FPB-RA4E1 - User's Manual`_

3. Execute west command

	.. code-block:: console

		west flash -r jlink

Debugging
=========

You can use Segger Ozone (`Segger Ozone Download`_) for a visual debug interface

Once downloaded and installed, open Segger Ozone and configure the debug project
like so:

* Target Device: R7FA4E10D
* Target Interface: SWD
* Target Interface Speed: 4 MHz
* Host Interface: USB
* Program File: <path/to/your/build/zephyr.elf>

**Note:** It's verified that we can debug OK on Segger Ozone v3.30d so please use this or later
version of Segger Ozone

References
**********

- `FPB-RA4E1 Website`_
- `RA4E1 MCU group Website`_

.. _FPB-RA4E1 Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/fpb-ra4e1-fast-prototyping-board-ra4e1-mcu-group

.. _RA4E1 MCU group Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra4e1-100mhz-arm-cortex-m33-entry-line-balanced-low-power-consumption-optimized-feature-integration

.. _FPB-RA4E1 - User's Manual:
   https://www.renesas.com/en/document/mat/fpb-ra4e1-users-manual

.. _RA4E1 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/ra4e1-group-users-manual-hardware

.. _Segger Ozone Download:
   https://www.segger.com/downloads/jlink#Ozone
