.. _ds18b20:

DS18B20: Temperature Sensor
###########################

Overview
********
This sample periodically reads temperature from the DS18B20
Sensor and displays it on the console


Requirements
************

This sample uses the DS18B20 sensor controlled with 1-Wire interface.

References
**********

- DS18B20: https://www.maximintegrated.com/en/app-notes/index.mvp/id/162

Building and Running
********************

This project outputs sensor data to the console. It requires an DS18B20
sensor.

.. zephyr-app-commands::
   :zephyr-app: samples/sensors/ds18b20
   :board: stm32_min_dev
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   Temperature:25.3 C
   Temperature:25.3 C
   Temperature:25.3 C
   Temperature:25.3 C

   <repeats endlessly every 2 seconds>
