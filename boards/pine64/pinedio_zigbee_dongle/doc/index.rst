.. zephyr:board:: pinedio_zigbee_dongle

Overview
********

The `PineDio USB-C Zigbee Dongle`_ is a small USB-C stick from Pine64 built around the
Bouffalo Lab BL706 (QFN48, BL706C-22 with 2 MB SiP flash and 2MB SiP PSRAM) RISC-V wireless SoC.
It is intended as a Zigbee 3.0 coordinator/router for home-automation hubs.

.. note::

   This board is the same hardware as the **ThirdReality Zigbee 3.0 USB
   Dongle**. Use ``pinedio_zigbee_dongle`` for either product.

Hardware
********

- Bouffalo Lab BL706C22
- 32 MHz and 32.768 kHz crystals
- WCH CH340E USB-to-UART bridge on USB-C connected to GPIO14 and GPIO15
- RGB status LED (GPIO10, 16, and 12)
- Boot select button (GPIO31, also readable inverted via GPIO17) accessible via pin hole
- Test points for GPIO18 and 19

Supported Features
==================

.. zephyr:board-supported-hw::

The PSRAM is not yet supported.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: pinedio_zigbee_dongle
   :goals: build

Flashing
========

Press the pin-hole boot button while plugging the dongle in to enter the ROM
bootloader, then run:

.. code-block:: console

   west flash

Console
=======

The Zephyr console is on UART0, available through the CH340E USB-serial port at 115200 baud.

References
**********

.. target-notes::

.. _PineDio USB-C Zigbee Dongle: https://wiki.pine64.org/wiki/Pinedio_Zigbee

.. _Schematic REV0.7: https://pine64.sfo3.cdn.digitaloceanspaces.com/doc/PineDio/PineDio%20Zigbee%20Dongle%20Schematic%20REV0.7.PDF

.. _ThirdReality Zigbee 3.0 USB Dongle: https://github.com/thirdreality/ThirdReality-Zigbee-3.0-USB-dongle
