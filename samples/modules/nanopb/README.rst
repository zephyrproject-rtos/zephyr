.. _nanopb_sample:

Nanopb sample
#############

Overview
********

A simple protocol buffer sample using Nanopb for serializing structured data
to platform independent raw buffers or streams.


Requirements
************

Nanopb uses the protocol buffer compiler to generate source and header files,
make sure the ``protoc`` executable is installed and available.

.. tabs::

   .. group-tab:: Ubuntu

      Use ``apt`` to install dependency:

         .. code-block:: bash

            sudo apt install protobuf-compiler

   .. group-tab:: macOS

      Use ``brew`` to install dependency:

         .. code-block:: bash

            brew install protobuf

   .. group-tab:: Windows

      Use ``choco`` to install dependency:

         .. code-block:: console

            choco install protoc

Building and Running
********************

This application can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/nanopb
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:
