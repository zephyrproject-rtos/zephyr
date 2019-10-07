.. _x-nucleo-iks01a3-shub-sample:

X-NUCLEO-IKS01A3: shield (Mode 2) sample
########################################

Overview
********
This sample is provided as an example to test the X-NUCLEO-IKS01A3 shield
configured in Sensor Hub mode (Mode 2).
Please refer to :ref:`x-nucleo-iks01a3` for more info on this configuration.

This sample enables LIS2DW12 and LSM6DSO sensors. Since all other shield
devices are connected to LSM6DSO, the LSM6DSO driver is configured in sensorhub
mode (CONFIG_LSM6DSO_SENSORHUB=y) with a selection of two maximum slaves
among LPS22HH, HTS221 and LIS2MDL (default is LIS2MDL + LPS22HH).

Then sensor data are displayed periodically

- LIS2DW12 3-Axis acceleration
- LSM6DSO 6-Axis acceleration and angular velocity
- LSM6DSO (from LIS2MDL) 3-Axis magnetic field intensity
- LSM6DSO (from LPS22HH) ambient temperature and atmospheric pressure

Optionally HTS221 can substitute one between LIS2MDL and LPS22HH

- LSM6DSO (from HTS221): ambient temperature and relative humidity


Requirements
************

This sample communicates over I2C with the X-NUCLEO-IKS01A3 shield
stacked on a board with an Arduino connector. The board's I2C must be
configured for the I2C Arduino connector (both for pin muxing
and devicetree). See for example the :ref:`nucleo_f401re_board` board
source code:

- :file:`$ZEPHYR_BASE/boards/arm/nucleo_f401re/nucleo_f401re.dts`
- :file:`$ZEPHYR_BASE/boards/arm/nucleo_f401re/pinmux.c`

Please note that this sample can't be used with boards already supporting
one of the sensors available on the shield (such as disco_l475_iot1)
as sensors multiple instances are not supported.

References
**********

- X-NUCLEO-IKS01A3: http://www.st.com/en/ecosystems/x-nucleo-iks01a3.html

Building and Running
********************

This sample runs with X-NUCLEO-IKS01A3 stacked on any board with a matching
Arduino connector. For this example, we use a :ref:`nucleo_f401re_board` board.

.. zephyr-app-commands::
   :zephyr-app: samples/shields/x_nucleo_iks01a3/sensorhub/
   :host-os: unix
   :board: nucleo_f401re
   :goals: build
   :compact:

Sample Output
=============

 .. code-block:: console


    X-NUCLEO-IKS01A3 sensor dashboard

    LIS2DW12: Accel (m.s-2): x: -0.077, y: 0.536, z: 9.648
    LSM6DSO: Accel (m.s-2): x: -0.062, y: -0.028, z: 10.035
    LSM6DSO: GYro (dps): x: -0.003, y: -0.001, z: 0.000
    LSM6DSO: Magn (gauss): x: -0.052, y: -0.222, z: -0.059
    LSM6DSO: Temperature: 27.9 C
    LSM6DSO: Pressure:100.590 kpa
    1:: lsm6dso acc trig 208
    1:: lsm6dso gyr trig 208

    <updated endlessly every 2 seconds>
