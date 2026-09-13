.. _x-nucleo-iks4a1-std-i3c-sample:

X-NUCLEO-IKS4A1 shield Standard (Mode 1 + I3C) sample
#####################################################

Overview
********
This sample is provided as an example to test the X-NUCLEO-IKS4A1 shield
configured in Standard mode (Mode 1) with the I3C interface enabled.

This sample enables the following four sensors of a X-NUCLEO-IKS4A1 shield, and then
periodically reads and displays data from the shield sensors:

- LSM6DSV16X 6-Axis acceleration and angular velocity
- LSM6DSO16IS 6-Axis acceleration and angular velocity
- LPS22DF ambient temperature and atmospheric pressure
- LIS2MDL 3-Axis magnetic field intensity

Requirements
************

This sample communicates with the X-NUCLEO-IKS4A1 shield over I3C and targets
boards exposing an Arduino connector with I3C support.

The sample is configured to use the ``x_nucleo_iks4a1_i3c`` shield variant and
provides a board overlay for :zephyr:board:`nucleo_h503rb`.

Building and Running
********************

This sample runs with X-NUCLEO-IKS4A1 stacked on a board with a matching
Arduino connector and I3C-capable bus. For this example, we use a
:zephyr:board:`nucleo_h503rb` board.

.. zephyr-app-commands::
   :zephyr-app: samples/shields/x_nucleo_iks4a1/standard_i3c/
   :host-os: unix
   :board: nucleo_h503rb
   :goals: build flash
   :compact:

Sample Output
=============

 .. code-block:: console


    X-NUCLEO-IKS4A1 sensor dashboard

    LIS2MDL: Magn (G): x: -0.261, y: -0.339, z: -0.261
    LIS2MDL: Temperature: 24.6 C
    LSM6DSO16IS: Accel (m/s^2): x: 0.177, y: -0.024, z: 9.882
    LSM6DSO16IS: Gyro (rad/s): x: 0.049, y: -0.063, z: -0.033
    LSM6DSO16IS: Temperature: 27.9 C
    LSM6DSV16X: Accel (m/s^2): x: 0.067, y: 0.167, z: 9.820
    LSM6DSV16X: Gyro (rad/s): x: -0.005, y: 0.005, z: 0.004
    LSM6DSV16X: Temperature: 27.0 C
    LPS22DF: Temperature: 27.1 C
    LPS22DF: Pressure:102.202 kPa

    2: lis2mdl trig 215
    2: lsm6dsv16x acc trig 467

    <updated endlessly every 2 seconds>

References
**********

:ref:`x-nucleo-iks4a1`
