.. _snippet-video-native-fifo:

Video Native FIFO Snippet (video-native-fifo)
#############################################

.. code-block:: console

   west build -b native_sim/native/64 -S video-native-fifo [...]

Overview
********

This snippet instantiates the :ref:`native_sim host FIFO video source
<nsim_per_video_fifo>` as the ``zephyr,camera`` :ref:`devicetree` chosen node.

The FIFO path defaults to ``/tmp/zephyr-video-fifo-<pid>.fifo``, and is
printed when streaming starts. The application selects the resolution and
pixel format, 320x240 RGB565 by default.

See :ref:`the driver documentation <nsim_per_video_fifo>` for how to set the
FIFO path and feed it from a webcam or a test pattern, and for the details of
the driver behaviour.

Requirements
************

A :zephyr:board:`native_sim` target, and a host process writing raw frames of
the selected format to the FIFO. Sufficient memory for the video resolution
must be declared by :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE`.
