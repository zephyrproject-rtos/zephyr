.. zephyr:board:: xiao_rp2350

Overview
********

The XIAO RP2350 is an IoT mini development board from Seeed Studio.
It is equipped with an RP2350A SoC, an on-board WS2812 addressable
LED, and USB connector. The USB bootloader allows it
to be flashed without any adapter, in a drag-and-drop manner.

For more details see the `Seeed Studio XIAO RP2350`_ wiki page.

Hardware
********

The Seeed Studio XIAO RP2350 is a low-power microcontroller that
carries the powerful Dual-core RP2350A processor with a flexible
clock running up to 133 MHz. There is also 264KB of SRAM, and 2MB of
on-board Flash memory.

There are 14 GPIO pins on Seeed Studio XIAO RP2350, on which there
are 11 digital pins, 3 analog pins, 11 PWM Pins, 1 I2C interface,
1 UART interface, 1 SPI interface, 1 SWD Bonding pad interface.
There are 8 GPIO pins on PCB bottom side as test pads with additional
8 digital pins for secondary interfaces, 1 I2C, 1 UART, 1 SPI.

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the RP2350A SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS. Please refer to
the datasheet to see the possible routings for each peripheral.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rubric:: primary and secondary interfaces

.. rst-class:: rst-columns

- UART0_TX : P0
- UART0_RX : P1
- I2C1_SDA : P6
- I2C1_SCL : P7
- SPI0_CS : P5 (GPIO)
- SPI0_RX : P4
- SPI0_TX : P3
- SPI0_SCK : P2
- UART1_TX : P20
- UART1_RX : P21
- I2C0_SDA : P16
- I2C0_SCL : P17
- SPI1_CS : P9 (GPIO)
- SPI1_SCK : P10
- SPI1_RX : P11
- SPI1_TX : P12

Connections and IOs
===================

The board uses a standard XIAO pinout on edge connectors and additional XIAO
test pads on bottom side of the PCB. The `Seeed Studio XIAO RP2350`_ wiki has
detailed information about the board, including pinouts_ and schematic_.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Using UF2
---------

You can flash the Xiao RP2350 with a UF2 file.
By default, building an app for this board will generate a
:file:`build/zephyr/zephyr.uf2` file. If the Xiao RP2350 is powered on with
the ``BOOTSEL`` button pressed, it will appear on the host as a mass storage
device. The UF2 file should be copied to the device, which will
flash the Xiao RP2350.

References
**********

.. target-notes::

.. _Seeed Studio XIAO RP2350: https://wiki.seeedstudio.com/getting-started-xiao-rp2350/
.. _pinouts: https://wiki.seeedstudio.com/getting-started-xiao-rp2350/#hardware-overview
.. _schematic: https://wiki.seeedstudio.com/getting-started-xiao-rp2350/#assets--resources
