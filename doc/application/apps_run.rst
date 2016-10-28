.. _apps_run:

Run an Application
##################

An appliction image can be run on real or simulated hardware.
The kernel's built-in simulator is QEMU. It allows you to run and test
an application virtually, before (or in lieu of)
loading and running it on actual target hardware.

.. contents:: Procedures
   :local:
   :depth: 1

Running an Application using QEMU
=================================

Prerequisites
-------------

* Ensure the application can be built successfully using a QEMU-supported
  board configuration, such as qemu_cortex_m3 or qemu_x86, as described
  in :ref:`apps_build`.

* Ensure the Zephyr environment variables are set for each console terminal;
  see :ref:`apps_common_procedures`.

Steps
-----

#. Open a terminal console and navigate to the application directory
   :file:`~/appDir`.

#. Enter the following command to build and run the application
   using a QEMU-supported board configuration,
   such as qemu_cortex_m3 or qemu_x86.

   .. code-block:: console

       $ make [BOARD=<type> ...] qemu

   The Zephyr build system generates a :file:`zephyr.elf` image file
   and then begins running it in the terminal console.

#. Press :kbd:`Ctrl A, X` to stop the application from running
   in QEMU.

   The application stops running and the terminal console prompt
   redisplays.

Loading and Running an Application on Target Hardware
=====================================================

The procedure required to load an application image (:file:`zephyr.elf`)
on a target depends on the functionality available on its hardware,
and is often unique to that target board.
For this reason, loading instructions reside with the board-specific
documentation; see :ref:`board`.
