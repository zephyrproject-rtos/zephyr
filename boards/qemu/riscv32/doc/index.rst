.. _qemu_riscv32:

RISCV32 Emulation (QEMU)
########################

Overview
********

The RISCV32 QEMU board configuration is used to emulate the RISCV32 architecture.

Programming and Debugging
*************************

Applications for the ``qemu_riscv32`` board configuration can be built and run in
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
   :board: qemu_riscv32
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 27 2017 13:09:26 *****
        threadA: Hello World from riscv32!
        threadB: Hello World from riscv32!
        threadA: Hello World from riscv32!
        threadB: Hello World from riscv32!
        threadA: Hello World from riscv32!
        threadB: Hello World from riscv32!
        threadA: Hello World from riscv32!
        threadB: Hello World from riscv32!
        threadA: Hello World from riscv32!
        threadB: Hello World from riscv32!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
