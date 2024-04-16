.. zephyr:code-sample:: video-capture
   :name: Video capture
   :relevant-api: video_interface

   Use the video API to retrieve video frames from a capture device.

Description
***********

This sample application uses the :ref:`Video API <video_api>` to retrieve video frames from the
video capture device, writes a frame count message to the console, and then
discards the video frame data.

Requirements
************

This sample requires a video capture device (e.g. a camera).

- :ref:`mimxrt1064_evk`
- `MT9M114 camera module`_

Wiring
******

On :ref:`mimxrt1064_evk`, The MT9M114 camera module should be plugged in the
J35 camera connector. A USB cable should be connected from a host to the micro
USB debug connector (J41) in order to get console output via the freelink
interface.

Building and Running
********************

For :ref:`mimxrt1064_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: mimxrt1064_evk
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Found video device: CSI
    width (640,640), height (480,480)
    Supported pixelformats (fourcc):
     - RGBP
    Use default format (640x480)
    Capture started
    Got frame 743! size: 614400; timestamp 100740 ms
    Got frame 744! size: 614400; timestamp 100875 ms
    Got frame 745! size: 614400; timestamp 101010 ms
    Got frame 746! size: 614400; timestamp 101146 ms
    Got frame 747! size: 614400; timestamp 101281 ms
    Got frame 748! size: 614400; timestamp 101416 ms

   <repeats endlessly>

References
**********

.. _MT9M114 camera module: https://www.onsemi.com/PowerSolutions/product.do?id=MT9M114
