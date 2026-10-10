.. zephyr:board:: sam9x75_curiosity

Overview
********

The SAM9X75 Curiosity Development Board features a SAM9X75D2G MPU and is the
evaluation platform for the SAM9X7 Series MPU devices. The SAM9X75D2G is a
high-performance, ultra-low power ARM926EJ-S CPU-based embedded microprocessor
(MPU) running up to 800 MHz, with an integrated 2 Gbit DDR3L memory. The device
integrates powerful peripherals for connectivity and user interface
applications, including MIPI DSI®, LVDS, RGB and 2D graphics, MIPI-CSI-2,
Gigabit Ethernet with TSN, USB and CAN-FD. Advanced security functions are
offered, such as tamper detection, secure boot, secure key storage, TRNG,
PUF as well as high-performance crypto accelerators for AES and SHA.

The SAM9X75 Curiosity Development Board includes 4Gbit SLC NAND Flash, a 64Mb
QSPI Nor Flash, a Gigabit Ethernet interface through SODIMM 260-pin connector,
a LAN8840 EDS2 Daughter Card to evaluate Microchip LAN8840 Gigabit Ethernet
RGMII Ethernet PHY, MCP16502 Power Management IC optimized for the module and
one extra DC/DC converter to supply interfaces such as the Gigabit Ethernet
interface and the Wireless M.2 interface.

Hardware
********

- Microchip SAM9X75D2G-I/4TB Microprocessor (SiP)
- 24 MHz oscillator MEMS clock
- 32.768 kHz crystal
- 125 MHz reference clock generator for GMAC
- 25 MHz clock generator for ETH PHY
- 32.768 kHz clock generator for wireless M.2 Key E
- 2 Gb DDR3L (inside the SiP)
- One 4 Gb NAND Flash (Macronix MX30LF4G28AD-XKI)
- One 64 Mb QSPI NOR Flash (Microchip SST26VF064BEUIT-104I/MF)
- One microSD card interface
- One M.2 Key E connector/radio module interface
- One Micro-B USB device
- Two USB Type-A connectors (Microchip MIC2026-1YM)
- One Gigabit Ethernet interface through SODIMM 260-pin connector
- One Mono single CLASS D amplifier
- One LVDS connector
- One MIPI camera connector
- One UART debug connector
- One JTAG interface
- One RGB (Red, Green, Blue) LED
- Four push button switches
- Raspberry Pi® 40-pin GPIO connector(1)
- One mikroBUS™ connector with PTA
- Current sense (Microchip PAC1934T-I/JQ)
- Power supply unit (Microchip MCP16502TAB-E/S8B)
- System 5V DC from jack or USB A

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `SAM9X75-Curiosity User Guide`_ has detailed information about board connections.

References
**********

SAM9X7 Product Page:
    https://www.microchip.com/en-us/products/microprocessors/32-bit-mpus/sam9/sam9x7

SAM9X75 Curiosity LAN Kit Page:
    https://www.microchip.com/en-us/development-tool/ev31h43a

.. _SAM9X75-Curiosity User Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MPU32/ProductDocuments/UserGuides/SAM9X75-Curiosity-User-Guide-DS60001859.pdf
