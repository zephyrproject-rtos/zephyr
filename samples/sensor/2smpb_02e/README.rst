.. zephyr:code-sample:: 2smpb_02e
   :name: 2SMPB-02E pressure sensor
   :relevant-api: sensor_interface

   Read temperature and pressure data from a 2SMPB-02E sensor.

Overview
********

The sample periodically reads temperature and pressure data from the first available 2SMPB-02E device discovered in the system.

Building and Running
********************

The sample can be configured to support 2SMPB-02E sensors connected via I2C. Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "omron,2smpb-02e";``. See
:dtcompatible:`omron,2smpb-02e` for the devicetree binding and see below for
examples and common configurations.

If the sensor is not built into your board, start by wiring the sensor pins
according to the connection diagram given in the `2SMPB-02E datasheet`_ at
page 5.

.. _2SMPB-02E datasheet:
   https://omronfs.omron.com/en_US/ecb/products/pdf/en_2smpb_02e.pdf

Sample Output
=============

The sample prints output to the serial console. 2SMPB-02E device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings, assuming that only
one 2SMPB-02E sensor is connected to the I2C pins:

.. code-block:: none

   *** Booting Zephyr OS v4.2.0 ***
   Temperature: 24.887134°C
   Pressure: 1018.570556hPa
   Temperature: 24.892822°C
   Pressure: 1018.550231hPa
   Temperature: 24.891449°C
   Pressure: 1018.497009hPa
   Temperature: 24.892429°C
   Pressure: 1018.601867hPa
   Temperature: 24.885665°C
   Pressure: 1018.569580hPa
