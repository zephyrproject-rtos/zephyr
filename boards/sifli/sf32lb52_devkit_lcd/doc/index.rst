.. zephyr:board:: sf32lb52_devkit_lcd

Overview
********

SF32LB52-DevKit-LCD is a development board based on the SF32LB52x series chip
SoC. It is mainly used for developing various applications based on
SPI/DSPI/QSPI or MCU/8080 interface display screens.

More information about the board can be found at the
`SF32LB52-DevKit-LCD website`_.

Hardware
********

SF32LB52-DevKit-LCD provides the following hardware components:

- SF32LB52x-MOD-N16R8 module based on SF32LB525UC6

  - 8MB OPI-PSRAM @ 144MHz (from SF32LB525UC6)
  - 128Mb QSPI-NOR @ 72MHz, STR mode
  - 48MHz crystal
  - 32.768KHz crystal
  - Onboard antenna (default) or IPEX antenna, selectable via 0 ohm resistor
  - RF matching network and other R/L/C components

- Dedicated screen interface

  - SPI/DSPI/QSPI, supports DDR mode QSPI, led out through 22-pin FPC and 40-pin
    header
  - 8-bit MCU/8080, led out through 22pin FPC and 40pin header.
  - Supports touch screens with I2C interface.

- Audio

  - Analog MIC input.
  - Analog audio output, onboard Class-D audio PA.

- USB

  - Type C interface, connected to USB to serial chip, enabling program
    download and software debug, can also supply power.
  - Type C interface, supports USB-2.0 FS, can also supply power.

- SD card

  - Supports TF cards using SPI interface, onboard Micro SD card slot.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Refer to `sftool website`_ for more information.

References
**********

.. target-notes::

.. _SF32LB52-DevKit-LCD website:
   https://wiki.sifli.com/en/board/sf32lb52x/SF32LB52-DevKit-LCD.html

.. _sftool website:
   https://github.com/OpenSiFli/sftool
