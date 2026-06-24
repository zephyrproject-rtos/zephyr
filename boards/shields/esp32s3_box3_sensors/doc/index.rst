.. _esp32s3_box3_sensors_shield:

ESP32-S3-BOX-3 Sensor Accessories Dock Shield
##############################################

Overview
********

The ESP32-S3-BOX-3 Sensor Accessories Dock Shield enables sensor support for the
`ESP32-S3-BOX-3`_ board. It configures the I2C1 bus on GPIO40 (SCL) and GPIO41 (SDA)
and exposes an `Aosong AHT30`_ temperature and humidity sensor at address ``0x38``.

Requirements
************

This shield is designed exclusively for the `ESP32-S3-BOX-3`_ board
(``esp32s3_box3/esp32s3/procpu`` qualifier). The board must have I2C1 available
on the GPIO40/GPIO41 pins, which matches the default ESP32-S3-BOX-3 hardware layout.

Pin Assignments
===============

+-------+----------+
| Pin   | Function |
+=======+==========+
| GPIO41| I2C1 SDA |
+-------+----------+
| GPIO40| I2C1 SCL |
+-------+----------+

Sensors
=======

+--------+-----------------+-------------+
| Device | Compatible      | I2C Address |
+========+=================+=============+
| AHT30  | aosong,aht30    | 0x38        |
+--------+-----------------+-------------+

The AHT30 provides calibrated temperature and relative humidity measurements over I2C.
Refer to the `Aosong AHT30`_ datasheet for full accuracy specifications.

Programming
***********

Set ``-DSHIELD=esp32s3_box3_sensors`` when invoking ``west build``. For example,
using the :zephyr:code-sample:`dht_polling` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/dht_polling
   :board: esp32s3_box3/esp32s3/procpu
   :shield: esp32s3_box3_sensors
   :goals: build flash

References
**********

.. _Aosong AHT30:
   https://asairsensors.com/product/aht30-integrated-temperature-and-humidity-sensor/

.. _ESP32-S3-BOX-3:
   https://www.espressif.com/en/dev-board/esp32-s3-box-3-en
