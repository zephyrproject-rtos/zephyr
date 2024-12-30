.. zephyr:code-sample:: 6dof_motion_drdy
   :name: Generic 6DOF Motion Dataready
   :relevant-api: sensor_interface

   Get 6-Axis accelerometer and gyroscope data from a sensor (data ready interrupt mode).

Overview
********

This sample application periodically (100 Hz) measures the 6-axis IMU sensor with
temperature, acceleration, and angular velocity, displaying the
values on the console along with a timestamp since startup.
Trigger options could be configured through KConfig.

Wiring
******

This sample uses an external breakout for the sensor.  A devicetree
overlay must be provided to identify the 6-axis motion sensor, the SPI or I2C bus interface and the interrupt
sensor GPIO.

Building and Running
********************

This sample supports up to 6-Axis IMU devices. Each device needs
to be aliased as ``6dof-motion-drdyN`` where ``N`` goes from ``0`` to ``9``. For example:

.. code-block:: devicetree

 / {
	aliases {
			6dof-motion-drdy0 = &icm42670p;
		};
	};

Make sure the aliases are in devicetree, then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/6dof_motion_drdy
   :board: nrf52dk/nrf52832
   :goals: build flash

Sample Output
=============

.. code-block:: console

Found device "icm42670p@68", getting sensor data
[0:00:01.716]: temp 23.00 Cel   accel 0.150839 -0.140065 9.994899 m/s/s   gyro  -0.001597 0.005859 0.001597 rad/s
[0:00:01.726]: temp 23.00 Cel   accel 0.140065 -0.146050 9.988914 m/s/s   gyro  -0.002663 0.005859 0.003195 rad/s
[0:00:01.736]: temp 23.50 Cel   accel 0.146050 -0.130487 9.988914 m/s/s   gyro  -0.001597 0.006391 0.003195 rad/s
[0:00:01.746]: temp 23.00 Cel   accel 0.149642 -0.136473 9.990111 m/s/s   gyro  -0.002663 0.004261 0.002663 rad/s
[0:00:01.756]: temp 23.00 Cel   accel 0.146050 -0.136473 9.979337 m/s/s   gyro  -0.002130 0.005326 0.001597 rad/s
[0:00:01.766]: temp 23.00 Cel   accel 0.136473 -0.147247 9.986519 m/s/s   gyro  -0.001065 0.005859 0.002663 rad/s

<repeats endlessly>
