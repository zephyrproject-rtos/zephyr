.. _x-nucleo-iks4a1-std-sample:

X-NUCLEO-IKS4A1 shield Standard (Mode 1) sample
###############################################

Overview
********
This sample is provided as an example to test the X-NUCLEO-IKS4A1 shield
configured in Standard mode (Mode 1).
Please refer to :ref:`x-nucleo-iks4a1-mode-1` for more info on this configuration.

This sample enables the following four sensors of a X-NUCLEO-IKS4A1 shield, and then
periodically reads and displays data from the shield sensors:

- LSM6DSV16X 6-Axis acceleration and angular velocity
- LSM6DSO16IS 6-Axis acceleration and angular velocity
- LPS22DF ambient temperature and atmospheric pressure
- LIS2MDL 3-Axis magnetic field intensity

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
   :zephyr-app: samples/shields/x_nucleo_iks4a1/standard/
   :host-os: unix
   :board: nucleo_f411re
   :goals: build flash
   :compact:

Sample Output
=============

 .. code-block:: console

    X-NUCLEO-IKS4A1 sensor dashboard

    LIS2MDL: Magn (gauss): x: -0.364, y: -0.523, z: -0.399
    LIS2MDL: Temperature: 22.4 C
    LSM6DSO16IS: Accel (m.s-2): x: -0.167, y: -0.249, z: 9.954
    LSM6DSO16IS: GYro (dps): x: 0.047, y: -0.052, z: -0.042
    LSM6DSO16IS: Temperature: 25.8 C
    LSM6DSV16X: Accel (m.s-2): x: 0.005, y: 0.053, z: 9.930
    LSM6DSV16X: GYro (dps): x: -0.000, y: 0.000, z: 0.005
    LPS22DF: Temperature: 25.2 C
    LPS22DF: Pressure:98.121 kpa
    10:: lis2mdl trig 1839
    10:: lsm6dso16is acc trig 3892
    10:: lsm6dsv16x acc trig 4412
    10:: lps22df trig 174

    <updated endlessly every 2 seconds>

References
**********

:ref:`x-nucleo-iks4a1`
