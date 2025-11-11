.. zephyr:code-sample:: vcnl4040
   :name: VCNL4040 Proximity and Ambient Light Sensor
   :relevant-api: sensor_interface

   Get proximity and ambient light data from a VCNL4040 sensor (polling & trigger mode).

Overview
********

 This sample periodically measures proximity and light for
 5 sec in the interval of 300msec in polling mode. Then threshold trigger mode
 is enabled with the high threshold value of 127 and data is fetched based
 on interrupt. The result is displayed on the console.

Requirements
************

 This sample uses the VCNL4040 sensor controlled using the I2C interface.
 Connect sensor INT for interrupt to Feather D5 pin on the Adafruit STM32F405 Feather.

References
**********

 - VCNL4040: https://www.vishay.com/docs/84274/vcnl4040.pdf

Building and Running
********************

 This project outputs sensor data to the console. It requires a VCNL4040
 sensor to be connected to the desired board.

 .. zephyr-app-commands::
    :zephyr-app: samples/sensor/vcnl4040/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

    get device vcnl4040
    Testing the polling mode.
    Proximity: 31
    Light (lux): 288

    <repeats for 5sec every 300ms>

    Polling mode test finished.
    Testing the trigger mode.
    Testing proximity trigger.
    ...
    Triggered.
    Proximity: 122

    <repeats whenever triggered for 5sec>

    Threshold trigger test finished.
    Trigger mode test finished.
