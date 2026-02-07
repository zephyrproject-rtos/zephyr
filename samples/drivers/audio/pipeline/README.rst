.. zephyr:code-sample:: audio-pipeline
   :name: Audio pipeline

      An audio pipeline built on libMP captures audio frames and applies gain control before playing them back.
        
Overview
********

::

    +---------------+     +---------------+     +------------- ----+     +------------------+
    |  DMIC Source  | --> |  Caps Filter  | --> |  Gain Transform  | --> |  I2S Codec Sink  |
    +---------------+     +---------------+     +------------------+     +------------------+

This sample demonstrates an audio pipeline consisting of maximum 4 elements. 
The DMIC source element captures audio frames.
The caps filter is used to enforce an audio frame interval.
The audio frames are then processed by a gain transform element and finally rendered through an I2S codec to a speaker.

The sample showcases:

* Digital microphone audio capture
* Audio frame interval enforcement through caps filter
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

This sample can be found under :zephyr_file:`samples/drivers/audio/pipeline/main.c`.

For :zephyr:board:`mimxrt685_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/audio/pipeline
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

The sample creates a four-element pipeline:

1. **DMIC Source** (``zaud_dmic_src``): Captures audio from digital microphone
2. **Caps Filter** (``caps_filter``): Enforces audio frame interval
3. **Gain Transform** (``zaud_gain``): Applies configurable gain to audio signal
4. **I2S Codec Sink** (``zaud_i2s_codec_sink``): Outputs processed audio

The pipeline uses DMA-compatible memory slabs for efficient audio buffer management.
The ``__nocache`` attribute ensures proper DMA operation by preventing cache
coherency issues.
