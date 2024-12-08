.. zephyr:code-sample:: lps22hh
   :name: LPSS22HH Temperature and Pressure Sensor
   :relevant-api: sensor_interface

   Get pressure and temperature data from an LPS22HH sensor (polling & trigger mode).

Overview
********
This sample periodically reads pressure from the LPS22HH MEMS pressure
sensor and displays it on the console.


Requirements
************

This sample uses the LPS22HH sensor controlled using the I2C interface.

References
**********

- LPS22HH: https://www.st.com/en/mems-and-sensors/lps22hh.html

Building and Running
********************

This project outputs sensor data to the console. It requires an LPS22HH
sensor, which is present on the X-NUCLEO-IKS01A3 shield.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lps22hh
   :board: nrf52dk/nrf52832
   :shield: x_nucleo_iks01a3
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   Configured for triggered collection at 1 Hz
   Observation: 1
   Pressure: 97.474 kPa
   Temperature: 22.19 C
   Observation: 2
   Pressure: 97.466 kPa
   Temperature: 22.21 C
   Observation: 3
   Pressure: 97.473 kPa
   Temperature: 22.21 C
   Observation: 4
   Pressure: 97.455 kPa
   Temperature: 22.21 C

   <repeats endlessly every second>
