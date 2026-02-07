.. _esp_threadbr_zigbeegw_ethernet:

ESP Thread BR / Zigbee GW Ethernet
##################################

Overview
********

ARCELI W5500 Ethernet is breakout board with SPI bus access over 10 pin header.
`W5500`_ is 10/100 MBPS stand alone Ethernet controller with on-board MAC & PHY,
16 KiloBytes for FIFO buffer and SPI serial interface.

Pins Assignment of the ETH Shield
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

This shield/breakout board can be used with ESP Thread BR / Zigbee GW board
or any board that interfaces the W5500 ethernet chip.

Programming
***********

Set ``--shield esp_threadbr_zigbeegw_ethernet`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/net/dhcpv4_client
   :board: esp_threadbr_zigbeegw/esp32s3/procpu
   :shield: esp_threadbr_zigbeegw_ethernet
   :goals: build

References
**********

.. target-notes::

.. _W5500:
   https://wiznet.io/products/iethernet-chips/w5500
