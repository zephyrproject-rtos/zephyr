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

- :zephyr:board:`mimxrt1064_evk`
- `MT9M114 camera module`_

- :zephyr:board:`stm32n6570_dk`
- :ref:`ST B-CAMS-IMX-MB1854 <st_b_cams_imx_mb1854>`

Wiring
******

On :zephyr:board:`mimxrt1064_evk`, The MT9M114 camera module should be plugged in the
J35 camera connector. A USB cable should be connected from a host to the micro
USB debug connector (J41) in order to get console output via the freelink
interface. Ethernet cable must be connected to RJ45 connector.

On :zephyr:board:`stm32n6570_dk`, the MB1854 IMX335 camera module must be plugged in
the CSI-2 camera connector. A RJ45 ethernet cable must be plugged in the ethernet CN6
connector. For an optimal image experience, it is advice to embed STM32 image signal
processing middleware: https://github.com/stm32-hotspot/zephyr-stm32-mw-isp.


Building and Running
********************

For :zephyr:board:`mimxrt1064_evk`, the sample can be built with the following command.
If a mt9m114 camera shield is missing, video software generator will be used instead.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/tcpserversink
   :board: mimxrt1064_evk
   :shield: dvp_fpc24_mt9m114
   :goals: build
   :compact:

For testing purpose and without the need of any real video capture hardware,
a video software pattern generator is supported by using :ref:`snippet-video-sw-generator`:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: native_sim/native/64
   :snippets: video-sw-generator
   :goals: build
   :compact:

For :zephyr:board:`stm32n6570_dk`, the sample can be built with H264 video compression
support using the venc file_suffix at the end of the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/tcpserversink
   :board: stm32n6570_dk
   :shield: st_b_cams_imx_mb1854
   :gen-args: -DFILE_SUFFIX=venc
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

When using video compression support, use this GStreamer command line:

.. code-block:: console

    gst-launch-1.0 tcpclientsrc host=192.0.2.1 port=5000 \
        ! queue ! decodebin ! queue ! fpsdisplaysink sync=false

References
**********

.. target-notes::

.. _MT9M114 camera module: https://www.onsemi.com/PowerSolutions/product.do?id=MT9M114
