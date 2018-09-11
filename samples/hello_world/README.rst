.. _hello_world:

Hello World
###########

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
prints "Hello World" to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

To build the semihosting version (ARM boards only), use the supplied
configuration file for semihosting: :file:`prj_semihosting.conf`:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: stm32f072b_disco
   :conf: prj_semihosting.conf
   :goals: debug
   :compact:

To enable semihosting output, run the following in your GDB session:

.. code-block:: console

    (gdb) monitor arm semihosting enable
    semihosting is enabled

Sample Output
=============

.. code-block:: console

    Hello World! x86

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
