.. zephyr:board:: sama7d65_curiosity

Overview
********

The EV63J76A (SAMA7D65 Curiosity Kit) is a development kit for evaluating and
prototyping with Microchip SAMA7D65 microprocessor (MPU). The SAMA7D65 MPU is a
high-performance ARM Cortex-A7 CPU-based embedded MPU running up to 1GHz.

The board allows evaluation of powerful peripherals for connectivity, audio and
user interface applications, including MIPI-DSI and LVDS w/ 2D graphics, dual
Gigabit Ethernet w/ TSN and CAN-FD.  The MPUs offer advanced security functions,
like tamper detection, secure boot, secure key stoarge, TRNG, PUF as well as
higher-performance crypto accelerators for AES and SHA.

The SAMA7D65 series is supported by Microchip MPLAB-X development tools, Harmony
V3, Linux distributions and Microchip Graphic Suite (MGS) for Linux. The
SAMA7D65 is well-suited for industrial and automotive applications with
graphical displays support up to WXGA/720p.

Hardware
********
EV63J76A provides the following hardware components:

- Processor

  - Microchip SAMA7D65-V/4HB (SoC 343-ball TFBGA, 14x14 mm, 0.65 mm pitch)

- Memory

  - 8 Gb DDR3L (AS4C512M16D3LA-10BIN)
  - 64 Mb QSPI NOR Flash with EUI-48 (Microchip SST26VF064BEUI-104I/MF)
  - 4Gbit SLC NAND Flash (MX30LF4G28AD-XKI)
  - 2Kb EEPROM with EUI-48 (Microchip 24AA025E48)

- SD/MMC

  - One SD card socket, 4 bit
  - One M.2 Radio Module interface, SDIO I/F

- USB

  - One device USB Type-C connector
  - Two host USB Type-A connectors (Microchip MIC2026-1YM)

- Ethernet

  - One 10/100/1000 RGMII on board
  - One 10/100/1000 RGMII SODIMM add-on slot (Microchip LAN8840-V/PSA)

- Display

  - One MIPI-DSI 34-pin FPC connector
  - One LVDS 30-pin FPC connector

- Debug port

  - One UART Debug connector
  - One JTAG interface

- User interaction

  - One RGB (Red, Green, Blue) LED
  - Four push button switches

- CAN-FD

  - Three onboard CAN-FD transceivers
  - Two interfaces available on mikroBUS slots

- Expansion

  - Raspberry Pi 40-pin GPIO connector
  - Two mikroBUS™ connectors
  - Two PIOBU/System headers

- Power management

  - Power Supply Unit (Microchip MCP16502TAB-E/S8B)
  - Power Monitoring (Microchip PAC1934)
  - Daughter cards power supply (Microchip MIC23450-AAAYML-TR)
  - Backup power supply (CR1220 battery holder)

- Board supply

  - System 5 VDC from USB Type-C
  - System 5 VDC from DC Jack

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `SAMA7D65-Curiosity Kit User Guide`_ has detailed information about board connections.

References
**********

SAMA7D65 Product Page:
    https://www.microchip.com/en-us/product/sama7d65

SAMA7D65 Curiosity Kit Page:
    https://www.microchip.com/en-us/development-tool/EV63J76A

.. _SAMA7D65-Curiosity Kit User Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MPU32/ProductDocuments/UserGuides/SAMA7D65-Curiosity-Kit-User-Guide-DS50003806.pdf
