.. zephyr:board:: sama7g54_cpro

Overview
********

The SAMA7G54 Curiosity Pro kit is designed for evaluating and prototyping with
the Microchip SAMA7G54 microprocessor (MPU). Based on the SAMA7G54-V/4HB-SL3,
the kit provides a feature-rich platform for memory, Ethernet, display, USB,
CAN FD, wireless, and debug evaluation.

The SAMA7G54 Curiosity Pro board itself features on-board and expansion Ethernet
interfaces, MIPI CSI® and ISC display interfaces, USB host and device
connectivity, CAN FD, an M.2 radio-module interface, Raspberry Pi®-compatible
GPIO expansion, and two mikroBUS™ connectors.

Hardware
********

- SAMA7G54-V/4HB Microprocessor
- 128Mb 16-bit low-power mobile DRAM (EM6HF16EBXC)
- 64 Mb QSPI NOR Flash with EUI-48 (Microchip SST26VF064BEUI-104I/MF)
- 4 Gb SLC NAND Flash (MX30LF4G28AD-XKI)
- 2 Kb EEPROM with EUI-48 (Microchip 24AA025E48)
- One 4-bit SD card socket
- One M.2 radio module with SDIO interface
- One device USB Type-C® connector
- Two host USB Type-A connectors
- One 10/100 RMII on board (Microchip VSC8541XMV)
- One 10/100/1000 RGMII SODIMM add-on slot
- One MIPI CSI® 34-pin FPC connector
- One 30-pin ISC connector
- One UART debug connector (FTDI-compatible)
- One on-board USB-C®-to-UART converter (Microchip MCP2221)
- One JTAG interface
- One RGB (Red, Green, Blue) LED
- Four push-button switches
- Three CAN interfaces
- 40-pin Raspberry Pi expansion header
- 2 mikroBUS™ connectors
- Power supply unit (Microchip MCP16502TAB-E/S8B)
- Power monitoring (Microchip PAC1934)
- Daughter card power supply (Microchip MIC23450)
- Backup power supply (CR1220 battery holder)

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Programming
***********

Prerequisite
============

Before Zephyr runs on SAMA7G54 Curiosity Pro, `at91bootstrap`_ (the second stage
bootloader for Microchip MPU) needs to run first. It will initialize the
peripherals and memory controllers, download and jump to Zephyr entry point.

Loading the firmware
====================

Using a bootloader compiled with sama7g54_curiosity_prosd_zephyr_defconfig
configuration, the binary for Zephyr (zephyr.bin) copied to a SD card can be
loaded and run on the SAMA7G54-CPRO board.

References
**********

SAMA7G54 Product Page:
    https://www.microchip.com/en-us/product/sama7g54

SAMA7G54 Curiosity Pro Kit Page:
    https://www.microchip.com/en-us/development-tool/ev53h00a

.. _at91bootstrap:
    https://developerhelp.microchip.com/xwiki/bin/view/products/mcu-mpu/32bit-mpu/at91bootstrap/
