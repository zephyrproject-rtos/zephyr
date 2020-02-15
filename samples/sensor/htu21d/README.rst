.. _htu21d:

HTU21D Humidity Sensor
##########################

Overview
********

A sensor application that demonstrates how to get data from the htu21d
humidity sensor.

Building and Running
********************

This project configures the htu21d sensor on the :ref:`hexiwear_k64` board to
periodically poll the sensor and display the measured relative humidity and
temperature.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/htu21d
   :board: hexiwear_k64
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console


Humidity (%RH) = 22.95
Temperature (Celcius) = 28.86

<repeats endlessly>
