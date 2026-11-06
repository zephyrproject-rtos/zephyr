.. zephyr:code-sample:: mpipe-net-stream
   :name: Multimedia Pipeline network streaming
   :relevant-api: mpipe_net

   Stream an MJPEG file to a TCP client through a two-element pipeline.

Overview
********

.. graphviz::

   digraph pipeline {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     file_src -> tcp_server_sink;
   }

``file_src`` reads an MJPEG file from a FAT volume and ``tcp_server_sink`` pushes the
bytes to one TCP client. Parsing and decoding are left to the client.

The sample builds for ``native_sim/native/64``.

Building and Running
********************

Streaming to a player
=====================

The board configuration brings up TAP networking, which needs a ``zeth``
interface and root privileges (see :ref:`networking_with_native_sim`). Copy an
MJPEG file named ``sample.mjp`` onto the FAT volume first.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/net_stream
   :board: native_sim/native/64
   :goals: build
   :compact:

Run it, then connect a player. The pipeline starts once a client connects::

  ./build/zephyr/zephyr.exe
  ffplay -f mjpeg tcp://192.0.2.1:5000

Self-test over loopback
=======================

``loopback.conf`` swaps TAP for the loopback interface and adds a client thread
that writes the input file, counts the frames it gets back and logs PASS or
FAIL. It needs no player and no root.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/net_stream
   :board: native_sim/native/64
   :gen-args: -DEXTRA_CONF_FILE=loopback.conf
   :goals: build
   :compact:

Run it with a clean flash image::

  ./build/zephyr/zephyr.exe --flash_erase

  [selftest] PASS: received 3 frames
