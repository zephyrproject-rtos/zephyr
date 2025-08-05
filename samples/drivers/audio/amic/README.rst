.. zephyr:code-sample:: amic
   :name: AMIC
   :relevant-api: audio_amic_interface

   Process an audio stream from AUDADC.

Overview
********

This sample demonstrates how to use an AUDADC (LP audio ADC) driver in a simple
processing of an audio stream. It configures and starts from AMIC to memory
buffer.

Requirements
************

This sample has been tested on the Apollo510 EVB with the ICS40310 AMIC (Analog Microphone) board connected to the J12 header.

Hardware setup requirements:
- Connect the ICS40310 AMIC board to the J12 header on the Apollo510 EVB.

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/audio/amic`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/audio/amic
   :board: apollo510_evb
   :goals: build flash
   :compact:
