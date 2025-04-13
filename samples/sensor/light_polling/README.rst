.. zephyr:code-sample:: light_sensor_polling
   :name: Generic Light Sensor Polling
   :relevant-api: sensor_interface

   Get illuminance data from a light sensor.

Overview
********

This sample application gets the output of the light sensor and prints it to the console, in
units of lux, once every second.

Requirements
************

To use this sample, the following hardware is required:

* A board with ADC support
* A supported light sensor (e.g., `Grove Light Sensor`_), available as ``light-sensor`` Devicetree alias.

Wiring
******

The wiring depends on the specific light sensor and board being used. Provide a devicetree
overlay that specifies the sensor configuration for your setup.

Building and Running
********************

Build and flash the sample as follows, changing ``nrf52dk_nrf52832`` to your board:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/light_polling
   :board: nrf52dk_nrf52832
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build v3.6.0-rc1-32-gba639ed6a893 ***
    lux: 0.945751
    lux: 0.882292
    lux: 0.755973

.. _Grove Light Sensor: https://wiki.seeedstudio.com/Grove-Light_Sensor/
