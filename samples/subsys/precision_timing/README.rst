.. zephyr:code-sample:: precision_timing
   :name: Precision timing
   :relevant-api: precision_time precision_clock precision_pi

   Use checked precision-time arithmetic, a precision-clock adapter, and a PI controller.

Overview
********

This sample implements a small software-backed precision clock and uses the
precision-timing API to set its absolute time and apply phase and rate
corrections. The rate correction is calculated by an independent PI controller.

Building and Running
********************

Build and run the sample on ``native_sim``:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/precision_timing
   :board: native_sim/native/64
   :goals: build run
   :compact:

Sample Output
=============

.. code-block:: console

   Precision clock: time 1000250000 ns, rate 16.25 ppm
