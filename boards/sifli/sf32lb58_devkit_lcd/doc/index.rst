.. zephyr:board:: sf32lb58_devkit_lcd

Overview
********

SF32LB58-DevKit-LCD is a development board based on the SF32LB58-MOD module,
primarily used for developing various applications with displays via DSI, DPI,
and QSPI interfaces.

The development board is equipped with analog MIC input, analog audio output,
SDIO interface, USB-C interface, TF card support, etc., providing developers
with rich hardware interface resources. It can be used to develop drivers for
various peripheral interfaces, helping developers simplify hardware development
processes and shorten product time-to-market.

More information about the board can be found at the
`SF32LB58-DevKit-LCD website`_.

Hardware
********

The development board has the following features:

- Module: Onboard SF32LB58-MOD-N16R32N1 or SF32LB58-MOD-A128R32N1 module based on SF32LB58x chip, module configuration as follows:

- Standard configuration SF32LB586VDD36 chip, internally integrated configuration:
  - 16MB HPI-PSRAM, interface frequency 144MHz
  - 16MB HPI-PSRAM, interface frequency 144MHz
  - 1MB QSPI-NOR Flash, interface frequency 48MHz
  - 16MB QSPI-NOR Flash, interface frequency 72MHz, STR mode (SF32LB58-MOD-N16R32N1 version)
  - 128MB QSPI-Nand Flash, interface frequency 72MHz, STR mode (SF32LB58-MOD-A128R32N1 version)
  - 48MHz crystal
  - 32.768KHz crystal
  - IPEX antenna socket
  - RF matching network and other passive components

- Specialized screen interface

  - DSI/RGB888, up to 2-lane data transmission, standard 30-pin FPC connector
  - DPI/RGB888, supports serial 8-bit RGB, Alpha & Omega 40-pin FPC connector
  - Dual SPI/DSPI/QSPI, supports DDR mode QSPI, extended via 40-pin header
  - Supports I2C interface touch screen

- Audio

  - Supports dual analog MIC input, onboard one analog MIC input by default,
    selectable via resistor jumper between onboard MIC or 40-pin header input
  - Supports stereo analog audio output, onboard Class-D audio PA, maximum
    2.8W/4 ohm speaker output, speaker connected via 40-pin header

- USB
  - Type C interface, supports onboard USB-to-serial chip for program download
    and software DEBUG, capable of power supply
  - Type C interface, supports USB2.0 HS, capable of power supply

- SD Card

  - Supports TF card using SDIO interface, onboard Micro SD card slot

- Headers

  - Large core GPIO input/output interface, 40-pin header
  - Small core GPIO input/output interface, 40-pin header


Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

to use `JLink`_::

  west debug -r jlink`

to use sftool::

  west flash -r sftool -- --port /dev/ttyACM1 # or your specified serial port


References
**********

.. target-notes::

.. _SF32LB58-DevKit-LCD website:
   https://wiki.sifli.com/en/board/sf32lb58x/SF32LB58-DevKit-LCD.html

.. _sftool website:
   https://github.com/OpenSiFli/sftool

.. _JLink:
   https://github.com/zephyrproject-rtos/zephyr/pull/100788#issuecomment-3649508761
