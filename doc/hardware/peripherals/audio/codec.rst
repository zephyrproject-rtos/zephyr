.. _audio_codec_api:

Audio Codec
###########

Overview
********

An audio codec bridges the digital audio streams used by the SoC and the physical audio world.
Depending on the hardware, a codec can convert digital PCM data into analog signals for speakers,
headphones, or line outputs, convert analog microphone or line inputs into digital samples for
capture, or do both.

Codecs typically exchange samples with the rest of the system over a Digital Audio Interface (DAI),
such as I2S or TDM, and expose on-chip controls for selecting signal paths, adjusting volume, muting
channels, and reporting device faults.

In Zephyr, a codec device is configured with the desired DAI format and stream parameters, then
used for playback, capture, or full-duplex audio transfer.

Key concepts
************

The Audio Codec API splits initial configuration into two pieces that mirror the data path: how the
codec is connected to the SoC on the digital audio side, and which signal path through the codec is
active. Both are combined in :c:struct:`audio_codec_cfg` and applied with
:c:func:`audio_codec_configure`.

**DAI configuration** (:c:member:`audio_codec_cfg.dai_cfg`)
  Describes the codec's digital audio interface: master clock frequency, DAI type, and the
  format-specific configuration in :c:type:`audio_dai_cfg_t` (PCM sample width, sample rate, channel
  count, block size, and transfer direction).

**Routing** (:c:member:`audio_codec_cfg.dai_route`)
  Selects the active signal path through the codec: playback, capture, or both. Codecs that
  multiplex several physical inputs or outputs can additionally map individual channels to specific
  input or output terminals with :c:func:`audio_codec_route_input` and
  :c:func:`audio_codec_route_output`.

Once a codec is configured, applications adjust runtime behavior such as volume or mute state with
:c:func:`audio_codec_set_property`, and can subscribe to transfer-completion and device-error events
through :c:func:`audio_codec_register_done_callback` and
:c:func:`audio_codec_register_error_callback`.

Devicetree representation
=========================

A codec is typically described by two devicetree nodes that reflect its two interfaces:

- A node for the codec on its **control bus** (usually I2C or SPI), used by the driver to read and
  write codec registers. The codec's master clock (MCLK) is referenced from this node through
  ``clocks`` / ``clock-names``.
- A separate node for the **audio data path** (I2S, TDM, ...) that physically clocks PCM samples in
  and out of the codec.

The application gets the codec device from the codec node and configures it through the Audio Codec
API, while the data path node is used to actually move samples.

.. code-block:: devicetree
   :caption: Codec on I2C with audio data on I2S (Wolfson WM8904 example)

   &i2c0 {
       audio_codec: wm8904@1a {
           compatible = "wolfson,wm8904";
           reg = <0x1a>;
           clocks = <&audio_mclk>;
           clock-names = "mclk";
       };
   };

   &i2s0 {
       status = "okay";
       /* Pin control and other I2S properties go here. */
   };

The exact properties depend on the codec; for example, some codecs expose a ``clock-source``
property to select between an external MCLK pin and an internal PLL/FLL. See the
:zephyr_file:`dts/bindings/audio/` directory for the supported codec bindings, and
:zephyr_file:`samples/drivers/i2s/i2s_codec/` for a complete codec sample.

Typical application flow
========================

Typical use of the Audio Codec API is:

#. Get the codec device, usually from devicetree.
#. Fill an :c:struct:`audio_codec_cfg` structure with clock, DAI, route, and stream settings.
#. Call :c:func:`audio_codec_configure`.
#. Optionally adjust per-channel routing, volume, mute state, or register callbacks.
#. Start playback, capture, or both with :c:func:`audio_codec_start`.
#. Send playback data with :c:func:`audio_codec_write` and/or receive captured data via the
   registered receive callback.
#. Stop transfers with :c:func:`audio_codec_stop` when audio is no longer needed.

Transfer model
==============

Playback data flows from the application to the codec: the application hands PCM blocks to the
driver with :c:func:`audio_codec_write`, and the driver streams them out over the DAI.

Capture data flows the other way: the driver delivers received PCM buffers to the application
through the registered receive callback. Both directions can be active at the same time for
full-duplex use, and the optional transmit/receive completion callbacks let applications recycle
buffers without polling.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_AUDIO_CODEC`

API Reference
*************

.. doxygengroup:: audio_codec_interface
