.. zephyr:board:: ek_ra2l1

Overview
********

The EK-RA2L1 is an evaluation kit for Renesas RA2L1 Microcontroller Group.

Renesas RA2L1 Microcontroller Group has following features

- 48MHz, Arm Cortex-M23 core
- 256kB or 128kB Code Flash, 8kB Data Flash, 32kB SRAM (divided on 2 equal areas
  with- and without- ECC support)
- SCI x 5
- SPI x 2
- I2C x 2
- CAN x 1
- 12-bit A/D Converter
- 12-bit D/A Converter
- Low-Power Analog Comparator x 2
- Temperature Sensor
- General PWM Timer 32-bit x 4
- General PWM Timer 16-bit x 6
- Low Power Asynchronous General-Purpose Timer x 2
- Watchdog Timer (WDT)
- Independent Watchdog Timer (IWDT)
- up to 85 Input/Output pins (depends on the package type)

Hardware
********

EK-RA2L1 has following features.

- Native pin access through 1 x 40-pin and 3 x 20-pin male headers
- MCU current measurement points for precision current consumption measurement
- Multiple clock sources – Low-precision clocks are available internal to the MCU.
  Additionally, MCU oscillator and sub-clock oscillator crystals,
  20.000 MHz and 32,768 Hz, are provided for precision
- SEGGER J-Link on-board programmer and debugger
- Two Digilent Pmod (SPI and UART)
- Three user LEDs (red, blue, green)
- Power LED (white) indicating availability of regulated power
- Debug LED (yellow) indicating the debug connection
- Two user buttons
- One reset button

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and debugging
*************************

Building & Flashing
===================

You can build and flash an application with onboard J-Link debug adapter.
:ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ek_ra2l1
   :goals: build flash


Debugging
=========

Debugging also can be done with onboard J-Link debug adapter.
The following command is debugging the :zephyr:code-sample:`blinky` application.
Also, see the instructions specific to the debug server that you use.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ek_ra2l1
   :maybe-skip-config:
   :goals: debug

Or you can use Segger Ozone (`Segger Ozone Download`_) for a visual debug interface

Once downloaded and installed, open Segger Ozone and configure the debug project
like so:

* Target Device: R7FA2L1AB
* Target Interface: SWD
* Target Interface Speed: 4 MHz
* Host Interface: USB
* Program File: <path/to/your/build/zephyr.elf>


References
**********

.. EK-RA2L1 Web site:
   https://www.renesas.com/us/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ek-ra2l1-evaluation-kit-ra2l1-mcu-group

.. _Segger Ozone Download:
   https://www.segger.com/downloads/jlink#Ozone
