.. zephyr:code-sample:: si_sensor
   :name: Silabs Si7006/13/20/21 temperature and humidity polling sample
   :relevant-api: sensor_interface

   Get temperature and humidity data from a sensor using polling.

Overview
********

This sample periodically reads temperature and humidity from the Si
sensor and displays the results.

Building and Running
********************

To run this sample, enable the sensor node that supports ``SENSOR_CHAN_AMBIENT_TEMP``
and ``SENSOR_CHAN_HUMIDITY`` and create an alias named ``si-sensor`` to link to the node.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/si_sensor
   :board: xg23_rb4210a
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Temperature: 25.5 °C
    Humidity: 35.5%
    Temperature: 25.7 °C
    Humidity: 35.2%
