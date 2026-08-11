.. zephyr:code-sample:: net-rtp
   :name: RTP
   :relevant-api: rtp

   Send or receive an RTP audio stream over a UDP multicast.

Overview
********

This sample demonstrates how to use the Zephyr RTP API to transmit and receive
an audio stream over a UDP multicast socket. The role is selected at build time
via an extra configuration file: ``source.conf`` for a transmitter or
``sink.conf`` for a receiver. Both roles share the multicast address
``239.0.1.1`` on port ``5004``. Adding the ``ipv6.conf`` extra configuration
file switches the session to the IPv6 multicast address ``ff02::5004`` on the
same port instead.

The **source** role sends a 100 Hz sine wave (16-bit mono, 8000 samples/s) as
RTP packets using payload type 97. Packets are sent every 50 ms, each carrying
800 samples (one complete period of the tone).

The **sink** role joins the multicast group and receives the stream using a
callback-based API. It logs a heartbeat everty 5 seconds with the received
number of packets, and logs a warning if no packet arrives within 500 ms.

Requirements
************

- :ref:`networking_with_host`

Running on native_sim
*********************

To run this sample on the :zephyr:board:`native_sim` board, first set up the
``zeth`` tap interface on the host by following :ref:`networking_with_host`.

When testing with FFmpeg, the Linux kernel requires an explicit route for the
multicast address so it delivers packets on ``zeth`` rather than the default
network interface. After the ``zeth`` interface is up, run:

.. code-block:: console

   sudo ip route add 239.0.1.1 dev zeth

Building
********

Build as a source (transmitter):

.. zephyr-app-commands::
   :zephyr-app: samples/net/rtp
   :board: <board to use>
   :goals: build
   :conf: source.conf
   :compact:

Build as a sink (receiver):

.. zephyr-app-commands::
   :zephyr-app: samples/net/rtp
   :board: <board to use>
   :goals: build
   :conf: sink.conf
   :compact:

Faster RTP Stack with net_pkt
==============================

The ``net_pkt.conf`` extra configuration file tunes the network packet pool for
higher throughput. It can be combined with either role:

.. zephyr-app-commands::
   :zephyr-app: samples/net/rtp
   :board: <board to use>
   :goals: build run
   :conf: "source.conf net_pkt.conf"
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/net/rtp
   :board: <board to use>
   :goals: build run
   :conf: "sink.conf net_pkt.conf"
   :compact:

Building with IPv6
===================

The ``ipv6.conf`` extra configuration file switches the RTP session to the
IPv6 multicast address ``ff02::5004``. It can be combined with either role,
and with ``net_pkt.conf``:

.. zephyr-app-commands::
   :zephyr-app: samples/net/rtp
   :board: <board to use>
   :goals: build run
   :conf: "source.conf ipv6.conf"
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/net/rtp
   :board: <board to use>
   :goals: build run
   :conf: "sink.conf ipv6.conf"
   :compact:

Testing with FFmpeg
===================

The sample can be tested against `FFmpeg <https://ffmpeg.org>`_ on either side.

Testing the Source
------------------

To receive the stream from the **source** role with FFmpeg, first create a
:file:`stream.sdp` file describing the RTP session:

.. code-block:: none

   v=0
   o=- 0 0 IN IP4 127.0.0.1
   s=No Name
   c=IN IP4 239.0.1.1
   t=0 0
   a=tool:libavformat 58.76.100
   m=audio 5004 RTP/AVP 97
   a=rtpmap:97 L16/8000/1

Then run FFmpeg to receive and save the stream to a WAV file:

.. code-block:: console

   ffmpeg -protocol_whitelist file,udp,rtp -i stream.sdp \
          -acodec pcm_s16le -ar 8000 -ac 1 \
          -f wav out.wav

Testing the Sink
----------------

To feed the **sink** role from FFmpeg, run the following command to send a
silent mono stream at 8 kHz over RTP multicast:

.. code-block:: console

   ffmpeg -re -f lavfi -i anullsrc=r=8000:cl=mono \
          -acodec pcm_s16le \
          -f rtp -rtp_flags latm \
          -pkt_size 1440 \
          rtp://239.0.1.1:5004

Sample Output
*************

Source
======

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-3146-g5ef8f883a77f ***
   [00:00:00.000,000] <inf> net_config: Initializing network
   [00:00:00.000,000] <inf> net_config: IPv4 address: 192.0.2.102
   [00:00:00.000,000] <inf> main: RTP source on native_sim
   [00:00:00.000,000] <dbg> rtp.rtp_session_start_tx: (0x428e60): my_rtp_session started transmitter to 239.0.1.1:5004
   [00:00:00.000,000] <inf> main: Start sending RTP packets

Sink
====

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-3146-g5ef8f883a77f ***
   [00:00:00.000,000] <inf> net_config: Initializing network
   [00:00:00.000,000] <inf> net_config: IPv4 address: 192.0.2.101
   [00:00:00.000,000] <inf> main: RTP sink on native_sim
   [00:00:00.000,000] <dbg> rtp.rtp_session_start_rx: (0x420fc0): my_rtp_session started receiver on 239.0.1.1:5004

When the source is running alongside, the sink produces:

.. code-block:: console

   [00:00:00.120,000] <inf> main: Receiving rtp stream
   [00:00:05.120,000] <inf> main: Received 100 packets
   [00:00:10.120,000] <inf> main: Received 200 packets
   ...

If the source stops, a timeout warning is logged after 500 ms:

.. code-block:: console

   [00:00:10.620,000] <wrn> main: Timeout occurred
