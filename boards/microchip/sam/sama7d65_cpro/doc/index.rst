.. zephyr:board:: sama7d65_cpro

Overview
********

The SAMA7D65 Curiosity Pro (EV75M38A) kit is designed for evaluating and
prototyping with the Microchip SAMA7D65 microprocessor (MPU). Based on the
SAMA7D65-V/4HB-SL3, it provides a feature-rich platform for memory, Ethernet,
display, USB, CAN FD, wireless, and debug evaluation.

The kit includes a VSC8541 EDS2 Gigabit Ethernet daughter card, an OpenOCD debug
probe, and two mikroBUS™ connectors for expanding with Click board™ modules
featuring Microchip functions. It also supports MIPI DSI® and LVDS displays,
dual Ethernet options, and a wide range of interfaces for system development.

Hardware
********

- SAMA7D65-V/4HB-SL3 Microprocessor
- 8 Gb DDR3L (EM6HF16EBXC-12SH)
- 64 Mb QSPI NOR Flash with EUI-48 (Microchip SST26VF064BEUI-104I/MF)
- 4 Gb SLC NAND Flash (MX30LF4G28AD-XKI)
- 2 Kb EEPROM with EUI-48 (Microchip 24AA025E48)
- One 4-bit SD card socket
- One M.2 radio module with SDIO interface
- One device USB Type-C® connector
- Two host USB Type-A connectors
- One 10/100/1000 RGMII on board (Microchip VSC8541XMV)
- One 10/100/1000 RGMII SODIMM add-on slot
- One MIPI CSI® 34-pin FPC connector
- One 30-pin ISC connector
- One UART debug connector (FTDI-compatible)
- One on-board USB-C®-to-UART converter (Microchip MCP2221)
- One JTAG interface
- One RGB (Red, Green, Blue) LED
- Four push-button switches
- Three on-board CAN FD interfaces
- Two CAN FD interfaces available on mikroBUS™ slots
- Raspberry Pi® 40-pin GPIO connector
- Two mikroBUS™ connectors
- Power supply unit (Microchip MCP16502TAB-E/S8B)
- Power monitoring (Microchip PAC1934)
- Daughter card power supply (Microchip MIC23450)
- Backup power supply (CR1220 battery holder)

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `SAMA7D65 Curiosity Pro Kit User Guide`_ has detailed information about board connections.

Programming
***********

Prerequisite
============

Before Zephyr runs on SAMA7D65 Curiosity Pro, `at91bootstrap`_ (the second stage
bootloader for Microchip MPU) needs to run first. It will initialize the
peripherals and memory controllers, download and jump to Zephyr entry point.

References
**********

SAMA7D65 Product Page:
    https://www.microchip.com/en-us/product/sama7d65

SAMA7D65 Curiosity Pro Kit Page:
    https://www.microchip.com/en-us/development-tool/ev75m38a

.. _SAMA7D65 Curiosity Pro Kit User Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MPU32/ProductDocuments/UserGuides/SAMA7D65-Curiosity-Pro-User-Guide-DS50004143.pdf

.. _at91bootstrap:
    https://developerhelp.microchip.com/xwiki/bin/view/products/mcu-mpu/32bit-mpu/at91bootstrap/
