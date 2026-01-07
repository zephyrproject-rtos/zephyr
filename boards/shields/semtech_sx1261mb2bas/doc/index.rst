.. _semtech_sx1261mb2bas:

Semtech SX1261MB2BAS LoRa Shield
################################

Overview
********

The Semtech SX1261MB2BAS LoRa shield is an Arduino
compatible shield based on the SX1261 LoRa transceiver
from Semtech.

More information about the shield can be found
at the `mbed SX126xMB2xAS website`_.

Pins Assignment of the Semtech SX1261MB2BAS LoRa Shield
=======================================================

+-----------------------+-----------------+
| Shield Connector Pin  | Function        |
+=======================+=================+
| A0                    | SX1261 RESET    |
+-----------------------+-----------------+
| D3                    | SX1261 BUSY     |
+-----------------------+-----------------+
| D5                    | SX1261 DIO1     |
+-----------------------+-----------------+
| D7                    | SX1261 SPI NSS  |
+-----------------------+-----------------+
| D8                    | SX1261 ANT SW   |
+-----------------------+-----------------+
| D11                   | SX1261 SPI MOSI |
+-----------------------+-----------------+
| D12                   | SX1261 SPI MISO |
+-----------------------+-----------------+
| D13                   | SX1261 SPI SCK  |
+-----------------------+-----------------+

The SX1261 signals DIO2 and DIO3 are not available at the shield connector.

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors (see :ref:`shields` for more details).

Programming
***********

Set ``--shield semtech_sx1261mb2bas`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/class_a
   :board: nucleo_f429zi
   :shield: semtech_sx1261mb2bas
   :goals: build

References
**********

.. target-notes::

.. _mbed SX126xMB2xAS website:
   https://os.mbed.com/components/SX126xMB2xAS/
