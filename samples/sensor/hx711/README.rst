.. _hx711:

HX711: Avia Analog-to-digital Converter for Load Cells
######################################################

Description
***********

This sample application periodically (0.5 Hz) measures the sensor
load (calibrated and raw), displaying the values on the console,
alone with a timestamp since startup.

When triggered mode is enabled the measurements are displayed at the
rate they are produced by the sensor.

Wiring
******

This sample uses an external breakout for the sensor.  A devicetree
overlay must be provided to identify the GPIO pins used to control
the sensor.

Building and Running
********************

After providing a devicetree overlay that specifies the sensor location,
build this sample app using:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/hx711
   :board: blackpill_f401ce
   :goals: build flash

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.4.0-2085-gd9666ff4d091  ***
    [0:00:00.005]:
    calibrated: 6892.000000 grams
    raw: 6892.000000 grams
    [0:00:02.011]:
    calibrated: 6898.000000 grams
    raw: 6898.000000 grams
    [0:00:04.018]:
    calibrated: 6963.000000 grams
    raw: 6963.000000 grams
    [0:00:06.024]:
    calibrated: 6911.000000 grams
    raw: 6911.000000 grams
    [0:00:08.030]:
    calibrated: 6880.000000 grams
    raw: 6880.000000 grams
    [0:00:10.036]:
    calibrated: 6922.000000 grams
    raw: 6922.000000 grams
    [0:00:12.043]:
    calibrated: 6920.000000 grams
    raw: 6920.000000 grams

<repeats endlessly>
