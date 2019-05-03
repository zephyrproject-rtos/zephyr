.. _vl53l1x:

VL53L1X: Time Of Flight sensor
##############################

Overview
********
This sample periodically measures the distance between the VL53L1X
sensor and a target. The result is displayed on the console.
It also shows how we can use the VL53L1X as a proximity sensor.


Requirements
************

This sample uses the VL53L1X sensor controlled using the I2C interface.

References
**********

 - VL53L1X: http://www.st.com/en/imaging-and-photonics-solutions/vl53l1x.html

Building and Running
********************

 This project outputs sensor data to the console. It requires a VL53L1X
 sensor.

 .. zephyr-app-commands::
    :app: samples/sensor/vl53l1x/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

   prox is 1
   distance is 0.053000m
   prox is 1
   distance is 0.051000m
   prox is 0
   distance is 1.929000m
   prox is 0
   distance is 1.930000m

    <repeats endlessly every second>
