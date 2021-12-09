.. _max30101:

MAX30101 Heart Rate Sensor
##########################

Overview
********

A sensor application that demonstrates how to poll data from the max30101 heart
rate sensor.

Building and Running
********************

This project configures the max30101 sensor on the :ref:`hexiwear_k64` or
:ref:`nrf52dk_nrf52832` board to enable the red LED (heart rate mode) and
measure the reflected light with a photodiode. The raw ADC data prints to
the console. Further processing (not included in this sample) is required
to extract a heart rate signal from the light measurement.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/max30101
   :board: hexiwear_k64
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   RED=5731
   RED=5750
   RED=5748
   RED=5741
   RED=5735
   RED=5737
   RED=5736
   RED=5748

<repeats endlessly>
