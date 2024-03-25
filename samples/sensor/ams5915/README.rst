.. _ams5915:

AMS5915 Pressure and Temperature Sensor
#######################################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`AMS 5915`_ sensor.

.. _AMS 5915:
   https://www.analog-micro.com/de/produkte/drucksensoren/board-mount-drucksensoren/ams5915/

The sample periodically reads temperature and pressure data from the
first available AMS5915 device discovered in the system.

Building and Running
********************

Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "analogmicro,ams5915";``. See
:dtcompatible:`analogmicro,ams5915` for the devicetree binding and see below for
examples and common configurations.

If the sensor is not built into your board, start by wiring the sensor pins
according to the connection diagram given in the `AMS5915 datasheet`_ at
page 7.

.. _AMS5915 datasheet:
   https://www.analog-micro.com/products/pressure-sensors/board-mount-pressure-sensors/ams5915/ams5915-datasheet.pdf

AMS5915 via Arduino I2C pins
============================

If you wired the sensor to an I2C peripheral on an Arduino header, build and
flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ams5915
   :goals: build flash
   :gen-args: -DDTC_OVERLAY_FILE=arduino_i2c.overlay

The devicetree overlay :zephyr_file:`samples/sensor/ams5915/arduino_i2c.overlay`
works on any board with a properly configured Arduino pin-compatible I2C
peripheral.

Board-specific overlays
=======================

If your board's devicetree does not have a AMS5915 node already, you can create
a board-specific devicetree overlay adding one in the :file:`boards` directory.
See existing overlays for examples.

The build system uses these overlays by default when targeting those boards, so
no ``DTC_OVERLAY_FILE`` setting is needed when building and running.

For example, to build for the :ref:`nucleo_g474re` using the
:zephyr_file:`samples/sensor/ams5915/boards/nucleo_g474re.overlay`
overlay provided with this sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ams5915
   :goals: build flash
   :board: nucleo_g474re

Sample Output
=============

The sample prints output to the serial console. ams5915 device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings:

.. code-block:: none

   *** Booting Zephyr OS build v3.6.0-665-gad7086a2dfe6 ***
   device is 0x8014630, name is ams5915@28
   temp: 22.851562; press: 6.103608;
   temp: 22.851562; press: 6.103608;
   temp: 22.851562; press: 6.103608;
   temp: 22.851562; press: 6.103608;
   

