.. zephyr:code-sample:: producer_consumer
   :name: Producer Consumer
   :relevant-api: rtio

   Implement a producer->consumer pipe using RTIO.

Overview
********

Requirements
************

* A board with flash support or native_sim target

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/rtio` in the Zephyr tree.

The sample can be built for most platforms, the following commands build the
application for the native_sim target.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/rtio
   :board: native_sim
   :goals: build run
   :compact:

When running, the output on the console shows the operations of
submitting for, consuming, and processing the data.
