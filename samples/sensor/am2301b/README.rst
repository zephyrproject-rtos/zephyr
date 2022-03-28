.. _am2301b:

AM2301B: Aosong AM2301B Humidity and Temperature Module
#######################################################

Description
***********

This sample application periodically (1.0 Hz) measures the ambient
temperature and humidity. The result is written to the console.

Wiring
*******

The sample can be configured to support AM2301B sensors connected via I2C.
Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "aosong,am2301b";``.
See :dtcompatible:`aosong,am2301b` for the devicetree binding and see below for
examples and common configurations.

Building and Running
********************

After providing a devicetree overlay that specifies the sensor location,
build this sample app using:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/am2301b
   :board: stm32_min_dev_blue
   :goals: build flash

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.7.1  ***
   Found device "AM2301B_I2C", getting sensor data
   temp: 25.265000; humidity: 44.272000
   temp: 25.273000; humidity: 44.163000
   temp: 25.293000; humidity: 44.093000
   temp: 25.282000; humidity: 43.988000

<repeats endlessly>
