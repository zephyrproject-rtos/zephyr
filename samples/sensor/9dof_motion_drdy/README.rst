.. zephyr:code-sample:: 9dof_motion_drdy
   :name: Generic 9DOF Motion Dataready
   :relevant-api: sensor_interface

   Get temperature, acceleration, angular velocity, and magnetic field from an ICM-20948 sensor.

Description
***********

This sample application periodically (0.5 Hz) measures the sensor
temperature, acceleration, angular velocity, and magnetic field,
displaying the values on the console along with a timestamp since startup.

The ICM-20948 is a 9-axis motion tracking device that includes:

* 3-axis accelerometer
* 3-axis gyroscope
* 3-axis magnetometer (AK09916)
* Temperature sensor

When triggered mode is enabled the measurements are displayed at the
sensor's configured data rate.

Wiring
******

This sample uses an external breakout for the sensor. A devicetree
overlay must be provided to identify the I2C bus and GPIO used to
control the sensor.

Building and Running
********************

After providing a devicetree overlay that specifies the sensor location,
build this sample app using:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/9dof_motion_drdy
   :board: nrf52840dk/nrf52840
   :goals: build flash

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v3.x.0 ***
   Found ICM-20948 device icm20948@68
   [0:00:00.010]: 25.3 Cel
     accel  0.123456 -0.234567  9.812345 m/s/s
     gyro   0.001234 -0.002345  0.003456 rad/s
     magn  23.456789 -12.345678  45.678901 uT
   [0:00:02.022]: 25.4 Cel
     accel  0.124567 -0.235678  9.823456 m/s/s
     gyro   0.001345 -0.002456  0.003567 rad/s
     magn  23.567890 -12.456789  45.789012 uT

<repeats endlessly>
