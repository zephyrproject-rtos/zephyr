.. _ens160_ath2x_sample:

ATH20+BMP280 Board
##################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver and configure the Devicetree for the *ENS160+ATH2x Board* that has both the
`ams ENS160`_ gas sensor and `Aosong AHT20`_ temperature sensor.

.. _Bosch BMP280:
   https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf

.. _Aosong AHT20:
   http://www.aosong.com/en/products-32.html

The gas sensor outputs *Air Quality Index*, *Equivalent CO2* and *Total Volatile Organic Compounds*
The sample periodically reads from the humidity and temperature sensor and applies the values as calibration to the gas sensor.

Building and Running
********************

The devicetree overlay :zephyr_file:`samples/sensor/ens160_ath2x_sample/app.overlay` should work on any board with a properly configured ``i2c0`` bus.
The devicetree must have an enabled node with ``compatible = "ams,ens160";``.



.. _ENS160 datasheet:
   https://www.sciosense.com/wp-content/uploads/documents/SC-001224-DS-7-ENS160-Datasheet.pdf

.. _AHT20 datasheet:
   http://www.aosong.com/userfiles/files/media/Data%20Sheet%20AHT20.pdf


Make sure this node has ``status = "okay";``, then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ens160_ath2x_sample
   :goals: build flash
   :board: esp32



Sample Output
*************

The sample prints output to the serial console. Both of the sensor's device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output:

.. code-block:: none

   *** Booting Zephyr OS build zephyr-v3.2.0-871-g51cf5c5a4aa2  ***
   [00:00:00.435,000] <inf> ENS160: Sensor is in initial start up mode
   [00:00:00.435,000] <inf> app: ENS160+AHT2x board sample started
   [00:00:00.436,000] <wrn> ENS160: Sensor doesn't have a new sample to read - waiting 250ms
   [00:00:00.686,000] <wrn> ENS160: Sensor doesn't have a new sample to read - waiting 250ms
   [00:00:00.937,000] <wrn> ENS160: Sensor doesn't have a new sample to read - waiting 250ms
   [00:00:01.243,000] <inf> app: Relative Humidity (%): 66.009140
   [00:00:01.243,000] <inf> app: Temperature (C): 26.366043
   [00:00:01.244,000] <inf> app: Air Quality Index: 1
   [00:00:01.244,000] <inf> app: Equivalent CO2 (ppm): 401
   [00:00:01.244,000] <inf> app: TVOC (ppb): 24
   [00:00:03.300,000] <inf> app: Relative Humidity (%): 65.822792
   [00:00:03.300,000] <inf> app: Temperature (C): 26.392555
   [00:00:03.301,000] <inf> app: Air Quality Index: 1
   [00:00:03.301,000] <inf> app: Equivalent CO2 (ppm): 400
   [00:00:03.301,000] <inf> app: TVOC (ppb): 19
