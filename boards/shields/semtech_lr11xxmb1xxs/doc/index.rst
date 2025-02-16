.. semtech_lr11xxmb1xxs:

Semtech LR11xxMB1xxS LoRa Shields
#################################

Overview
********

Semtech provides a series of Arduino compatible shields based on their LoRa
transceivers LR1110, LR1120, and LR1121.
Those shields are mostly similar and include a LIS2DE12 3-axis i2c accelerometer
from STMicroelectronics.

More information about the shield can be found at the `mbed LR1110MB1xxS
website`_.

Pins Assignment of the Semtech LR1110MB1xxS LoRa Shield
=======================================================

+-------------+---------------------+
| Shield Pin  | Function            |
+=============+=====================+
| A0          | LR1110 NRESET       |
+-------------+---------------------+
| A1          | 32kHz Osc out       |
+-------------+---------------------+
| A2          | LR111x / LR1110     |
+-------------+---------------------+
| A3          | LNA Control         |
+-------------+---------------------+
| A4          | LED TX              |
+-------------+---------------------+
| A5          | LED RX              |
+-------------+---------------------+
| D2          | To Display Shield   |
+-------------+---------------------+
| D3          | LR1110 DIO0 (BUSY)  |
+-------------+---------------------+
| D4          | LED Sniffing        |
+-------------+---------------------+
| D5          | LR1110 DIO9         |
+-------------+---------------------+
| D7          | LR1110 SPI NSS      |
+-------------+---------------------+
| D8          | MEMS Accel INT1     |
+-------------+---------------------+
| D9          | Display D/C         |
+-------------+---------------------+
| D10         | Display CS          |
+-------------+---------------------+
| D11         | LR1110 SPI MOSI     |
+-------------+---------------------+
| D12         | LR1110 SPI MISO     |
+-------------+---------------------+
| D13         | LR1110 SPI SCK      |
+-------------+---------------------+
| D14         | MEMS+EEPROM i2c SDA |
+-------------+---------------------+
| D15         | MEMS+EEPROM i2c SCL |
+-------------+---------------------+

LR1110 and LR1120 based shields use a TCXO and LR1121 based shields use a crystal.

Requirements
************

This shield can only be used with a board which provides a configuration for
Arduino connectors and defines node aliases for SPI and GPIO interfaces (see
:ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=semtech_lr1110mb1xxs`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/class_a
   :board: nucleo_l073rz
   :shield: semtech_lr1110mb1xxs
   :goals: build

References
**********

.. target-notes::

.. _mbed LR1110MB1xxS website:
   https://os.mbed.com/components/LR1110MB1xxS/


License
*******

This document Copyright (c) 2024 Semtech Corporation

SPDX-License-Identifier: Apache-2.0
