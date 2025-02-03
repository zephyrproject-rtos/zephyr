.. zephyr:code-sample:: bmg160
   :name: BMG160 3-axis gyroscope
   :relevant-api: sensor_interface

   Get temperature, and angular velocity data from a BMG160 sensor.

Overview
********

This sample shows how to use the Zephyr :ref:`sensor` API driver for the
`Bosch BMG160`_ environmental sensor.

.. _Bosch BMG160:
   https://www.mouser.com/datasheet/2/783/BST-BMG160-DS000-1509613.pdf

The sample first runs in polling mode for 15 seconds, then in trigger mode, display the temperature and angular velocity data from the BMG160 sensor.

Building and Running
********************

The sample can be configured to support BMG160 sensors connected via I2C. Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "bosch,bmg160";``. See
:dtcompatible:`bosch,bmg160` for the devicetree binding and see below for
examples and common configurations.

If the sensor is not built into your board, start by wiring the sensor pins
according to the connection diagram given in the `BMG160 datasheet`_ at
page 78.

.. _BMG160 datasheet:
   https://www.mouser.com/datasheet/2/783/BST-BMG160-DS000-1509613.pdf
