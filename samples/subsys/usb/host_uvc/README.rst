.. zephyr:code-sample:: usb-host-uvc
   :name: USB Host UVC Camera
   :relevant-api: video_interface usb_host_core_api

   Capture video from a USB camera and display on LCD.

Overview
********

This sample demonstrates how to use the USB Host UVC (USB Video Class) driver
to capture video frames from a USB camera connected to a Zephyr device acting
as a USB host.

Upon connection, the USB camera is detected and configured automatically.
The sample captures video frames and can optionally display them on an LCD screen.

Any UVC-compliant USB camera can be used as a video source.

Requirements
************

This sample uses the USB host stack and requires the USB host controller
ported to the :ref:`uhc_api`.

A USB camera supporting UVC (USB Video Class) specification is required.

Building and Running
********************

If a board does not have an LCD display, the sample can still capture frames
and log information about the video stream:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_uvc
   :board: rd_rw612_bga/rw612
   :goals: build flash
   :compact:

If a board is equipped with a supported LCD display configured as the ``zephyr,display``
chosen node, then captured frames will be displayed. The sample can be built with LCD
support as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_uvc
   :board: rd_rw612_bga/rw612
   :shield: lcd_par_s035_8080
   :goals: build flash
   :compact:

The device is expected to detect the USB camera automatically when connected.

Sample Output
=============

When a USB camera is connected, you should see:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   <inf> main: USB host: uvc_host
   <inf> usbh_uvc: Initializing UVC device structure
   <inf> usbh_uvc: UVC device structure initialized successfully
   <inf> usbh_dev: New device with address 1 state 2
   <inf> usbh_dev: Configuration 1 bNumInterfaces 4
   <inf> usbh_uvc: UVC device connected
   <inf> usbh_uvc: Interface 0 associated with UVC class
   <inf> usbh_uvc: UVC device configured successfully (control: interface 0, streaming: interface 1)
   <inf> usbh_uvc: Set default format: YUYV 640x480@30fps (format_idx=1, frame_idx=1)
   <inf> usbh_uvc: Successfully initialized 10 UVC controls
   <inf> usbh_uvc: UVC device (addr=1) initialization completed
   <inf> main: UVC device connected successfully!

Viewing Supported Formats
==========================

The sample will enumerate and display all supported video formats:

.. code-block:: console

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
   <inf> main:   JPEG width [1920; 1920; 1] height [1080; 1080; 1]
   <inf> main:   JPEG width [1280; 1280; 1] height [720; 720; 1]
   ...

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
   <inf> main:    333333/10000000    # 30 fps
   <inf> main:    416666/10000000    # 24 fps
   <inf> main:    500000/10000000    # 20 fps
   <inf> main:    666666/10000000    # 15 fps
   <inf> main:    1000000/10000000   # 10 fps
   <inf> main:    1333333/10000000   # 7.5 fps
   <inf> main:    2000000/10000000   # 5 fps
   <inf> usbh_uvc: Setting frame rate: 15 fps -> 15 fps
   <inf> usbh_uvc: Frame rate successfully set to 15 fps
   <inf> usbh_dev: Set Interfaces 1, alternate 3 -> 2
   <inf> usbh_uvc: Set streaming interface 1 alternate 2 successfully
   <inf> main: - Target frame rate set to: 15.000000 fps

Accessing the Video Controls
=============================

The sample enumerates available camera controls:

.. code-block:: console

   <inf> main: - Supported controls:
   <inf> main:     device: uvc_host
   <inf> video_ctrls:   Brightness 0x00980900 (int) : min=0 max=255 step=1 default=128 value=128
   <inf> video_ctrls:   Contrast 0x00980901 (int) : min=0 max=255 step=1 default=128 value=128
   <inf> video_ctrls:   Saturation 0x00980902 (int) : min=0 max=255 step=1 default=128 value=128
   <inf> video_ctrls:   Exposure 0x00980911 (int) : min=3 max=2047 step=1 default=250 value=250
   <inf> video_ctrls:   Gain 0x00980913 (int) : min=0 max=255 step=1 default=0 value=0
   <inf> video_ctrls:   White Balance Temperature 0x0098091a (int) : min=2000 max=6500 step=1 default=4000 value=4000
   <inf> video_ctrls:   Focus, Absolute 0x009a090a (int) : min=0 max=250 step=5 default=0 value=0
   <inf> video_ctrls:   Zoom, Absolute 0x009a090d (int) : min=100 max=500 step=1 default=100 value=100

Displaying Video
================

When an LCD display is available:

.. code-block:: console

   <inf> main: Display device: st7796s@0
   <inf> main: Display Capabilities:
   <inf> main:   Resolution: 480 * 320 pixels
   <inf> main:   Current format: RGB565X (0x20)
   <inf> main: Display configured successfully
   <inf> main: Allocating 6 video buffers, size=115200
   <inf> usbh_dev: Set Interfaces 1, alternate 2 -> 2
   <inf> main: Capture started

The video stream will be displayed on the LCD in real-time.

Configuration Options
*********************

The sample can be configured through ``prj.conf``:

Video Format
============

.. code-block:: cfg

   CONFIG_VIDEO_FRAME_WIDTH=320        # Frame width (160-2304)
   CONFIG_VIDEO_FRAME_HEIGHT=180       # Frame height (90-1536)
   CONFIG_VIDEO_TARGET_FPS=15          # Target frame rate (5-30)

Memory Configuration
====================

.. code-block:: cfg

   CONFIG_VIDEO_BUFFER_POOL_NUM_MAX=6          # Number of video buffers (3-8)
   CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE=716800   # Buffer pool size in bytes
   CONFIG_HEAP_MEM_POOL_SIZE=16384             # General heap size
   CONFIG_MAIN_STACK_SIZE=10240                # Main thread stack size

USB Transfer Configuration
===========================

.. code-block:: cfg

   CONFIG_USBH_VIDEO_MULTIPLE_PRIME_COUNT=8    # Concurrent ISO transfers (4-16)
   CONFIG_UHC_XFER_COUNT=32                    # Max transfer requests (16-64)
   CONFIG_UHC_BUF_COUNT=32                     # Transfer buffer count (16-64)
   CONFIG_UHC_BUF_POOL_SIZE=4096               # Transfer buffer pool size

Logging Configuration
=====================

.. code-block:: cfg

   CONFIG_LOG=y
   CONFIG_LOG_BUFFER_SIZE=10240                # Log buffer size
   CONFIG_USBH_UVC_LOG_LEVEL_INF=y             # UVC driver log level

Example Configurations
======================

High Resolution (640x480 @ 15fps):

.. code-block:: cfg

   CONFIG_VIDEO_FRAME_WIDTH=640
   CONFIG_VIDEO_FRAME_HEIGHT=480
   CONFIG_VIDEO_TARGET_FPS=15
   CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE=2764800
   CONFIG_HEAP_MEM_POOL_SIZE=32768

Low Memory (160x120 @ 10fps):

.. code-block:: cfg

   CONFIG_VIDEO_FRAME_WIDTH=160
   CONFIG_VIDEO_FRAME_HEIGHT=120
   CONFIG_VIDEO_TARGET_FPS=10
   CONFIG_VIDEO_BUFFER_POOL_NUM_MAX=3
   CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE=115200
   CONFIG_HEAP_MEM_POOL_SIZE=8192

High Frame Rate (320x240 @ 30fps):

.. code-block:: cfg

   CONFIG_VIDEO_FRAME_WIDTH=320
   CONFIG_VIDEO_FRAME_HEIGHT=240
   CONFIG_VIDEO_TARGET_FPS=30
   CONFIG_USBH_VIDEO_MULTIPLE_PRIME_COUNT=16
   CONFIG_UHC_XFER_COUNT=64
   CONFIG_UHC_BUF_COUNT=64

Troubleshooting
***************

Memory Allocation Failures
===========================

If you see memory allocation errors:

* Increase heap size:

  .. code-block:: cfg

     CONFIG_HEAP_MEM_POOL_SIZE=32768

* Reduce buffer count:

  .. code-block:: cfg

     CONFIG_VIDEO_BUFFER_POOL_NUM_MAX=3

* Use lower resolution:

  .. code-block:: cfg

     CONFIG_VIDEO_FRAME_WIDTH=160
     CONFIG_VIDEO_FRAME_HEIGHT=120

Dropped or Corrupted Frames
============================

If frames are dropped or corrupted:

* Increase video buffer count:

  .. code-block:: cfg

     CONFIG_VIDEO_BUFFER_POOL_NUM_MAX=8

* Increase USB buffer pool:

  .. code-block:: cfg

     CONFIG_UHC_BUF_POOL_SIZE=8192
     CONFIG_UHC_BUF_COUNT=64

* Reduce frame rate:

  .. code-block:: cfg

     CONFIG_VIDEO_TARGET_FPS=10

* Increase concurrent transfers:

  .. code-block:: cfg

     CONFIG_USBH_VIDEO_MULTIPLE_PRIME_COUNT=16

* Lower resolution:

  .. code-block:: cfg

     CONFIG_VIDEO_FRAME_WIDTH=320
     CONFIG_VIDEO_FRAME_HEIGHT=180

Display Issues
==============

If the display shows corrupted or incorrect images:

* Verify display resolution matches or is larger than camera resolution
* Check color format compatibility (YUYV requires conversion to RGB565)
* Ensure sufficient memory for frame buffers
* Try reducing camera resolution to match display

Performance Issues
==================

If video playback is slow or choppy:

* Use lower resolution:

  .. code-block:: cfg

     CONFIG_VIDEO_FRAME_WIDTH=160
     CONFIG_VIDEO_FRAME_HEIGHT=120

* Reduce frame rate:

  .. code-block:: cfg

     CONFIG_VIDEO_TARGET_FPS=10

* Disable debug logging:

  .. code-block:: cfg

     CONFIG_LOG_DEFAULT_LEVEL=2
     CONFIG_USBH_UVC_LOG_LEVEL_WRN=y

* Enable compiler optimizations:

  .. code-block:: cfg

     CONFIG_DEBUG_OPTIMIZATIONS=n
     CONFIG_SPEED_OPTIMIZATIONS=y

* Use MJPEG format for higher resolutions (compressed format)

* Match camera resolution to display resolution to avoid scaling

Warning Messages
================

Some warning messages are normal and do not affect functionality:

.. code-block:: console

   <wrn> usbh_uvc: Control cid 134217729 not supported by device
   <wrn> usbh_uvc: Control cid 134217730 not supported by device

These indicate that some optional controls (e.g., auto-gain, auto-exposure)
are not supported by your specific camera model.

.. code-block:: console

   <wrn> usbh_uvc: VS GET: expected 48 bytes, got 26 bytes

This indicates the camera returned a shorter response than the maximum
possible size, which is normal for many cameras.

Known Limitations
*****************

* Maximum resolution depends on available memory (tested up to 2304x1536)
* Frame rate is limited by USB bandwidth and processing power
* Some cameras may not support all advertised formats
* MJPEG frames are stored compressed (no built-in JPEG decoder)
* Some camera-specific controls may not be available

Supported Pixel Formats
************************

The sample supports the following pixel formats:

Uncompressed Formats
====================

* **YUYV** (VIDEO_PIX_FMT_YUYV): YUV 4:2:2 format, 16 bits per pixel
* **RGB565** (VIDEO_PIX_FMT_RGB565): RGB 5:6:5 format, 16 bits per pixel
* **GREY** (VIDEO_PIX_FMT_GREY): 8-bit grayscale

Compressed Formats
==================

* **MJPEG** (VIDEO_PIX_FMT_JPEG): Motion JPEG, variable compression

The actual supported formats depend on your camera's capabilities.

Testing Without Hardware
*************************

For testing without a physical USB camera, you can use the software video
generator on native_sim:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_uvc
   :board: native_sim/native/64
   :snippets: video-sw-generator
   :goals: build
   :compact:
