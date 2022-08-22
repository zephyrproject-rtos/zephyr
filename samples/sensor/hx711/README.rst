.. _hx711:

HX711: 24-Bit Analog-to-Digital Converter (ADC) for Weigh Scales
################################################################

Overview
********
This sample makes use of an HX711.
Offset is calculated without any weight put on the scale.
Then the user is prompted to put the calibration-weight defined in device tree
on the scale and slope calibration is performed. After calibration the ADC is
periodicaly read for new measurements.

Requirements
************

This sample uses a HX711 sensor module from AliExpress (shengyang XFW-HX711), although
any HX711 hardware implementation should work.

To test the RATE pin functionality the RATE pin had to be desoldered from the module
and connected to the RATE pin defined in nrf52dk_nrf52832.overlay.
This is not necessary to be done for the example to run, although the actual sampling rate will not be affected.

If CONFIG_PM_DEVICE_RUNTIME = y rate will not be higher than 2~3Hz due to the long wake-up time (~600ms)
needed for HX711 to provide the first reading.

The formula used to calculate weight is :
weight = slope * (reading - offset)

References
**********

 - HX711 Datasheet  : https://datasheetspdf.com/pdf/842201/Aviasemiconductor/HX711/1

Building and Running
********************

 This project outputs sensor data to the console. It requires an HX711
 sensor.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/hx711
   :board: nrf52dk_nrf52832
   :goals: build
   :compact:

Sample Output
=============

 .. code-block:: console

*** Booting Zephyr OS build v4.1.0-952-g31cdc5be49e0 ***
[00:00:00.000,000] <inf> main: ============= start =============
[00:00:00.000,000] <inf> main: device is 0x8008138, name is hx711_node
[00:00:00.000,000] <inf> main: Calculating offset...
[00:00:01.192,000] <inf> main: waiting for known weight of 500.000000 grams...
[00:00:01.192,000] <inf> main:  5..
[00:00:02.192,000] <inf> main:  4..
[00:00:03.193,000] <inf> main:  3..
[00:00:04.193,000] <inf> main:  2..
[00:00:05.193,000] <inf> main:  1..
[00:00:06.193,000] <inf> main:  0..
[00:00:07.193,000] <inf> main: Calculating slope...
[00:00:08.350,000] <inf> main: == Test measure ==
[00:00:08.439,000] <inf> main: weight: 500.220107
[00:00:09.439,000] <inf> main: weight: 500.483162
[00:00:10.439,000] <inf> main: weight: 499.915763
[00:00:11.440,000] <inf> main: weight: 499.535871
[00:00:12.440,000] <inf> main: weight: 500.011957
