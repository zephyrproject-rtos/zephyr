.. _hello_rust_world:

Hello Rust World
################

Overview
********

A simple :ref:`Rust <language_rust>` sample that can be used with many
supported boards and prints a Hello World message to the console.

Building and Running
********************

Once the proper rust toolchain has been installed.  This configuration
can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/rust/hello_world
   :host-os: unix
   :board: qemu_riscv32
   :goals: run
   :compact:

To build for another board, change "qemu_riscv32" above to that
board's name.

Sample Output
=============

.. code-block: console

   Hello world from Rust on qemu_riscv32

Exit QEMU by pressing :kbd:`CTRL+C`
