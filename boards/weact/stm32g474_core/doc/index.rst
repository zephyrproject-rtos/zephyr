.. zephyr:board:: weact_stm32g474_core

The WeAct STM32G474 Core Board is based on the STM32G474CBU6 micro-controller.

The board provides essential features in a compact form factor with USB Type-C connectivity.

Hardware Features
=================

.. zephyr:board-supported-hw::

USB Type-C Configuration
------------------------

The board features a USB Type-C interface with configurable routing through solder bridges:

+---------------+---------+-----------------------------------------------+
| Solder bridge | Default | Description                                   |
+===============+=========+===============================================+
| SB3/SB4       | Open    | Connect PA9/PA10 to CC routing                |
+---------------+---------+-----------------------------------------------+
| SB5/SB6       | Open    | Connect PB6/PB4 to CC routing                 |
+---------------+---------+-----------------------------------------------+
| SB7/SB8       | Closed  | Connect CC1/CC2 to pull-down resistors        |
+---------------+---------+-----------------------------------------------+

Pin Mapping
===========

Key Pin Assignments
-------------------

- USB

  - D+: PA12
  - D-: PA11
- User LED: PC6
- User Button: PC13
- SPI1: PA5 (SCK), PA6 (MISO), PA7 (MOSI)
- Debug

  - SWDIO: PA13
  - SWCLK: PA14

Clock System
------------

The board features two crystals:

- Low-speed crystal (LSE): NX3215SA-32.768K
- High-speed crystal (HSE): 8MHz XTAL3225

Power Supply
------------

The board includes a 3.3V voltage regulator (ME6209A33M3G) for stable power supply, with support for:

- USB VBUS input
- VCC input range: 3.3V - 5.5V

Programming and Debugging
=========================

The board can be programmed and debugged through:

- SWD interface (SWDIO: PA13, SWCLK: PA14)
- USB DFU bootloader

References
==========

.. target-notes::

.. _STM32G474CB website:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g474cb.html
.. _WEACT_STM32G474_core_board website:
   https://github.com/WeActStudio/WeActStudio.STM32G474CoreBoard/tree/master
