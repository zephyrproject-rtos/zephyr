.. _vl6180x:

VL6180X: Time Of Flight sensor
##############################

Overview
********
This sample periodically measures distance between vl6180x sensor
and target. The result is displayed on the console.
It also shows how we can use the vl6180x as a proximity sensor.


Requirements
************

This sample uses the VL6180X sensor controlled using the I2C interface.

References
**********

 - VL6180X: https://www.st.com/en/imaging-and-photonics-solutions/vl6180x.html

Building and Running
********************

 This project outputs sensor data to the console. It requires a VL6180X
 sensor.

 .. zephyr-app-commands::
    :app: samples/sensor/vl6180x/
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
