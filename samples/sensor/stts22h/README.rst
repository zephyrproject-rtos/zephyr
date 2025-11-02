.. zephyr:code-sample:: stts22h
   :name: STTS22H Temperature sensor
   :relevant-api: sensor_interface

   Get ambient temperature from the STTS22H sensor.

Overview
********

This sample reads the onboard STTS22H ambient temperature sensor using the Zephyr
sensor API and prints the value periodically to the console.

Requirements
************

This sample uses the STTS22H sensor controlled using the I2C interface.

References
**********

 - STTS22H: https://www.st.com/en/mems-and-sensors/stts22h.html

Building and Running
********************

This sample outputs STTS22H temperature data to the console.

Supported boards: STM32U5G9J-DK1.

.. zephyr-app-commands::
    :zephyr-app: samples/sensor/stts22h/
    :board: stm32u5g9j_dk1
    :goals: build flash


Sample Output
=============

.. code-block:: console

   Zephyr STTS22H sensor sample. Board: stm32u5g9j_dk1
   [    10 ms] Temperature: 27.7 C
   [  2013 ms] Temperature: 27.6 C
   [  4016 ms] Temperature: 27.6 C
   [  6019 ms] Temperature: 27.6 C

   <repeats endlessly every 2 seconds>
