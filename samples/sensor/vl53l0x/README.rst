.. _vl53l0x:

VL53L0X: Time Of Flight sensor
##############################

Overview
********
This sample periodically measures distance between vl53l0x sensor
and target. The result is displayed on the console.
It also shows how we can use the vl53l0x as a proximity sensor.


Requirements
************

This sample uses the VL53L0X sensor controlled using the I2C interface.

References
**********

 - VL53L0X: https://www.st.com/en/imaging-and-photonics-solutions/vl53l0x.html

Building and Running
********************

 This project outputs sensor data to the console. It requires a VL53L0X
 sensor, which is present on the disco_l475_iot1 board.

 .. zephyr-app-commands::
    :app: samples/sensor/vl53l0x/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

    prox is 0
    distance is 1938
    prox is 1
    distance is 70
    prox is 0
    distance is 1995

    <repeats endlessly every second>
