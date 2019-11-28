.. _shell-fs-sample:

File system shell example
#########################

Overview
********

This example provides shell access to a NFFS file system partition in flash.

Requirements
************

A board with NFFS file system support and UART console

Building
********

Native Posix
============

Before starting a build, make sure that the i386 pkgconfig directory is in your
search path and that a 32-bit version of libfuse is installed. For more
background information on this requirement see :ref:`native_posix`.

.. code-block:: console

  export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/fs
   :board: native_posix
   :goals: build
   :compact:

See :ref:`native_posix` on how to connect to the UART.

Reel Board
==========

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/fs
   :board: reel_board
   :goals: build
   :compact:

Particle Xenon
==============

This target is customized to support the same SPI NOR partition table as
the :ref:`littlefs-sample`.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/fs
   :board: particle_xenon
   :goals: build
   :compact:

Running
*******

Once the board has booted, you will be presented with a shell prompt.
All file system related commands are available as sub-commands of fs.

Begin by mounting the NFSS file system.

.. code-block:: console

  fs mount nffs /nffs

Files System Shell Commands
===========================

Mount
-----

Mount a file system partition to a given mount point

.. code-block:: console

  fs mount (nffs|fat) <path>

Ls
--

List all files and directories in a given path

.. code-block:: console

  fs ls [path]

Cd
--

Change current working directory to given path

.. code-block:: console

  fs cd [path]

Pwd
---

List current working directory

.. code-block:: console

  fs pwd

Write
-----

Write hexadecimal numbers to a given file.
Optionally a offset in the file can be given.

.. code-block:: console

  fs write <path> [-o <offset>] <hex number> ...

Read
----

Read file and dump in hex and ASCII format

.. code-block:: console

  fs read <path>

Trunc
-----

Truncate a given file

.. code-block:: console

  fs trunc <path>

Mkdir
-----

Create a directory

.. code-block:: console

  fs mkdir <path>

Rm
--

Remove a file or directory

.. code-block:: console

  fs rm <path>

Flash Host Access
=================

For the Native POSIX board the flash partitions can be accessed from the host
Linux system.

By default the flash partitions are accessible through the directory *flash*
relative to the directory where the build is started.


