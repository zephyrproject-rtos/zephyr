.. _bno055:

BNO055 accelerometer, gyroscope, magnetometer and orientation sensor
###################################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`Bosch BNO055`_ orientation sensor.

.. _Bosch BNO055:
   https://www.bosch-sensortec.com/products/smart-sensors/bno055/

The sample periodically reads euler angles from the
first available BNO055 device discovered in the system.

Building and Running
********************

The sample can be configured to support BNO055 sensors connected via I2C.
Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "bosch,bno055";``. See
:dtcompatible:`bosch,bno055` for the devicetree binding and see below for
examples and common configurations.

If the sensor is not built into your board, start by wiring the sensor pins
according to the connection diagram given in the `BNO055 datasheet`_ at
page 107.

.. _BNO055 datasheet:
   https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bno055-ds000.pdf

Board-specific overlays
=======================

If your board's devicetree does not have a BNO055 node already, you can create
a board-specific devicetree overlay adding one in the :file:`boards` directory.
See existing overlays for examples.

The build system uses these overlays by default when targeting those boards, so
no ``DTC_OVERLAY_FILE`` setting is needed when building and running.

For example, to build for the :ref:`disco_l475_iot1` using the
:zephyr_file:`samples/sensensor/bno055/boards/disco_l475_iot1.overlay
overlay provided with this sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bno055
   :goals: build flash
   :board: disco_l475_iot1

Sample Output
=============

The sample prints output to the serial console. BNO055 device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings, assuming that only
one BNO055 sensor is connected to the stm32l475 discovery board:

.. code-block:: none
   [00:00:00.000,000] <dbg> BNO055.bno055_chip_init: ID OK
   [00:00:00.600,000] <dbg> BNO055.bno055_chip_init: "BNO055" OK
   *** Booting Zephyr OS build v2.7.99-550-g3858cd6e53de  ***
   Found device "BNO055", getting sensor data
   Euler angles, H: 358.937500 R: -1.-625000 P: -5.-625000
   Euler angles, H: 358.937500 R: -1.-625000 P: -5.-625000
   Euler angles, H: 358.937500 R: -1.-625000 P: -5.-625000
