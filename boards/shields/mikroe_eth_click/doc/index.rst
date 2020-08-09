.. _mikroe_eth_click:

MikroElektronika ETH Click
##########################

Overview
********

ETH Click is an accessory board in mikroBus™ form factor. It features `ENC28J60`_,
a 28-pin, 10BASE-T stand alone Ethernet Controller with an on-board MAC & PHY,
8K Bytes of Buffer RAM and SPI serial interface.
More information at `Eth Click Shield website`_.

Pins Assignment of the Eth Click Shield
=======================================

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

This shield can only be used with a board which provides a configuration
for Mikro-BUS connectors and defines node aliases for SPI and GPIO interfaces
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=mikroe_eth_click`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/net/dhcp_client
   :board: lpcxpresso55s69
   :shield: mikroe_eth_click
   :goals: build

References
**********

.. target-notes::

.. _Eth Click Shield website:
   https://www.mikroe.com/eth-click

.. _ENC28J60:
   https://www.microchip.com/wwwproducts/en/en022889
