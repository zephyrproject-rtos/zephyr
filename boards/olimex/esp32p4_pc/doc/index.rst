.. zephyr:board:: esp32p4_pc

Overview
********

The OLIMEX ESP32-P4-PC is a compact computer board built around the ESP32-P4
SoC and an ESP32-P4NRW32 module with 16 MB flash and 32 MB PSRAM. The board
provides USB Serial/JTAG, USB OTG, Ethernet, microSD, MIPI CSI/DSI connectors,
audio output, and expansion headers for application development.

This board definition provides both high-performance (HP) core and low-power
(LP) core targets. On the HP core, the Zephyr console is routed to the on-chip
USB Serial/JTAG controller.

Hardware
********

The board includes:

- ESP32-P4 SoC paired with 16 MB flash and 32 MB PSRAM
- USB Serial/JTAG for power, flashing and console
- USB OTG connector
- 4 USB host ports (via USB hub)
- 10/100 Ethernet connector (with optional PoE support)
- MIPI CSI camera connector
- MIPI DSI display connector
- a MIPI DSI to HDMI converter
- microSD card slot
- Boot and reset buttons
- LiPo battery charger and step up
- Audio output via 3.5mm jack

.. include:: ../../../espressif/common/soc-esp32p4-features.rst
   :start-after: espressif-soc-esp32p4-features

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ESP32-P4-PC Product Page`: https://www.olimex.com/Products/IoT/ESP32-P4/ESP32-P4-PC/open-source-hardware
.. _`ESP32-P4-PC Hardware Repository`: https://github.com/OLIMEX/ESP32-P4-PC
