.. zephyr:code-sample:: producer_consumer
   :name: Producer Consumer
   :relevant-api: rtio

   Implement a producer->consumer pipe using RTIO.

Overview
********

A simple sample that be used with any :ref:`supported board <boards>` showing  a
producer and consumer pattern implemented using RTIO. In this case the producer
is a k_timer generating cycle accurate timestamps of when the timer callback ran
in an ISR.

Building and Running
********************

The sample can be built for the native_sim target and run as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/rtio/producer_consumer
   :board: native_sim
   :goals: build run
   :compact:

When running, the output on the console shows the operations of submitting for,
consuming, and processing the data.
