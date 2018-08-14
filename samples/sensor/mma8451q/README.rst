.. _mma8451q:

MMA8451Q Accelerometer demo
#############################

Overview
********
A simple demo showing how to use the MMA8451Q accelerometer

Building and Running
********************

This project outputs accelerometer data to the console.  It can be built and executed
for the FRDM-KL25Z as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mma8451q
   :board: frdm_kl25z
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    XYZ Sensor readings:
    X:  -1.297658, Y:   2.262520, Z:  10.503362
    X:  -0.124499, Y:   0.378283, Z:   9.823409
    X:   0.004788, Y:   0.454898, Z:   9.768342
    X:  -0.119711, Y:   0.383072, Z:   9.773131
    X:  -0.155624, Y:   0.368707, Z:   9.818621
    X:  -0.153229, Y:   0.383072, Z:   9.821015
    X:  -0.122105, Y:   0.361524, Z:   9.782707
    X:  -0.179566, Y:   0.383072, Z:   9.770736
    X:  -0.146047, Y:   0.359130, Z:   9.773131
    X:  -0.155624, Y:   0.332794, Z:   9.751583
    X:  -0.129287, Y:   0.359130, Z:   9.761160
    X:  -0.124499, Y:   0.385466, Z:   9.797073
    X:  -0.134076, Y:   0.392649, Z:   9.761160
