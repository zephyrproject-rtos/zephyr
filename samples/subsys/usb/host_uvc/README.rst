.. zephyr:code-sample:: usb-host-uvc
   :name: USB Host UVC (Video Camera)
   :relevant-api: video_interface

   Capture video frames from a USB camera using USB Host UVC driver.

Overview
********

This sample demonstrates how to use the USB Host UVC (USB Video Class) driver
to capture video frames from a USB camera. The captured frames can optionally
be displayed on an LCD screen.

Requirements
************

Hardware
========

* A board with USB Host support (e.g., rd_rw612_bga)
* A USB camera that supports UVC (USB Video Class)
* (Optional) An LCD display for viewing captured frames

The sample has been tested with:

* :zephyr:board:`rd_rw612_bga` with USB camera shield and LCD display

Building and Running
********************

To build this sample for rd_rw612_bga board with USB camera and LCD:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_uvc
   :board: rd_rw612_bga/rw612
   :shield: lcd_par_s035_8080
   :goals: build flash
   :compact:

To build for native_sim with software video generator (for testing):

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_uvc
   :board: native_sim/native/64
   :snippets: video-sw-generator
   :gen-args: -DCONFIG_VIDEO_SHELL=y
   :goals: build
   :compact:

Sample Output
=============

When a USB camera is connected, you should see:


Using the Video Shell
=====================

Build with ``-DCONFIG_VIDEO_SHELL=y`` to enable interactive control:

.. code-block:: console

   uart:~$ video --help
   video - Video driver commands
   Subcommands:
     start    : Start a video device and its sources
     stop     : Stop a video device and its sources
     capture  : Capture a given number of buffers from a device
     format   : Query or set the video format of a device
     frmival  : Query or set the video frame rate/interval of a device
     ctrl     : Query or set video controls of a device

Troubleshooting
***************

If the USB camera is not detected:

1. Verify USB Host support is enabled on your board
2. Check that the USB camera supports UVC (most modern webcams do)
3. Ensure sufficient memory is allocated (check heap and stack sizes)
4. Try reducing video resolution or buffer count if running out of memory

If frames are dropped or corrupted:

1. Increase ``CONFIG_VIDEO_BUFFER_POOL_NUM_MAX``
2. Increase ``CONFIG_UHC_BUF_POOL_SIZE``
3. Adjust frame rate to match camera and system capabilities
