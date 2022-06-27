.. _wsen-pdus:

WSEN-PDUS: Differential Pressure Sensor
#######################################

Overview
********

This sample uses the Zephyr :ref:`sensor_api` API driver to periodically
read pressure and temperature from the Würth Elektronik WSEN-PDUS differential
pressure sensor and displays it on the console.

Requirements
************

This sample requires a WSEN-PDUS sensor connected via the I2C interface.

References
**********

- WSEN-PDUS: https://www.we-online.com/catalog/en/WSEN-PDUS

Building and Running
********************

This sample supports WSEN-PDUS sensors connected via I2C. Configuration is
done via the :ref:`devicetree <dt-guide>`. The devicetree must have an
enabled node with ``compatible = "we,wsen-pdus";``. See
:dtcompatible:`we,wsen-pdus` for the devicetree binding.

The sample reads from the sensor and outputs sensor data to the console at
regular intervals.

Note that the ``sensor-type`` devicetree parameter has to be set to match
the PDUS sensor variant (pressure measurement range) being used.

.. zephyr-app-commands::
   :app: samples/sensor/wsen_pdus/
   :goals: build flash

Sample Output
=============

.. code-block:: console
   [00:00:00.372,680] <inf> MAIN: PDUS device initialized.
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
