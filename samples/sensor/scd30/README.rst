.. _scd30:

SCD30: CO2 and RH/T Sensor Module
#################################

Description
***********

This sample application periodically measures the CO2 concentration in
the air. The result is printed to console. The sampling time can be
change by changing the value of SCD30_SAMPLE_TIME_SECONDS.

References
**********

 - `SCD30 sensor <https://www.sensirion.com/en/environmental-sensors/carbon-dioxide-sensors/carbon-dioxide-sensors-scd30/>`_

Wiring
*******

This sample uses the SCD30 sensor controlled using the I2C interface.
Connect Supply: **VDD**, **GND** and Interface: **SDA** and **SCL**.
The supply voltage can be in the 3.3V to 5.0V range.
Depending on the baseboard used, the **SDA** and **SCL** lines require Pull-Up
resistors.

Building and Running
********************

This project outputs sensor data to the console. It requires a SCD30
sensor. It should work with any platform featuring a I2C peripheral
interface.  It does not work on QEMU.  In this example below the
:ref:`nrf9160dk_nrf9160` board is used.


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/scd30
   :board: nrf9160dk_nrf9160
   :goals: build flash

Sample Output
=============

.. code-block:: console

   SCD30: 848 ppm
   SCD30: 848 ppm
   SCD30: 840 ppm
   SCD30: 836 ppm
   SCD30: 836 ppm
   SCD30: 832 ppm
   SCD30: 836 ppm
   SCD30: no new measurment yet
   Waiting for 1 second and retrying...
   SCD30: no new measurment yet
   Waiting for 1 second and retrying...
   SCD30: 832 ppm

<repeats endlessly>
