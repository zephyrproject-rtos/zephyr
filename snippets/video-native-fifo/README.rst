.. _snippet-video-native-fifo:

Video Native FIFO Snippet (video-native-fifo)
#############################################

.. code-block:: console

   west build -b native_sim/native/64 -S video-native-fifo [...]

Overview
********

This snippet instantiates a video source reading raw frames from a named pipe
(FIFO) on the host running the native simulator. It is selected as the
``zephyr,camera`` :ref:`devicetree` chosen node, so applications using the
:ref:`video_api` see it as any other camera.

A host process is responsible for feeding the FIFO with raw frames of exactly
``width * height * bytes-per-pixel`` bytes. This allows an application built for
:ref:`native_sim` to consume frames coming from a real webcam, from a video
file, or from a generated test pattern, without any camera hardware.

The snippet declares a 320x240 RGB565 source reading from
``/tmp/zephyr-cam.fifo``. The path can be changed at runtime with the
``--video-fifo=<path>`` command line option, and the resolution and pixel format
can be changed by providing another devicetree overlay instead of this snippet.

Using a webcam as the video source with ``ffmpeg``:

.. code-block:: console

   ffmpeg -f v4l2 -i /dev/video0 \
     -vf "scale=320:240:force_original_aspect_ratio=increase,crop=320:240,fps=10" \
     -pix_fmt rgb565le -f rawvideo -y /tmp/zephyr-cam.fifo

Using a generated test pattern instead, which requires no webcam:

.. code-block:: console

   ffmpeg -re -f lavfi -i testsrc2=size=320x240:rate=10 \
     -pix_fmt rgb565le -f rawvideo -y /tmp/zephyr-cam.fifo

The application and the feeder can be started in any order, and the feeder can
be restarted at any time: the driver creates the FIFO if needed, keeps polling
it while no writer is attached, and drops the partially received frame when a
writer disconnects.

Requirements
************

A :ref:`native_sim` target, and a host process writing raw frames of the
declared size to the FIFO. Sufficient memory for the video resolution must be
declared by :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE`.
