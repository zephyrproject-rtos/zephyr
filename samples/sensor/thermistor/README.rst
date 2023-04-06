.. _thermistor_sensor:

THERMISTOR Sensor
########################

Overview
********

This sample application periodically determines the temperature with a
thermistor and displays the results.

Building and Running
********************

Enable the ``thermistor`` node in your board's DT file.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/thermistor
   :board: cy8cproto_062_4343w
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Temperature: 21.4 °C
    Temperature: 21.3 °C
    Temperature: 21.5 °C
