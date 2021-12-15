.. _max30208:

MAX30208 I2C Digital Temperature Sensor
#######################################

Overview
********

A sensor application that demonstrates how to poll data from the max30208 temperature sensor.

Building and Running
********************

This project configures the max30208 sensor on the :ref:`nrf52dk_nrf52832`.
It triggers a measurement and reads then the value (with interrupt or polling).
The temperature value is then printed to the console.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/max30208
   :board: nrf52dk_nrf52832
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

POLLED-TEMP=25.970
POLLED-TEMP=25.975
POLLED-TEMP=25.975
POLLED-TEMP=25.965
POLLED-TEMP=25.980
POLLED-TEMP=27.15
POLLED-TEMP=29.155
POLLED-TEMP=29.160
POLLED-TEMP=28.450
POLLED-TEMP=28.175
POLLED-TEMP=28.35
POLLED-TEMP=27.865

<repeats endlessly>
