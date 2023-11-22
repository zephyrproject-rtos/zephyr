.. zephyr:code-sample:: Speaker
   :name: Codec Speaker Sample
   :relevant-api: audio_codec_interface

   I2S Audio Codec Sample

I2S Audio Codec Sample
======================

Overview
--------
This sample code demonstrates the integration of the :ref:`I2S Interface <i2s_interface>` and
:ref:`Audio Codec API <audio_codec_interface>` to enable audio playback. The code initializes the
I2S and a audio codec device, configures them for audio playback, preloads silence, plays a sample
audio stream, and finally stops the audio playback.

Prerequisites
-------------
- Hardware setup with an I2S-compatible audio codec and a speaker,
  e.g. a NXP RT1060 EVK-B with an TAS2X63EVM.

Building and Running
--------------------

The code can be found in :zephyr_file:`samples/drivers/audio/speaker`.

To build and run the sample, follow these steps:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/audio/dmic
   :board: mimxrt1060_evkb
   :goals: build flash
   :compact:
