.. _video_tcpserversink-sample:

VIDEO TCP SERVER SINK
#####################

Description
***********

This sample application gets frames from video capture device and sends
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

For :ref:`mimxrt1064_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/video/mt9m114
   :board: mimxrt1064_evk
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Video device detected, format: RGBP 640x480
    TCP: Waiting for client...

Then from a peer on the same network you can connect and grab frames.

Example with gstreamer:

.. code-block:: console

    gst-launch-1.0 tcpclientsrc host=192.0.2.1 port=5000 \
        ! videoparse format=rgb16 width=640 height=480 \
        ! queue \
	! videoconvert \
        ! fpsdisplaysink sync=false


References
**********

.. _MT9M114 camera module: https://www.onsemi.com/PowerSolutions/product.do?id=MT9M114
