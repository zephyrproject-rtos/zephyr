.. zephyr:code-sample:: vl53l0x
   :name: VL53L0X Time Of Flight sensor
   :relevant-api: sensor_interface

   Get distance data from a VL53L0X sensor (polling mode).

Overview
********

This sample periodically measures distance between vl53l0x sensor
and target. The result is displayed on the console.
It shows the usage of all available channels including private ones.

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
   :zephyr-app: samples/sensor/vl53l0x/
   :goals: build flash


Sample Output
=============

.. code-block:: console

   prox is 0
   distance is 1874 mm
   Max distance is 000 mm
   Signal rate is 33435 Cps
   Ambient rate is 17365 Cps
   SPADs used: 195
   Status: OK

   prox is 0
   distance is 1888 mm
   Max distance is 000 mm
   Signal rate is 20846 Cps
   Ambient rate is 25178 Cps
   SPADs used: 195
   Status: OK

   <repeats endlessly every 5 seconds>
