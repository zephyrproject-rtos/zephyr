.. _dmic_app-sample:

Intel® S1000 Digital Microphone Sample Application
##################################################

Overview
********

This sample application uses a digital microphone driver to receive interleaved
audio samples from a microphone array.

This app configures the digital microphone driver, starts the driver,
receives a number of frames and then stops the driver. This process is
repeated a few times.

Requirements
************

This application uses an Intel® S1000 Customer Reference Board (CRB)
with a circular 8 microphone array

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/intel_s1000_crb/dmic
   :board:
   :goals: build
   :compact:

Sample Output
=============

Console output
--------------

.. code-block:: console

   [00:00:00.348,954] <inf> dmic_sample.dmic_sample_app: Starting DMIC sample app ...
   [00:00:00.510,004] <inf> dmic_sample.dmic_sample_app: Iteration 1/4 complete, 100 audio frames received.
   [00:00:00.670,004] <inf> dmic_sample.dmic_sample_app: Iteration 2/4 complete, 100 audio frames received.
   [00:00:00.830,004] <inf> dmic_sample.dmic_sample_app: Iteration 3/4 complete, 100 audio frames received.
   [00:00:00.990,004] <inf> dmic_sample.dmic_sample_app: Iteration 4/4 complete, 100 audio frames received.
   [00:00:00.990,005] <inf> dmic_sample.dmic_sample_app: Exiting DMIC sample app ...
