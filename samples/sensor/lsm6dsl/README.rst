.. _lsm6dsl:

LSM6DSL 3D accelerometer and 3D gyroscope
#########################################

Overview
********

A simple sensor application that demonstrates how to read values from the
LSM6DSL accelerometer and gyroscope sensor.

Building and Running
********************

This project outputs sensor data to the console. It requires an LSM6DSL
sensor, which is present on the :ref:`disco_l475_iot1` board.

.. code-block:: console

   $ cd samples/sensors/lsm6dsl
   $ make BOARD=disco_l475_iot1

Sample Output
=============

.. code-block:: console

   Angular rate in millidegrees per sec:
   X=     490.0 Y=   -1470.0 Z=   -1260.0

   Linear acceleration in mg:
   X=     -18.1 Y=       8.4 Z=    1034.4

   Angular rate in millidegrees per sec:
   X=     490.0 Y=   -1470.0 Z=   -1190.0

   Linear acceleration in mg:
   X=     -18.2 Y=       7.4 Z=    1034.4

   <repeats endlessly>

Linear acceleration will be around 1 on z axis while board is laying
on your desk. It should be around 0 on x and y axis.
Rotating the board, 1g acceleration should be seen on other axis.

Angular rate should theoretically be null on all axis. In reality a small
offset is seen. When board is moving higher value is seen.
