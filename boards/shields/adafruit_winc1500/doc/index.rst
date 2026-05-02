.. _adafruit_winc1500:

Adafruit WINC1500 Wifi Shield
#############################

Overview
********

The Adafruit WINC1500 Wifi shield is an Arduino
compatible shield based on the ATWINC1500 wifi controller
from Microchip.
The shield also provides a micro SD card socket

The SD card socket is currently not supported

More information about the shield can be found
at the `Adafruit WINC1500 website`_.

Pins Assignment of the Adafruit WINC1500 WiFi Shield
====================================================

+-----------------------+---------------------------------------------+
| Shield Connector Pin  | Function                                    |
+=======================+=============================================+
| D4                    | MicroSD SPI CSn  (Not supported)            |
+-----------------------+---------------------------------------------+
| D5                    | WINC1500 RST     (/Reset of winc1500)       |
+-----------------------+---------------------------------------------+
| D6 (b)                | WINC1500 EN      (Enable of winc1500)       |
+-----------------------+---------------------------------------------+
| D7                    | WINC1500 IRQ     (IRQ from winc1500)        |
+-----------------------+---------------------------------------------+
| D10                   | WINC1500 SPI CSn                            |
+-----------------------+---------------------------------------------+
| D11 (a)               | SPI MOSI         (Serial Data Input)        |
+-----------------------+---------------------------------------------+
| D12 (a)               | SPI MISO         (Serial Data Out)          |
+-----------------------+---------------------------------------------+
| D13 (a)               | SPI SCK          (Serial Clock Input)       |
+-----------------------+---------------------------------------------+

The pins marked (a) must be jumpered to the SPI port at the shield
To enable low power support, wire the pin marked (b) to the En connector
at the shield

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors and defines node aliases for SPI and GPIO interfaces
(see :ref:`shields` for more details).

Programming
***********

Set ``--shield adafruit_winc1500`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi
   :board: frdm_k64f
   :shield: adafruit_winc1500
   :goals: build

References
**********

.. target-notes::

.. _Adafruit WINC1500 website:
   https://learn.adafruit.com/adafruit-winc1500-wifi-shield-for-arduino
