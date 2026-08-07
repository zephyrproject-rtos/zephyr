.. zephyr:board:: weact_ra4m1_core

Overview
********

The MCU integrates multiple series of software- and pin-compatible Arm®-based 32-bit
cores that share a common set of Renesas peripherals to facilitate design scalability
and efficient platform-based product development.
The MCU provides an optimal combination of low-power, high-performance Arm Cortex®-M4 core
running up to 48 MHz with the following features:

**Renesas RA4M1 Microcontroller Group**

- R7FA4M1AB3CFM
- 64-pin LQFP package
- 48 MHz Arm® Cortex®-M4 core with Floating Point Unit (FPU)
- 32 KB SRAM
- 256 KB code flash memory
- 8 KB data flash memory

**Connectivity**

- A Device USB connector for the Main MCU
- A SWD interface is also provided for connecting optional external debuggers and
  programmers.
- 1 User button, 1 User LED
- Pin headers for access to power and signals for the Main MCU

**Multiple clock sources**

- Main MCU oscillator crystals, providing precision 16.000 MHz and 32,768 Hz external reference
  clocks
- Additional low-precision clocks are available internal to the Main MCU

**General purpose I/O ports**

- One jumper to allow measuring of Main MCU current
- Copper jumpers on PCB bottom side for configuration and access to selected MCU signals

**Operating voltage**

- External 5 V input through the USB-C connector supplies the on-board power regulator to power
  logic and interfaces on the board.
- A red board status LED indicating availability of regulated power.
- A blue User LED, controlled by the Main MCU firmware.
- A user push-button switch, controlled by the Main MCU firmware.
- MCU reset push-button switch.
- MCU boot configuration button.

Hardware
********

Detailed hardware features can be found at:

- RA4M1 MCU: `RA4M1 Group User's Manual Hardware`_
- RA4M1_Core Schematic: `Ra4m1-core Schematic`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``weact_ra4m1_core`` board can be built, flashed, and debugged
in the usual way. See :ref:`build_an_application` and :ref:`application_run`
for more details on building and running.

Flashing
========

Program can be flashed to WeAct RA4M1-Core via an external SEGGER J-Link debugger.
SEGGER J-link's drivers are available at https://www.segger.com/downloads/jlink/

To flash the program to board

1. Connect to J-Link OB via USB port to host PC

2. Make sure J-Link connection is correct to the board's SWD pins.

3. Execute west command

  .. code-block:: console

    west flash -r jlink

Debugging
=========

You can use Segger Ozone (`Segger Ozone Download`_) for a visual debug interface

Once downloaded and installed, open Segger Ozone and configure the debug project
like so:

* Target Device: R7FA4M1AB
* Target Interface: SWD
* Target Interface Speed: 4 MHz
* Host Interface: USB
* Program File: <path/to/your/build/zephyr.elf>

**Note:** It's verified that we can debug OK on Segger Ozone v3.30d so please use this or later
version of Segger Ozone

References
**********
.. _RA4M1 Group User's Manual Hardware:
   https://www.renesas.com/us/en/document/mah/renesas-ra4m1-group-users-manual-hardware?r=1054146

.. _Ra4m1-core Schematic:
  https://github.com/WeActStudio/WeActStudio.RA4M1_64Pin_CoreBoard/blob/master/Hardware/WeAct-RA4M1_CoreBoard_V10%20SchDoc.pdf

.. _Segger Ozone Download:
   https://www.segger.com/downloads/jlink#Ozone
