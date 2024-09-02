.. zephyr:code-sample:: fxas21002
   :name: FXAS21002 Gyroscope Sensor
   :relevant-api: sensor_interface

   Get gyroscope data synchronously from an FXAS21002 sensor.

Overview
********

A sensor application that demonstrates how to use the fxas21002 data ready
interrupt to read gyroscope data synchronously.

Building and Running
********************

This project outputs sensor data to the console. It requires an fxas21002
sensor, which is present on the :ref:`hexiwear` board. It does not work on
QEMU.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxas21002
   :board: hexiwear/mk64f12
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   X=    -2.063 Y=     0.813 Z=     0.188
   X=    -1.750 Y=     1.063 Z=     0.063
   X=    -2.000 Y=     0.625 Z=     0.063
   X=    -2.188 Y=     0.625 Z=     0.188
   X=    -2.125 Y=     1.313 Z=    -0.063
   X=    -2.188 Y=     1.188 Z=     0.313

<repeats endlessly>
