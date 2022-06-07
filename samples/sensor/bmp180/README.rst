.. _bmp180:

BMP180 Temperature and Pressure Sensor
###################################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`Bosch BMP180`_ environmental sensor.

.. _Bosch BMP180:
   https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf`

The sample periodically reads temperature and pressure data from the
first available BMP180 device discovered in the system. The sample checks the
sensor in polling mode (without interrupt trigger).

Building and Running
********************

The sample is configured to support BMP180 sensors connected via I2C.
Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "bosch,bmp180";``. See
:dtcompatible:`bosch,bmp180` for the devicetree binding and see below for
examples and common configurations.

If the sensor is not built into your board, start by wiring the sensor pins
according to the connection diagram given in the `BMP180 datasheet`_ at
page 10.

.. _BMP180 datasheet:
   https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf


BMP180 via Arduino I2C pins
===========================

If you wired the sensor to an I2C peripheral on an Arduino header, build and
flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bmp180
   :goals: build flash
   :gen-args: -DDTC_OVERLAY_FILE=arduino_i2c.overlay

The devicetree overlay :zephyr_file:`samples/sensor/bmp180/arduino_i2c.overlay`
works on any board with a properly configured Arduino pin-compatible I2C
peripheral.

Board-specific overlays
=======================

If your board's devicetree does not have a BMP180 node already, you can create
a board-specific devicetree overlay adding one in the :file:`boards` directory.
See existing overlays for examples.

The build system uses these overlays by default when targeting those boards, so
no ``DTC_OVERLAY_FILE`` setting is needed when building and running.

For example, to build for the :ref:`nrf52840dongle_nrf52840` using the
:zephyr_file:`samples/sensor/bmp180/boards/nrf52840dongle_nrf52840.overlay`
overlay provided with this sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bmp180
   :goals: build flash
   :board: nrf52840dongle_nrf52840

Sample Output
=============

The sample prints output to the serial console. BMP180 device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings, assuming that only
one BMP180 sensor is connected to the NRF52840 Dongle board:

.. code-block:: none

   [00:00:00.316,101] <dbg> BMP180.bmp180_chip_init: ID OK
   [00:00:00.322,570] <inf> BMP180: Compensation data:
   AC1 = 8232
   AC2 = -1228
   AC3 = -14378
   AC4 = 33362
   AC5 = 25742
   AC6 = 17764
   B1 = 6515
   B2 = 50
   MB -32768
   MC = -11786
   MD = 2368
   [00:00:00.323,638] <dbg> BMP180.bmp180_chip_init: "BMP180_I2C" OK
   *** Booting Zephyr OS build v2.7.99-ncs1-1  ***
   [00:00:00.328,002] <inf> usb_cdc_acm: Device suspended
   [00:00:00.488,830] <inf> usb_cdc_acm: Device resumed
   [00:00:00.626,007] <inf> usb_cdc_acm: Device configured
   [00:00:06.324,768] <inf> app: started!
   [00:00:06.324,798] <inf> app: Found device "BMP180_I2C", getting sensor data

   temperature: 23.9 C
   pressure: 101277 Pa
   Altitude: 3 m above sea level

   temperature: 23.9 C
   pressure: 101274 Pa
   Altitude: 4 m above sea level