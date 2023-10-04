.. _esp32_temp_sensor:

ESP32 Temperature Sensor
########################

Overview
********

This sample periodically reads temperature from the ESP32-S2 and ESP32-C3
Internal Temperature Sensor and display the results.

Building and Running
********************

In order to run this sample, make sure to enable ``esp32_temp`` node in your
board DT file.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/esp32_temp_sensor
   :board: esp32s2_saola
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Current temperature: 22.6 °C
    Current temperature: 22.8 °C
    Current temperature: 23.1 °C
