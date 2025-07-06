.. zephyr:code-sample:: amp_audio_output
    :name: Audio output AMP sample.

    AMP system example for NXP i.MX RTxxx platforms - audio output.

Overview
********

This sample demonstrates the use of the DSP domains on supported NXP i.MX RTxxx
platforms in an asymmetric multiprocessing (AMP) scenario. It's a sample with
separate projects for Cortex-M and DSP domains, that are built together into a
single resulting image using Sysbuild. The Cortex-M domain is responsible for
setting up the DSP domain (clock and power setup, code load and start), the DSP
domain is programmed to write a "hello world" message to the board's chosen
console UART, initialise hardware responsible for audio playback and perform
playback of a periodic audio signal.

Building and Running
********************

This sample can be built and started on supported boards as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/adsp/rtxxx/amp_audio_output
   :board: <board>
   :goals: build flash
   :build-args: --sysbuild
   :flash-args: -r jlink
   :compact:

Currently, these boards are supported:
- ``mimxrt685_evk/mimxrt685s/cm33``

The use of J-Link firmware on integrated debug probes of those boards or a
standalone J-Link probe is desired as the J-Link probes have the ability
to directly debug the Xtensa-based DSP cores.

Sample output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-1858-gab989bfb4894 ***
   Hello World! mimxrt685_evk/mimxrt685s/cm33
   [ARM] Starting DSP...
   *** Booting Zephyr OS build v4.1.0-1858-gab989bfb4894 ***
   [DSP] Hello World! mimxrt685_evk/mimxrt685s/hifi4
   [DSP] Will start playback.
   [DSP] Playback stopped.
