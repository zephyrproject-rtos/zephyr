.. zephyr:board:: sparkfun_pro_micro_rp2040

Overview
********

The SparkFun Pro Micro RP2040 is a small, low-cost, versatile board from
SparkFun. It is equipped with an RP2040 SoC, an on-board WS2812 addressable
LED, a USB connector, and a Qwiic connector. The USB bootloader allows it
to be flashed without any adapter, in a drag-and-drop manner.

Hardware
********
- Dual core Arm Cortex-M0+ processor running up to 133MHz
- 264KB on-chip SRAM
- 16MB on-board QSPI flash with XIP capabilities
- 18 GPIO pins
- 4 Analog inputs
- 1 UART peripherals
- 1 SPI controllers
- 2 I2C controllers (one via Qwiic connector)
- 16 PWM channels
- USB 1.1 controller (host/device)
- 8 Programmable I/O (PIO) for custom peripherals
- On-board RGB LED
- 1 Watchdog timer peripheral

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the RP2040 SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS. Please refer to
the datasheet to see the possible routings for each peripheral.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART0_TX : P0
- UART0_RX : P1
- I2C1_SDA : P2
- I2C1_SCL : P3
- SPI0_RX : P20
- SPI0_SCK : P22
- SPI0_TX : P23

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Using UF2
---------

The Pro Micro board does make the SWD pins available on pads on the
underside of the board. You can solder to these pins, and use a JTag
debugger. You can also flash the SparkFun ProMicro RP2040 with a UF2 file.
By default, building an app for this board will generate a
:file:`build/zephyr/zephyr.uf2` file. If the Pro Micro RP2040 is powered on with
the ``BOOTSEL`` button pressed, it will appear on the host as a mass storage
device. The UF2 file should be copied to the device, which will
flash the Pro Micro RP2040.

.. target-notes::

.. _Getting Started with Raspberry Pi Pico:
  https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf

.. _Graphical Datasheet:
  https://cdn.sparkfun.com/assets/e/2/7/6/b/ProMicroRP2040_Graphical_Datasheet.pdf
