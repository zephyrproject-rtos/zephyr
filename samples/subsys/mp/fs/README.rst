.. zephyr:code-sample:: mp-fs
   :name: MediaPipe filesystem sample

Overview
********

.. graphviz::

   digraph pipeline {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     filesrc  [label="filesrc"];
     filesink [label="filesink"];
     filesrc -> filesink;
   }

The pipeline reads an input file from a mounted FAT filesystem and writes it to an output file.

Currently the following boards are supported:

- ``native_sim/native/64``
- ``mimxrt700_evk/mimxrt798s/cm33_cpu0``

Notes
*****

- On ``native_sim``, the sample uses a ``zephyr,flash-disk`` named ``SD`` backed
  by ``flash.bin``. Use ``--flash_erase`` (or delete ``flash.bin``) to start from
  a clean filesystem.

Building and Running
********************

Native simulator (flash-backed FAT volume)
==========================================

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mp/fs
   :board: native_sim/native/64
   :goals: build
   :compact:

Run (erase flash content first)::

  ./build/zephyr/zephyr.exe --flash_erase

After the run completes, the output file will be present in the FAT volume.

To reuse the same flash image across runs, omit ``--flash_erase``.

MIMXRT700-EVK
=============

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mp/fs
   :board: mimxrt700_evk/mimxrt798s/cm33_cpu0
   :goals: build
   :compact:
