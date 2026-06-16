.. zephyr:code-sample:: mp-jpeg-encode
   :name: MJPEG file encode (round-trip) pipeline

Description
***********

This sample demonstrates encoding JPEG frames with the MP subsystem using the
software ``mp_zjpeg_encoder`` element. It builds a self-contained round-trip
pipeline: an input MJPEG file is parsed and decoded to RGB565, then re-encoded to
JPEG and written back out as a new MJPEG file. No display or capture hardware is
required, so it runs on ``native_sim``.

Pipeline topology
=================

.. graphviz::

   digraph pipeline {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     filesrc      [label="filesrc\n(zfs)"];
     jpeg_parser  [label="jpeg_parser\n(zjpeg)"];
     capsfilter   [label="capsfilter\n(core)"];
     jpeg_decoder [label="jpeg_decoder\n(zjpeg, JPEG->RGB565)"];
     jpeg_encoder [label="jpeg_encoder\n(zjpeg, RGB565->JPEG)"];
     filesink     [label="filesink\n(zfs)"];
     filesrc -> jpeg_parser -> capsfilter -> jpeg_decoder -> jpeg_encoder -> filesink;
   }

Elements
--------

- ``mp_zfilesrc`` reads chunks from the file specified by
  :kconfig:option:`CONFIG_FILE_INPUT_PATH`.
- ``mp_zjpeg_parser`` splits the MJPEG byte stream into individual JPEG frames.
- ``mp_caps_filter`` constrains the JPEG format (width/height from Kconfig:
  :kconfig:option:`CONFIG_JPEG_IMAGE_WIDTH`, :kconfig:option:`CONFIG_JPEG_IMAGE_HEIGHT`),
  so the dimensions propagate down to the encoder before data flows.
- ``mp_zjpeg_decoder`` decodes JPEG frames to RGB565.
- ``mp_zjpeg_encoder`` re-encodes the RGB565 frames to JPEG. Its quality and chroma
  subsampling are set via :kconfig:option:`CONFIG_JPEG_ENC_QUALITY` and
  :kconfig:option:`CONFIG_JPEG_ENC_SUBSAMPLE` (mapped to ``PROP_ZJPEG_ENC_QUALITY``
  and ``PROP_ZJPEG_ENC_SUBSAMPLE``).
- ``mp_zfilesink`` concatenates the re-encoded JPEG frames into the file specified
  by :kconfig:option:`CONFIG_FILE_OUTPUT_PATH`, which is itself a valid MJPEG stream.

Notes
-----

- The decoder defaults to and advertises RGB565; the encoder negotiates RGB565-only
  on its sink pad, so the decoder→encoder link is RGB565 with no conversion stage.
- This first version operates full-frame: the whole RGB565 frame is encoded in one
  pass per buffer. Banded operation, YUV420 round-trip, and a continuous 1-100
  quality scale are future work.

Input and output
****************

The sample expects an MJPEG file at the path specified by
:kconfig:option:`CONFIG_FILE_INPUT_PATH` (default ``/SD:/test.mjpeg``) and writes
the re-encoded result to :kconfig:option:`CONFIG_FILE_OUTPUT_PATH`
(default ``/SD:/test_out.mjpeg``).

A test MJPEG file is **generated at build time** using GStreamer CLI. The
resolution matches :kconfig:option:`CONFIG_JPEG_IMAGE_WIDTH` and
:kconfig:option:`CONFIG_JPEG_IMAGE_HEIGHT`, and the number of frames is controlled
by :kconfig:option:`CONFIG_MJPEG_NUM_FRAMES`.

The generated file is placed in the build directory and embedded into the binary
via ``mjpeg.inc``. On first boot the sample checks whether the input file already
exists on the filesystem; if not, it writes the embedded data automatically. No
manual file preparation is required.

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
   :zephyr-app: samples/subsys/mp/jpeg_enc
   :board: native_sim/native/64
   :goals: build
   :compact:

Run::

  ./build/zephyr/zephyr.exe --flash_erase

The test MJPEG file is generated during build and embedded into the binary.
On first run it is written to the simulated SD card (``flash.bin``), and the
re-encoded output is written next to it.

Verifying the output
====================

The re-encoded MJPEG can be copied out of the simulated FAT volume (the partition
starts at offset ``0xf0000`` in ``flash.bin``) and inspected::

  mcopy -i build/zephyr/flash.bin@@0xf0000 ::/test_out.mjpeg /tmp/test_out.mjpeg

A valid output starts with a JPEG SOI marker (``FFD8``), ends with an EOI marker
(``FFD9``), and contains one SOI/EOI pair per input frame.

Measuring quality (PSNR)
========================

The transcode quality can be measured by comparing the original input MJPEG with
the re-encoded output using ffmpeg's ``psnr`` filter.

Extract both files from the simulated SD card. The FAT partition starts at offset
``0xf0000`` in ``flash.bin``, which is created in the directory where the binary
was run::

  mdir  -i flash.bin@@0xf0000
  mcopy -n -i flash.bin@@0xf0000 ::/test.mjpeg     test_in.mjpeg
  mcopy -n -i flash.bin@@0xf0000 ::/test_out.mjpeg test_out.mjpeg

Compute the PSNR (per-frame statistics are written to ``psnr.log``)::

  ffmpeg -i test_in.mjpeg -i test_out.mjpeg -lavfi psnr=stats_file=psnr.log -f null -

Example result (320x240, 150 frames, quality BEST, 4:2:0)::

  [Parsed_psnr_0 ...] PSNR y:26.52 u:27.66 v:24.11 average:25.84 min:25.83 max:25.84

  $ head -2 psnr.log
  n:1 mse_avg:169.51 mse_y:144.77 mse_u:111.49 mse_v:252.25 psnr_avg:25.84 psnr_y:26.52 psnr_u:27.66 psnr_v:24.11
  n:2 mse_avg:169.54 mse_y:144.76 mse_u:111.49 mse_v:252.36 psnr_avg:25.84 psnr_y:26.52 psnr_u:27.66 psnr_v:24.11

The low PSNR (~26 dB average, ~24 dB on the V/Cr channel) has several contributing
factors. They are not individually quantified, and they are not equal in magnitude:

* (1) YUV<->RGB color conversion (integer math, in both decoder and encoder)
* (2) RGB565 truncation
* (3) JPEG encoder quantization
* (4) 4:4:4 -> 4:2:0 chroma subsampling
* (5) fast/approximate DCTs (AA&N "IFAST") in both the decoder (inverse DCT) and
  the encoder (forward DCT)

Per-frame data flow:

The JPEG decoder (JPEGDEC) decodes each 4:4:4 frame using the fast AA&N inverse
DCT (loss 5), converts YUV->RGB (first half of loss 1), and truncates
RGB24->RGB565 (first half of loss 2).

The encoder (JPEGENC) expands RGB565->RGB24 by replicating the least-significant
bits into the freed low bits (the 5-bit red and blue components extend ``abcde``
to ``abcdecde``; the 6-bit green component extends ``abcdef`` to ``abcdefef``).
This is the second half of loss 2. It then converts RGB->YUV (second half of
loss 1), subsamples 4:4:4 -> 4:2:0 (loss 4), and encodes via the fast AA&N forward
DCT and quantization (losses 5 and 3). The AA&N "IFAST" DCT is analogous to
libjpeg's ``jidctfst`` / ``jfdctfst``: fast but less accurate than the accurate
integer (``jidctint`` / islow) or float (``jidctflt``) variants.

Finally, the host ffmpeg ``psnr`` filter compares the original and transcoded
streams in the Y/U/V planes. Because the output is 4:2:0 and the input 4:4:4, it
upsamples the output chroma back to 4:4:4 to align the planes (this surfaces
loss 4 in the comparison).
