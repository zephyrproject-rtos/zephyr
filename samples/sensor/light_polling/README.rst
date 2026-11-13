.. zephyr:code-sample:: light_sensor_polling
   :name: Generic Light Sensor Polling
   :relevant-api: sensor_interface

   Get illuminance data from a light sensor.

Overview
********

This sample application reads the light sensors with the sensor read and decode API and prints
their illuminance to the console, in units of lux, once every second.

Requirements
************

To use this sample, the following hardware is required:

* A board with ADC support
* One or more supported light sensors (e.g., `Grove Light Sensor`_), available as
  ``light-sensor`` Devicetree alias or as ``light-sensor0`` to ``light-sensor9`` aliases.

Wiring
******

The wiring depends on the specific light sensor and board being used. Provide a devicetree
overlay that specifies the sensor configuration for your setup.

Building and Running
********************

Build and flash the sample as follows, changing ``nrf52dk/nrf52832`` to your board:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/light_polling
   :board: nrf52dk/nrf52832
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build v4.4.0-21507-g44c94e15f96d ***
         veml7700@10: 301.05 lx
         veml7700@10: 301.05 lx
         veml7700@10: 301.05 lx

.. _Grove Light Sensor: https://wiki.seeedstudio.com/Grove-Light_Sensor/
