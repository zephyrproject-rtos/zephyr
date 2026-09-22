.. zephyr:code-sample:: bmg160
   :name: BMG160 3-axis gyroscope
   :relevant-api: sensor_interface

   Get temperature, and angular velocity data from a BMG160 sensor.

Overview
********

This sample shows how to use the Zephyr :ref:`sensor` API driver for the
`Bosch BMG160`_ environmental sensor.

.. _Bosch BMG160:
   https://web.archive.org/web/20181111220522/https://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BMG160-DS000-09.pdf

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

Sample Output
=============

The sample prints output to the serial console. BMG160 device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings, assuming that only
one BMG160 sensor is connected to the standard Arduino I2C pins:

.. code-block:: none

   *** Booting Zephyr OS v3.7.0 ***
   Testing the polling mode.
   Gyro (rad/s): X=3.6, Y=14.0, Z=0.0f,
   Temperature (Celsius): 29.4
   Gyro (rad/s): X=5.1, Y=-9.0, Z=0.0f,
   Temperature (Celsius): 29.5
   ...
   Polling mode test finished.
   Testing the trigger mode.
   Gyro: Testing anymotion trigger.
   Gyro: rotate the device and wait for events.
   Gyro (rad/s): X=13.6, Y=14.0, Z=10.0f,
   Gyro: Anymotion trigger test finished.
   Gyro: Testing data ready trigger.
   Gyro (rad/s): X=3.6, Y=5.4, Z=1.1f,
   Gyro: Data ready trigger test finished.
   Trigger mode test finished
