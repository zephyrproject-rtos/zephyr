.. _qemu_riscv32e:

RISCV32E Emulation (QEMU)
#########################

Overview
********

The RISCV32E QEMU board configuration is used to emulate the RISCV32 (RV32E) architecture.

Programming and Debugging
*************************

Applications for the ``qemu_riscv32e`` board configuration can be built and run in
the usual way for emulated boards (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment. For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_riscv32e
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        *** Booting Zephyr OS build v3.1.0-rc1-59-g0d66cc1f6645  ***
        thread_a: Hello World from cpu 0 on qemu_riscv32e!
        thread_b: Hello World from cpu 0 on qemu_riscv32e!
        thread_a: Hello World from cpu 0 on qemu_riscv32e!
        thread_b: Hello World from cpu 0 on qemu_riscv32e!
        thread_a: Hello World from cpu 0 on qemu_riscv32e!
        thread_b: Hello World from cpu 0 on qemu_riscv32e!
        thread_a: Hello World from cpu 0 on qemu_riscv32e!
        thread_b: Hello World from cpu 0 on qemu_riscv32e!
        thread_a: Hello World from cpu 0 on qemu_riscv32e!
        thread_b: Hello World from cpu 0 on qemu_riscv32e!
        thread_a: Hello World from cpu 0 on qemu_riscv32e!
        thread_b: Hello World from cpu 0 on qemu_riscv32e!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
