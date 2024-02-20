.. _icp101xx:

ICP101xx: Barometric Pressure and Temperature Sensor
####################################################

Description
***********

This sample application periodically measures the sensor
temperature and pressure, displaying the
values on the console along with a timestamp since startup.
It also displays the estimated altitude.

Wiring
*******

This sample uses an external breakout for the sensor.  A devicetree
overlay must be provided to identify the I2C used to control the sensor.

Building and Running
********************

After providing a devicetree overlay that specifies the sensor location,
build this sample app using:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/icp101xx
   :board: nrf52dk_nrf52832
   :goals: build flash

Sample Output
=============

.. code-block:: console

## Default configuration

Found device "icp101xx@63", getting sensor data
[00:00:00.266,479] <inf> ICP101XX_SAMPLE: Starting ICP101xx sample.
[00:00:00.273,803] <inf> ICP101XX_SAMPLE: temp 25.49 Cel, pressure 96.271438 kPa, altitude 447.208465 m
[00:00:00.280,914] <inf> ICP101XX_SAMPLE: temp 25.50 Cel, pressure 96.271331 kPa, altitude 447.234161 m
[00:00:00.288,024] <inf> ICP101XX_SAMPLE: temp 25.49 Cel, pressure 96.266685 kPa, altitude 447.636077 m
[00:00:00.295,135] <inf> ICP101XX_SAMPLE: temp 25.50 Cel, pressure 96.267951 kPa, altitude 447.537078 m
[00:00:00.302,246] <inf> ICP101XX_SAMPLE: temp 25.51 Cel, pressure 96.268577 kPa, altitude 447.488281 m
[00:00:00.309,356] <inf> ICP101XX_SAMPLE: temp 25.50 Cel, pressure 96.269340 kPa, altitude 447.414978 m
[00:00:00.316,467] <inf> ICP101XX_SAMPLE: temp 25.50 Cel, pressure 96.268562 kPa, altitude 447.473663 m
[00:00:00.323,547] <inf> ICP101XX_SAMPLE: temp 25.50 Cel, pressure 96.267341 kPa, altitude 447.596496 m

<repeats endlessly>
