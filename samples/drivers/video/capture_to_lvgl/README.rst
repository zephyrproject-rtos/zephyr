.. zephyr:code-sample:: video-capture-to-lvgl
   :name: Video capture to LVGL
   :relevant-api: video_interface

   Capture video frames and display them on an LCD using LVGL.

Description
***********

The application uses the :ref:`Video API <video_api>` to retrieve video frames from
a video capture device, write a frame count message to the console, and then send
the frame to an LCD display.

Requirements
************

This sample requires a supported :ref:`video capture device <video_api>` (e.g., a camera)
and a :ref:`display <display_api>`.

Wiring
******

On the `WeAct Studio STM32H743`_, connect the OV2640 camera module and the 0.96" ST7735
TFT LCD display. Connect a USB cable from a host to the micro USB-C connector on the
board to receive console output messages.

Building and Running
********************

For :zephyr:board:`mini_stm32h743`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture_to_lvgl/
   :board: mini_stm32h743
   :shield: weact_ministm32h7xx_ov2640
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=2000
   :compact:

Sample Output
=============

.. code-block:: console

    [00:00:02.779,000] <inf> main: - Device name: dcmi@48020000
    [00:00:02.779,000] <inf> main: - Capabilities:
    [00:00:02.779,000] <inf> main:   RGBP width [160; 160; 0] height [120; 120; 0]
    [00:00:02.779,000] <inf> main:   RGBP width [176; 176; 0] height [144; 144; 0]
    [00:00:02.780,000] <inf> main:   RGBP width [240; 240; 0] height [160; 160; 0]
    [00:00:02.780,000] <inf> main:   RGBP width [320; 320; 0] height [240; 240; 0]
    [00:00:02.780,000] <inf> main:   RGBP width [352; 352; 0] height [288; 288; 0]
    [00:00:02.780,000] <inf> main:   RGBP width [640; 640; 0] height [480; 480; 0]
    [00:00:02.780,000] <inf> main:   RGBP width [800; 800; 0] height [600; 600; 0]
    [00:00:02.780,000] <inf> main:   RGBP width [1024; 1024; 0] height [768; 768; 0]
    [00:00:02.780,000] <inf> main:   RGBP width [1280; 1280; 0] height [1024; 1024; 0]
    [00:00:02.780,000] <inf> main:   RGBP width [1600; 1600; 0] height [1200; 1200; 0]
    [00:00:02.780,000] <inf> main:   JPEG width [160; 160; 0] height [120; 120; 0]
    [00:00:02.780,000] <inf> main:   JPEG width [176; 176; 0] height [144; 144; 0]
    [00:00:02.780,000] <inf> main:   JPEG width [240; 240; 0] height [160; 160; 0]
    [00:00:02.780,000] <inf> main:   JPEG width [320; 320; 0] height [240; 240; 0]
    [00:00:02.780,000] <inf> main:   JPEG width [352; 352; 0] height [288; 288; 0]
    [00:00:02.780,000] <inf> main:   JPEG width [640; 640; 0] height [480; 480; 0]
    [00:00:02.780,000] <inf> main:   JPEG width [800; 800; 0] height [600; 600; 0]
    [00:00:02.780,000] <inf> main:   JPEG width [1024; 1024; 0] height [768; 768; 0]
    [00:00:02.780,000] <inf> main:   JPEG width [1280; 1280; 0] height [1024; 1024; 0]
    [00:00:02.780,000] <inf> main:   JPEG width [1600; 1600; 0] height [1200; 1200; 0]
    [00:00:02.852,000] <inf> main: - Format: RGBP 160x120 320
    [00:00:02.854,000] <inf> main: - Capture started

References
**********

.. target-notes::

.. _WeAct Studio STM32H743: https://github.com/WeActStudio/MiniSTM32H7xx
