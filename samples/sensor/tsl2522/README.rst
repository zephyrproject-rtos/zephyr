.. zephyr:code-sample:: tsl2522_sensor
   :name: TSL2522 Light Sensor Sample
   :relevant-api: sensor_interface

   Get illuminance data from and configure a ams OSRAM TSL2522.

Overview
********

This sample application gets the output of the TLS2522 light sensor and prints it to the console,
in units of lux. It periodically registers and unregisters a overflow and a data_ready handler,
and modifies the
- SENSOR_ATTR_GAIN
- SENSOR_ATTR_NUMBER_OF_SAMPLES
- SENSOR_ATTR_MEASUREMENT_TIME_STEPS
attributes

Requirements
************

To use this sample, the following hardware is required:

* A board with I2C support and a GPIO as interrupt pin
* A board/shield with a TSL2522 sensor (e.g., `Ambient 25 Click`_), available as
* ``light-sensor`` Devicetree alias.

Wiring
******

The wiring depends on the specific light sensor and board being used. Provide a devicetree
overlay that specifies the sensor configuration for your setup.

Building and Running
********************

Build and flash the sample as follows, changing ``frdm_mcxn947/mcxn947/cpu0`` to your board:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/tsl2522
   :board: frdm_mcxn947/mcxn947/cpu0
   :shield: mikroe_ambient_25_click
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build 077a143b4ade ***
    lux: 221.460763 - overflow_handler 0
    lux: 222.944368 - overflow_handler 0
    lux: 277.157482 - overflow_handler 0
    lux: 266.994689 - overflow_handler 0
    lux: 272.179171 - overflow_handler 0
    lux: 197.628198 - overflow_handler 0
    change SENSOR_ATTR_GAIN to 16.0x.
    lux: 214.265106 - overflow_handler 0
    lux: 214.951170 - overflow_handler 0
    lux: 214.176189 - overflow_handler 0
    change SENSOR_ATTR_GAIN to 8.0x.
    lux: 222.893759 - overflow_handler 0
    lux: 222.741358 - overflow_handler 0
    register overflow handler.
    lux: 221.966506 - overflow_handler 0
    lux: 222.357869 - overflow_handler 0
    lux: 222.291770 - overflow_handler 0
    lux: 221.438491 - overflow_handler 0

.. _Ambient 25 Click: https://www.mikroe.com/ambient-25-click
