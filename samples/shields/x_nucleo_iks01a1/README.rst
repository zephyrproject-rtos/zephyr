.. zephyr:code-sample:: x-nucleo-iks01a1
   :name: X-NUCLEO-IKS01A1 shield
   :relevant-api: sensor_interface

   Interact with all the sensors of an X-NUCLEO-IKS01A1 shield.

Overview
********
This sample enables all sensors of a X-NUCLEO-IKS01A1 shield, and then
periodically reads and displays data from the shield sensors:

- HTS221: Temperature and humidity
- LPS25HB: Atmospheric pressure
- LIS3MDL: 3-axis Magnetic field intensity
- LSM6DSL: 3-Axis Acceleration

Requirements
************

This sample communicates over I2C with the X-NUCLEO-IKS01A1 shield
stacked on a board with an Arduino connector. The board's I2C must be
configured for the I2C Arduino connector (both for pin muxing
and devicetree).
Please note that this sample can't be used with boards already supporting
one of the sensors available on the shield (such as disco_l475_iot1) as zephyr
does not yet support sensors multiple instances.

References
**********

-X-NUCLEO-IKS01A1: https://www.st.com/en/ecosystems/x-nucleo-iks01a1.html

Building and Running
********************

This sample runs with X-NUCLEO-IKS01A1 stacked on any board with a matching
Arduino connector. For this example, we use a :ref:`nucleo_f429zi_board` board.

.. zephyr-app-commands::
   :zephyr-app: samples/shields/x_nucleo_iks01a1
   :board: nucleo_f429zi
   :goals: build
   :compact:

Sample Output
=============

 .. code-block:: console

    X-NUCLEO-IKS01A1 sensor dashboard

    HTS221: Temperature:29.1 C
    HTS221: Relative Humidity:46.0%
    LPS25HB: Pressure:100.0 kpa
    LIS3MDL: Magnetic field (gauss): x: 0.1, y: -0.4, z: 0.4
    LSM6DS0: Acceleration (m.s-2): x: -0.0, y: -0.1, z: 9.7


    <updated endlessly every 2 seconds>
