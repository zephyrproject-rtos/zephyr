.. zephyr:code-sample:: grove_temperature
   :name: Grove Temperature Sensor
   :relevant-api: sensor_interface

   Get temperature data from a Grove temperature sensor and display it on an LCD display.

Overview
********

This sample application gets the output of the grove temperature sensor and prints it to the
console, in units of celsius, once every second. When the :kconfig:option:`CONFIG_GROVE_LCD_RGB`
and :kconfig:option:`CONFIG_I2C` options are set, the temperature will also be displayed on the
Grove LCD display.

Requirements
************

To use this sample, the following hardware is required:

* A board with ADC support
* `Grove Temperature Sensor`_
* `Grove Base Shield`_
* `Grove LCD`_ [optional]

Wiring
******

The easiest way to connect the sensor is to connect it to a Grove shield on a board that supports
Arduino shields. Provide a devicetree overlay that specifies the sensor location. If using the
overlay provided for the sample, the sensor should be connected to A0 on the Grove shield. The
Grove LCD, if being used, should be connected to I2C on the Grove shield and the overlay needs to
contain an entry for it.

Building and Running
********************

Build and flash the sample as follows, changing ``nrf52dk_nrf52832`` to your board:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/grove_temperature
   :board: nrf52dk_nrf52832
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build v3.6.0-rc1-32-gba639ed6a893 ***
    Temperature: 22.90 C
    Temperature: 22.96 C
    Temperature: 22.82 C

.. _Grove Base Shield: https://wiki.seeedstudio.com/Base_Shield_V2/
.. _Grove Temperature Sensor: https://wiki.seeedstudio.com/Grove-Temperature_Sensor_V1.2/
.. _Grove LCD: https://wiki.seeedstudio.com/Grove-LCD_RGB_Backlight/
