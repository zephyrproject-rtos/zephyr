.. zephyr:code-sample:: helloworld_user
   :name: Hello World

   Print a simple "Hello World" from userspace.

Overview
********
A simple Hello World example that can be used with any supported board and
prints 'Hello World from UserSpace!' to the console.
If unavailable or unconfigured then 'Hello World from privileged mode.'
is printed instead.

This application can be built into modes:

* single thread
* multi threading

Building and Running
********************

This project outputs 'Hello World from UserSpace!' to the console.
It can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/userspace/hello_world_user
   :host-os: unix
   :board: qemu_riscv32
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

    Hello World from UserSpace! qemu_riscv32

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
