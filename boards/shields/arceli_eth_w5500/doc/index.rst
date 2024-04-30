.. _arceli_eth_w5500:

ARCELI W5500 ETH
################

Overview
********

ARCELI W5500 etherner is breakout board with SPI bus access over 10 pin header.
`W5500`_ is 10/100 MBPS stand alone Ethernet controller with on-board MAC & PHY,
16 KiloBytes for FIFO buffer and SPI serial interface.

Pins Assignment of the W5500 Shield
===================================

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


Requirements
************

This shield/breakout board can be used with any board with SPI interfaces in
Arduino header or custom header (by adjusting the overlay).

Programming
***********

Set ``-DSHIELD=arceli_eth_w5500`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/net/dhcpv4_client
   :board: nrf52840dk/nrf52840
   :shield: arceli_eth_w5500
   :goals: build

References
**********

.. target-notes::

.. _W5500:
   https://www.wiznet.io/product-item/w5500/
