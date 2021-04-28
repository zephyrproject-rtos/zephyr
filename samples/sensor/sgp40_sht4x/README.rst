.. _sgp40_sht4x:

SGP40 and SHT4X: High accuracy digital I2C humidity sensor and multipixel gas sensor
#################################################

Description
***********

This sample application periodically (2 Hz) measures the ambient
temperature and humidity. The result is written to the console.
Optionally, you can choose to use the on-chip T/RH compensation of the SGP40
by feeding the values measured by the SHT4X into it.

References
**********

 - `SHT4X sensor <https://www.sensirion.com/en/environmental-sensors/humidity-sensors/humidity-sensor-sht4x/>`_
 - `SGP40 sensor <https://www.sensirion.com/en/environmental-sensors/gas-sensors/sgp40/>`_

Wiring
*******

This sample uses the SHT4X and SGP40 sensor controlled using the I2C interface.
Connect Supply: **VDD**, **GND** and Interface: **SDA**, **SCL**.
The supply voltage can be in the 1.7V to 3.6V range.
Depending on the baseboard used, the **SDA** and **SCL** lines require Pull-Up
resistors.

Building and Running
********************

This project outputs sensor data to the console. It requires a SHT4X and a SGP40
sensor. It should work with any platform featuring a I2C peripheral
interface.  It does not work on QEMU.  In this example below the
:ref:`seeeduino_xiao` board is used.


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sgp40_sht4x
   :board: blackpill_f411ce
   :goals: build flash



Sample Output
=============

.. code-block:: console

        SHT4X: 23.62 Temp. [C] ; 35.71 RH [%] -- SGP40: 31448 Gas [a.u.]
        SHT4X: 23.62 Temp. [C] ; 34.17 RH [%] -- SGP40: 31437 Gas [a.u.]
        SHT4X: 25.23 Temp. [C] ; 52.28 RH [%] -- SGP40: 31005 Gas [a.u.]
        SHT4X: 26.04 Temp. [C] ; 72.26 RH [%] -- SGP40: 30560 Gas [a.u.]
        SHT4X: 25.68 Temp. [C] ; 80.21 RH [%] -- SGP40: 30341 Gas [a.u.]
        SHT4X: 25.54 Temp. [C] ; 82.86 RH [%] -- SGP40: 30352 Gas [a.u.]
        SHT4X: 25.25 Temp. [C] ; 80.69 RH [%] -- SGP40: 30373 Gas [a.u.]
        SHT4X: 25.22 Temp. [C] ; 76.35 RH [%] -- SGP40: 30413 Gas [a.u.]
        SHT4X: 25.03 Temp. [C] ; 72.78 RH [%] -- SGP40: 30452 Gas [a.u.]


        <repeats endlessly>

The datasheet states that the raw sensor signal for the SGP40 ist proportional
to the logarithm of the sensors resistance, that is why it is labeled
as [a.u.] (arbitrary units) in the example.

