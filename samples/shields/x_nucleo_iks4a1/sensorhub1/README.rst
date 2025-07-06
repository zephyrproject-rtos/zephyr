.. _x-nucleo-iks4a1-shub1-sample:

X-NUCLEO-IKS4A1 shield SHUB1 (Mode 3) sample
############################################

Overview
********
This sample is provided as an example to test the X-NUCLEO-IKS4A1 shield
configured in SHUB1 (Mode 3).
Please refer to :ref:`x-nucleo-iks4a1-mode-3` for more info on this configuration.

This sample enables LSM6DSV16X IMU in sensorhub mode with LIS2MDL magnetometer and
LPS22DF pressure and temperature sensor.

Then sensor data are displayed periodically

- LSM6DSV16X 6-Axis acceleration and angular velocity
- LSM6DSV16X (from LIS2MDL) 3-Axis magnetic field intensity
- LSM6DSV16X (from LPS22DF) ambient temperature and atmospheric pressure

Requirements
************

This sample communicates over I2C with the X-NUCLEO-IKS4A1 shield
stacked on a board with an Arduino connector, e.g. the
:zephyr:board:`nucleo_f411re` board.

Building and Running
********************

This sample runs with X-NUCLEO-IKS4A1 stacked on any board with a matching
Arduino connector. For this example, we use a :zephyr:board:`nucleo_f411re` board.

.. zephyr-app-commands::
   :zephyr-app: samples/shields/x_nucleo_iks4a1/sensorhub1/
   :host-os: unix
   :board: nucleo_f411re
   :goals: build flash
   :compact:

Sample Output
=============

 .. code-block:: console

    X-NUCLEO-IKS01A4 sensor dashboard

    LSM6DSV16X: Accel (m.s-2): x: 0.081, y: -0.177, z: 9.945
    LSM6DSV16X: GYro (dps): x: 0.001, y: -0.000, z: 0.004
    LSM6DSV16X: Magn (gauss): x: 0.217, y: 0.015, z: -0.415
    LSM6DSV16X: Temperature: 19.8 C
    LSM6DSV16X: Pressure:99.655 kpa
    16:: lsm6dso16is acc trig 6432

    <updated endlessly every 2 seconds>

References
**********

:ref:`x-nucleo-iks4a1`
