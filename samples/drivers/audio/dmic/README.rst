.. zephyr:code-sample:: dmic
   :name: Digital Microphone (DMIC)
   :relevant-api: audio_dmic_interface

   Perform PDM transfers using different configurations.

Overview
********

This is a very simple application intended to show how to use the :ref:`Audio DMIC
API <audio_dmic_api>` and also to be an aid in developing drivers to implement this API.
It performs two PDM transfers with different configurations (using one channel
and two channels) but does not in any way process the received audio data.

Requirements
************

The device to be used by the sample is specified by the ``dmic0`` devicetree alias.
The sample provides an overlay file for :zephyr:board:`xiao_ble` (xiao_ble/nrf52840/sense),
which has an on-board PDM microphone. To exercise a driver on a board without a microphone,
use the :zephyr_file:`tests/drivers/audio/dmic_api` test instead.

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/audio/dmic`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/audio/dmic
   :board: xiao_ble/nrf52840/sense
   :goals: build flash
   :compact:
