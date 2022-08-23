.. _hx711:

HX711: 24-Bit Analog-to-Digital Converter (ADC) for Weigh Scales
################################################################

Overview
********
This sample makes use of an HX711.
Offset is calculated without any weight put on the scale.
Then the user is prompted to put the calibration-weight defined in device tree
on the scale and slope calibration is performed.

After calibration :
- Sampling rate is set to 10Hz
- A measurement is performed

- Sampling rate is set to 80Hz
- A measurement is performed

- Reboot after 5sec.

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

*** Booting Zephyr OS build v3.0.0-rc3-1-g655b0d21274a  ***
[00:00:00.325,775] <inf> main: ============= start =============
[00:00:00.325,805] <inf> main: device is 0x81c8, name is HX711
[00:00:00.325,805] <inf> main: Calculating offset...
[00:00:00.890,899] <inf> main: waiting for known weight of 500.00000000 grams...
[00:00:00.890,930] <inf> main:  5..
[00:00:01.890,991] <inf> main:  4..
[00:00:02.891,082] <inf> main:  3..
[00:00:03.891,174] <inf> main:  2..
[00:00:04.891,265] <inf> main:  1..
[00:00:05.891,357] <inf> main:  0..
[00:00:06.891,448] <inf> main: Calculating slope...
[00:00:08.456,665] <inf> main: == Test measure ==
[00:00:08.456,695] <inf> main: = Setting sampling rate to 10Hz.
[00:00:08.959,747] <inf> main: weight: 500.679776
[00:00:09.959,808] <inf> main: = Setting sampling rate to 80Hz.
[00:00:10.026,550] <inf> main: weight: 500.528026
[00:00:10.026,550] <inf> main: ====== Reboot in 5sec. =========

<repeats endlessly every 10 seconds>
