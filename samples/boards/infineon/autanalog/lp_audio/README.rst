.. zephyr:code-sample:: autanalog-lp-audio
   :name: AutAnalog Low-Power Audio
   :relevant-api: adc_interface

   Sample audio from an analog microphone using the AutAnalog LP mode.

Overview
********

This sample demonstrates the low-power autonomous analog subsystem for
audio input, replicating the Infineon Battery Powered Local Voice
reference application using Zephyr APIs.

The analog signal path is:

1. **CTB0 OA0** — open-loop opamp amplifies the analog microphone signal.
2. **PTCOMP** — two comparators perform Acoustic Activity Detection (AAD)
   by comparing the preamp output against PRB voltage thresholds.
3. **SAR ADC** — samples the preamp output in LP mode through an internal
   MUX channel.  Results are collected in a 512-word FIFO.
4. A watermark interrupt fires every 160 samples (~10 ms) and the main
   loop prints a summary of each audio frame along with any AAD events.

Building and Running
********************

The sample requires:

* An analog microphone connected to the CTB0 OA0 inputs (P14.0 / P14.1).
* The ``io-channels`` property in the ``zephyr,user`` node to reference
  MUX channel 0 of the SAR ADC.

For :zephyr:board:`kit_pse84_eval`, the
``kit_pse84_eval/pse846gps2dbzc4a/m55`` overlay provides these settings.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/infineon/autanalog/lp_audio
   :board: kit_pse84_eval/pse846gps2dbzc4a/m55
   :goals: build flash
   :compact:

Sample output
=============

.. code-block:: console

   AutAnalog LP audio sample
   Opamp OPAMP_0 ready
   Comparators ready (AAD enabled)
   ADC ready (LP mode, MUX ch0 → FIFO)
   Waiting for audio data...

   Frame 0: 160 samples, min=-42 max=38 avg=-2
   Frame 1: 160 samples, min=-40 max=35 avg=-1
     AAD: comp0=1 comp1=0 (output: 1 / 0)
