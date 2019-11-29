.. _video_capture_display-sample:

Video Capture And Display
#########################

Description
***********

This sample application uses the Video API to retrieve video frames from the
video capture device, and writes frames to a video display device.

Requirements
************

This sample requires a video capture device (e.g. a camera) and a video output
device (e.g. display).

- :ref:`mimxrt1064_evk`
- `MT9M114 camera module`_
- `RK043FN02H-CT TFT display`_

Wiring
******

On :ref:`mimxrt1064_evk`, The MT9M114 camera module should be plugged in the
J35 camera connector. The RK043FN02H-CT TFT display should be connected to
to J8(A1-A40). A USB cable can be connected from a host to the micro
USB debug connector (J41) in order to get console output via the freelink
interface.

Building and Running
********************

For :ref:`mimxrt1064_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/video/capture_and_display
   :board: mimxrt1064_evk
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    - Display (ELCDIF_1) Capabilities:
      RGBP width [480; 960; 1] height [272; 544; 1]
      Default format: RGBP 480x272
    - Camera (CSI) Capabilities:
      RGBP width [640; 640; 0] height [480; 480; 0]
      Default format: RGBP 640x480
    Capture started

Once capture has started, the display shows camera captured frames. For
:ref:`mimxrt1064_evk` board it could be necessary to hard reset the board (SW3
button) to get good image.


References
**********

.. target-notes::

.. _MT9M114 camera module: https://www.onsemi.com/PowerSolutions/product.do?id=MT9M114
.. _RK043FN02H-CT TFT display: https://www.nxp.com/design/development-boards/i.mx-evaluation-and-development-boards/4.3-lcd-panel:RK043FN02H-CT
