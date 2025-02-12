.. zephyr:code-sample:: shell-fs
   :name: File system shell
   :relevant-api: file_system_api

   Access a LittleFS file system partition in flash using the file system shell.

Overview
********

This example provides shell access to a LittleFS file system partition in flash.

Requirements
************

A board with LittleFS file system support and UART console

Building
********

native_sim
==========

You can build this sample for :ref:`native_sim <native_sim>` with:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/fs
   :board: native_sim
   :goals: build
   :compact:

Which by default will use the flash simulator. The flash simulator will use a file,
:file:`flash.bin`, in the current directory to keep the flash content.
You have several options to control this. You can check them with:

.. code-block:: console

  zephyr/zephyr.exe -help

Check the :ref:`native_sim UART documentation <native_ptty_uart>` for information on how to connect
to the UART.

With FUSE access in the host filesystem
---------------------------------------

If you enable the :ref:`host FUSE filsystem access <native_fuse_flash>`
you will also have the flash filesystem mounted and accessible from your Linux host filesystem.

Before starting a build, make sure that the i386 pkgconfig directory is in your
search path and that a 32-bit version of libfuse is installed. For more
background information on this requirement check
:ref:`the native_sim documentation <native_fuse_flash>`.

.. code-block:: console

  export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/fs
   :board: native_sim
   :gen-args: -DCONFIG_FUSE_FS_ACCESS=y
   :goals: build
   :compact:

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
the :zephyr:code-sample:`littlefs` sample.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/fs
   :board: particle_xenon
   :goals: build
   :compact:

Flash load
==========

If you want to use the 'flash load' command then build the sample with the
'prj_flash_load.conf' configuration file. It has defined a larger RX buffer.
If the buffer is too small then some data may be lost during transfer of large
files.

Running
*******

Once the board has booted, you will be presented with a shell prompt.
All file system related commands are available as sub-commands of fs.

Begin by mounting the LittleFS file system.

.. code-block:: console

  fs mount littlefs /lfs

Loading filesystem from host PC to flash memory
===============================================

Use command:

.. code-block:: console

  flash load <address> <size>

It allows loading the data via UART, directly into flash memory at a given
address. Data must be aligned to a value dependent on the target flash memory,
otherwise it will cause an error and nothing will be loaded.

From the host side file system must be loaded with 'dd' tool with 'bs=64'
(if the file is loaded in chunks greater than 64B the data is lost and isn't
received by the Zephyr shell).

Example in Zephyr console:

.. code-block:: console

  flash load 0x7a000 0x5000

Example in the host PC:

.. code-block:: console

  dd if=filesystem of=/dev/ttyACM0 bs=64

During the transfer there are printed messages indicating how many chunks are
already written. After the successful transfer the 'Read all' message is
printed.

Files System Shell Commands
===========================

Mount
-----

Mount a file system partition to a given mount point

.. code-block:: console

  fs mount (littlefs|fat) <path>

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

For the :ref:`native sim board <native_sim>` the flash partitions can be accessed from the host
Linux system.

By default the flash partitions are accessible through the directory *flash*
relative to the directory where the build is started.
