.. zephyr:code-sample:: stwinbx1_sensors
   :name: STWIN.box sensors

   Read sensor data from the various STWIN SensorTile wireless industrial node sensors.

Overview
********
This sample provides an example of how to read sensors data
from the STWIN.box board.

This sample enables all sensors of STWIN.box board, and then
periodically reads and displays data on the console from the following
sensors:

- STTS22H: Digital temperature sensor
- IIS2MDC: 3-axis magnetometer
- ISM330DHCX: IMU, 3D accelerometer and 3D gyroscope with Machine Learning Core and Finite State Machine
- IIS2DLPC: high-performance ultra-low-power 3-axis accelerometer for industrial applications
- IIS2ICLX: high-accuracy, high-resolution, low-power, 2-axis digital inclinometer with Machine Learning Core
- ILPS22QS: ultra-compact piezoresistive absolute pressure sensor

Requirements
************

The application requires a STWIN.box board connected to the PC
through USB. The board shows up as a USB CDC class standard device.

References
**********

- :zephyr:board:`steval_stwinbx1`

Building and Running
********************

Build and flash the sample in the following way:

.. zephyr-app-commands::
    :zephyr-app: samples/boards/st/steval_stwinbx1/sensors
    :board: steval_stwinbx1
    :goals: build flash

Please note that flashing the board requires a few preliminary steps described
in :zephyr:board:`steval_stwinbx1`.

Then, power cycle the board by disconnecting and reconnecting the USB cable.
Run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the correct device path automatically created on
the host after the STWIN.box board gets connected to it,
usually :code:`/dev/ttyUSBx` or :code:`/dev/ttyACMx` (with x being 0, 1, 2, ...).
The ``-b`` option sets baud rate ignoring the value from config.

Sample Output
=============

The sample code outputs sensors data on the STWIN.box console.

 .. code-block:: console

    STWIN.box dashboard

    STTS22H: Temperature: 24.4 C
    IIS2DLPC: Accel (m.s-2): x: -5.590, y: -0.536, z: 8.040
    IIS2MDC: Magn (gauss): x: 0.420, y: -0.116, z: -0.103
    IIS2MDC: Temperature: 21.0 C
    ISM330DHCX: Accel (m.s-2): x: 0.000, y: 5.704, z: 7.982
    ISM330DHCX: Gyro (dps): x: 0.026, y: -0.006, z: -0.008
    IIS2ICLX: Accel (m.s-2): x: -0.157, y: 5.699
    ILPS22QS: Temperature: 26.4 C
    ILPS22QS: Pressure: 100.539 kpa
    1:: iis2dlpc trig 2021
    1:: iis2mdc trig 993
    1:: ism330dhcx acc trig 4447
    1:: ism330dhcx gyr trig 2223
    1:: iis2iclx trig 2091

    <repeats endlessly every 2s>
