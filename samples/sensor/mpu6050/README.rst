.. zephyr:code-sample:: mpu6050
   :name: MPU6050 motion tracking device
   :relevant-api: sensor_interface

   Get temperature, acceleration, and angular velocity from an MPU6050 sensor (polling & trigger
   mode).

Description
***********

This sample application periodically (0.5 Hz) measures the sensor
temperature, acceleration, and angular velocity, displaying the values
on the console along with a timestamp since startup.

When triggered mode is enabled the measurements are displayed at a rate
of 32Hz governed by the sensor's internal clock and divider.

Wiring
*******

This sample uses an external breakout for the sensor.  A devicetree
overlay must be provided to identify the I2C bus and GPIO used to
control the sensor.

Building and Running
********************

After providing a devicetree overlay that specifies the sensor location,
build this sample app using:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mpu6050
   :board: nrf52dk/nrf52832
   :goals: build flash

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.1.0-576-g4b38659b0661  ***
   [0:00:00.008]:23.6359 Cel
     accel -5.882554 -6.485893  5.868188 m/s/s
     gyro   0.014522  0.002264 -0.036905 rad/s
   [0:00:02.020]:23.6359 Cel
     accel -5.841853 -6.435615  5.911283 m/s/s
     gyro   0.017852  0.001199 -0.034640 rad/s
   [0:00:04.032]:23.6829 Cel
     accel -5.930438 -6.461951  6.009446 m/s/s
     gyro   0.012923  0.002131 -0.037171 rad/s
   [0:00:06.044]:23.6359 Cel
     accel -5.884948 -6.524200  5.961562 m/s/s
     gyro   0.012390 -0.001732 -0.045964 rad/s
   [0:00:08.056]:35.7712 Cel
     accel -5.863400 -12.872426 -0.154427 m/s/s
     gyro  -0.034373 -0.034373 -0.034373 rad/s
   [0:00:10.068]:23.6829 Cel
     accel -5.906496 -6.461951  5.899312 m/s/s
     gyro   0.015321 -0.000399 -0.039169 rad/s

<repeats endlessly>
