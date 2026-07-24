.. zephyr:board:: sama5d27_som1_ek1

Overview
********

The ATSAMA5D27-SOM1-EK1 is a fast prototyping and evaluation platform for the
SAMA5D2 based System in Packages (SiPs) and the SAMA5D27-SOM1 (SAMA5D27 System
On Module).
The kit comprises a baseboard with a soldered ATSAMA5D27-SOM1 module. The module
features an ATSAMA5D27C-D1G-CU SIP embedding a 1-Gbit (128 MB) DDR2 DRAM.  The
SOM integrates a Power Management IC (PMIC), a QSPI memory, a 10/100 Mbps
Ethernet PHY and a serial EEPROM with a MAC address. 128 GPIO pins are provided
by the SOM for general use in the system. The board features a wide range of
peripherals, as well as a user interface and expansion options, including two
mikroBUS™ click interface headers to support MIKROE Click boards™ and one PMOD™
interface. Linux distribution and software package allows you to easily get
started with your development.

Hardware
********

- a soldered SAMA5D27 SOM1 module
  - a SAMA5D27-D1G-CU SIP embedding a 1-Gbit DDR2 SDRAM
  - a QSPI memory
  - a 10/100 Mbps Ethernet controller
  - 128 GPIO pins
- Pad for one QSPI Flash (unmounted)
- One CryptoAuthentication™ device ATECC608
- One USB host (Connector type C)
- One USB device (Connector type microAB)
- One USB HSIC (2-pin header, not populated)
- One Ethernet interface (RJ45 connector)
- One CAN interface (ATA6561)
- One LCD RGB 24-bit interface (50-pin FPC connector)
- One ISC 12-bit interface (2x15 male connector)
- One standard SD card interface (With 3.3V/1.8V power switch)
- One microSD card interface
- One J-Link-OB and J-Link-CDC (Microchip SAM3U micro-controller with embedded J-Link firmware)
- One JTAG interface
- One RGB (Red, Green, Blue) LED
- Four push button switches (Power ON, Reset, Wakeup, User Free)
- One tamper connector (10-pin male connector)
- One Pmod connector (6-pin female connector)
- Two mikroBUS interfaces (2x8-pin female connector)
- Board Supply From USB A and/or USB J-Link-OB 5 VDC
- SuperCap for Power saving

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `SAMA5D27-SOM1-EK1 User Guide`_ has detailed information about board connections.

References
**********

SAMA5D2 Series Page:
    https://www.microchip.com/en-us/products/microprocessors/32-bit-mpus/sama5/sama5d2-series

SAMA5D27-SOM1 Product Page:
    https://www.microchip.com/en-us/product/ATSAMA5D27-SOM1

SAMA5D27 SOM1 Kit1 Page:
    https://www.microchip.com/en-us/development-tool/ATSAMA5D27-SOM1-EK1

.. _SAMA5D27-SOM1-EK1 User Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MPU32/ProductDocuments/UserGuides/SAMA5D27-SOM1-Kit1-User%27s-Guide-DS50002667.pdf
