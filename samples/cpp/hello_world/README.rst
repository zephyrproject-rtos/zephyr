.. zephyr:code-sample:: hello_cpp_world
   :name: Hello C++ world

   Print "Hello World" to the console in C++.

Overview
********

A simple :ref:`C++ <language_cpp>` sample that can be used with many supported board and prints
"Hello, C++ world!" to the console.

Building and Running
********************

This configuration can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cpp/hello_world
   :host-os: unix
   :board: qemu_riscv//rv32
   :goals: run
   :compact:

To build for another board, change "qemu_riscv//rv32" above to that board's name.

Sample Output
=============

.. code-block:: console

    Hello C++, world! qemu_riscv

Exit QEMU by pressing :kbd:`CTRL+C`
