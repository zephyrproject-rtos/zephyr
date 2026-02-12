.. zephyr:code-sample:: net-rtp-receive
   :name: RTP Receive
   :relevant-api: rtp

   Receive an RTP audio stream over a UDP multicast socket.

Overview
********

This sample demonstrates how to use the Zephyr RTP API to receive an RTP
audio stream from a UDP multicast socket. It joins the multicast group
``224.0.1.100`` on port ``51100``.

Reception is callback-based. The receiver tracks its state (idle, active, or
timed out) and logs a warning if no packet arrives within 500 ms. This sample
works neatly together with the :zephyr:code-sample:`net-rtp-transmit` sample.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/net/rtp/rtp_receive
   :board: <board to use>
   :goals: build
   :compact:

Testing with FFmpeg
===================

The receiver can also be tested using `FFmpeg <https://ffmpeg.org>`_ as the
transmitter. The following command sends a silent stereo stream at 48 kHz over
RTP multicast:

.. code-block:: console

   ffmpeg -re -f lavfi -i anullsrc=r=48000:cl=stereo \
          -acodec pcm_s24be \
          -f rtp -rtp_flags latm \
          -pkt_size 1440 \
          rtp://224.0.1.100:51100

Sample Output
*************

When the :zephyr:code-sample:`net-rtp-transmit` sample is running alongside, the
receiver produces output similar to:

.. code-block:: console

   [00:00:00.000,000] <inf> main: RTP receiver on native_sim
   [00:00:00.010,000] <inf> main: Receiving rtp stream

If the transmitter stops, a timeout warning is logged after 500 ms:

.. code-block:: console

   [00:00:00.560,000] <wrn> main: Timeout occured
