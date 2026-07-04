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
     dmic  [label="DMIC\nSource"];
     caps    [label="Caps\nFilter"];
     transform [label="Gain\nTransform"];
     i2s_codec [label="I2S Codec\nSink"];
     dmic -> caps -> transform -> i2s_codec;
   }

This pipeline consists of up to four elements:

- **DMIC source** - captures audio frames.
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

* A board with digital microphone (DMIC) support
* A board with I2S support
* Sufficient RAM for audio buffering
* DMA support for audio operations

This sample has been tested on mimxrt685_evk/mimxrt685s/cm33

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/mp/dmic_i2s/main.c`.

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
