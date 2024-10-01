.. zephyr:code-sample:: video-tcpserversink
   :name: Video TCP server sink
   :relevant-api: video_interface bsd_sockets

   Capture video frames and send them over the network to a TCP client.

Description
***********

This sample application gets frames from a video capture device and sends
them over the network to the connected TCP client.

Requirements
************

This samples requires a video capture device and network support.

- :ref:`mimxrt1064_evk`
- `MT9M114 camera module`_

Wiring
******

On :ref:`mimxrt1064_evk`, The MT9M114 camera module should be plugged in the
J35 camera connector. A USB cable should be connected from a host to the micro
USB debug connector (J41) in order to get console output via the freelink
interface. Ethernet cable must be connected to RJ45 connector.

Building and Running
********************

For :ref:`mimxrt1064_evk`, the sample can be built with the following command.
If a mt9m114 camera shield is missing, video software generator will be used instead.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/tcpserversink
   :board: mimxrt1064_evk
   :shield: dvp_fpc24_mt9m114
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Video device detected, format: RGBP 480x272
    TCP: Waiting for client...

Then from a peer on the same network you can connect and grab frames.

Example with gstreamer:

.. code-block:: console

    gst-launch-1.0 tcpclientsrc host=192.0.2.1 port=5000 \
        ! videoparse format=rgb16 width=480 height=272 \
        ! queue \
	! videoconvert \
        ! fpsdisplaysink sync=false

For video software generator, the default resolution should be width=320 and height=160.

References
**********

.. _MT9M114 camera module: https://www.onsemi.com/PowerSolutions/product.do?id=MT9M114
