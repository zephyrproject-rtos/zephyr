.. zephyr:code-sample:: mp-jpeg-decode
   :name: MJPEG file decode pipeline

Description
***********

This sample demonstrates decoding Motion-JPEG (MJPEG) from a file and displaying the decoded frames
using the MP subsystem.

Pipeline topology
=================

The sample builds one of two slightly different pipelines depending on whether a
hardware JPEG decoder is available (``zephyr,jpegdec`` chosen node).

Pipeline A: HW JPEG decode
---------------------------

When ``zephyr,jpegdec`` is present, decoding is performed by a hardware-backed
``mp_zvid_transform``. The HW decoder typically outputs NV12 (or another YUV
format), so a software ``mp_zvid_convert`` stage converts the output to RGB565
for the display controller.

.. graphviz::

   digraph pipeline_a {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     filesrc      [label="filesrc\n(zfs)"];
     jpeg_parser  [label="jpeg_parser\n(zjpeg)"];
     capsfilter   [label="capsfilter\n(core)"];
     hw_jpegdec   [label="HW jpegdec\n(zvid_transform)"];
     videoconvert [label="videoconvert\n(zvid)"];
     display      [label="display\n(zdisp)"];
     filesrc -> jpeg_parser -> capsfilter -> hw_jpegdec -> videoconvert -> display;
   }

Pipeline B: SW JPEG decode
---------------------------

When no ``zephyr,jpegdec`` is present, decoding falls back to the software
``mp_zjpeg_decoder``. The SW decoder currently outputs RGB565, so no
``mp_zvid_convert`` stage is needed.

.. graphviz::

   digraph pipeline_b {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     filesrc      [label="filesrc\n(zfs)"];
     jpeg_parser  [label="jpeg_parser\n(zjpeg)"];
     capsfilter   [label="capsfilter\n(core)"];
     sw_jpegdec   [label="SW jpegdec\n(zjpeg)"];
     display      [label="display\n(zdisp)"];
     filesrc -> jpeg_parser -> capsfilter -> sw_jpegdec -> display;
   }

Elements
--------

- ``mp_zfilesrc`` reads chunks from the file specified by :kconfig:option:`CONFIG_FILE_INPUT_PATH`.
- ``mp_zjpeg_parser`` splits the MJPEG byte stream into individual JPEG frames.
- ``mp_caps_filter`` constrains the JPEG format (width/height from Kconfig:
  :kconfig:option:`CONFIG_JPEG_IMAGE_WIDTH`, :kconfig:option:`CONFIG_JPEG_IMAGE_HEIGHT`).
- ``mp_zjpeg_decoder`` (SW) or ``mp_zvid_transform`` (HW) decodes JPEG frames.
- ``mp_zvid_convert`` performs pixel-format conversion (NV12 → RGB565) when using HW decode.
- ``mp_zdisp_sink`` renders decoded frames to the display.

Notes
-----

- If ``zephyr,videotrans`` is also available, an additional ``mp_zvid_transform``
  is inserted before the display sink (e.g. for rotation via
  :kconfig:option:`CONFIG_VIDEO_ROTATION_ANGLE`).
- The capsfilter is placed between the parser and decoder to enforce a fixed
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
============

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
   :zephyr-app: samples/subsys/mp/jpeg_dec
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
   :zephyr-app: samples/subsys/mp/jpeg_dec
   :board: mimxrt700_evk/mimxrt798s/cm33/cpu0
   :goals: build
   :compact:

The test MJPEG file is generated during build and embedded into the binary.
On first boot it is written to the SD card automatically.
Make sure the SD card is FAT formatted and writable.
