.. zephyr:code-sample:: mpipe-jpeg-decode
   :name: MJPEG file decode pipeline
   :relevant-api: mpipe_img mpipe_fs mpipe_disp mpipe_base

   Decode Motion-JPEG from a file and display the frames, using a hardware
   decoder where the board has one and a software decoder otherwise.

Overview
********

This sample demonstrates decoding Motion-JPEG (MJPEG) from a file and displaying the decoded frames
using the Multimedia Pipeline subsystem.

Pipeline topology
=================

The sample builds one of two slightly different pipelines depending on whether a
hardware JPEG decoder is available (``zephyr,jpegdec`` chosen node).

Pipeline A: HW JPEG decode
---------------------------

When ``zephyr,jpegdec`` is present, decoding is performed by a hardware-backed
``mpipe_vid_transform``. The HW decoder typically outputs NV12 (or another YUV
format), so a software ``mpipe_vid_convert`` stage converts the output to RGB565
for the display controller.

.. graphviz::

   digraph pipeline_a {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     file_src      [label="file_src\n(fs)"];
     jpeg_parser  [label="jpeg_parser\n(img)"];
     caps_filter   [label="caps_filter\n(base)"];
     hw_jpegdec   [label="HW jpegdec\n(vid_transform)"];
     vid_convert  [label="vid_convert\n(vid)"];
     display      [label="display\n(disp)"];
     file_src -> jpeg_parser -> caps_filter -> hw_jpegdec -> vid_convert -> display;
   }

Pipeline B: SW JPEG decode
---------------------------

When no ``zephyr,jpegdec`` is present, decoding falls back to the software
``mpipe_img_jpeg_decoder``. The SW decoder currently outputs RGB565, so no
``mpipe_vid_convert`` stage is needed.

.. graphviz::

   digraph pipeline_b {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     file_src      [label="file_src\n(fs)"];
     jpeg_parser  [label="jpeg_parser\n(img)"];
     caps_filter   [label="caps_filter\n(base)"];
     sw_jpegdec   [label="SW jpegdec\n(img)"];
     display      [label="display\n(disp)"];
     file_src -> jpeg_parser -> caps_filter -> sw_jpegdec -> display;
   }

Elements
--------

- ``mpipe_file_src`` reads chunks from the file specified by
  :kconfig:option:`CONFIG_FILE_INPUT_PATH`.
- ``mpipe_img_jpeg_parser`` splits the MJPEG byte stream into individual JPEG frames.
- ``mpipe_caps_filter`` constrains the JPEG format (width/height from Kconfig:
  :kconfig:option:`CONFIG_JPEG_IMAGE_WIDTH`, :kconfig:option:`CONFIG_JPEG_IMAGE_HEIGHT`).
- ``mpipe_img_jpeg_decoder`` (SW) or ``mpipe_vid_transform`` (HW) decodes JPEG frames.
- ``mpipe_vid_convert`` performs pixel-format conversion (NV12 -> RGB565) when using HW decode.
- ``mpipe_disp_sink`` renders decoded frames to the display.

Notes
-----

- If ``zephyr,videotrans`` is also available, an additional ``mpipe_vid_transform``
  is inserted before the display sink (e.g. for rotation via
  :kconfig:option:`CONFIG_VIDEO_ROTATION_ANGLE`).
- The caps_filter is placed between the parser and decoder to enforce a fixed
  JPEG frame format before decoding begins.

Input
*****

The sample expects an MJPEG file at the path specified by
:kconfig:option:`CONFIG_FILE_INPUT_PATH` (default ``/SD:/test.mjpeg``).

A test MJPEG file is **generated at build time** using GStreamer CLI.
The resolution matches :kconfig:option:`CONFIG_JPEG_IMAGE_WIDTH` and
:kconfig:option:`CONFIG_JPEG_IMAGE_HEIGHT`, and the number of frames
is controlled by :kconfig:option:`CONFIG_MJPEG_NUM_FRAMES`.

The generated file is placed in the build directory and embedded into the binary
via ``mjpeg.inc``. On first boot the sample checks whether the file already exists
on the filesystem; if not, it writes the embedded data automatically. No manual
file preparation is required.

Requirements
************

The build host must have GStreamer installed (specifically the ``gst-launch-1.0``
tool and the ``good`` plugins which include ``videotestsrc`` and ``jpegenc``).

On Debian/Ubuntu::

  sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-good

On Fedora::

  sudo dnf install gstreamer1-plugins-good

On macOS (Homebrew)::

  brew install gstreamer

Building and Running
********************

Native simulator
================

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/jpeg_dec
   :board: native_sim/native/64
   :goals: build
   :compact:

Run::

  ./build/zephyr/zephyr.exe

The test MJPEG file is generated during build and embedded into the binary.
On first run it is written to the simulated SD card (``flash.bin``).

NXP RT EVK platforms
====================

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/jpeg_dec
   :board: mimxrt700_evk/mimxrt798s/cm33/cpu0
   :goals: build
   :compact:

The test MJPEG file is generated during build and embedded into the binary.
On first boot it is written to the SD card automatically.
Make sure the SD card is FAT formatted and writable.
