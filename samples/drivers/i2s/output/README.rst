.. zephyr:code-sample:: i2s-output
   :name: I2S output
   :relevant-api: i2s_interface

   Send I2S output stream

Overview
********

This sample demonstrates how to use an I2S driver to send an output stream of
audio data. Currently, no codec is used with this sample. The I2S output can
be verified with a signal analyzer.

The sample will send a short burst of audio data, consisting of a sine wave.
The I2S TX queue will then be drained, and audio output will stop.

Requirements
************

The I2S device to be used by the sample is specified by defining
a devicetree alias named ``i2s_tx``

This sample has been tested on :ref:`mimxrt1060_evk` (mimxrt1060_evkb)

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/i2s/output`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i2s/output
   :board: mimxrt1060_evkb
   :goals: build flash
   :compact:
