.. zephyr:code-sample:: heart_rate
   :name: Heart Rate Sensor
   :relevant-api: sensor_interface

   Get heart rate data from a sensor (polling/interrupt mode).

Overview
********

A sensor application that demonstrates how to get data from a heart rate
sensor either by polling or interrupt.

Requirements
************

* A supported heart rate sensor (e.g., MAX30101 or BH1790), available as ``heart-rate-sensor`` Devicetree alias.

Building and Running
********************

This project configures a sensor on the board to enable the green LED and
measure the reflected light with a photodiode. The raw data prints to the
console. Further processing (not included in this sample) is required to
extract a heart rate signal from the light measurement.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/heart_rate
   :board: hexiwear/mk64f12
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   GREEN=5731
   GREEN=5750
   GREEN=5748
   GREEN=5741
   GREEN=5735
   GREEN=5737
   GREEN=5736
   GREEN=5748

<repeats endlessly>
