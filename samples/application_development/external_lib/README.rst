.. _external_library:

External Library
#################

Overview
********

A simple example that demonstrates how to include an external static library
into the Zephyr build system.
The demonstrates both how to build the external library using a different build
system and how to include the built static library.

Windows Note
************

To use this sample on a Windows host operating system, GNU Make needs to be in
your path. This can be setup using chocolatey or by manually installing it.

Chocolatey Method
=================

Install make using the following command:

.. code-block:: bash

   choco install make

Once installed, build the application as normal.

Manual Install Method
=====================

The pre-built make application can be downloaded from
https://gnuwin32.sourceforge.net/packages/make.htm by either getting both the
``Binaries`` and ``Dependencies`` and extracting them to the same folder, or
getting the ``Complete package`` setup. Once installed and in the path, build
the application as normal.
