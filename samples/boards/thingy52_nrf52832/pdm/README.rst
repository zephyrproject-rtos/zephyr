.. _pdm_microphone_sample:

Nordic® Thingy:52 Digital PDM Microphone Sample Application
###########################################################

Overview
********

This sample application uses the Nordic PDM driver to sample audio from the onboard digital
microphone on the Thingy:52.

This app configures the PDM driver, starts the sampling, receives a number of frames,
and then stops the driver. This process is repeated a few times.

Requirements
************

This application uses a Nordic® Thingy:52 board, which includes a digital PDM microphone.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/thingy52_nrf52832/pdm
   :board: thingy52_nrf52832
   :goals: build
   :compact:

Sample Output
=============

Console Output
--------------

.. code-block:: console

   Successfully bound PDM driver.
   Successfully bound GPIO driver.
   [ITERATION 1/4] Received 100 samples of size 256
   [ITERATION 2/4] Received 100 samples of size 256
   [ITERATION 3/4] Received 100 samples of size 256
   [ITERATION 4/4] Received 100 samples of size 256
   Program finished.
