.. zephyr:board:: xiao_rp2040

Overview
********

The XIAO RP2040 is an IoT mini development board from Seeed Studio.
It is equipped with an RP2040 SoC, an on-board WS2812 addressable
LED, and USB connector. The USB bootloader allows it
to be flashed without any adapter, in a drag-and-drop manner.

For more details see the `Seeed Studio XIAO RP2040`_ wiki page.

Hardware
********

The Seeed Studio XIAO RP2040 is a low-power microcontroller that
carries the powerful Dual-core RP2040 processor with a flexible
clock running up to 133 MHz. There is also 264KB of SRAM, and 2MB of
on-board Flash memory.

There are 14 GPIO PINs on Seeed Studio XIAO RP2040, on which there
are 11 digital pins, 4 analog pins, 11 PWM Pins,1 I2C interface,
1 UART interface, 1 SPI interface, 1 SWD Bonding pad interface.

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
- I2C1_SDA : P6
- I2C1_SCL : P7
- SPI0_RX : P4
- SPI0_SCK : P2
- SPI0_TX : P3

Connections and IOs
===================

The board uses a standard XIAO pinout, the default pin mapping is the following:

.. figure:: img/xiao_rp2040_pinout.webp
   :align: center
   :alt: XIAO RP2040 Pinout

   XIAO RP2040 Pinout

Programming and Debugging
*************************

Flashing
========

Using UF2
---------

You can flash the Xiao RP2040 with a UF2 file.
By default, building an app for this board will generate a
:file:`build/zephyr/zephyr.uf2` file. If the Xiao RP2040 is powered on with
the ``BOOTSEL`` button pressed, it will appear on the host as a mass storage
device. The UF2 file should be copied to the device, which will
flash the Xiao RP2040.

References
**********

.. target-notes::

.. _`Seeed Studio XIAO RP2040`: https://wiki.seeedstudio.com/XIAO-RP2040/
