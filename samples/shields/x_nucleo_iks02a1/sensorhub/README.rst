.. zephyr:code-sample:: x-nucleo-iks02a1-shub
   :name: X-NUCLEO-IKS02A1 shield - SensorHub (Mode 2)
   :relevant-api: sensor_interface

   Interact with all the sensors of an X-NUCLEO-IKS02A1 shield using Sensor Hub mode.

Overview
********
This sample is provided as an example to test the X-NUCLEO-IKS02A1 shield
configured in Sensor Hub mode (Mode 2).
Please refer to :ref:`x-nucleo-iks02a1` for more info on this configuration.

This sample enables IIS2DLPC and ISM330DHCX sensors. Since all other shield
devices are connected to ISM330DHCX, the ISM330DHCX driver is configured in sensorhub
mode (CONFIG_ISM330DHCX_SENSORHUB=y) with the IIS2MDC magnetometer as external
slave.

Then sensor data are displayed periodically

- IIS2DLPC 3-Axis acceleration
- ISM330DHCX 6-Axis acceleration and angular velocity
- ISM330DHCX (from IIS2MDC) 3-Axis magnetic field intensity


Requirements
************

This sample communicates over I2C with the X-NUCLEO-IKS02A1 shield
stacked on a board with an Arduino connector. The board's I2C must be
configured for the I2C Arduino connector (both for pin muxing
and devicetree). See for example the :zephyr:board:`nucleo_f401re` board
source code:

- :zephyr_file:`boards/st/nucleo_f401re/nucleo_f401re.dts`
- :zephyr_file:`boards/st/nucleo_f401re/pinmux.c`

Please note that this sample can't be used with boards already supporting
one of the sensors available on the shield (such as disco_l475_iot1)
as sensors multiple instances are not supported.

References
**********

- X-NUCLEO-IKS02A1: https://www.st.com/en/ecosystems/x-nucleo-iks02a1.html

Building and Running
********************

This sample runs with X-NUCLEO-IKS02A1 stacked on any board with a matching
Arduino connector. For this example, we use a :zephyr:board:`nucleo_f401re` board.

.. zephyr-app-commands::
   :zephyr-app: samples/shields/x_nucleo_iks02a1/sensorhub/
   :host-os: unix
   :board: nucleo_f401re
   :goals: build
   :compact:

Sample Output
=============

 .. code-block:: console


    X-NUCLEO-IKS02A1 sensor Mode 2 dashboard

    IIS2DLPC: Accel (m.s-2): x: 0.077, y: -0.766, z: 9.878
    ISM330DHCX: Accel (m.s-2): x: 0.383, y: -0.234, z: 9.763
    ISM330DHCX: GYro (dps): x: 0.004, y: 0.003, z: -0.005
    ISM330DHCX: Magn (gauss): x: 0.171, y: 0.225, z: -0.363
    7:: iis2dlpc trig 1215
    7:: ism330dhcx acc trig 2494
    7:: ism330dhcx gyr trig 2494

    <updated endlessly every 2 seconds>
