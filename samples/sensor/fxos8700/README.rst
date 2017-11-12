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
sensor, which is present on the :ref:`frdm_k64f`, :ref:`frdm_kw41z`, and
:ref:`hexiwear_k64` boards.  It does not work on QEMU.

.. zephyr-app-commands::
   :zephyr-app: samples/sensors/fxos8700
   :board: frdm_k64f
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   AX= -0.191537 AY=  0.067037 AZ=  9.902418 MX=  0.379000 MY=  0.271000 MZ= -0.056000 T= 22.080000
   AX= -0.162806 AY=  0.143652 AZ=  9.940725 MX=  0.391000 MY=  0.307000 MZ= -0.058000 T= 22.080000
   AX= -0.172383 AY=  0.134075 AZ=  9.969455 MX=  0.395000 MY=  0.287000 MZ= -0.017000 T= 22.080000
   AX= -0.210690 AY=  0.105344 AZ=  9.911994 MX=  0.407000 MY=  0.306000 MZ= -0.068000 T= 22.080000
   AX= -0.153229 AY=  0.124498 AZ=  9.950302 MX=  0.393000 MY=  0.301000 MZ= -0.021000 T= 22.080000
   AX= -0.153229 AY=  0.095768 AZ=  9.921571 MX=  0.398000 MY=  0.278000 MZ= -0.040000 T= 22.080000
   AX= -0.162806 AY=  0.105344 AZ=  9.902418 MX=  0.372000 MY=  0.300000 MZ= -0.046000 T= 22.080000

<repeats endlessly>
