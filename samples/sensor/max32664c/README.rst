.. zephyr:code-sample:: max32664c
   :name: MAX32664C + MAX86141 Sensor Hub
   :relevant-api: sensor_interface

Get health data from a MAX32664C and a MAX86141 sensor (polling mode).

NOTE: This example requires sensor hub firmware 30.13.31!

Overview
********

This sample measures the heart rate and the blood oxygen saturation on a wrist.
It uses the MAX32664C sensor to control the MAX86141 sensor.

Requirements
************

This sample uses the MAX32664 sensor controlled using the I2C30 interface at
the nRF54L15-DK board.

References
**********

- MAX32664C: https://www.analog.com/en/products/max32664.html

Building and Running
********************

This project outputs sensor data to the console. It requires a MAX32664C
sensor to be connected to the desired board. An additional MAX86141 sensor
must be connected to the MAX32664C to provide the sensor data for the algorithms.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/max32664c/
   :goals: build flash

Sample Output
=============

.. code-block:: console

    [00:00:00.000,000] <inf> sensor: MAX32664C: Initializing...
    [00:00:01.600,000] <inf> sensor: MAX32664C: Initialization complete.
    [00:00:01.600,000] <inf> sensor: MAX32664C: HR: 75 bpm
    [00:00:01.600,100] <inf> sensor: MAX32664C: HR Confidence: 98
    [00:00:02.600,000] <inf> sensor: MAX32664C: HR: 76 bpm
    [00:00:02.600,100] <inf> sensor: MAX32664C: HR Confidence: 97
    [00:00:03.600,000] <inf> sensor: MAX32664C: HR: 74 bpm
    [00:00:03.600,100] <inf> sensor: MAX32664C: HR Confidence: 98
