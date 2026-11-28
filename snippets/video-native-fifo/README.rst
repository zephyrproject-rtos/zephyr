.. _snippet-video-native-fifo:

Video Native FIFO Snippet (video-native-fifo)
#############################################

.. code-block:: console

   west build -b native_sim/native/64 -S video-native-fifo [...]

Overview
********

This snippet enables the :ref:`native_sim host video source
<nsim_per_video_fifo>` of the board, the ``video_fifo`` node, and selects it
as the ``zephyr,camera`` :ref:`devicetree` chosen node.

The node points at the host V4L2 device ``/dev/video0``. Pass
``--video-fifo=<path>`` to use another device or a FIFO. The application
selects the resolution and pixel format, 320x240 RGB565 by default.

See :ref:`the driver documentation <nsim_per_video_fifo>` for how to capture
from a webcam or feed a FIFO with a test pattern, and for the details of the
driver behaviour.

Requirements
************

A :zephyr:board:`native_sim` target, and either a host V4L2 capture device or
a host process writing raw frames of the selected format to a FIFO. Sufficient
memory for the video resolution must be declared by
:kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE`.
