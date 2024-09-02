.. zephyr:code-sample:: tmp112
   :name: TMP112 Temperature Sensor
   :relevant-api: sensor_interface

   Get temperature data from a TMP112 sensor (polling & trigger mode).

Overview
********

A sample showing how to use the :dtcompatible:`ti,tmp112` sensor.

Requirements
************

A board with this sensor built in to its :ref:`devicetree <dt-guide>`, or a
devicetree overlay with such a node added.

Building and Running
********************

To build and flash the sample for the :ref:`frdm_k64f`:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/tmp112
   :board: frdm_k64f
   :goals: build flash
   :compact:
