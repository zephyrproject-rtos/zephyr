.. _riscv32-virtual:

RISCV32 Virtual
###############

Overview
********

The RISCV32 Virtual board is a virtual platform made with Renode as an alternative to QEMU.
Contrary to QEMU, the peripherals of this platform can be easily configured by editing the
``riscv32_virtual.repl`` script and the devicetree files accordingly, this allows certain hardware
configurations that only exist in proprietary boards/SoCs to be tested in upstream CI.

Programming and debugging
*************************

Building
========

Applications for the ``riscv32_virtual`` board configuration can be built as usual
(see :ref:`build_an_application`):

.. zephyr-app-commands::
   :board: riscv32_virtual
   :goals: build

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the Renode
emulated environment. For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: riscv32_virtual
   :goals: run

This will build an image with the synchronization sample app, boot it using
Renode, and display the following console output:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v3.5.0-1511-g56f73bde0fb0 ***
        thread_a: Hello World from cpu 0 on riscv32_virtual!
        thread_b: Hello World from cpu 0 on riscv32_virtual!
        thread_a: Hello World from cpu 0 on riscv32_virtual!
        thread_b: Hello World from cpu 0 on riscv32_virtual!

Exit Renode by pressing :kbd:`CTRL+C`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
