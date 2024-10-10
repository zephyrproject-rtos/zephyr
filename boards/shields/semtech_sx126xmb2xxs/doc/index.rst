.. semtech_sx126xmb2xxs:

Semtech SX126xMB2xxS LoRa Shields
#################################

Overview
********

Semtech provides a series of Arduino compatible shields based on their LoRa
transeivers SX1261, SX1262, and SX126
Those shields are mostly similar and have Grove compatible connectors passthroughs.

More information about the shield can be found at the `mbed SX1261MB2xxs
website`_.

Pins Assignment of the Semtech SX126xMB2xxS LoRa Shield
=======================================================

+-------------+---------------------+
| Shield Pin  | Function            |
+=============+=====================+
| A0          | SX1261 NRESET       |
+-------------+---------------------+
| A1          | FR0/FR1             |
+-------------+---------------------+
| A2          | SX1261 /            |
+-------------+---------------------+
| A3          | OPT (XTAL / TCXO)   |
+-------------+---------------------+
| A4          | Grove A5            |
+-------------+---------------------+
| A5          | Grove A6            |
+-------------+---------------------+
| D3          | SX1261 BUSY         |
+-------------+---------------------+
| D5          | SX1261 DIO1         |
+-------------+---------------------+
| D7          | SX1261 SPI NSS      |
+-------------+---------------------+
| D8          | Antenna Switch      |
+-------------+---------------------+
| D9          | Grove D9            |
+-------------+---------------------+
| D10         | Grove D10           |
+-------------+---------------------+
| D11         | SX1261 SPI MOSI     |
+-------------+---------------------+
| D12         | SX1261 SPI MISO     |
+-------------+---------------------+
| D13         | SX1261 SPI SCK      |
+-------------+---------------------+
| D14         | Grove i2c SDA       |
+-------------+---------------------+
| D15         | Grove i2c SCL       |
+-------------+---------------------+

Requirements
************

This shield can only be used with a board which provides a configuration for
Arduino connectors and defines node aliases for SPI and GPIO interfaces (see
:ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=semtech_sx126xmb2xxs`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/class_a
   :board: nucleo_l073rz
   :shield: semtech_sx126xmb2xxs
   :goals: build

References
**********

.. target-notes::

.. _mbed SX126xMB2xxS website:
   https://os.mbed.com/components/SX126xMB2xxS/


License
*******

This document Copyright (c) 2024 Semtech Corporation

SPDX-License-Identifier: Apache-2.0
