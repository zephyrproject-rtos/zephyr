.. _wsen-hids:

WSEN-HIDS: Humidity and Temperature Sensor
##########################################

Overview
********

This sample uses the Zephyr :ref:`sensor_api` API driver to periodically
read humidity and temperature from the Würth Elektronik WSEN-HIDS
humidity & temperature sensor and displays it on the console.

By default, samples are read in polling mode. If desired, the data-ready
interrupt of the sensor can be used to trigger reading of samples.

Requirements
************

This sample requires a WSEN-HIDS sensor connected via the I2C or SPI interface.

References
**********

- WSEN-HIDS: https://www.we-online.com/catalog/en/WSEN-HIDS

Building and Running
********************

This sample can be configured to support WSEN-HIDS sensors connected via
either I2C or SPI. Configuration is done via the :ref:`devicetree <dt-guide>`.
The devicetree must have an enabled node with ``compatible = "we,wsen-hids";``.
See :dtcompatible:`we,wsen-hids` for the devicetree binding.

The sample reads from the sensor and outputs sensor data to the console at
regular intervals. If you want to test the sensor's trigger mode, specify
the trigger configuration in the prj.conf file and connect the interrupt
output from the sensor to your board.

.. zephyr-app-commands::
   :app: samples/sensor/wsen_hids/
   :goals: build flash

Sample Output
=============

.. code-block:: console

   [00:00:00.383,209] <inf> MAIN: HIDS device initialized.
   [00:00:00.384,063] <inf> MAIN: Sample #1
   [00:00:00.384,063] <inf> MAIN: Humidity: 29.8 %
   [00:00:00.384,063] <inf> MAIN: Temperature: 24.9 C
   [00:00:02.384,979] <inf> MAIN: Sample #2
   [00:00:02.385,009] <inf> MAIN: Humidity: 29.7 %
   [00:00:02.385,009] <inf> MAIN: Temperature: 24.9 C

   <repeats endlessly every 2 seconds>
