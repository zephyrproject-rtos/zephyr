.. _ds18b20_sample:

DS18B20 1-Wire Temperature Sensor
#################################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`Maxim DS18B20`_ temperature sensor.

.. _Maxim DS18B20:
   https://www.maximintegrated.com/en/products/sensors/DS18B20.html

The sample periodically reads temperature data from the
first available DS18B20 device discovered in the system. The sample checks the
sensor in polling mode (without interrupt trigger).

Building and Running
********************

The devicetree must have an enabled node with ``compatible = "maxim,ds18b20";``.
See below for examples and common configurations.

If the sensor is not built into your board, start by wiring the sensor pins
as shown in the Figure Hardware Configuration of the `DS18B20 datasheet`_ at
page 10.

.. _DS18B20 datasheet:
   https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf

Boards with a built-in DS18B20 or a board-specific overlay
==========================================================

Your board may have a DS18B20 node configured in its devicetree by default,
or a board specific overlay file with an DS18B20 node is available.
Make sure this node has ``status = "okay";``, then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ds18b20
   :goals: build flash
   :board: nucleo_g0b1re

DS18B20 via Arduino Serial pins
===============================

Make sure that you have an external circuit to provide an open-drain interface
for the 1-Wire bus.
Once you have wired the sensor and the serial peripheral on the Arduino header
to the 1-Wire bus, build and flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ds18b20
   :goals: build flash
   :gen-args: -DDTC_OVERLAY_FILE=arduino_serial.overlay

The devicetree overlay :zephyr_file:`samples/sensor/ds18b20/arduino_serial.overlay`
should work on any board with a properly configured Arduino pin-compatible Serial
peripheral.

Sample Output
=============

The sample prints output to the serial console. DS18B20 device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings, assuming that only
one DS18B20 sensor is connected to the standard Arduino Serial pins:

.. code-block:: none

   *** Booting Zephyr OS build zephyr-v2.6.0-1929-gf7abe4a6689e  ***
   Found device "DS18B20", getting sensor data
   [00:00:00.000,039] <dbg> w1_serial: w1_serial_init: w1-serial initialized, with 1 devices
   [00:00:00.015,140] <dbg> DS18B20: ds18b20_init: Using external power supply: 1
   [00:00:00.021,213] <dbg> DS18B20: ds18b20_init: Init DS18B20: ROM=28b1bb3f070000b9

   Temp: 25.040000
   Temp: 25.030000
