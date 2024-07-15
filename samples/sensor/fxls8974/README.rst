.. _fxls8974:

FXLS8974 Accelerometer Sensor
##########################################

Overview
********

This sample application shows how to use the FXLS8974 driver.

Building and Running
********************

This project outputs sensor data to the console. FXLS8974
sensor is present on the :ref:`mimxrt1040_evk` and :ref:`frdm_mcxn947` boards.
It does not work on QEMU.


Sample Output
=============

.. code-block:: console

   AX=     -0.08 AY=      0.14 AZ=     10.25 TEMP=30C
   AX=      0.03 AY=      0.21 AZ=     10.07 TEMP=30C
   AX=     -0.08 AY=      0.21 AZ=     10.29 TEMP=30C
   AX=     -0.08 AY=      0.03 AZ=     10.36 TEMP=30C
   AX=     -0.14 AY=      0.35 AZ=     10.25 TEMP=30C
   AX=     -0.08 AY=      0.14 AZ=     10.25 TEMP=30C

<repeats endlessly>
