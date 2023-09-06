.. zephyr:code-sample:: gdb-debug
   :name: GDB debug

   Use GDB Remote Serial Protocol to debug a Zephyr application running on QEMU.

Overview
********

A simple sample that can be used with QEMU to show debug using GDB
Remote Serial Protocol (RSP) capabilities.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/debug/gdbstub
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Open a new terminal and use gdb to connect to the running qemu as follows:

.. code-block:: bash

    gdb build/zephyr/zephyr.elf
    (gdb) target remote :5678

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
