.. _qemu_microblaze:

Microblaze Emulation (QEMU)
###########################

Overview
********

The Microblaze QEMU board configuration is used to emulate the Microblaze architecture.
The Microblaze QEMU machine instantiates its peripherals using a Devicetree Blob (DTB)
file located at `boards/microblaze/qemu_microblaze/board-qemu-microblaze-demo.dtb`.
This file has been produced by compiling the QEMU system devicetree inside
`boards/microblaze/qemu_microblaze/hw-dtb`. This directory also includes a Makefile
and a README explaining the compilation process to produce the DTB file.
For the applications to work properly, Zephyr devicetree and QEMU system devicetree must match.

Programming and Debugging
*************************

Applications for the ``qemu_microblaze`` board configuration can be built and run in
the usual way for emulated boards (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment. For example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_microblaze
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        *** Booting Zephyr OS build v3.4.0-rc3-271-g230a121f6740 ***
        thread_a: Hello World from cpu 0 on qemu_microblaze!
        thread_b: Hello World from cpu 0 on qemu_microblaze!
        thread_a: Hello World from cpu 0 on qemu_microblaze!
        thread_b: Hello World from cpu 0 on qemu_microblaze!
        thread_a: Hello World from cpu 0 on qemu_microblaze!
        thread_b: Hello World from cpu 0 on qemu_microblaze!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
