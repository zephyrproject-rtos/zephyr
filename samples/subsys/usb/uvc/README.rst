.. zephyr:code-sample:: uvc
   :name: USB Video sample
   :relevant-api: usbd_api video_interface

   USB Video sample sending frames over USB.

Overview
********

This sample demonstrates how to use an USB Video Class instance to
send video data over USB.

Upon connection, a video interface would show-up in the operating system,
using the same protocol as most webcams, and be detected as such.

Any software application to read and process video would then be able
to access the stream.

Requirements
************

USB is the only requirement for this sensor, as an emulated image
sensor and MIPI receiver is used to provide a test pattern from
software.

The USB descriptor configuration is extracted from these drivers, and
the user does not need to provide them.

Building and Running
********************

Build the sample application and flash the resulting binaries.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/uvc
   :board: mini_stm32h734
   :goals: build flash
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/uvc
   :board: rpi_pico
   :goals: build flash
   :compact:

Upon reboot, the device is expected to be detected by the operating
system.

On Linux, OSX and BSDs this is visible using the ``dmesg`` and ``lsusb`` commands.

On Windows, this is visible using the "Windows Device Manager" application.

Playing the Stream
==================

Some example of client to open the video stream on a Linux
environment, assuming ``/dev/video2`` is your Zephyr demo freshly
attached.

Play it using `FFplay <https://ffmpeg.org/ffplay.html>`_

.. code-block:: console

   ffplay /dev/video2

Play it using `GStramer <https://gstreamer.freedesktop.org/>`_:

.. code-block:: console

   gst-launch-1.0 v4l2src device=/dev/video2 ! videoconvert ! autovideosink

Play it using `MPV <https://mpv.io/>`_:

.. code-block:: console

   mpv /dev/video2

Play it using `VLC <https://www.videolan.org/vlc/>`_:

.. code-block:: console

   vlc v4l2:///dev/video2

The video device can also be used by web applications and video
conferencing systems, as it is recognized by the system as a native
webcam.

On Windows, accessing the video can be done with the default "Camera" application,
by switching the camera source.

Android and iPad (but not yet iOS) are also expected to work via
dedicated applications.

Accessing the Video Controls
============================

On Linux, the ``v4l2-ctl`` command permits to list the supported controls:

.. code-block:: console

   $ v4l2-ctl --device /dev/video2 --list-ctrls

   Camera Controls

                     auto_exposure 0x009a0901 (menu)   : min=0 max=3 default=1 value=1 (Manual Mode)
        exposure_dynamic_framerate 0x009a0903 (bool)   : default=0 value=0
            exposure_time_absolute 0x009a0902 (int)    : min=10 max=2047 step=1 default=384 value=384 flags=inactive

   $ v4l2-ctl --device /dev/video2 --set-ctrl auto_exposure=1
   $ v4l2-ctl --device /dev/video2 --set-ctrl exposure_time_absolute=1500

On Windows, the `VLC <https://www.videolan.org/vlc/>`_ client and `Pot Player <https://potplayer.tv/>`_
client permit to further access the video controls.

Software Processing
===================

Software processing tools can also use the video interface directly.

Here is an example with OpenCV:

.. code-block:: python

   import cv2

   # Number of the /dev/video# interface
   num = 2

   cv2.namedWindow("preview")
   vc = cv2.VideoCapture(num)

   while (val := vc.read())[0]:
       cv2.waitKey(20)
       cv2.imshow("preview", val[1])

   cv2.destroyWindow("preview")
   vc.release()
