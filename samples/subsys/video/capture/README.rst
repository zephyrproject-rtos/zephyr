.. zephyr:code-sample:: video-capture
   :name: Video capture
   :relevant-api: video_interface

   Use the video API to retrieve video frames from a capture device.

Description
***********

This sample application uses the :ref:`video_api` to capture frames from a video capture
device then uses the :ref:`display_api` to display them onto an LCD screen (if any).

Requirements
************

This sample needs a video capture device (e.g. a camera) but it is not mandatory.
Supported camera modules on some i.MX RT boards can be found below.

- `Camera iMXRT`_

- :ref:`mimxrt1064_evk`
- `MT9M114 camera module`_

- :ref:`mimxrt1170_evk`
- `OV5640 camera module`_

Wiring
******

On :ref:`mimxrt1064_evk`, the MT9M114 camera module should be plugged in the
J35 camera connector. A USB cable should be connected from a host to the micro
USB debug connector (J41) in order to get console output via the freelink interface.

On :ref:`mimxrt1170_evk`, the OV5640 camera module should be plugged into the
J2 camera connector. A USB cable should be connected from a host to the micro
USB debug connector (J11) in order to get console output via the daplink interface.

Building and Running
********************

For :ref:`mimxrt1064_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: mimxrt1064_evk
   :shield: dvp_fpc24_mt9m114,rk043fn66hs_ctg
   :goals: build
   :compact:

For :ref:`mimxrt1170_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :shield: nxp_btb44_ov5640,rk055hdmipi4ma0
   :goals: build
   :compact:

For testing purpose without the need of any real video capture and/or display hardwares,
a video software pattern generator is supported by the above build commands without
specifying the shields.

Sample Output
=============

.. code-block:: console

    Video device: csi@402bc000
    - Capabilities:
      RGBP width [480; 480; 0] height [272; 272; 0]
      YUYV width [480; 480; 0] height [272; 272; 0]
      RGBP width [640; 640; 0] height [480; 480; 0]
      YUYV width [640; 640; 0] height [480; 480; 0]
      RGBP width [1280; 1280; 0] height [720; 720; 0]
      YUYV width [1280; 1280; 0] height [720; 720; 0]
    - Default format: RGBP 480x272

    Display device: display-controller@402b8000
    - Capabilities:
      x_resolution = 480, y_resolution = 272, supported_pixel_formats = 40
      current_pixel_format = 32, current_orientation = 0

    Capture started
    Got frame 0! size: 261120; timestamp 249 ms
    Got frame 1! size: 261120; timestamp 282 ms
    Got frame 2! size: 261120; timestamp 316 ms
    Got frame 3! size: 261120; timestamp 350 ms
    Got frame 4! size: 261120; timestamp 384 ms
    Got frame 5! size: 261120; timestamp 418 ms
    Got frame 6! size: 261120; timestamp 451 ms

   <repeats endlessly>

References
**********

.. _Camera iMXRT: https://community.nxp.com/t5/i-MX-RT-Knowledge-Base/Connecting-camera-and-LCD-to-i-MX-RT-EVKs/ta-p/1122183
.. _MT9M114 camera module: https://www.onsemi.com/PowerSolutions/product.do?id=MT9M114
.. _OV5640 camera module: https://cdn.sparkfun.com/datasheets/Sensors/LightImaging/OV5640_datasheet.pdf
