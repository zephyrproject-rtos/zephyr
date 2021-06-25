.. _nanopb_sample2:

Nanopb sample2
##############

Overview
********

A simple protocol buffer sample using Nanopb for serializing structured data
to platform independent raw buffers or streams.

The person structure used in this sample program contains three properties as
shown below.

.. code-block:: C

message Person {
    required int32 id = 1;
    required string name = 2;
    required string email = 3;
}

Requirements
************

Nanopb uses the protocol buffer compiler to generate source and header files,
make sure the ``protoc`` executable is intalled and available.

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
   :zephyr-app: samples/modules/nanopb2
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============


.. code-block:: console

   size of data : 41
   Person information [ID:12345]
      name : John Smith
      email: john.smith@mydomain.com

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

