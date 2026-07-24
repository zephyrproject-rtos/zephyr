.. zephyr:code-sample:: ntc-thermistor
   :name: NTC Thermistor Generic
   :relevant-api: sensor_interface

   Read temperature from an NTC thermistor using the generic driver.

Overview
********

This sample demonstrates how to read ambient temperature from an NTC
thermistor using Zephyr's generic NTC thermistor driver and the sensor API.

The sample reads temperature once per second and prints it to the console.

Requirements
************

A board with an ADC peripheral and an NTC thermistor connected to an ADC
channel. A board overlay is required to describe the thermistor circuit.

Building and Running
********************

For ESP32 DevKitC:

.. code-block:: console

   west build -p auto -b esp32_devkitc samples/sensor/ntc_thermistor

Sample Output
*************

.. code-block:: console

   Temperature: 27.500000 C
   Temperature: 27.312500 C
   Temperature: 27.437500 C
