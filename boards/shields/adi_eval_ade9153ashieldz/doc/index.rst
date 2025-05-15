.. _adafruit_data_logger_shield:

Adafruit Data Logger Shield
###########################

Overview
********

The `Adafruit Data Logger Shield`_ rev. B features an `NXP PCF8523 Real-Time Clock/Calendar with
Battery Backup`_, an SD card interface, two user LEDs, and a prototyping area.

.. figure:: adafruit_data_logger.jpg
   :align: center
   :alt: Adafruit Data Logger Shield

   Adafruit Data Logger Shield (Credit: Adafruit)

.. note::
   The older revision A of the Adafruit Data Logger Shield is not supported.

Pin Assignments
===============

+-----------------------+---------------------------------------------+
| Shield Connector Pin  | Function                                    |
+=======================+=============================================+
| D3                    | User LED1 (green) [1]_                      |
+-----------------------+---------------------------------------------+
| D4                    | User LED2 (red) [1]_                        |
+-----------------------+---------------------------------------------+
| D7                    | PCF8523 RTC INT1 [2]_                       |
+-----------------------+---------------------------------------------+
| D10                   | SD card SPI CS                              |
+-----------------------+---------------------------------------------+
| D11                   | SD card SPI MOSI                            |
+-----------------------+---------------------------------------------+
| D12                   | SD card SPI MISO                            |
+-----------------------+---------------------------------------------+
| D13                   | SD card SPI SCK                             |
+-----------------------+---------------------------------------------+
| SDA                   | PCF8523 RTC I2C SDA                         |
+-----------------------+---------------------------------------------+
| SCL                   | PCF8523 RTC I2C SCL                         |
+-----------------------+---------------------------------------------+

.. [1] The user LEDs are not connected to ``D3`` and ``D4`` by default. Jumper or jumper wire
       connections must be established between the ``L1`` and ``Digital I/O 3`` pins for ``LED1``
       and ``L2`` and ``Digital I/O 4`` pins for ``LED2`` if they are to be used.

.. [2] The PCF8523 RTC ``INT1`` interrupt output pin is not connected to ``D7`` by default. A jumper
       wire connection must be established between the ``SQ`` pin and the ``Digital I/O 7`` pin in
       order to use the RTC interrupt functionality (i.e. alarm callback, 1 pulse per second
       callback). The ``INT1`` interrupt output is open-drain, but the shield definition enables an
       internal GPIO pull-up and thus no external pull-up resistor is needed.

Requirements
************

This shield can only be used with a board which provides a configuration for Arduino connectors and
defines node aliases for SPI and GPIO interfaces (see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=adafruit_data_logger`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/rtc/rtc_api
   :board: frdm_k64f
   :shield: adafruit_data_logger
   :goals: build

.. _Adafruit Data Logger Shield:
   https://learn.adafruit.com/adafruit-data-logger-shield/

.. _NXP PCF8523 Real-Time Clock/Calendar with Battery Backup:
   https://www.nxp.com/docs/en/data-sheet/PCF8523.pdf
