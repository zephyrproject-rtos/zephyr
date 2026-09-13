.. zephyr:board:: glyph_mini_2040

Overview
********

Glyph-Mini-2040 is a compact development board based on the Raspberry Pi
RP2040 dual-core Arm Cortex-M0+ processor, with 8 MB of on-board W25Q64
SPI flash. This board is intended for IoT, robotics, and general embedded
prototyping.

For more information, check `Glyph-Mini-2040`_.

Hardware
********

- Dual-core Arm Cortex-M0+ processor running up to 133MHz
- 264KB on-chip SRAM
- 8MB on-board W25Q64 SPI flash with XIP capabilities
- 20 broken-out multi-function GPIO pins (GP0-GP15, GP26-GP29)
- 4 Analog inputs
- 2 UART peripherals
- 2 SPI controllers
- 2 I2C controllers
- USB 1.1 controller (host/device), Type-C connector
- On-board status LED (GP16, internal)
- BOOT and RESET buttons

Supported Features
===================

.. zephyr:board-supported-hw::

Programming and Debugging
**************************

Applications for the ``glyph_mini_2040`` board can be built in the usual way,
as documented in :ref:`build_an_application`.

The board comes with a built-in USB UF2 bootloader. Hold the BOOT button
while connecting via USB (or hold BOOT while pressing RESET) to expose it
as a mass storage device for drag-and-drop ``.uf2`` flashing.

.. target-notes::

.. _Glyph-Mini-2040:
   https://learn.pcbcupid.com/documentation/modules/glyph-mini/glyph-mini-2040/overview
