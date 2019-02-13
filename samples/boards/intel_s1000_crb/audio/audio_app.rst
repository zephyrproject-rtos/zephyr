.. _audio_app-sample:

Intel® S1000 2-Way Audio Sample Application
###########################################

Overview
********

This sample application demonstrates audio capture and playback on an
Intel® S1000 CRB.
The application uses the following drivers

- DMIC driver (For microphone audio capture)
- I2S driver (For audio input from host,
  audio output to host and audio output to codec)
- Codec driver

Intel® S1000 is the master on the I2S interfaces.
The host is a slave on I2S and is expected to send and receive  audio at a
sampling frequency of 48KHz, 32 bits per sample.

This sample application captures 8 channels of audio one from each of the
8 microphones.
The first 2 channels are buffered and forwarded to the host.
Simultaneously, 2 channels of audio from the host are buffered and forwarded
to the codec.
This bidirectional forwarding of audio demonstrates a 2-way audio stream
between S1000's microphones/audio line-out and the host.

After the app starts, one may use the ALSA aplay/arecord commands on
a Linux host connected to S1000 over the I2S bus to play and record the audio.

``aplay -f S32_LE -r 48000 -c 2 -D <Device> <WAV file to Play>``

``arecord -f S32_LE -r 48000 -c 2 -D <Device> <WAV file to Record>``

Requirements
************

This application uses an Intel® S1000 Customer Reference Board (CRB)
with a circular 8 microphone array board.
The Microphone array board contains a TI TLV320DAC3101 DAC for line-level
Audio output.

Cable Rework
============

A rework of the host to CRB cable is required for playing audio from host.
Pin 12 needs to be wired to Pin 29 on the 40-pin connector of the cable.

.. image:: ../i2s/cable_rework.png
   :width: 442px
   :alt: Cable rework to play audio from host

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/intel_s1000_crb/audio
   :board:
   :goals: build
   :compact:

Sample Output
=============

Console output
--------------

.. code-block:: console

   [00:00:00.342,117] <inf> audio_io: Starting Audio Driver thread (0xbe0142c4)
   [00:00:00.342,117] <inf> audio_io: Configuring Host Audio Streams ...
   [00:00:00.342,123] <inf> audio_io: Configuring Peripheral Audio Streams ...
   [00:00:00.343,631] <inf> audio_io: Initializing Audio Core Engine ...
   [00:00:00.343,632] <inf> audio_io: Starting Audio I/O...
