.. _apps_run:

Run an Application
##################

The kernel's built-in simulator is QEMU. It creates an environment
where you can run and test an application virtually, before (or
in lieu of) loading and running it on actual target hardware.

Procedures
**********

.. _run_app_qemu:

Running an Application using QEMU
=================================

Run your application in QEMU for testing and demonstration purposes.

Prerequisites
-------------

* You must have already generated a .elf image file for a
  QEMU-supported platform configuration, such as
  basic_cortex_m3 or basic_minuteia.

* The environment variable must be set for each console
  terminal using :ref:`apps_common_procedures`.

Steps
-----

1. Open a terminal console and navigate to the application directory
   :file:`~/appDir`.

2. Enter the following command to build and run an application
   in QEMU:

  .. code-block:: console

   $ make qemu


  The application begins running in the terminal console.

3. Press :kbd:`Ctrl A, X` to stop the application from running
   in QEMU.

  The application stops running and the terminal console prompt
  redisplays.

.. _loading_on_target:

Loading and Running an Application on Target Hardware
=====================================================

An application image is loaded on the target based on functionality
available on the hardware device. Loading instructions are often
unique to the particular target board. For this reason, loading
instructions reside with the platform-specific documentation.

For more information see :ref:`platform`.
