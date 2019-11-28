.. _adxl362:

ADXL362: Three Axis Accelerometer
#################################

Overview
********

This sample application demonstrates how to use the ADXL362 with data ready and
threshold triggers. The upper and lower threshold triggers are configured in
link mode with referenced detection. See the `ADXL362 Datasheet`_ for additional
details.

Building and Running
********************

This sample requires an ADXL362 sensor. It should work with any platform
featuring a I2C peripheral interface. It does not work on QEMU.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/adxl362
   :board: <board to use>
   :goals: build flash
   :compact:

Sample Output
=============

The application will print acceleration values to the console at the default
sampling rate of 12.5 Hz. Shake the board continuously to trigger an upper
threshold event. Stop shaking the board to trigger a lower threshold event. In
both cases, ``Threshold trigger`` will be printed to the console.

.. code-block:: console

    x: -0.1, y: -0.0, z: 16.0 (m/s^2)
    x: -1.0, y: 7.0, z: 21.0 (m/s^2)
    Threshold trigger
    x: -3.1, y: 4.0, z: 0.0 (m/s^2)
    x: 1.1, y: 4.0, z: 15.1 (m/s^2)

References
**********

ADXL362 Datasheet and Product Info:
  https://www.analog.com/en/products/adxl362.html

.. _ADXL362 Datasheet:
   https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL362.pdf
