.. _veml7700:

VEML7700: High Accuracy Ambient Light Sensor with I2C interface
###############################################################

Description
***********

This sample application periodically (every 2s) measures the ambient light
in lux. The result is written to the console.

References
**********

 - VEML7700: https://www.vishay.com/docs/84286/veml7700.pdf

Wiring
*******

This sample uses the VEML7700 sensor controlled using the I2C interface.
Connect Supply: **VDD**, **GND** and Interface: **SDA**, **SCL**.
The supply voltage must be between 2.5V and 3.6V.
Depending on the baseboard used, the **SDA** and **SCL** lines require Pull-Up
resistors.

Building and Running
********************

This project outputs sensor data to the console. It requires a VEML7700
sensor. It should work with any platform featuring a I2C peripheral interface.
It does not work on QEMU.
In this example below the :ref:`nrf52840dk_nrf52840` board is used.


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/veml7700
   :board: nrf52840dk_nrf52840
   :goals: build flash

Configuration
*************

All configuration of the sensor is done through properties in the DT node. For a
sample configuration have a look at the provided DT overlay!
