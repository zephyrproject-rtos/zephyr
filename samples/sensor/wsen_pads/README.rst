.. _wsen-pads:

WSEN-PADS: Absolute Pressure Sensor
###################################

Overview
********

This sample uses the Zephyr :ref:`sensor_api` API driver to periodically
read pressure and temperature from the Würth Elektronik WSEN-PADS
absolute pressure sensor and displays it on the console.

By default, sensor samples are read in polling mode. If desired, the
data-ready interrupt of the sensor can be used to trigger reading of samples.

Requirements
************

This sample requires a WSEN-PADS sensor connected via the I2C or SPI interface.

References
**********

- WSEN-PADS: https://www.we-online.com/catalog/en/WSEN-PADS

Building and Running
********************

This sample can be configured to support WSEN-PADS sensors connected via
either I2C or SPI. Configuration is done via the :ref:`devicetree <dt-guide>`.
The devicetree must have an enabled node with ``compatible = "we,wsen-pads";``.
See :dtcompatible:`we,wsen-pads` for the devicetree binding.

The sample reads from the sensor and outputs sensor data to the console at
regular intervals. If you want to test the sensor's trigger mode, specify
the trigger configuration in the prj.conf file and connect the interrupt
output from the sensor to your board.

Note that the LSB of the sensor's I2C slave address can be modified using
the SAO pin. The I2C address used by the sensor (0x5C or 0x5D) needs to be
specified in the devicetree. The WSEN-PADS evaluation board uses 0x5D as
its default address.

.. zephyr-app-commands::
   :app: samples/sensor/wsen_pads/
   :goals: build flash

Sample Output
=============

.. code-block:: console

   [00:00:00.372,680] <inf> MAIN: PADS device initialized.
   [00:00:00.374,023] <inf> MAIN: Sample #1
   [00:00:00.374,053] <inf> MAIN: Pressure: 97.8630 kPa
   [00:00:00.374,084] <inf> MAIN: Temperature: 25.7 C
   [00:00:02.375,549] <inf> MAIN: Sample #2
   [00:00:02.375,610] <inf> MAIN: Pressure: 97.8620 kPa
   [00:00:02.375,610] <inf> MAIN: Temperature: 25.7 C
   [00:00:04.377,075] <inf> MAIN: Sample #3
   [00:00:04.377,105] <inf> MAIN: Pressure: 97.8590 kPa
   [00:00:04.377,136] <inf> MAIN: Temperature: 25.7 C

   <repeats endlessly every 2 seconds>
