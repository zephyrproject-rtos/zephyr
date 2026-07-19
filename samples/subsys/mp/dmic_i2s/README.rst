.. zephyr:code-sample:: dmic-i2s
   :name: dmic-i2s pipeline

      Build an audio pipeline that captures audio frames and applies gain control before playing them back.

Description
***********

This sample replaces and extends the basic :zephyr:code-sample:`i2s-codec` sample
(``samples/drivers/i2s/i2s_codec``) by rebuilding the same dmic-to-i2s workflow
on top of the MP pipeline framework.  The pipeline approach makes it easy to insert
additional processing stages (gain, filtering …) without modifying the application logic.

.. graphviz::

   digraph pipeline {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     dmic  [label="DMIC or I2S\nSource"];
     caps    [label="Caps\nFilter"];
     transform [label="Gain\nTransform"];
     i2s_codec [label="I2S Codec\nSink"];
     dmic -> caps -> transform -> i2s_codec;
   }

This pipeline consists of up to four elements:

- **Capture source** - captures audio frames, either from a digital microphone
  or from an I2S receiver, selected by ``CONFIG_SAMPLE_AUDIO_SOURCE``.
- **Capsfilter** *(optional)* - enforces a specific audio frame interval. 
Without it the pipeline still works but uses the default negotiated format.
- **Gain transform** *(optional)* – applies audio processing such as volume control.
- **I2S codec sink** – renders the resulting audio frames through an I2S codec to a speaker.

The sample showcases:

- Digital microphone audio capture
- Audio frame interval enforcement through caps filter
- Real-time audio gain processing
- I2S codec audio output
- Media pipeline creation and management

The pipeline uses DMA-compatible memory slabs for efficient audio buffer management.
The ``__nocache`` attribute ensures proper DMA operation by preventing cache
coherency issues.

Requirements
************

* A board with a digital microphone (DMIC) or an I2S receiver to capture from
* A board with I2S support for playback
* Sufficient RAM for audio buffering
* DMA support for audio operations

This sample has been tested on mimxrt685_evk/mimxrt685s/cm33.

It also runs on :zephyr:board:`native_sim`, which needs no hardware: the
native_sim DMIC and I2S drivers read and write PCM files, so the pipeline can
be exercised entirely on the host. See `Running on native_sim`_.

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/mp/dmic_i2s/src/main.c`.

For :zephyr:board:`mimxrt685_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mp/dmic_i2s
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
   [00:01:57.816,263] <inf> mp_zaud_dmic_src: Capture started

Running on native_sim
*********************

On :zephyr:board:`native_sim` the DMIC source reads PCM from a file and the I2S
sink writes PCM to another, so the pipeline runs without any audio hardware.

Build it with:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mp/dmic_i2s
   :board: native_sim/native/64
   :goals: build
   :compact:

Then run it, pointing the DMIC source at an input file and the I2S sink at an
output file:

.. code-block:: console

   build/zephyr/zephyr.exe --dmic0_file=input.pcm --i2s_tx_tx=output.pcm -stop_at=15

The relevant command line options are:

* ``--dmic0_file=<path>`` - PCM fed to the DMIC source. The driver loops the
  file, so the pipeline never runs out of input. With
  ``CONFIG_SAMPLE_AUDIO_SOURCE_I2S`` the input comes from ``--i2s_rx_rx=<path>``
  instead.
* ``--i2s_tx_tx=<path>`` - PCM written by the I2S codec sink.
* ``-stop_at=<seconds>`` - stop the simulation after the given time; without it
  the sample runs forever.

``zephyr.exe --help`` lists every option.

The dummy codec advertises 48 kHz, 16-bit stereo only, so that is what the
pipeline negotiates here, and ``input.pcm`` must be raw signed 16-bit
little-endian stereo at 48 kHz. ``output.pcm`` ends up longer than the input
because the DMIC driver loops the file, so compare only the first pass. The
pipeline applies gain and nothing else, so each output sample must equal the
input sample scaled by the gain and saturated to the 16-bit range.

The same check runs automatically under Twister, which feeds a generated full
scale ramp through the pipeline and compares the output:

.. code-block:: console

   west twister -p native_sim/native/64 -T samples/subsys/mp/dmic_i2s

Configuration Options
*********************

The sample supports the following configuration options:

* ``CONFIG_SAMPLE_AUDIO_SOURCE`` - which element the pipeline captures with:
  ``CONFIG_SAMPLE_AUDIO_SOURCE_DMIC`` (default) for a digital microphone, or
  ``CONFIG_SAMPLE_AUDIO_SOURCE_I2S`` for an I2S receiver. The I2S source needs
  an ``i2s-codec-rx`` alias. If the capture path runs through a codec, add an
  ``audio-codec-capture`` alias as well: the sink only ever configures a codec
  for playback, and codecs that treat the routes as mutually exclusive leave
  their ADC untouched otherwise.
* ``CONFIG_SAMPLE_AUDIO_GAIN_PERCENT`` - gain applied by the pipeline, 0 to
  1000 %, default 90 %. 0 mutes, 100 is unity, and values above 100 can clip,
  in which case samples saturate at full scale.

Buffer pool sizing
******************

The pipeline elements share one buffer pool, sized during caps negotiation from
the largest ``min_num_buffers`` any element reports. An element that
underreports it starves the pipeline: the pool is sized for the steady state
with no slack, so the first scheduling jitter leaves the source with no free
buffer. Depending on the driver that surfaces as a dropped frame or a stalled
capture, often after a suspiciously round number of blocks.

If the pipeline stops shortly after starting on a new board, raise the buffer
count that board's capture driver reports from ``get_caps()``. Raising
``CONFIG_MP_ZAUD_BUFFER_POOL_NUM_MAX`` alone does not help: it sizes the static
backing array, but the number of buffers actually used comes from negotiation.

Devicetree Configuration
************************

The sample requires proper devicetree configuration for:

* ``dmic_dev`` node label for the DMIC source.
* ``i2s-codec-rx`` node alias for the I2S source.
* ``i2s-codec-tx`` node alias for the I2S sink.
* ``audio_codec`` node label for the playback codec.
* ``audio-codec-capture`` node alias for a capture codec, if one is needed.
