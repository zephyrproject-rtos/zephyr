.. zephyr:code-sample:: lsmd6dso
   :name: LSM6DSO IMU sensor
   :relevant-api: sensor_interface

   Get accelerometer and gyroscope data from an LSM6DSO sensor (polling & trigger
   mode).

Overview
********
This sample sets the date rate of LSM6DSO accelerometer and gyroscope to
12.5Hz and enables a trigger on data ready. It displays on the console
the values for accelerometer and gyroscope.

Requirements
************

This sample uses the LSM6DSO sensor controlled using the I2C interface.
It has been tested on the :zephyr:board:`stm32l562e_dk`.

References
**********

- LSM6DSO https://www.st.com/en/mems-and-sensors/lsm6dso.html

Building and Running
********************

 This project outputs sensor data to the console. It requires an LSM6DSO
 sensor, which is present on the :zephyr:board:`stm32l562e_dk`.

Building on stm32l562e_dk board
===============================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lsm6dso
   :host-os: unix
   :board: stm32l562e_dk
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Testing LSM6DSO sensor in trigger mode.

    accel x:-0.650847 ms/2 y:-5.300102 ms/2 z:-8.163114 ms/2
    gyro x:-0.167835 dps y:-0.063377 dps z:0.002367 dps
    trig_cnt:1

    accel x:0.341575 ms/2 y:5.209773 ms/2 z:-7.938787 ms/2
    gyro x:-0.034284 dps y:-0.004428 dps z:-0.003512 dps
    trig_cnt:2

    <repeats endlessly>
