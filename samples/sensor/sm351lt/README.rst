.. _sm351lt:

SM351LT: Magnetoresistive Sensor Example
########################################

Overview
********

This sample application periodically polls an SM351LT magnetoresistive sensor
and displays if a magnet near to the sensor, via the console.

Requirements
************

This sample uses the Honeywell SM351LT sensor.

References
**********

For more information about the SM351LT magnetoresistive sensor see
https://sensing.honeywell.com/SM351LT-low-power

Building and Running
********************

The SM351LT (or compatible) sensors are available on the following boards:

* :ref:`bt510`

Building on bt510
==================

:ref:`bt510` includes a Honeywell SM351LT magnetoresistive sensor.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sm351lt
   :board: bt510
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    Polling at 0.5 Hz
    #1 @ 6 ms: 1
    #2 @ 2007 ms: 0
    #3 @ 4009 ms: 0
    #4 @ 6010 ms: 1
    #5 @ 8012 ms: 1
