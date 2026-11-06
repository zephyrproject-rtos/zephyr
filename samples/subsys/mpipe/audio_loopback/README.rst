.. zephyr:code-sample:: audio-loopback
   :name: audio loopback pipeline
   :relevant-api: mpipe_aud mpipe_base

   Capture audio from a digital microphone or I2S input, apply gain control,
   and play it through an I2S codec.

Overview
********

This sample replaces and extends the basic :zephyr:code-sample:`i2s_codec`
sample by rebuilding the same audio loopback workflow on top of the Multimedia
Pipeline framework. The pipeline approach makes it easy to insert additional
processing stages without modifying the application logic.

.. graphviz::

   digraph pipeline {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     source  [label="DMIC or I2S\nSource"];
     caps    [label="Caps\nFilter"];
     transform [label="Gain\nTransform"];
     i2s_codec [label="I2S Codec\nSink"];
     source -> caps -> transform -> i2s_codec;
   }

This pipeline consists of up to four elements:

- **DMIC or I2S source** - captures audio frames.
- **Caps filter** *(optional)* - enforces a specific audio frame interval.
  Without it the pipeline still works but uses the default negotiated format.
- **Gain transform** - applies audio processing such as volume control.
- **I2S codec sink** - renders the resulting audio frames through an I2S codec to a speaker.

The sample showcases:

- Digital microphone or I2S audio capture
- Audio frame interval enforcement through caps filter
- Real-time audio gain processing
- I2S codec audio output
- Media pipeline creation and management

The pipeline uses DMA-compatible memory slabs for efficient audio buffer management.
The ``__nocache`` attribute ensures proper DMA operation by preventing cache
coherency issues.

Requirements
************

* A board with digital microphone (DMIC) or I2S capture support
* A board with I2S support
* Sufficient RAM for audio buffering
* DMA support for audio operations

This sample has been tested on mimxrt685_evk/mimxrt685s/cm33

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/mpipe/audio_loopback/src/main.c`.

For :zephyr:board:`mimxrt685_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/audio_loopback
   :board: mimxrt685_evk/mimxrt685s/cm33
   :goals: build flash
   :compact:

Connect headphones or speakers to the audio output to hear the processed
audio from the DMIC.

Sample Output
*************

The application will start the pipeline and process audio in real-time.
Check for any error messages during initialization:

.. code-block:: console

   *** Booting Zephyr OS build ***
   [00:01:56.811,938] <inf> wolfson_wm8904: blk 512000
   [00:01:57.816,263] <inf> mpipe_aud_dmic_src: Capture started

Configuration Options
*********************

* :kconfig:option:`CONFIG_USE_I2S_TARGET_CODEC_CONTROLLER` selects which side
  generates the I2S BCK and WS signals. When set, the codec generates them and
  the I2S peripheral consumes them; when unset, the roles are reversed.
* :kconfig:option:`CONFIG_SAMPLE_AUDIO_SOURCE_I2S` selects I2S capture. Leave it
  disabled to use the DMIC source.

The gain is not a Kconfig option: ``main.c`` sets it to 90 % through
``MPIPE_PROP_AUD_TRANSFORM_GAIN``. The gain element accepts 0 to 1000 %.

Devicetree Configuration
************************

The sample requires proper devicetree configuration for:

* ``dmic_dev`` node label for DMIC capture, or ``i2s_codec_rx`` node alias for
  I2S capture.
* ``i2s_codec_tx`` node alias for i2s.
* ``audio_codec`` node label for audio codec.
