.. zephyr:code-sample:: mp-net-stream
   :name: MediaPipe network streaming

Overview
********

.. graphviz::

   digraph pipeline {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     filesrc [label="filesrc"];
     tcpsink [label="tcpsink"];
     filesrc -> tcpsink;
   }

The pipeline reads an MJPEG file from a mounted FAT filesystem and pushes the raw bytes to a
single TCP client. Frame parsing and decoding are left to the client.

Currently the following boards are supported:

- ``native_sim/native/64``

Notes
*****

- On ``native_sim``, the sample uses a ``zephyr,flash-disk`` named ``SD`` backed by
  ``flash.bin``. Use ``--flash_erase`` (or delete ``flash.bin``) to start from a clean
  filesystem.
- The board configuration brings up TAP networking, which needs a ``zeth`` interface and root
  privileges. ``loopback.conf`` replaces it with the Zephyr loopback interface and needs neither.

Building and Running
********************

Streaming to an external player
===============================

Copy an MJPEG file named ``sample.mjp`` into the FAT volume first.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mp/net_stream
   :board: native_sim/native/64
   :goals: build
   :compact:

Run it::

  ./build/zephyr/zephyr.exe

The sink waits for a client before the pipeline starts playing. Connect with::

  ffplay -f mjpeg tcp://192.0.2.1:5000

Self-test over loopback
=======================

The self-test generates its own input file, connects a client thread to the sink, and checks
that every JPEG frame survives the round trip. No external player and no TAP device are
involved.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mp/net_stream
   :board: native_sim/native/64
   :gen-args: -DEXTRA_CONF_FILE=loopback.conf
   :goals: build
   :compact:

Run it (erase the flash content first)::

  ./build/zephyr/zephyr.exe --flash_erase

Sample output::

  [selftest] Connected to the sink
  [selftest] PASS: received 3 frames
