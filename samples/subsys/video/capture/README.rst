.. zephyr:code-sample:: video-capture
   :name: Video capture
   :relevant-api: video_interface

   Use the video API to retrieve video frames from a capture device.

Description
***********

This sample application uses the :ref:`Video API <video_api>` to capture frames from
a video capture device and then display them onto an LCD screen (if any).

Requirements
************

This sample requires a video capture device (e.g. a camera).

- :ref:`mimxrt1064_evk`
- `MT9M114 camera module`_

- :ref:`mimxrt1170_evk`
- `OV5640 camera module`_

Wiring
******

On :ref:`mimxrt1064_evk`, The MT9M114 camera module should be plugged in the
J35 camera connector. A USB cable should be connected from a host to the micro
USB debug connector (J41) in order to get console output via the freelink
interface.

On :ref:`mimxrt1170_evk`, The OV5640 camera module should be plugged into the
J2 camera connector. A USB cable should be connected from a host to the micro
USB debug connector (J11) in order to get console output via the daplink
interface.

Building and Running
********************

For testing purpose without the need of any real video capture hardware and / or a real
display device, a software pattern generator is supported and can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: [mimxrt1064_evk | mimxrt1170_evk/mimxrt1176/cm7]
   :shield: [rk055hdmipi4ma0]
   :goals: build
   :compact:

For :ref:`mimxrt1170_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: mimxrt1064_evk
   :shield: huatian_mt9m114
   :goals: build
   :compact:

For :ref:`mimxrt1170_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :gen-args: -DEXTRA_CONF_FILE="boards/nxp_rt11xx.conf"
   :shield: "wuxi_ov5640 rk055hdmipi4ma0"
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Video device: csi@402bc000
    - Capabilities:
      RGBP width [640; 640; 0] height [480; 480; 0]
      YUYV width [640; 640; 0] height [480; 480; 0]
    - Default format: RGBP 640x480

    Display device: display-controller@402b8000
    - Capabilities:
      x_resolution = 480, y_resolution = 272, supported_pixel_formats = 40
      current_pixel_format = 32, current_orientation = 0

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
.. _OV5640 camera module: https://cdn.sparkfun.com/datasheets/Sensors/LightImaging/OV5640_datasheet.pdf
