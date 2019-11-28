.. _ens210:

ams ens210 Relative Humidity and Temperature Sensor
###################################################

Overview
********

This sample application demonstrates how to use the ams ens210 sensor to
measure the ambient temperature and relative humidity.

Building and Running
********************

This sample application uses the sensor connected to the i2c stated in the
ens210.overlay file.
Flash the binary to a board of choice with a sensor connected.
For example build for a nucleo_f446re board:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ens210
   :board: nucleo_f446re
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    device is 0x20001174, name is ENS210
    Temperature: 28.28881222 C; Humidity: 25.25689737%
    Temperature: 28.28912472 C; Humidity: 25.25799105%
    Temperature: 28.28959347 C; Humidity: 25.25760045%
