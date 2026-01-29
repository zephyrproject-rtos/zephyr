.. zephyr:code-sample:: bhi260ap
   :name: BHI260AP smart sensor
   :relevant-api: sensor_interface

   Configure and read game rotation vector data from a BHI260AP sensor.

Description
***********

This sample application configures the game rotation virtual sensor to
provide data at 50Hz. The result is written to the console.

References
**********

 - BHI260AP: https://www.bosch-sensortec.com/products/smart-sensor-systems/bhi260ap/

Wiring
*******

This sample uses the BHI260AP sensor controlled using the I2C interface.
Connect Supply: **VDD**, **VDDIO**, **GND** and Interface: **SDA**, **SCL**.
The supply voltage should be 1.8 V.
Depending on the baseboard used, the **SDA** and **SCL** lines require Pull-Up
resistors.

Building and Running
********************

This project outputs sensor data to the console. It requires a BHI260AP
sensor. It should work with any platform featuring a I2C peripheral interface.
It does not work on QEMU.
In this example below the :zephyr:board:`nrf54l15dk` board is used.
The default firmware file for the BHI260AP sensor is used, provided by the driver.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bhi260ap
   :board: nrf54l15dk/nrf54l15/cpuapp
   :goals: build flash

Sample Output
=============

.. code-block:: console

   Device 0x81dc name is bhi260ap@28
   GRVX:  0.017150; GRVY:  0.003723; GRVZ: -0.000122; GRVW:  0.999816
   GRVX:  0.017089; GRVY:  0.003540; GRVZ:  0.000000; GRVW:  0.999816
   GRVX:  0.016967; GRVY:  0.003540; GRVZ:  0.000000; GRVW:  0.999816
   GRVX:  0.016906; GRVY:  0.003540; GRVZ:  0.000061; GRVW:  0.999816
   GRVX:  0.016845; GRVY:  0.003479; GRVZ:  0.000122; GRVW:  0.999816

   <repeats endlessly>
