.. _veml7700-sample:

VEML7700 Sensor Sample
####################

Overview
********

This sample application retrieves the ambient light data in lux from VEML7700 I2C sensor.

Requirements
************

- Custom board
	- Innblue BlufDALI nRF52840 board
	- USB <-> TTL converter for UART logging
- DK board
	- `nRF52840 Preview development kit`_
	- `Adafruit VEML7700 Ambient Light Sensor`_

Wiring
******

Innblue BlufDALI already has VEML6030 (VEML7700 compatible) sensor on board.
To use nrf52840 Preview development kit it should be connected as follows to the
external VEML7700 sensor board.

+-------------+------------+
| | nrf52840  | | VEML7700 |
| | Pin       | | Pin      |
+=============+============+
| P0.3        | SCL        |
+-------------+------------+
| P0.31       | SDA        |
+-------------+------------+

Building and Running
********************

Build this sample using the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/veml7700
   :board: blufdali_nrf52840
   :goals: build
   :compact:

See :ref:`blufdali_nrf52840` on how to flash the build.

References
**********

.. target-notes::

.. _nRF52840 Preview development kit: http://www.nordicsemi.com/eng/Products/nRF52840-Preview-DK
.. _Adafruit VEML7700 Ambient Light Sensor: https://learn.adafruit.com/adafruit-veml7700
