.. zephyr:code-sample:: uvc
   :name: USB Video webcam
   :relevant-api: usbd_api usbd_uvc video_interface

   Send video frames over USB.

Overview
********

This sample demonstrates how to use a USB Video Class instance to send video data over USB.

Upon connection, a video device will show-up on the host, usable like a regular webcam device.

Any software on the host can then access the video stream as a local video source.

Requirements
************

This sample uses the new USB device stack and requires the USB device
controller ported to the :ref:`udc_api`.

Building and Running
********************

If a board does not have a camera supported, the :ref:`snippet-video-sw-generator` snippet can be
used to test without extra hardware than the USB interface, via a software-generated test pattern:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/uvc
   :board: frdm_mcxn947/mcxn947/cpu0
   :snippets: video-sw-generator
   :goals: build flash
   :compact:

If a board is equipped with a supported image sensor configured as the ``zephyr,camera`` chosen
node, then it will be used as the video source. The sample can then be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/uvc
   :board: arduino_nicla_vision/stm32h747xx/m7
   :goals: build flash
   :compact:

The device is expected to be detected as a webcam device:

.. tabs::

   .. group-tab:: Ubuntu

      The ``dmesg`` logs are expected to mention a ``generic UVC device``.

      The ``lsusb`` is expected to show an entry for a Zephyr device.

      Refers to `Ideas on board FAQ <https://www.ideasonboard.org/uvc/faq/>`_
      for how to get more debug information.

   .. group-tab:: MacOS

      The ``dmesg`` logs are expected to mention a video device.

      The ``ioreg -p IOUSB`` command list the USB devices including cameras.

      The ``system_profiler SPCameraDataType`` command list video input devices.

   .. group-tab:: Windows

      The Device Manager or USBView utilities permit to list the USB devices.

      The 3rd-party USB Tree View allows to review and debug the descriptors.

      In addition, the `USB3CV <https://www.usb.org/document-library/usb3cv>`_ tool
      from USB-IF can check that the device is compliant with the UVC standard.

Playing the Stream
==================

The device is recognized by the system as a native webcam and can be used by any video application.

For instance with VLC:
:menuselection:`Media --> Open Capture Device --> Capture Device --> Video device name`.

Or with Gstreamer and FFmpeg:

.. tabs::

   .. group-tab:: Ubuntu

      Assuming ``/dev/video0`` is your Zephyr device.

      .. code-block:: console

         ffplay -i /dev/video0

      .. code-block:: console

         gst-launch-1.0 v4l2src device=/dev/video0 ! videoconvert ! autovideosink

   .. group-tab:: MacOS

      Assuming ``0:0`` is your Zephyr device.

      .. code-block:: console

         ffplay -f avfoundation -i 0:0

      .. code-block:: console

         gst-launch-1.0 avfvideosrc device-index=0 ! autovideosink

   .. group-tab:: Windows

      Assuming ``UVC sample`` is your Zephyr device.

      .. code-block:: console

         ffplay.exe -f dshow -i video="UVC sample"

      .. code-block:: console

         gst-launch-1.0.exe ksvideosrc device-name="UVC sample" ! videoconvert ! autovideosink

The video device can also be used by web and video call applications systems.

Android and iPad (but not yet iOS) are also expected to work via dedicated applications.

Accessing the Video Controls
============================

On the host system, the controls would be available as video source
control through various applications, like any webcam.

.. tabs::

   .. group-tab:: Ubuntu

      Assuming ``/dev/video0`` is your Zephyr device.

      .. code-block:: console

         $ v4l2-ctl --device /dev/video0 --list-ctrls

         Camera Controls

                           auto_exposure 0x009a0901 (menu)   : min=0 max=3 default=1 value=1 (Manual Mode)
              exposure_dynamic_framerate 0x009a0903 (bool)   : default=0 value=0
                  exposure_time_absolute 0x009a0902 (int)    : min=10 max=2047 step=1 default=384 value=384 flags=inactive

         $ v4l2-ctl --device /dev/video0 --set-ctrl auto_exposure=1
         $ v4l2-ctl --device /dev/video0 --set-ctrl exposure_time_absolute=1500

   .. group-tab:: MacOS

      The `VLC <https://www.videolan.org/vlc/>`_ client and the system Webcam Settings panel
      allows adjustment of the supported video controls.

   .. group-tab:: Windows

      The `VLC <https://www.videolan.org/vlc/>`_ client and `Pot Player <https://potplayer.tv/>`_
      client permit to further access the video controls.

Software Processing
===================

Software processing tools can also use the video interface directly.

Here is an example with OpenCV (``pip install opencv-python``):

.. code-block:: python

   import cv2

   # Number of the /dev/video# interface
   devnum = 2

   cv2.namedWindow("preview")
   vc = cv2.VideoCapture(devnum)

   while (val := vc.read())[0]:
       cv2.waitKey(20)
       cv2.imshow("preview", val[1])

   cv2.destroyWindow("preview")
   vc.release()
