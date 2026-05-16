.. zephyr:code-sample:: lvgl_anim
   :name: LVGL Frame Animation
   :relevant-api: display_interface

   Play extracted RGB frame animation on a display using LVGL.

Overview
********

Plays a frame-by-frame animation extracted from a video file using the LVGL
animation API. Frames are generated at build time by ``video_to_lvgl_frames.py``
and stored as C arrays. Display width, height, and pixel format are extracted from
the ``zephyr,display`` chosen node in device tree source.

Requirements
************

* ``ffmpeg`` / ``ffprobe`` in ``PATH``
* Python 3 + Pillow: ``pip install Pillow``
* A board with a display (see :ref:`boards with display support <boards>`)

Building and Running
********************

Set the video path in ``prj.conf``:

.. code-block:: kconfig

   CONFIG_LVGL_ANIMATION_VIDEO_PATH="/path/to/video.mp4"

Example building for :zephyr:board:`native_sim <native_sim>`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl_anim
   :board: native_sim
   :goals: build run

Example building for :zephyr:board:`uedx24240013_md50e`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl_anim
   :board: uedx24240013_md50e
   :goals: build flash

.. code-block:: console

   Memory region         Used Size  Region Size  %age Used
              FLASH:     1508404 B    4194048 B     35.97%
        iram0_0_seg:       37344 B     378640 B      9.86%
        dram0_0_seg:      110208 B     378640 B     29.11%
        irom0_0_seg:      252566 B         4 MB      6.02%
        drom0_0_seg:     1442868 B         4 MB     34.40%
       rtc_iram_seg:          36 B         8 KB      0.44%
           IDT_LIST:           0 B         8 KB      0.00%
