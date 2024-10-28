.. _ek_ra2a1:

RA2A1 Evaluation Kit
####################

Overview
********

The EK-RA2A1 is an evaluation kit for Renesas RA2A1 Microcontroller Group.

Renesas RA2A1 Microcontroller Group has following features

- 48MHz, Arm Cortex-M23 core
- 256kB Code Flash, 8kB Data Flash, 32kB SRAM
- USB 2.0 Full-Sppeed
- SCI x 3
- SPI x 2
- I2C x 2
- CAN x 1
- 16-bit A/D Converter
- 24-bit Sigma-Delta A/D Converter
- 12-bit D/A Converter
- 8-bit D/A Converter x 2
- High-Speed Analog Comparator
- Low-Power Analog Comparator
- OPAMP x 3
- Temperature Sensor
- General PWM Timer 32-bit x 1
- General PWM Timer 16-bit x 6
- Low Power Asynchronous General-Purpose Timer x 2
- Watchdog Timer
- 49 Input/Output pins

Hardware
********

EK-RA2A1 has following features.

- Native pin access through 4x 40-pin male headers
- MCU current measurement points
- SEGGER J-Link on-board programmer and debugger
- Two Digilent Pmod (SPI and UART)
- User LED
- Mechanical user button
- Capacitive user button

Supported Features
==================

The Renesas EK-RA2A1 board configuration supports the following
hardware features:

+-----------+------------+-------------------------------+
| Interface | Controller | Driver/components             |
+===========+============+===============================+
| PINCTRL   | on-chip    | pinctrl                       |
+-----------+------------+-------------------------------+
| CLOCK     | on-chip    | clock_control                 |
+-----------+------------+-------------------------------+
| GPIO      | on-chip    | gpio                          |
+-----------+------------+-------------------------------+
| UART      | on-chip    | uart                          |
+-----------+------------+-------------------------------+

The default configuration can be found in
:zephyr_file:`boards/renesas/ek_ra2a1/ek_ra2a1_defconfig`


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
   :board: ek_ra2a1
   :goals: build flash


Debugging
=========

Debugging also can be done with onboard J-Link debug adapter.
The following command is debugging the :zephyr:code-sample:`blinky` application.
Also, see the instructions specific to the debug server that you use.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ek_ra2a1
   :maybe-skip-config:
   :goals: debug


References
**********

.. target-notes::

.. EK-RA2A1 Web site:
   https://www.renesas.com/us/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ek-ra2a1-evaluation-kit-ra2a1-mcu-group
