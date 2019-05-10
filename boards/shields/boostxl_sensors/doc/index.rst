.. _boostxl-sensors:

BOOSTXL-SENSORS: Sensors BoosterPack Plug-in Module
###################################################

Overview
********

The Sensors BoosterPack adds digital sensors to a TI LaunchPad |trade|
development kit. The plug-in module features gyroscope, accelerometer,
magnetometer, pressure, temperature, humidity, and light sensors.

Currently only the pressure, temperature, humidity and light sensors are
supported.

More information about the board can be found at the
`BOOSTXL_SENSORS website`_.

Requirements
************

This shield can be used with any TI LaunchPad |trade| development kit with
BoosterPack connectors.

Programming
***********

Set ``-DSHIELD=boostxl_sensors`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bme280
   :board: cc1352r1_launchxl
   :shield: boostxl_sensors
   :goals: build

References
**********

.. target-notes::

.. _BOOSTXL_SENSORS website:
   http://www.ti.com/tool/BOOSTXL-SENSORS
