.. _wiznet_wiz550io:

WIZnet WIZ550io
###############

Overview
********

WIZnet `WIZ550io`_ is a W5500 Ethernet breakout board with an extra PIC12F519
MCU. The PIC12F519 MCU initialises the W5500 with a unique MAC address after
a GPIO reset.

`W5500`_  is 10/100 MBPS stand alone Ethernet controller with on-board MAC &
PHY, 16 KiloBytes for FIFO buffer and SPI serial interface.

Pins Assignment of the WIZ550io Shield
======================================

+-----------------------+---------------------------------------------+
| Shield Connector Pin  | Function                                    |
+=======================+=============================================+
| RST#                  | Ethernet Controller's Reset                 |
+-----------------------+---------------------------------------------+
| CS#                   | SPI's Chip Select                           |
+-----------------------+---------------------------------------------+
| SCK                   | SPI's ClocK                                 |
+-----------------------+---------------------------------------------+
| SDO                   | SPI's Slave Data Output  (MISO)             |
+-----------------------+---------------------------------------------+
| SDI                   | SPI's Slave Data Input   (MISO)             |
+-----------------------+---------------------------------------------+
| INT                   | Ethernet Controller's Interrupt Output      |
+-----------------------+---------------------------------------------+
| RDY                   | WIZ550io Ready Output                       |
+-----------------------+---------------------------------------------+

Requirements
************

This shield/breakout board can be used with any board with SPI interfaces in
Arduino header or custom header (by adjusting the overlay).

Usage
*****

Set ``--shield wiznet_wiz550io`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/net/dhcpv4_client
   :board: nrf52840dk/nrf52840
   :shield: wiznet_wiz550io
   :goals: build

References
**********

.. target-notes::

.. _WIZ550io:
   https://wiznet.io/products/network-modules/wiz550io

.. _W5500:
   https://wiznet.io/products/iethernet-chips/w5500
