.. zephyr:code-sample:: sensing
   :name: Sensing subsystem
   :relevant-api: sensing_api

   Get high-level sensor data in defined intervals.

Overview
********

A simple sample that shows how to use the sensors with sensing subsystem APIs. It defines
two sensors, with the underlying device bmi160 emulator, and gets the sensor
data in defined interval.

The program runs in the following sequence:

#. Define the sensor in the dts

#. Open the sensor

#. Register call back.

#. Set sample interval

#. Run forever and get the sensor data.

Building and Running
********************

This application can be built and executed on :zephyr:board:`native_sim <native_sim>` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/sensing/simple
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

To build for another board, change "native_sim" above to that board's name.
At the current stage, it only support native sim.
