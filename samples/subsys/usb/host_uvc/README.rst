.. zephyr:code-sample:: usb-host-uvc
   :name: USB Host UVC Camera
   :relevant-api: video_interface usb_host_core_api

   Capture video from a USB camera.

Overview
********

This sample demonstrates how to use the USB Host UVC (USB Video Class) driver
to capture video frames from a USB camera connected to a Zephyr device acting
as a USB host.

Upon connection, the USB camera is detected and configured automatically.
The sample captures video frames and logs frame statistics.

Requirements
************

This sample uses the USB host stack and requires the USB host controller driver.

A USB camera supporting UVC (USB Video Class) specification is required.

Building and Running
********************

The sample can be built and flashed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_uvc
   :board: rd_rw612_bga/rw612
   :goals: build flash
   :compact:

The device is expected to detect the USB camera automatically when connected.

Sample Output
=============

When a USB camera is connected, you should see:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-8828-g14f382c2002c ***
   <inf> main: USB host video device usbh_uvc_0 is ready
   <inf> usbh_dev: New device with address 1 state 2
   <inf> usbh_dev: Configuration 1 bNumInterfaces 4
   <inf> usbh_uvc: UVC device connected
   <inf> usbh_uvc: Interface 0 associated with UVC class
   <inf> usbh_dev: Set Interfaces 0, alternate 0 -> 0
   <inf> usbh_dev: Set Interfaces 1, alternate 0 -> 0
   <inf> usbh_uvc: Device configured successfully: control interface 0, streaming interface 1
   <wrn> usbh_uvc: Control cid 134217729 not supported by device
   <wrn> usbh_uvc: Control cid 134217730 not supported by device
   <inf> usbh_uvc: Successfully initialized 8 UVC controls
   <inf> usbh_uvc: UVC device (addr=1) initialization completed
   <inf> usbh_class: Class 'uvc_host_c_data_0' matches interface 0
   <inf> main: Video device connected!

Viewing Supported Formats
==========================

The sample will enumerate and display all supported video formats:

.. code-block:: console

   <inf> usbh_uvc: Created 36 format capabilities based on UVC descriptors
   <inf> main: - Capabilities:
   <inf> main:   YUYV width [640; 640; 1] height [480; 480; 1]
   <inf> main:   YUYV width [160; 160; 1] height [90; 90; 1]
   <inf> main:   YUYV width [160; 160; 1] height [120; 120; 1]
   <inf> main:   YUYV width [176; 176; 1] height [144; 144; 1]
   <inf> main:   YUYV width [320; 320; 1] height [180; 180; 1]
   <inf> main:   YUYV width [320; 320; 1] height [240; 240; 1]
   <inf> main:   YUYV width [352; 352; 1] height [288; 288; 1]
   <inf> main:   YUYV width [432; 432; 1] height [240; 240; 1]
   <inf> main:   YUYV width [640; 640; 1] height [360; 360; 1]
   <inf> main:   YUYV width [800; 800; 1] height [448; 448; 1]
   <inf> main:   YUYV width [800; 800; 1] height [600; 600; 1]
   <inf> main:   YUYV width [864; 864; 1] height [480; 480; 1]
   <inf> main:   YUYV width [960; 960; 1] height [720; 720; 1]
   <inf> main:   YUYV width [1024; 1024; 1] height [576; 576; 1]
   <inf> main:   YUYV width [1280; 1280; 1] height [720; 720; 1]
   <inf> main:   YUYV width [1600; 1600; 1] height [896; 896; 1]
   <inf> main:   YUYV width [1920; 1920; 1] height [1080; 1080; 1]
   <inf> main:   YUYV width [2304; 2304; 1] height [1296; 1296; 1]
   <inf> main:   YUYV width [2304; 2304; 1] height [1536; 1536; 1]
   <inf> main:   JPEG width [640; 640; 1] height [480; 480; 1]
   <inf> main:   JPEG width [160; 160; 1] height [90; 90; 1]
   <inf> main:   JPEG width [160; 160; 1] height [120; 120; 1]
   <inf> main:   JPEG width [176; 176; 1] height [144; 144; 1]
   <inf> main:   JPEG width [320; 320; 1] height [180; 180; 1]
   <inf> main:   JPEG width [320; 320; 1] height [240; 240; 1]
   <inf> main:   JPEG width [352; 352; 1] height [288; 288; 1]
   <inf> main:   JPEG width [432; 432; 1] height [240; 240; 1]
   <inf> main:   JPEG width [640; 640; 1] height [360; 360; 1]
   <inf> main:   JPEG width [800; 800; 1] height [448; 448; 1]
   <inf> main:   JPEG width [800; 800; 1] height [600; 600; 1]
   <inf> main:   JPEG width [864; 864; 1] height [480; 480; 1]
   <inf> main:   JPEG width [960; 960; 1] height [720; 720; 1]
   <inf> main:   JPEG width [1024; 1024; 1] height [576; 576; 1]
   <inf> main:   JPEG width [1280; 1280; 1] height [720; 720; 1]
   <inf> main:   JPEG width [1600; 1600; 1] height [896; 896; 1]
   <inf> main:   JPEG width [1920; 1920; 1] height [1080; 1080; 1]

Configuring Video Format
=========================

The sample configures the video format based on ``prj.conf`` settings:

.. code-block:: console

   <inf> main: - Expected video format: YUYV 320x180
   <inf> usbh_uvc: Setting format: YUYV 320x180 (format_index=1, frame_index=5, interval=333333)
   <wrn> usbh_uvc: VS GET: expected 48 bytes, got 26 bytes
   <inf> usbh_dev: Set Interfaces 1, alternate 0 -> 3
   <inf> usbh_dev: Modify interface 1 ep 0x81 by op 0
   <inf> usbh_dev: Modify interface 1 ep 0x81 by op 1
   <inf> usbh_uvc: Set streaming interface 1 alternate 3 successfully
   <inf> usbh_uvc: UVC format set successfully: YUYV 320x180@30fps

Adjusting Frame Rate
====================

The sample displays supported frame intervals and sets the target frame rate:

.. code-block:: console

   <inf> main: - Default frame rate : 30.000000 fps
   <inf> main: - Supported frame intervals for the default format:
   <inf> main:    333333/10000000
   <inf> main:    416666/10000000
   <inf> main:    500000/10000000
   <inf> main:    666666/10000000
   <inf> main:    1000000/10000000
   <inf> main:    1333333/10000000
   <inf> main:    2000000/10000000
   <inf> usbh_uvc: Setting frame rate: 15 fps -> 15 fps
   <wrn> usbh_uvc: VS GET: expected 48 bytes, got 26 bytes
   <inf> usbh_uvc: Frame rate successfully set to 15 fps
   <inf> usbh_dev: Set Interfaces 1, alternate 3 -> 2
   <inf> usbh_dev: Modify interface 1 ep 0x81 by op 0
   <inf> usbh_dev: Modify interface 1 ep 0x81 by op 2
   <inf> usbh_dev: Modify interface 1 ep 0x81 by op 1
   <inf> usbh_uvc: Set streaming interface 1 alternate 2 successfully
   <inf> main: - Target frame rate set to: 15.000000 fps

Accessing the Video Controls
=============================

The sample enumerates available camera controls:

.. code-block:: console

   <inf> main: - Supported controls:
   <inf> main: 		device: usbh_uvc_0
   <inf> video_ctrls:                       Brightness 0x00980900 (int)      (flags=0x00) : min=0 max=255 step=1 default=128 value=128
   <inf> video_ctrls:                         Contrast 0x00980901 (int)      (flags=0x00) : min=0 max=255 step=1 default=128 value=128
   <inf> video_ctrls:                       Saturation 0x00980902 (int)      (flags=0x00) : min=0 max=255 step=1 default=128 value=128
   <inf> video_ctrls:                         Exposure 0x00980911 (int)      (flags=0x00) : min=3 max=2047 step=1 default=250 value=250
   <inf> video_ctrls:                             Gain 0x00980913 (int)      (flags=0x00) : min=0 max=255 step=1 default=0 value=0
   <inf> video_ctrls:        White Balance Temperature 0x0098091a (int)      (flags=0x00) : min=2000 max=6500 step=1 default=4000 value=4000
   <inf> video_ctrls:                  Focus, Absolute 0x009a090a (int)      (flags=0x00) : min=0 max=250 step=5 default=0 value=0
   <inf> video_ctrls:                   Zoom, Absolute 0x009a090d (int)      (flags=0x00) : min=100 max=500 step=1 default=100 value=100

Frame Statistics
================

The sample logs frame statistics every 100 frames:

.. code-block:: console

   <inf> main: Allocating 6 video buffers, size=115200
   <inf> usbh_dev: Set Interfaces 1, alternate 2 -> 2
   <inf> main: Capture started
   <inf> main: Received 100 frames
   <inf> main: Received 200 frames
   <inf> main: Received 300 frames

Configuration Options
*********************

The sample can be configured through ``prj.conf``:

Video Format
============

- :kconfig:option:`CONFIG_APP_VIDEO_PIXEL_FORMAT` - Pixel format (YUYV, MJPEG, etc.)
- :kconfig:option:`CONFIG_APP_VIDEO_FRAME_WIDTH` - Frame width
- :kconfig:option:`CONFIG_APP_VIDEO_FRAME_HEIGHT` - Frame height
- :kconfig:option:`CONFIG_APP_VIDEO_TARGET_FPS` - Target frame rate

Memory Configuration
====================

- :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_NUM_MAX` - Number of video buffers
- :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE` - Buffer pool size in bytes
- :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_ALIGN` - Buffer alignment
- :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` - General heap size
- :kconfig:option:`CONFIG_MAIN_STACK_SIZE` - Main thread stack size

USB Transfer Configuration
===========================

- :kconfig:option:`CONFIG_USBH_VIDEO_CONCURRENT_TRANSFERS` - Concurrent ISO transfers
- :kconfig:option:`CONFIG_USBH_USB_DEVICE_HEAP` - USB device heap size
- :kconfig:option:`CONFIG_UHC_XFER_COUNT` - Max transfer requests
- :kconfig:option:`CONFIG_UHC_BUF_COUNT` - Transfer buffer count
- :kconfig:option:`CONFIG_UHC_BUF_POOL_SIZE` - Transfer buffer pool size

Logging Configuration
=====================

- :kconfig:option:`CONFIG_LOG` - Enable logging
- :kconfig:option:`CONFIG_LOG_BUFFER_SIZE` - Log buffer size
- :kconfig:option:`CONFIG_USBH_UVC_LOG_LEVEL_INF` - UVC driver log level

Troubleshooting
***************

Memory Allocation Failures
===========================

If you see memory allocation errors, increase heap size or reduce buffer count:

- :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` - Increase general heap size
- :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_NUM_MAX` - Reduce number of video buffers
- :kconfig:option:`CONFIG_APP_VIDEO_FRAME_WIDTH` - Reduce frame width to lower memory usage
- :kconfig:option:`CONFIG_APP_VIDEO_FRAME_HEIGHT` - Reduce frame height to lower memory usage

Dropped or Corrupted Frames
============================

If frames are dropped or corrupted, increase buffer resources or reduce data rate:

- :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_NUM_MAX`
- :kconfig:option:`CONFIG_UHC_BUF_POOL_SIZE`
- :kconfig:option:`CONFIG_UHC_BUF_COUNT`
- :kconfig:option:`CONFIG_APP_VIDEO_TARGET_FPS`
- :kconfig:option:`CONFIG_USBH_VIDEO_CONCURRENT_TRANSFERS`
- :kconfig:option:`CONFIG_APP_VIDEO_FRAME_WIDTH`
- :kconfig:option:`CONFIG_APP_VIDEO_FRAME_HEIGHT`

Performance Issues
==================

If video capture is slow or choppy, you can try to reduce the data rate or logging,
or enable optimizations:

- :kconfig:option:`CONFIG_APP_VIDEO_FRAME_WIDTH`
- :kconfig:option:`CONFIG_APP_VIDEO_FRAME_HEIGHT`
- :kconfig:option:`CONFIG_APP_VIDEO_TARGET_FPS`
- :kconfig:option:`CONFIG_USBH_UVC_LOG_LEVEL_WRN`
- :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS`
- :kconfig:option:`CONFIG_SPEED_OPTIMIZATIONS`

UVC buffer allocation is configured from the USB Host and Video areas,
which can be useful in case of memory allocation issues or to sustain
higher FPS:

- :kconfig:option:`CONFIG_UHC_BUF_COUNT`
- :kconfig:option:`CONFIG_UHC_BUF_POOL_SIZE`
- :kconfig:option:`CONFIG_UHC_XFER_COUNT`
- :kconfig:option:`CONFIG_USBH_VIDEO_CONCURRENT_TRANSFERS`
- :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE`
- :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_NUM_MAX`
