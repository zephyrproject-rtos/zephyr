.. zephyr:board:: sparkfun_rp2040_mikrobus

Overview
********

The `SparkFun RP2040 mikroBUS Development Board`_ is a low-cost, high
performance platform with flexible digital interfaces. It is equipped
with an RP2040 SoC, an on-board RESET and BOOTSEL button, a blue user LED
for status or test, a WS2812 addressable LED, a USB connector, a microSD
card slot, a mikroBUS socket, a Qwiic connector, a JST single cell battery
connector with a charging circuit and fuel gauge sensor and yellow LED as
battery charging indicator, a Thing Plus or Feather PTH pin layout, and
JTAG PTH pins. The USB bootloader allows it to be flashed without any
adapter, in a drag-and-drop manner.

Hardware
********

- Dual core Arm Cortex-M0+ processor running up to 133MHz
- 264KB on-chip SRAM
- 16MB on-board QSPI flash with XIP capabilities
- 8 GPIO pins
- 4 Analog inputs
- 1 UART peripherals
- 1 SPI controllers
- 2 I2C controllers (one via Qwiic connector)
- 16 PWM channels
- USB 1.1 controller (host/device)
- 8 Programmable I/O (PIO) for custom peripherals
- 1 Watchdog timer peripheral
- On-board digital RGB LED (WS2812)
- On-board general purpose user LED (blue)
- On-board charging status LED (yellow)
- On-board power status LED (red)
- 1 microSD card slot
- 1 `Qwiic`_ or `STEMMA QT`_ connector
- 1 `mikroBUS`_ socket
- 1 `Thing Plus`_ or `Feather`_ pin header

Supported Features
==================

.. zephyr:board-supported-hw::

Flash Partitioning
==================

The 16MB QSPI flash is partitioned into the following two main blocks by
default. At the beginning is a 1MB section for program code (including
the 256 bytes of boot information), followed by a 15MB storage area for
application data or file systems.

Pin Mapping
===========

The peripherals of the RP2040 SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS. Please refer
to the datasheet to see the possible routings for each peripheral.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART0_TX : P0
- UART0_RX : P1
- SPI0_SCK : P2
- SPI0_TX : P3
- SPI0_RX : P4
- SPI0_CSn : P5
- I2C1_SDA : P6
- I2C1_SCL : P7 & P23
- WS2812_DI : P8 (PIO0)
- SDIO_DAT3 : P9 (SPI1_CSn)
- SDIO_DAT2 : P10
- SDIO_DAT1 : P11
- SDIO_DAT0 : P12 (SPI1_RX)
- SDIO_CLK : P14 (SPI1_SCK)
- SDIO_CMD : P15 (SPI1_TX)
- BATT_ALERT : P24
- LED_BUILTIN : P25
- ADC0 : P26
- ADC1 : P27
- ADC2 : P28
- ADC3 : P29
- AVDD = 3V3

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Using JTAG
----------

The RP2040 mikroBUS Development Board does make the SWD pins available on
JTAG PTH pins in the middle of the board. You can solder a 2x5 1.27mm IDC
pin header here, and use a JTAG/SWD debugger.

Using UF2
---------

You can also flash the RP2040 mikroBUS Development Board with a UF2 file.
By default, building an app for this board will generate a
:file:`build/zephyr/zephyr.uf2` file. If the RP2040 mikroBUS Development
Board is powered on with the ``BOOTSEL`` button pressed, it will appear
on the host as a mass storage device. The UF2 file should be copied to
the device, which will flash the RP2040 mikroBUS Development Board.

.. target-notes::

.. _Getting Started with Raspberry Pi Pico:
  https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf

.. _SparkFun RP2040 mikroBUS Development Board:
   https://www.sparkfun.com/sparkfun-rp2040-mikrobus-development-board.html

.. _SparkFun RP2040 mikroBUS Development Board Schematic:
   https://cdn.sparkfun.com/assets/b/8/a/9/b/RP2040_MikroBUS_schematic.pdf

.. _SparkFun RP2040 mikroBUS Development Board Hookup Guide:
   https://learn.sparkfun.com/tutorials/rp2040-mikrobus-development-board-hookup-guide

.. _`mikroBUS`: https://www.mikroe.com/mikrobus

.. _`Qwiic`: https://www.sparkfun.com/qwiic

.. _`Thing Plus`: https://www.sparkfun.com/thing-plus

.. _`Feather`: https://learn.adafruit.com/adafruit-feather

.. _`STEMMA QT`: https://learn.adafruit.com/introducing-adafruit-stemma-qt
