.. _adxl345:

ADXL345: Three Axis Accelerometer
#################################

Overview
********

This sample application demonstrates how to use the ADXL345.

Building and Running
********************

This sample requires an ADXL345 sensor. It should work with any platform
featuring a I2C peripheral interface. It does not work on QEMU.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/adxl345
   :board: <board to use>
   :goals: build flash
   :compact:

Sample Output
=============

The application will print acceleration values to the console at the default
sampling rate of 12.5 Hz.

.. code-block:: console

    accel x:31.000000 y:0.000000 z:906.000000 (m/s^2)
    accel x:375.000000 y:-281.000000 z:906.000000 (m/s^2)
    accel x:250.000000 y:875.000000 z:937.000000 (m/s^2)
    accel x:156.000000 y:593.000000 z:937.000000 (m/s^2)
    accel x:62.000000 y:-31.000000 z:937.000000 (m/s^2)
    accel x:62.000000 y:0.000000 z:906.000000 (m/s^2)


References
**********

ADXL345 Datasheet and Product Info:
  https://www.analog.com/en/products/adxl345.html

.. _ADXL345 Datasheet:
   https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf
