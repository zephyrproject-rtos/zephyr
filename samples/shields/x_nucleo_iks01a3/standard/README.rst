.. zephyr:code-sample:: x-nucleo-iks01a3-std
   :name: X-NUCLEO-IKS01A3 shield - Standard (Mode 1)
   :relevant-api: sensor_interface

   Interact with all the sensors of an X-NUCLEO-IKS01A3 shield using Standard mode.

Overview
********
This sample is provided as an example to test the X-NUCLEO-IKS01A3 shield
configured in Standard mode (Mode 1).
Please refer to :ref:`x-nucleo-iks01a3` for more info on this configuration.

This sample enables all sensors of a X-NUCLEO-IKS01A3 shield, and then
periodically reads and displays data from the shield sensors:

- HTS221: ambient temperature and relative humidity
- LPS22HH ambient temperature and atmospheric pressure
- LIS2MDL 3-Axis magnetic field intensity
- LIS2DW12 3-Axis acceleration
- LSM6DSO 6-Axis acceleration and angular velocity
- STTS751 temperature sensor

Requirements
************

This sample communicates over I2C with the X-NUCLEO-IKS01A3 shield
stacked on a board with an Arduino connector. The board's I2C must be
configured for the I2C Arduino connector (both for pin muxing
and devicetree). See for example the :zephyr:board:`nucleo_f401re` board
source code:

- :file:`$ZEPHYR_BASE/boards/arm/nucleo_f401re/nucleo_f401re.dts`
- :file:`$ZEPHYR_BASE/boards/arm/nucleo_f401re/pinmux.c`

Please note that this sample can't be used with boards already supporting
one of the sensors available on the shield (such as disco_l475_iot1)
as sensors multiple instances are not supported.

References
**********

- X-NUCLEO-IKS01A3: https://www.st.com/en/ecosystems/x-nucleo-iks01a3.html

DIL24 socket
************

In addition to sensors on board it is possible to place any other compatible
sensor on DIL24 socket. The sample is written in such a way that, if sensor is
not present, it will just be skipped.

List of sensors currently supported on DIL24 by this sample:

- LIS2DE12

Building and Running
********************

This sample runs with X-NUCLEO-IKS01A3 stacked on any board with a matching
Arduino connector. For this example, we use a :zephyr:board:`nucleo_f401re` board.

.. zephyr-app-commands::
   :zephyr-app: samples/shields/x_nucleo_iks01a3/standard/
   :host-os: unix
   :board: nucleo_f401re
   :goals: build
   :compact:

Sample Output
=============

 .. code-block:: console


    X-NUCLEO-IKS01A3 sensor dashboard

    HTS221: Temperature: 27.5 C
    HTS221: Relative Humidity: 27.0%
    LPS22HH: Temperature: 27.3 C
    LPS22HH: Pressure:99.150 kpa
    STTS751: Temperature: 27.6 C
    LIS2MDL: Magn (gauss): x: -0.445, y: -0.054, z: -0.066
    LIS2MDL: Temperature: 26.8 C
    LIS2DW12: Accel (m.s-2): x: -0.413, y: 0.077, z: 10.337
    LSM6DSO: Accel (m.s-2): x: 0.133, y: -0.133, z: 10.102
    LSM6DSO: GYro (dps): x: 0.000, y: -0.006, z: -0.058
    1:: lis2mdl trig 208
    1:: lps22hh trig 214
    1:: lsm6dso acc trig 426
    1:: lsm6dso gyr trig 426

    <updated endlessly every 2 seconds>
