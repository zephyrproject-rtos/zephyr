.. zephyr:code-sample:: sensortile_box_pro_sensors
   :name: SensorTile.box Pro sensors

   Read sensor data from the various SensorTile.box Pro sensors.

Overview
********
This sample provides an example of how to read sensors data
from the SensorTile.box Pro board.

This sample enables all sensors of SensorTile.box Pro board, and then
periodically reads and displays data on the console from the following
sensors:

- LPS22DF: ambient temperature and atmospheric pressure
- LSM6DSV16X: 6-Axis acceleration and angular velocity
- LIS2DU12: 3-Axis acceleration

Requirements
************

The application requires a SensorTile.box Pro board connected to the PC
through USB. The board shows up as a USB CDC class standard device.

References
**********

- :zephyr:board:`sensortile_box_pro`

Building and Running
********************

Build and flash the sample in the following way:

.. zephyr-app-commands::
    :zephyr-app: samples/boards/st/sensortile_box_pro/sensors-on-board
    :board: sensortile_box_pro
    :goals: build flash

Please note that flashing the board requires a few preliminary steps described
in :zephyr:board:`sensortile_box_pro`.

Then, power cycle the board by disconnecting and reconnecting the USB cable.
Run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the correct device path automatically created on
the host after the SensorTile.box Pro board gets connected to it,
usually :code:`/dev/ttyUSBx` or :code:`/dev/ttyACMx` (with x being 0, 1, 2, ...).
The ``-b`` option sets baud rate ignoring the value from config.

Sample Output
=============

The sample code outputs sensors data on the SensorTile.box Pro console.

 .. code-block:: console

    SensorTile.box Pro dashboard

    LPS22DF: Temperature: 28.4 C
    LPS22DF: Pressure:99.694 kpa
    LSM6DSV16X: Accel (m.s-2): x: -0.158, y: 0.158, z: 9.811
    LSM6DSV16X: GYro (dps): x: 0.003, y: 0.000, z: -0.005
    LIS2DU12: Accel (m.s-2): x: -0.756, y: -0.249, z: -9.629
    1:: lps22df trig 199
    1:: lsm6dsv16x acc trig 836
    1:: lsm6dsv16x gyr trig 836
    1:: lis2mdl trig 402
    1:: lis2du12 trig 1589

    <repeats endlessly every 2s>
