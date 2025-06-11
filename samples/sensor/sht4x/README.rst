.. _sht4x:

SHT4X: High Accuracy Digital I2C Humidity Sensor
#################################################

Description
***********

This sample application periodically reads the ambient temperature and humidity
from an SHT4X device. The result is written to the console.

The SHT4X sensor has an optional heater that can be useful for specific
environments or applications (refer to the datasheet for more information).
To utilize the heater, check the Kconfig options for this application.

References
**********

 - `SHT4X sensor <https://sensirion.com/products/catalog/SHT45>`_

Wiring
******

This sample uses the SHT4X sensor controlled via the I2C interface.
Connect Supply: **VDD**, **GND** and Interface: **SDA**, **SCL**.
The supply voltage can be in the 1.7V to 3.6V range.
Depending on the baseboard used, the **SDA** and **SCL** lines may require Pull-Up
resistors.

Building and Running
********************

This project outputs sensor data to the console. It requires an SHT4X
sensor. It should work with any platform featuring an I2C peripheral
interface. This example includes a device tree overlay
for the :zephyr:board:`blackpill_f411ce` board.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sht4x
   :board: blackpill_f411ce
   :goals: build flash

Sample Output
=============

.. code-block:: console

        *** Booting Zephyr OS build v2.6.0-rc1-315-g50d8d1187138  ***
        SHT4X: 23.64 Temp. [C] ; 30.74 RH [%]
        SHT4X: 23.66 Temp. [C] ; 32.16 RH [%]
        SHT4X: 23.63 Temp. [C] ; 30.83 RH [%]

The datasheet states that the sensor measures temperature and humidity in degrees Celsius
and percent relative humidity respectively.
