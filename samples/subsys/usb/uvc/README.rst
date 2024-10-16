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
   :board: nrf52840dongle
   :goals: build flash
   :compact:

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

Upon reboot, the device is expected to be detected as a webcam device:

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

      The Device Manager or USBView utilities permit to list the presence of the device.

      The 3rd-party USB Tree View allows to review and debug the descriptors.

      In addition, the `USB3CV <https://www.usb.org/document-library/usb3cv>`_ tool
      from USB-IF can check that the device is compliant with the UVC standard.


Playing the Stream
==================

The device is recognized by the system as a native webcam and can be used by any video application.

For instance with VLC: ``Media`` > ``Open Capture Device`` > ``Capture Device`` > ``Video device name``.
Or with Gstreamer and FFmpeg:

.. tabs::

   .. group-tab:: Ubuntu

      Assuming ``/dev/video0`` is your Zephyr demo device.

      .. code-block:: console

         ffplay -i /dev/video0

      .. code-block:: console

         gst-launch-1.0 v4l2src device=/dev/video0 ! videoconvert ! autovideosink

   .. group-tab:: MacOS

      Assuming ``0:0`` is your Zephyr demo device.

      .. code-block:: console

         ffplay -f avfoundation -i 0:0

      .. code-block:: console

         gst-launch-1.0 avfvideosrc device-index=0 ! autovideosink

      With VLC: ``Media`` > ``Open Capture Device`` > ``Capture Device`` > ``Video device name``

   .. group-tab:: Windows

      Assuming ``UVC sample`` is your Zephyr demo device.

      .. code-block:: console

         ffplay -f dshow -i video="UVC sample"

      .. code-block:: console

         gst-launch-1.0 device=/dev/video0 ! videoconvert ! autovideosink

The video device can also be used by web applications and video conferencing systems.

Android and iPad (but not yet iOS) are also expected to work via dedicated applications.

Accessing the Video Controls
============================

On the host system, the controls would be available as video source
control through various applications, like any webcam.

.. tabs::

   .. group-tab:: Ubuntu

      Assuming ``/dev/video0`` is your Zephyr demo device.

      .. code-block:: console

         $ v4l2-ctl --device /dev/video0 --list-ctrls

         Camera Controls

                           auto_exposure 0x009a0901 (menu)   : min=0 max=3 default=1 value=1 (Manual Mode)
              exposure_dynamic_framerate 0x009a0903 (bool)   : default=0 value=0
                  exposure_time_absolute 0x009a0902 (int)    : min=10 max=2047 step=1 default=384 value=384 flags=inactive

         $ v4l2-ctl --device /dev/video0 --set-ctrl auto_exposure=1
         $ v4l2-ctl --device /dev/video0 --set-ctrl exposure_time_absolute=1500

   .. group-tab:: Windows

      The `VLC <https://www.videolan.org/vlc/>`_ client and `Pot Player <https://potplayer.tv/>`_
      client permit to further access the video controls.

   .. group-tab:: MacOS

      The `VLC <https://www.videolan.org/vlc/>`_ client and the system Webcam Settings panel
      allows adjustment of the supported video controls.


Software Processing
===================

Software processing tools can also use the video interface directly.

Here is an example with OpenCV:

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
