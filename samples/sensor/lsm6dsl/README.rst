.. _lsm6dsl:

LSM6DSL: IMU sensor Monitor
###########################

Overview
********
This sample sets the LSM6DSL accelerometer and gyroscope to 104Hz
and enable a trigger on data ready. It displays on the console the
values for accelerometer and gyroscope, plus optionally the values of
any magnetometer or pressure sensor attached to it (sensorhub function).


Requirements
************

This sample uses the LSM6DSL sensor controlled using the I2C or SPI interface.
It has been tested on both :ref:`96b_argonkey` and disco_l475_iot1 board.

References
**********

- LSM6DSL http://www.st.com/en/mems-and-sensors/lsm6dsl.html

Building and Running
********************

 This project outputs sensor data to the console. It requires an LSM6DSL
 sensor, which is present on both the :ref:`96b_argonkey` and disco_l475_iot1 board.

Building on ArgonKey board
==========================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lsm6dsl
   :host-os: unix
   :board: 96b_argonkey
   :goals: build
   :compact:

Building on disco_l475_iot1 board
=================================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lsm6dsl
   :host-os: unix
   :board: disco_l475_iot1
   :goals: build
   :compact:

Building on nrf52840dk_nrf52840 board with x-nucleo-iks01a2 shield
==================================================================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lsm6dsl
   :host-os: unix
   :board: nrf52840dk_nrf52840
   :shield: x_nucleo_iks01a2
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    LSM6DSL sensor samples:

    accel (-3.184000 -0.697000 9.207000) m/s2
    gyro (0.065000 -0.029000 0.002000) dps
    magn (-0.042000 0.294000 -0.408000) gauss
    - (0) (trig_cnt: 190474)

    <repeats endlessly every 2 seconds>

.. note:: The magn row is displayed only when running sample onto 96b_argonkey board, where a magnetometer is connected to LSM6DSL.
