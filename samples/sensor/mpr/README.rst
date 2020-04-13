.. _mpr-sample:

MPR Pressure Sensor
###################

Overview
********

This sample application periodically (1Hz) measures atmospheric pressure in
kilopascal. The result is written to the console.

References
**********

- MPR: https://sensing.honeywell.com/micropressure-mpr-series

Wiring
******

This sample uses an MPRLS0025PA00001A sensor controlled using the i2c
interface. Connect **VIN**, **GND** and Interface: **SDA**, **SCL**.

Building and Running
********************

This project outputs sensor data to the console. It requires a sensor from the
MPR series.
It does not work on QEMU.
In the sample below the :ref:`arduino_due` board is used.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mpr
   :board: arduino_due
   :goals: build flash

Sample Output
*************

.. code-block:: console

   pressure value: 101.976303 kPa
   pressure value: 101.986024 kPa
   pressure value: 101.989736 kPa
   pressure value: 101.987424 kPa
   pressure value: 101.992099 kPa
   pressure value: 101.989171 kPa
   pressure value: 101.984226 kPa

   <repeats endlessly>
