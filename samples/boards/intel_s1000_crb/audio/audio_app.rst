.. _audio_app-sample:

Sue Creek 2-Way Audio Sample Application
########################################

Overview
********

This sample application demonstrates audio capture and playback on an
Sue Creek S1000 CRB from Intel with control from a Linux host connected
over USB.
The application uses the following drivers

- DMIC driver (For microphone audio capture)
- I2S driver (For audio input from host,
  audio output to host and audio output to codec)
- Codec driver
- USB driver and USB HID stack

Sue Creek is the master on the I2S interfaces.
The host is a slave on I2S and is expected to send and receive  audio at a
sampling frequency of 48KHz, 32 bits per sample.

This sample application captures 8 channels of audio one from each of the
8 microphones.
The first 2 channels are buffered and forwarded to the host.
Simultaneously, 2 channels of audio from the host are buffered and forwarded
to the codec.
This bidirectional forwarding of audio demonstrates a 2-way audio stream
between the Sue Creek board's microphones/audio line-out and
the host.

After the app starts, one may use the ALSA aplay/arecord commands on
a Linux host connected to Sue Creek board over the I2S bus to play and
record audio.

.. code-block:: bash

   $ aplay -f S32_LE -r 48000 -c 2 -D <Device> <WAV file to Play>
   $ arecord -f S32_LE -r 48000 -c 2 -D <Device> <WAV file to Record>

The same (or a different) Linux host may be connected to the USB interface
of Sue Creek board. This host will then be able to send control commands
over USB. In this sample application, control of starting and stopping audio
transfers is implemented.

Requirements
************

This application uses an Sue Creek Customer Reference Board (CRB)
with a circular 8 microphone array board.
The Microphone array board contains a TI TLV320DAC3101 DAC for line-level
audio output.

Cable Rework
============

A rework of the host to CRB cable is required for playing audio from host.
Pin 12 needs to be wired to Pin 29 on the 40-pin connector of the cable.

.. image:: ../i2s/cable_rework.png
   :width: 442px
   :alt: Cable rework to play audio from host

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/boards/intel_s1000_crb/audio
   :board:
   :goals: build
   :compact:

Running
*******

Upon power up and downloading the application image (or booting the board
with the image in flash), the application starts without user intervention
and audio is transferred from the microphones to the connected host and
from the host to the audio codec output.

If a Linux host is connected via USB to the Sue Creek board,
the python script ``audio.py`` can be used to stop/[re]start audio transfer.

.. code-block:: bash

   $ cd $ZEPHYR_BASE/samples/boards/intel_s1000_crb/audio
   $ sudo -E python3 audio.py stop
   $ sudo -E python3 audio.py start

Sample Output
=============

Console output
--------------

.. code-block:: console

   [00:00:00.370,000] <inf> tuning: Starting tuning driver I/O thread ...
   [00:00:00.370,000] <inf> audio_io: Starting Audio Driver thread ,,,
   [00:00:00.370,000] <inf> audio_io: Configuring Host Audio Streams ...
   [00:00:00.370,000] <inf> audio_io: Configuring Peripheral Audio Streams ...
   [00:00:00.370,000] <inf> audio_proc: Starting small block processing thread ...
   [00:00:00.370,000] <inf> audio_proc: Starting large block processing thread ...
   [00:00:00.370,000] <inf> audio_io: Initializing Audio Core Engine ...
   [00:00:00.370,000] <inf> audio_io: Starting Audio I/O...
   [00:00:00.420,000] <inf> framework: Starting framework background thread ...
   [00:00:06.520,000] <inf> audio_io: Stopped Audio I/O...
   [00:00:11.200,000] <inf> audio_io: Starting Audio I/O...
