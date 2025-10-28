.. zephyr:code-sample:: dmic-gain-speaker
   :name: DMIC-Gain-Speaker Audio Pipeline

Overview
********

This sample demonstrates audio processing. It creates a simple audio pipeline
that captures audio from a digital microphone (DMIC), applies gain control, and
outputs the processed audio through an I2S codec to a speaker.

The sample showcases:

* Digital microphone audio capture
* Real-time audio gain processing
* I2S codec audio output
* Media pipeline creation and management

Requirements
************

* A board with digital microphone (DMIC) support
* A board with I2S support
* Sufficient RAM for audio buffering
* DMA support for audio operations

This sample has been tested on mimxrt685_evk/mimxrt685s/cm33

Building and Running
********************

This sample can be found under :zephyr_file:`examples/audio_example`.

For :zephyr:board:`mimxrt685_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: examples/audio_example
   :board: mimxrt685_evk/mimxrt685s/cm33
   :goals: build flash
   :compact:

Connect headphones or speakers to the audio output to hear the processed
audio from the DMIC.

Sample Output
*************

The application will start the audio pipeline and process audio in real-time.
Check for any error messages during initialization:

.. code-block:: console

   *** Booting Zephyr OS build ***
   [00:01:56.811,938] <inf> wolfson_wm8904: blk 512000
   [00:01:57.816,263] <inf> mp_zaud_dmic_src: Capture started

Configuration Options
*********************

The sample supports the following configuration options:

* **Gain Level**: Adjustable audio gain (0-1000 %), default is 90 %

Devicetree Configuration
************************

The sample requires proper devicetree configuration for:

* ``dmic_dev`` node label for dmic.
* ``i2s_codec_tx`` node alias for i2s.
* ``audio_codec`` node label for audio codec.

Implementation Details
**********************

The sample creates a three-element pipeline:

1. **DMIC Source** (``zaud_dmic_src``): Captures audio from digital microphone
2. **Gain Transform** (``zaud_gain``): Applies configurable gain to audio signal
3. **I2S Codec Sink** (``zaud_i2s_codec_sink``): Outputs processed audio

The pipeline uses DMA-compatible memory slabs for efficient audio buffer management.
The ``__nocache`` attribute ensures proper DMA operation by preventing cache
coherency issues.
