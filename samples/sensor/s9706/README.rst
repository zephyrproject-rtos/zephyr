.. zephyr:code-sample:: s9706
   :name: S9706 digital color sensor
   :relevant-api: sensor_interface

   Get color intensity from a Hamamatsu S9706 digital color sensor.

Overview
********

This sample shows how to use the Zephyr :ref:`sensor` API driver for the
`Hamamatsu S9706`_ digital color sensor.

.. _Hamamatsu S9706:
   https://www.hamamatsu.com/eu/en/product/optical-sensors/photo-ic/color-sensor/rgb-color-sensor/S9706.html

The sample periodically reads the color intensity from the sensor and logs it to the console.
The integration time can be configured via :ref:`devicetree <dt-guide>` or during runtime. It
defaults to 5ms.

Building and Running
********************

The sample supports S9706 sensors connected via standard GPIOs. The devicetree must have an enabled
node with ``compatible = "hamamatsu,s9706";``. See :dtcompatible:`hamamatsu,s9706` for the
devicetree binding and see below for examples.

S9706 on an Arduino MKRZero
===========================

This sample includes a devicetree overlay for the Arduino MKRZero. Connect the data pins as follows:

=========== =====
Arduino Pin S9706
=========== =====
D0          DATA
D1          RANGE
D2          CLK
D3          GATE
=========== =====

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/s9706
   :goals: build flash
   :board: arduino_mkrzero

Sample Output
=============

The sample prints output to the serial console. Refer to your board's documentation for information
on connecting to its serial console.

Here is example output for the default application settings, where the sensor was covered after a
couple of seconds:

.. code-block:: console

   S9706 integration time : 5000 us
   [00:00:00.000,000] <dbg> S9706: s9706_sensor_init: Initializing S9706 sensor
   [00:00:00.000,000] <dbg> S9706: s9706_sensor_init: S9706 initialized.
   *** Booting Zephyr OS build v4.4.0-544-gb3f1f1e4c4ca ***
   S9706: R 129 G 76 B 69
   S9706: R 128 G 75 B 68
   S9706: R 127 G 74 B 68
   S9706: R 130 G 76 B 70
   S9706: R 128 G 75 B 68
   S9706: R 128 G 74 B 68
   S9706: R 129 G 76 B 69
   S9706: R 8 G 4 B 3
   S9706: R 8 G 4 B 3
   S9706: R 8 G 4 B 3
   S9706: R 8 G 4 B 3
   S9706: R 8 G 4 B 3
   S9706: R 8 G 4 B 3
