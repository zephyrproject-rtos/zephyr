.. zephyr:board:: xiao_ra4m1

Overview
********

The XIAO RA4M1 integrates Renesas' RA4M1 chip (32-bit ARM® Cortex®-M4 MCU up to 48 MHz) into the
classic XIAO form factor. This development board offers 256KB Flash, 32KB SRAM, 8KB EEPROM, a USB
2.0 connector, reset and boot buttons, 3 LEDs, a 14-bit A/D converter, a 12-bit D/A converter, and a
CAN BUS interface. With onboard charging circuitry and low-power modes (as low as 45μA), it’s ideal
for battery-powered applications. Sharing the same 32-bit RA4M1 microcontroller as the Arduino Uno
R4, it's natively compatible with Arduino IDE and the extensive XIAO accessories, making it the
perfect starting point for electronics projects.

**Renesas RA4M1 Microcontroller Group**

- R7FA4M1AB3CNE
- 48-pin LQFP package
- 48 MHz Arm® Cortex®-M4 core with Floating Point Unit (FPU)
- 32 KB SRAM
- 256 KB code flash memory
- 8 KB data flash memory

**Connectivity**

- A Device USB connector for the Main MCU
- Pin headers for access to power and signals
- Bottom pads for access to signals, SWD, and battery hookup

**Operating voltage**

- External 5 V input through the Debug USB connector supplies the on-board power regulator to power
  logic and interfaces on the board. External 5 V or 3.3 V may be also supplied through alternate
  locations on the board.
- An external 3.7 V LiPo battery can be connected and the board includes the ability to charge the
  battery when 5 V power is supplied.
- A yellow User LED, controlled by the Main MCU firmware
- MCU reset push-button switch
- MCU boot push-button switch

Hardware
********

Detailed hardware features can be found at:

- RA4M1 MCU: `RA4M1 Group User's Manual Hardware`_
- XIOA RA4M1 board: `XIAO RA4M1 Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``xiao_ra4m1`` board can be built, flashed, and debugged
in the usual way. See :ref:`build_an_application` and :ref:`application_run`
for more details on building and running.

Flashing
========

Program can be flashed to XIAO RA4M1 via the built in RA4 USB bootloader.

To flash the program to board

1. Connect the XIAO RA4M1 USB port to the host.

2. Enter bootloader mode by holding the right Boot button, and tapping the left
   Reset button. The Boot button needs to be held for a second or two after the
   Reset has been released.

3. Execute west command

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xiao_ra4m1
   :goals: build flash

Debugging
=========

To debug on the XIAO RA4M1, connect an external debugger to the SWD pads on the
bottom of the controller. Once completed, debugging can be performed using
Zephyr's standard method (see :ref:`application_run`).
The following sample demonstrates how to debug using JLink:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: xiao_ra4m1
   :maybe-skip-config:
   :goals: debug


References
**********
- `XIAO RA4M1 Website`_
- `RA4M1 MCU group Website`_

.. _XIAO RA4M1 Website:
   https://wiki.seeedstudio.com/getting_started_xiao_ra4m1/

.. _RA4M1 MCU group Website:
   https://www.renesas.com/us/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra4m1-32-bit-microcontrollers-48mhz-arm-cortex-m4-and-lcd-controller-and-cap-touch-hmi

.. _RA4M1 Group User's Manual Hardware:
   https://www.renesas.com/us/en/document/mah/renesas-ra4m1-group-users-manual-hardware?r=1054146
