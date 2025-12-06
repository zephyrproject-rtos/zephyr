.. zephyr:code-sample:: lsmd6dsr
   :name: LSM6DSR IMU sensor
   :relevant-api: sensor_interface

   Get accelerometer and gyroscope data from an LSM6DSR sensor (polling).

Overview
********

This sample sets the LSM6DSR accelerometer and gyroscope to 104Hz and reads the
values (polling) in a loop. It displays the values for accelerometer and
gyroscope on the console.


Requirements
************

This sample uses the LSM6DSR sensor controlled using the I2C or SPI interface.
It has been tested on the nRF52840 DK board with an STEVAL-MKI194V1 attached.

References
**********

- LSM6DSR https://www.st.com/en/mems-and-sensors/lsm6dsr.html
- STEVAL-MKI194V1 https://www.st.com/en/evaluation-tools/steval-mki194v1.html
- nRF52840 DK https://www.nordicsemi.com/Products/Development-hardware/nRF52840-DK

Building and Running
********************

This project outputs sensor data to the console. It requires an LSM6DSR
sensor attached to the nRF52840 DK.

There are two devicetree overlays to either chose building for I2C or SPI

- i2c.overlay
- spi.overlay

Use DEXTRA_DTC_OVERLAY_FILE to specify which one to use.

Building on nRF52840 board
==========================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lsm6dsr
   :host-os: unix
   :board: nRF52840
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    LSM6DSR sensor samples:

    accel x:4.705485 ms/2 y:-8.873782 ms/2 z:0.023928 ms/2
    gyro x:0.006490 dps y:-0.007559 dps z:-0.002977 dps
    loop:4 trig_cnt:339

    <repeats endlessly every 1 seconds>
