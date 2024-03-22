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

The device to be used by the sample is specified by defining a devicetree node
label named ``dmic_dev``.
The sample has been tested on :ref:`nrf52840dk_nrf52840` (nrf52840dk/nrf52840)
and :ref:`nrf5340dk_nrf5340` (nrf5340dk/nrf5340/cpuapp), and provides overlay
files for both of these boards.

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/audio/dmic`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/audio/dmic
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:
