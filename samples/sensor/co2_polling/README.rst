.. co2:

Generic CO2 polling sample
##########################

Overview
********

A sensor sample that demonstrates how to poll a CO2 sensor.

Building and Running
********************

This sample reads the CO2 sensor and print the values continuously.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/co2_polling
   :board: <board to use>
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   CO2 940 ppm
   CO2 950 ppm

<repeats endlessly>
