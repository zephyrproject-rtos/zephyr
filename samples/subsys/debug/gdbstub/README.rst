.. _debug_sample:

Debug Sample
############

Overview
********

A simple sample that can be used with qemu to show debug using gdb
remote serial protocol capabilities.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/debug
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Open a new terminal and use gdb to connect to the running qemu as follows:

.. code-block:: bash

    gdb build/zephyr/zephyr.elf
    (gdb) target remote :1234

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
