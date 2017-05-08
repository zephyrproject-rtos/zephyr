.. _lis3mdl:

LIS3MDL 3 Axis Magnetometer Sensor
##################################

Overview
********

A sensor application that demonstrates how to use the lis3mdl to read
accelerometer/magnetometer data.

Building and Running
********************

This project outputs sensor data to the console. It requires an lis3mdl
sensor, which is present on the disco_l475_iot1 board.

.. code-block:: console

   $ cd samples/sensors/lis3mdl
   $ make BOARD=disco_l475_iot1


Sample Output
=============

.. code-block:: console

   MX= -0.257673 MY= -0.542239 MZ=  0.184448
   MX= -0.252411 MY= -0.554954 MZ=  0.175095
   MX= -0.262788 MY= -0.554662 MZ=  0.185033
   MX= -0.266296 MY= -0.552616 MZ=  0.187664
   MX= -0.263519 MY= -0.554370 MZ=  0.188833
   MX= -0.260157 MY= -0.553200 MZ=  0.178895
   MX= -0.259573 MY= -0.554954 MZ=  0.186495

   <repeats endlessly>

 Move the board in the 3 axis directions and check that values
 change. If absolute value interpretation could be difficult,
 you should be able to see sign variation.
