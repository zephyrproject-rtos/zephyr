.. zephyr:code-sample:: video-capture-to-lvgl
   :name: Video capture to LVGL
   :relevant-api: video_interface

   This sample application demonstrates how to use the video API to retrieve video
   frames from a capture device and display them on an LCD via the LVGL library.

Description
***********

The application uses the :ref:`Video API <video_api>` to retrieve video frames from
a video capture device, writes a frame count message to the console, and then sends
the frame to an LCD display.

Requirements
************

This sample requires a video capture device (e.g., a camera) and an LCD display.

- :ref:`mini_stm32h743`
- OV2640 camera module
- LCD display

Wiring
******

On the `WeAct Studio STM32H743`_, connect the OV2640 camera module and the 0.96" ST7735
TFT LCD display. Connect a USB cable from a host to the micro USB-C connector on the
board to receive console output messages.

Building and Running
********************

For :ref:`mini_stm32h743`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture_to_lvgl/
   :board: mini_stm32h743
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=5000
   :compact:

Sample Output
=============

.. code-block:: console

    - Device name: dcmi@48020000
    - Capabilities:
    RGBP width [160; 160; 0] height [120; 120; 0]
    RGBP width [176; 176; 0] height [144; 144; 0]
    RGBP width [240; 240; 0] height [160; 160; 0]
    RGBP width [320; 320; 0] height [240; 240; 0]
    RGBP width [352; 352; 0] height [288; 288; 0]
    RGBP width [640; 640; 0] height [480; 480; 0]
    RGBP width [800; 800; 0] height [600; 600; 0]
    RGBP width [1024; 1024; 0] height [768; 768; 0]
    RGBP width [1280; 1280; 0] height [1024; 1024; 0]
    RGBP width [1600; 1600; 0] height [1200; 1200; 0]
    JPEG width [160; 160; 0] height [120; 120; 0]
    JPEG width [176; 176; 0] height [144; 144; 0]
    JPEG width [240; 240; 0] height [160; 160; 0]
    JPEG width [320; 320; 0] height [240; 240; 0]
    JPEG width [352; 352; 0] height [288; 288; 0]
    JPEG width [640; 640; 0] height [480; 480; 0]
    JPEG width [800; 800; 0] height [600; 600; 0]
    JPEG width [1024; 1024; 0] height [768; 768; 0]
    JPEG width [1280; 1280; 0] height [1024; 1024; 0]
    JPEG width [1600; 1600; 0] height [1200; 1200; 0]
    - Default format: RGBP 800x600
    - New format: RGBP 160x120 320
    [00:00:02.847,000] <inf> video_stm32_dcmi: Start stream capture
    Capture started
    Frame 0, FPS 16, Size: 38400, Timestamp 2935 ms
    Frame 1, FPS 16, Size: 38400, Timestamp 3001 ms
    Frame 2, FPS 16, Size: 38400, Timestamp 8017 ms
    <repeats endlessly>

References
**********

.. _WeAct Studio STM32H743: https://github.com/WeActStudio/MiniSTM32H7xx
