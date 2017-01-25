.. _fxos8700:

FXOS8700 Accelerometer/Magnetometer Sensor
##########################################

Overview
********

A sensor application that demonstrates how to use the fxos8700 data ready
interrupt to read accelerometer/magnetometer data synchronously.

Building and Running
********************

This project outputs sensor data to the console. It requires an fxos8700
sensor, which is present on the frdm_k64f, frdm_kw41z, and hexiwear_k64 boards.
It does not work on QEMU.

.. code-block:: console

   $ cd samples/sensors/fxos8700
   $ make BOARD=frdm_k64f

Sample Output
=============

.. code-block:: console

   AX=  9.835380 AY=  0.009576 AZ=  0.383072 MX=  0.015000 MY=  0.509000 MZ=  1.346000
   AX=  9.816226 AY=  0.038307 AZ=  0.497993 MX=  0.029000 MY=  0.522000 MZ=  1.350000
   AX=  9.844957 AY=  0.067037 AZ=  0.430956 MX=  0.025000 MY=  0.518000 MZ=  1.353000
   AX=  9.835380 AY=  0.038307 AZ=  0.497993 MX=  0.026000 MY=  0.507000 MZ=  1.352000
   AX=  9.825803 AY=  0.057460 AZ=  0.421379 MX=  0.030000 MY=  0.502000 MZ=  1.342000
   AX=  9.816226 AY=  0.019153 AZ=  0.478840 MX=  0.017000 MY=  0.523000 MZ=  1.318000
   AX=  9.835380 AY=  0.000000 AZ=  0.507570 MX=  0.014000 MY=  0.502000 MZ=  1.367000

<repeats endlessly>
