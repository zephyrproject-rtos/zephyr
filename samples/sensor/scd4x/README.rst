.. _scd4x:

SCD40 and SCD41: CO2, Temperature, and Humidity sensor
####################################################################################

Description
***********

This sample application uses the Sensirion SCD40/SCD41 sensor family to measure
ambient temperature, humidity, and CO2 in the air.

References
**********

 - `SCD40 sensor <https://sensirion.com/products/catalog/SCD40/>`_
 - `SCD41 sensor <https://sensirion.com/products/catalog/SCD41/>`_

Wiring
******

The SCD4x sensors are I2C devices, see the documentation for your board and
your particular sensor device for details on connecting it properly. 

Settings
********

For SCD40 sensors the app.overlay settings should work as-is. You may want to check the 
temperature offset, and altitide (affects CO2 measurement).

For SCD41 sensors you will want to change the model and possibly the mode setting, as they
support a low power periodic measurement and single-shot manual mode that the SCD40 does not.

Building and Running
********************

There is a generic app.overlay included in the sample that can be used for all boards, however
you may need to change i2c0 to i2c1 or i2c2, depending on how your board device tree defines
the i2c interfaces and how you have wired the sensor to the board.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/scd4x
   :board: esp32
   :goals: build flash


Sample Output
=============

.. code-block:: console

        [00:00:05.584,000] <inf> main: SCD4x Temperature: 25.85°, Humidity: 54.14%, CO2: 871 ppm
        [00:00:10.586,000] <inf> main: SCD4x Temperature: 26.24°, Humidity: 53.83%, CO2: 842 ppm
        [00:00:15.587,000] <inf> main: SCD4x Temperature: 26.47°, Humidity: 53.37%, CO2: 835 ppm
