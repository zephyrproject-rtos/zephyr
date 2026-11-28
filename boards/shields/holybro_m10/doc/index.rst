.. _holybro_m10_shield:

Holybro M10 GPS
###############

Overview
********

The `Holybro M10 GPS`_ is a GNSS module for drones, built around a u-blox M10 receiver. It also
carries an IST8310 magnetometer, a safety switch, a buzzer and an RGB LED driven by an IS31FL3195.

The shield describes the GNSS receiver, the magnetometer, the RGB LED, the safety switch and its
LED.

Requirements
************

The module plugs into the 10-pin full GPS port of the `Pixhawk Connector Standard`_. This shield
can only be used with a board that defines the ``pixhawk_full_gps1_serial``,
``pixhawk_full_gps1_i2c`` and ``pixhawk_full_gps1_connector`` node labels for that port. See
:ref:`shields` for more details.

The receiver is expected at its factory baud rate of 115200.

Programming
***********

Set ``--shield holybro_m10`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/magn_polling
   :board: vmu_rt1170/mimxrt1176/cm7
   :shield: holybro_m10
   :goals: build

The shield sets the ``gnss`` and ``magn0`` aliases used by :zephyr:code-sample:`gnss` and
:zephyr:code-sample:`magn_polling`.

References
**********

- `Holybro M10 GPS`_

.. _Holybro M10 GPS:
   https://holybro.com/products/m10-gps

.. _Pixhawk Connector Standard:
   https://github.com/pixhawk/Pixhawk-Standards
