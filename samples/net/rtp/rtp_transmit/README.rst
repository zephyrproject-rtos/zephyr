.. zephyr:code-sample:: net-rtp-transmit
   :name: RTP Transmit
   :relevant-api: rtp

   Send an RTP audio stream over a UDP multicast socket.

Overview
********

This sample demonstrates how to use the Zephyr RTP API to transmit an audio
stream over a UDP multicast socket. It sends a 100 Hz sine wave (16-bit mono,
8000 samples/s) as RTP packets to the multicast address ``224.0.1.100`` on
port ``51100``, using payload type 97.

Packets are sent every 50 ms, each carrying 800 samples (one complete period
of the 100 Hz tone). This sample works together with the
:zephyr:code-sample:`net-rtp-receive` sample.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

Build and run the sample for the :zephyr:board:`native_sim` board:

.. zephyr-app-commands::
   :zephyr-app: samples/net/rtp/rtp_transmit
   :board: native_sim
   :goals: build run
   :compact:

The :file:`boards/native_sim.conf` overlay assigns the static IPv4 address
``192.0.2.101`` to the device. To test with the receiver, start the
:zephyr:code-sample:`net-rtp-receive` sample in a separate terminal and ensure
both instances share the same network interface (see :ref:`networking_with_host`
for host networking setup).

Testing with FFmpeg
===================

The transmitter can also be tested using `FFmpeg <https://ffmpeg.org>`_ as the
receiver. First, create a :file:`stream.sdp` file describing the RTP session:

.. code-block:: none

   v=0
   o=- 0 0 IN IP4 127.0.0.1
   s=No Name
   c=IN IP4 224.0.1.100
   t=0 0
   a=tool:libavformat 58.76.100
   m=audio 51100 RTP/AVP 97
   a=rtpmap:97 L16/8000/1

Then run FFmpeg to receive the stream and save it to a WAV file:

.. code-block:: console

   ffmpeg -protocol_whitelist file,udp,rtp -i stream.sdp \
          -acodec pcm_s16le -ar 8000 -ac 1 \
          -f wav out.wav

Sample Output
*************

.. code-block:: console

   [00:00:00.000,000] <inf> main: RTP transmitter on native_sim
   [00:00:00.010,000] <inf> main: Start sending RTP packets
