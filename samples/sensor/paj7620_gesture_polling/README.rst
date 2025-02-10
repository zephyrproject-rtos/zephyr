.. zephyr:code-sample:: paj7620_gesture_polling
   :name: PAJ7620 Gesture Polling
   :relevant-api: sensor_interface

   Get hand gesture data from PAJ7620 sensor.

Overview
********

This sample application gets the output of a gesture sensor (paj7620) in a polling manner and
prints the corresponding gesture to the console, each time one is detected.

Requirements
************

To use this sample, the following hardware is required:

* A board with I2C support
* PAJ7620 sensor

Building and Running
********************

This sample outputs data to the console. It requires a PAJ7620 sensor.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/gesture_polling
   :board: nucleo_f334r8
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   Gesture LEFT
   Gesture RIGHT
   Gesture UP
   Gesture DOWN
