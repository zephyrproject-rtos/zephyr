.. _grove_light:

Grove Light Sensor
##################

Overview
********

This sample application gets the output of the grove light sensor and prints it to the console, in
units of lux, once every second.

Requirements
************

To use this sample, the following hardware is required:

* A board with ADC support
* `Grove Light Sensor`_
* `Grove Base Shield`_

Wiring
******

The easiest way to connect the sensor is to connect it to a Grove shield on a board that supports
Arduino shields. Provide a devicetree overlay that specifies the sensor location. If using the
overlay provided for the sample, the sensor should be connected to A0 on the Grove shield.

Building and Running
********************

Build and flash the sample as follows, changing ``nrf52dk_nrf52832`` to your board:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/grove_light
   :board: nrf52dk_nrf52832
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build v3.6.0-rc1-32-gba639ed6a893 ***
    lux: 0.945751
    lux: 0.882292
    lux: 0.755973

.. _Grove Base Shield: https://wiki.seeedstudio.com/Base_Shield_V2/
.. _Grove Light Sensor: https://wiki.seeedstudio.com/Grove-Light_Sensor/
