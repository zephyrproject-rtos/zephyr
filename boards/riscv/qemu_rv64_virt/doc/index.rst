.. _qemu_rv64_virt:

RISCV64 virtpc Emulation (QEMU)
###############################

Overview
********

The RISCV64 QEMU board configuration is used to emulate the RISCV64 architecture.
This configuration can emulate SMP system.


Programming and Debugging
*************************

Applications for the ``qemu_rv64_virt`` board configuration can be built and run in
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
   :board: qemu_rv64_virt
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v2.4.0-1715-ga0699475cc3f  ***
        thread_a: Hello World from cpu 0 on QEMU RV64 virt board!
        thread_b: Hello World from cpu 0 on QEMU RV64 virt board!
        thread_a: Hello World from cpu 0 on QEMU RV64 virt board!
        thread_b: Hello World from cpu 0 on QEMU RV64 virt board!
        thread_a: Hello World from cpu 0 on QEMU RV64 virt board!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
