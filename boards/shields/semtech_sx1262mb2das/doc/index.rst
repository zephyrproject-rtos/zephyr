.. _semtech_sx1262mb2das:

Semtech SX1262MB2DAS LoRa Shield
################################

Overview
********

The Semtech SX1262MB2DAS LoRa shield is an Arduino
compatible shield based on the SX1262 LoRa transceiver
from Semtech.

More information about the shield can be found
at the `mbed SX1262MB2xAS website`_.

Pins Assignment of the Semtech SX1262MB2DAS LoRa Shield
=======================================================

+-----------------------+-----------------+
| Shield Connector Pin  | Function        |
+=======================+=================+
| A0                    | SX1262 RESET    |
+-----------------------+-----------------+
| D3                    | SX1262 BUSY     |
+-----------------------+-----------------+
| D5                    | SX1262 DIO1     |
+-----------------------+-----------------+
| D7                    | SX1262 SPI NSS  |
+-----------------------+-----------------+
| D8                    | SX1262 ANT SW   |
+-----------------------+-----------------+
| D11                   | SX1262 SPI MOSI |
+-----------------------+-----------------+
| D12                   | SX1262 SPI MISO |
+-----------------------+-----------------+
| D13                   | SX1262 SPI SCK  |
+-----------------------+-----------------+

The SX1262 signals DIO2 and DIO3 are not available at the shield connector.

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors (see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=semtech_sx1262mb2das`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/class_a
   :board: nucleo_f429zi
   :shield: semtech_sx1262mb2das
   :goals: build

References
**********

.. target-notes::

.. _mbed SX1262MB2xAS website:
   https://os.mbed.com/components/SX126xMB2xAS/
