.. zephyr:code-sample:: als_pt19
   :name: Everlight ALS-PT19 ambient light sensor
   :relevant-api: sensor_interface

   Read ambient light sensor data from an Everlight ALS-PT19 sensor.

Overview
********

This sample application reads ambient light sensor data from an
Everlight ALS-PT19 sensor and displays the values to the serial console.
The ALS-PT19 is a phototransistor that converts light intensity into a
proportional electrical signal, which can be read and processed by the
application. This sample demonstrates how to interface with the ALS-PT19
sensor and retrieve ambient light measurements.

Building and Running
********************

Currently, the sample is supported on the following boards:

:zephyr:board:`frdm_mcxw71`

In this example below the :zephyr:board:`frdm_mcxw71` board is used.


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/als_pt19
   :board: frdm_mcxw71/mcxw716c
   :goals: build flash

Sample Output
=============

Sample shows values read in lux.

.. code-block:: console

    *** Booting Zephyr OS build v4.1.0-rc1-11-g409ad4a50c45 ***
    [00:00:00.008,000] <inf> als_pt19_sample: Illuminance reading: 60.000

The sensor data is polled every second.
