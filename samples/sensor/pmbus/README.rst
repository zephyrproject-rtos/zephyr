.. zephyr:code-sample:: pmbus
   :name: PMBus sensor
   :relevant-api: sensor_interface

   Read the telemetry of a PMBus device through the sensor API.

Overview
********

This sample reads the output voltage, output current, output power and
temperature of a PMBus device every second, using the generic
:dtcompatible:`zephyr,pmbus` sensor driver. The device is reached through the
:dtcompatible:`zephyr,smbus-i2c` SMBus controller on an I2C controller of the
board.

Requirements
************

A PMBus device at address 0x40, with an output voltage in the ULINEAR16
mode. Channels of commands the device does not support are skipped.

The board needs an overlay that adds the ``smbus0`` node with the device,
as done for :zephyr:board:`native_sim` in
:zephyr_file:`samples/sensor/pmbus/boards/native_sim.overlay`.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/pmbus
   :board: native_sim
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   VOUT 3.300 V, IOUT 2.000 A, POUT 6.600 W, TEMP 35.000 C
