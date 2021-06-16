.. _stm32_temp_sensor:

STM32 Temperature Sensor
########################

Overview
********

This sample periodically reads temperature from the STM32 Internal
Temperature Sensor and display the results.

Building and Running
********************

In order to run this sample, make sure to enable ``stm32_temp`` node in your
board DT file.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/stm32_temp_sensor
   :board: nucleo_f103rb
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Current temperature: 22.6 °C
    Current temperature: 22.8 °C
    Current temperature: 23.1 °C
