.. zephyr:code-sample:: mpipe-tee-decode
   :name: MJPEG tee pipeline
   :relevant-api: mpipe_base mpipe_img mpipe_fs mpipe_disp

   Split one MJPEG stream into two branches, decoding it to a display on one and
   writing it back to a file on the other.

Overview
********

This sample demonstrates a **multi-branch** pipeline built with the
:ref:`Multimedia Pipeline <mpipe>` subsystem. A single MJPEG stream is read from
a file, parsed into frames, and then split by a ``tee`` element into two
independent branches: one decodes the frames and shows them on a display, the
other writes the untouched JPEG frames back out to a second file.

.. graphviz::
   :align: center

   digraph pipeline {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8", fontname="sans"];

     file_src    [label="file_src"];
     parser      [label="img_jpeg_parser"];
     caps_filter [label="caps_filter"];
     tee         [label="tee"];
     queue1      [label="queue"];
     decoder     [label="img_jpeg_decoder"];
     disp_sink   [label="disp_sink"];
     queue2      [label="queue"];
     file_sink   [label="file_sink"];

     file_src -> parser -> caps_filter -> tee;
     tee -> queue1 -> decoder -> disp_sink;
     tee -> queue2 -> file_sink;
   }

It is the sample that exercises the parts of the framework a single-branch graph
never reaches:

- The **tee** must find one format that satisfies every branch, since it has one
  input to supply them all. It also merges what the branches ask for into a
  single buffer pool proposal, rather than letting any one branch's pool travel
  upstream.
- The **queue** on each branch puts that branch on its own thread, so the
  decode and the file write proceed independently rather than one waiting on the
  other.
- **End-of-stream is aggregated.** Two sinks each report the end of the stream,
  but the pipeline passes on only the last, so the application is notified once
  and never tears the graph down while a branch is still running.

The ``caps_filter`` between the parser and the tee pins the JPEG frame format
before the split, so both branches negotiate against a format that is already
fixed.

Requirements
************

A board with a display, and a mounted FAT volume the sample can read its input
from and write its output to.

The build host must have GStreamer installed (specifically the ``gst-launch-1.0``
tool and the ``good`` plugins, which include ``videotestsrc`` and ``jpegenc``),
because the test MJPEG file is generated at build time.

On Debian/Ubuntu::

  sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-good

On Fedora::

  sudo dnf install gstreamer1-plugins-good

On macOS (Homebrew)::

  brew install gstreamer

Input
*****

The sample expects an MJPEG file at :kconfig:option:`CONFIG_FILE_INPUT_PATH`
(default ``/SD:/test.mjpeg``) and writes the tee's second branch to
:kconfig:option:`CONFIG_FILE_OUTPUT_PATH` (default ``/SD:/tee_out.mjpeg``).

The input file is generated at build time, with the resolution given by
:kconfig:option:`CONFIG_JPEG_IMAGE_WIDTH` and
:kconfig:option:`CONFIG_JPEG_IMAGE_HEIGHT` and the length by
:kconfig:option:`CONFIG_MJPEG_NUM_FRAMES`. It is embedded into the binary, and
the sample writes it to the volume on first boot if it is not already there, so
no manual file preparation is needed.

Building and Running
********************

Native simulator
================

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/tee_dec
   :board: native_sim/native/64
   :goals: build
   :compact:

Run, erasing the flash-backed FAT volume first::

  ./build/zephyr/zephyr.exe --flash_erase

NXP RT EVK platforms
====================

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/tee_dec
   :board: mimxrt1170_evk@B/mimxrt1176/cm7
   :shield: rk055hdmipi4ma0
   :goals: build flash
   :compact:

The sample is driven from the shell while it runs: ``p`` toggles play and pause,
``s`` stops, ``r`` replays, and ``q`` quits.

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0 ***
   [00:00:01.430,000] <inf> main: SDCard mounted at /SD:
   [00:00:01.447,000] <inf> main: Pipeline linked.
   [00:00:01.454,000] <inf> mpipe_player: Player shell ready. Interactive controls:
   [00:00:01.464,000] <inf> mpipe_player:   p = play/pause toggle, s = stop, r = replay, q = quit
   [00:00:01.497,000] <inf> mpipe_file_sink: Opened file for write: /SD:/tee_out.mjpeg
   [00:00:01.890,000] <inf> mpipe_player: Player state: PLAYING
   [00:00:13.923,000] <inf> mpipe_file_src: End of file
   [00:00:13.995,000] <inf> mpipe_player: End of stream
   [00:00:14.359,000] <inf> mpipe_player: Player state: STOPPED

The decoded frames appear on the display while the run proceeds, and
``/SD:/tee_out.mjpeg`` holds the JPEG frames written by the other branch.
