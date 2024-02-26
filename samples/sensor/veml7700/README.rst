.. VEML7700:

VEML7700: Ambient Light Sensor Monitor
########################################

Overview
********
This sample periodically reads als_data from the VELM7700
Ambient Light Sensor and displays it on the console


Requirements
************

This sample uses the VELM7700 sensor controlled using the I2C interface.

References
**********

 - VELM7700:

Building and Running
********************

 This project outputs sensor data to the console. It requires an
 sensor, which is present on the b_u585i_iot02a board.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/velm7700
   :board: b_u585i_iot02a
   :goals: build
   :compact:

Sample Output
=============

 .. code-block:: console
*** Booting Zephyr OS build zephyr-v3.5.0-5580-g475d78c2f387 ***
Ambient Light Sensor Data: 597.0
count:1
Ambient Light Sensor Data: 598.0
count:2
Ambient Light Sensor Data: 594.0
count:3
Ambient Light Sensor Data: 588.0
count:4
Ambient Light Sensor Data: 3.0
count:5
Ambient Light Sensor Data: 161.0
count:6
Ambient Light Sensor Data: 444.0
count:7
Ambient Light Sensor Data: 8.0
count:8
Ambient Light Sensor Data: 600.0
count:9
Ambient Light Sensor Data: 592.0
count:10

