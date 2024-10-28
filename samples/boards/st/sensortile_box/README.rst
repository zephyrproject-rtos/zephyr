.. zephyr:code-sample:: sensortile_box_sensors
   :name: SensorTile.box sensors

   Read sensor data from the various SensorTile.box sensors.

Overview
********
This sample provides an example of how to read sensors data
from the SensorTile.box board.

This sample enables all sensors of SensorTile.box board, and then
periodically reads and displays data on the console from the following
sensors:

- HTS221: ambient temperature and relative humidity
- LPS22HH: ambient temperature and atmospheric pressure
- LIS2DW12: 3-Axis acceleration
- LSM6DSOX: 6-Axis acceleration and angular velocity
- STTS751: temperature sensor

Requirements
************

The application requires a SensorTile.box board connected to the PC
through USB. The board declares itself as a USB CDC class device.

References
**********

- :zephyr:board:`sensortile_box`

Building and Running
********************

Build and flash the sample in the following way:

.. zephyr-app-commands::
    :zephyr-app: samples/boards/st/sensortile_box
    :board: sensortile_box
    :goals: build flash

Please note that flashing the board requires a few preliminary steps described
in :zephyr:board:`sensortile_box`.

Then, power cycle the board by disconnecting and reconnecting the USB cable.
Run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the SensorTile.box board
can be found. For example, under Linux, :code:`/dev/ttyUSB0`.
The ``-b`` option sets baud rate ignoring the value from config.

Sample Output
=============

The sample code outputs sensors data on the SensorTile.box console.

 .. code-block:: console

    SensorTile.box dashboard

    HTS221: Temperature: 26.4 C
    HTS221: Relative Humidity: 60.5%
    LPS22HH: Temperature: 28.4 C
    LPS22HH: Pressure:99.694 kpa
    LIS2DW12: Accel (m.s-2): x: 0.306, y: -0.459, z: 10.031
    IIS3DHHC: Accel (m.s-2): x: -0.581, y: 0.880, z: -9.933
    LSM6DSOX: Accel (m.s-2): x: -0.158, y: 0.158, z: 9.811
    LSM6DSOX: GYro (dps): x: 0.003, y: 0.000, z: -0.005
    STTS751: Temperature: 27.0 C
    1:: lps22hh trig 206
    1:: lis2dw12 trig 410
    1:: lsm6dsox acc trig 836
    1:: lsm6dsox gyr trig 836
    1:: iis3dhhc trig 2422

    <repeats endlessly every 2s>

If you move the board around or put your finger on the temperature
sensor, you will see the accelerometer, gyro, and temperature values change.
