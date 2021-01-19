.. _hx711:

HX711: 24-Bit Analog-to-Digital Converter (ADC) for Weigh Scales
################################################################

Overview
********
This sample makes use of an HX711.
Offset is calculated without any weight put on the scale.
Then the user is prompted to put the CONFIG_HX711_CALIBRATION_WEIGHT on the scale and
calibration is performed.

After calibration :
- Sampling rate is set to 10Hz
- A measurement is performed

- Sampling rate is set to 80Hz
- A measurement is performed

- Sampling rate is set to 10Hz
- HX711 is power cycled
- A measurement is performed

- Sampling rate is set to 80Hz
- HX711 is power cycled
- A measurement is performed

- Sampling rate is set to 10Hz
- HX711 is powered OFF
- A measurement is performed which returns -EACCES (-13)
- Sampling rate is set to 80Hz
- A measurement is performed which returns -EACCES (-13)

- sleep 10s and repeat

Requirements
************

This sample uses a HX711 sensor module from AliExpress, although any HX711 hardware implementation should work.
To test the RATE pin functionality the RATE pin had to be desoldered from the module and connected to the RATE pin
defined in nrf52dk_nrf52832.overlay.

This is not necessary to be done for the example to run, although the actual sampling rate will not be affected.

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

[00:00:00.579,620] <inf> main: device is 0x20000080, name is HX71
[00:00:01.097,961] <inf> main: Offset set to 852336
[00:00:01.097,961] <inf> main: waiting for known weight of 500 grams...
[00:00:01.097,991] <inf> main:  5..
[00:00:02.098,022] <inf> main:  4..
[00:00:03.098,083] <inf> main:  3..
[00:00:04.098,144] <inf> main:  2..
[00:00:05.098,205] <inf> main:  1..
[00:00:06.098,266] <inf> main:  0..
[00:00:07.098,327] <inf> main: Calculating slope..
[00:00:08.423,736] <inf> main: Test measure
[00:00:08.423,767] <inf> main: Setting sampling rate to 10Hz.
[00:00:08.423,919] <inf> main: weight: 500
[00:00:09.423,950] <inf> main: Setting sampling rate to 80Hz.
[00:00:09.481,231] <inf> main: weight: 500
[00:00:10.481,292] <inf> main: Test measure directly after reset
[00:00:10.481,323] <inf> main: Setting sampling rate to 10Hz.
[00:00:10.481,323] <inf> main: Setting HX711_POWER_OFF
[00:00:11.481,353] <inf> main: Setting HX711_POWER_ON
[00:00:12.149,902] <inf> main: weight: 500
[00:00:13.149,932] <inf> main: Setting sampling rate to 80Hz.
[00:00:13.149,963] <inf> main: Setting HX711_POWER_OFF
[00:00:14.149,993] <inf> main: Setting HX711_POWER_ON
[00:00:14.237,792] <inf> main: weight: 500
[00:00:15.237,854] <inf> main: Test measure during POWER_OFF
[00:00:15.237,884] <inf> main: Setting sampling rate to 10Hz.
[00:00:15.237,884] <inf> main: Setting HX711_POWER_OFF
[00:00:15.237,884] <err> main: Cannot take measurement: -13
[00:00:15.237,884] <inf> main: Setting sampling rate to 80Hz.
[00:00:15.237,915] <err> main: Cannot take measurement: -13

<repeats endlessly every 10 seconds>
