.. _audio_codec_api:

Audio Codec
###########

Overview
********

The audio codec interface provides access to audio codec devices.

An audio codec is the component that bridges digital audio streams used by the SoC and the physical
audio world. Depending on the hardware, a codec can convert digital PCM data into analog signals for
speakers, headphones, or line outputs, convert analog microphone or line inputs into digital data
for capture, or do both.

Codecs typically exchange audio samples with the rest of the system over a Digital Audio Interface
(DAI), such as I2S or TDM. They may also expose controls for routing signals, adjusting volume,
muting channels, and reporting device faults.

From a Zephyr point of view, a codec device is configured with the desired DAI format and stream
parameters, then used for playback, capture, or full-duplex audio transfer.

Key concepts
************

The Audio Codec API separates configuration into three pieces that mirror the data path:

#. How the codec is connected to the SoC on the digital audio side,
#. Which signal path through the codec is active, and
#. Which runtime properties or callbacks the application uses while audio is running.

These pieces are combined in :c:struct:`audio_codec_cfg` and used with the related control APIs.

**DAI configuration** (:c:member:`audio_codec_cfg.dai_cfg`)
  This describes the codec's digital audio interface. It includes the master clock frequency, the
  DAI type, and the format-specific configuration in :c:type:`audio_dai_cfg_t`, such as PCM sample
  width, sample rate, channel count, block size, and transfer direction.

**Routing** (:c:member:`audio_codec_cfg.dai_route`)
  This selects the intended signal path through the codec, such as playback, capture, or both. Some
  codecs also support explicit signal routing with :c:func:`audio_codec_route_input` and
  :c:func:`audio_codec_route_output`.

**Properties and callbacks**
  Applications control runtime behavior with :c:func:`audio_codec_set_property`, for example to
  adjust volume or mute inputs and outputs. Drivers may also expose callbacks for completed
  transfers and device errors through :c:func:`audio_codec_register_done_callback` and
  :c:func:`audio_codec_register_error_callback`.

Typical application flow
========================

Typical use of the Audio Codec API is:

#. Get the codec device, usually from devicetree.
#. Fill an :c:struct:`audio_codec_cfg` structure with clock, DAI, route, and stream settings.
#. Call :c:func:`audio_codec_configure`.
#. Optionally configure routing, volume, mute state, or callbacks.
#. Start playback, capture, or both with :c:func:`audio_codec_start`.
#. Provide transmit data with :c:func:`audio_codec_write` and/or consume received data through the
   registered receive callback, depending on the chosen direction.
#. Stop transfers with :c:func:`audio_codec_stop` when audio is no longer needed.

Transfer model
==============

The API supports playback, capture, and full-duplex operation. For playback, the application sends
PCM blocks to the driver with :c:func:`audio_codec_write`. For capture, the driver delivers received
audio through the registered receive callback. Completion callbacks can be used to keep audio
buffers flowing without polling.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_AUDIO_CODEC`

API Reference
*************

.. doxygengroup:: audio_codec_interface
