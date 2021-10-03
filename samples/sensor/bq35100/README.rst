.. _bq35100:

BQ35100: Lithium Primary Battery Fuel Gauge and End-Of-Service Monitor
#########################################

Overview
********

....


Wiring
*******

This sample uses the BQ35100 sensor controlled by the I2C interface.
Connect supply **VDD** and **GND**. The supply voltage comes directly from
the battery and can be max. 5.5 V.

Connect **GE** to a GPIO to control the Shutdown Mode.

Connect Interface: **SDA**, **SCL** and optionally connect **ALERT** to a
interrupt capable GPIO.


Building and Running
********************

This sample outputs sensor data to the console and can be read by any serial
console program. It should work with any platform featuring a I2C interface.
The platform in use requires a custom devicetree overlay.
In this example the :ref:`nrf52840_mdk` board is used. The devicetree
overlay of this board provides example settings for evaluation, which
you can use as a reference for other platforms.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bq35100
   :board: nrf52840_mdk
   :goals: build flash
   :compact:

Sample Output: 
==========================================

...


References
**********

BQ35100 Datasheet and Product Info:
 https://www.ti.com/product/BQ35100

.. _BQ35100 datasheet: https://www.ti.com/lit/gpn/bq35100
