.. zephyr:board:: qemu_riscv

Overview
********

The RISCV QEMU board configuration is used to emulate the RISCV architecture.

Get the Toolchain and QEMU
**************************

The minimum version of the `Zephyr SDK tools
<https://github.com/zephyrproject-rtos/sdk-ng/releases>`_
with toolchain and QEMU support for the RISCV architecture is v0.10.2.
Please see the :ref:`installation instructions <install-required-tools>`
for more details.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``qemu_riscv`` board configuration can be built and run in
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
   :board: qemu_riscv//rv32
   :goals: run


.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_riscv//rv32e
   :goals: run


.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_riscv//rv64
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 27 2017 13:09:26 *****
        threadA: Hello World from riscv!
        threadB: Hello World from riscv!
        threadA: Hello World from riscv!
        threadB: Hello World from riscv!
        threadA: Hello World from riscv!
        threadB: Hello World from riscv!
        threadA: Hello World from riscv!
        threadB: Hello World from riscv!
        threadA: Hello World from riscv!
        threadB: Hello World from riscv!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
