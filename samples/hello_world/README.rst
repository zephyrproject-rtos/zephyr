.. _hello_world:

Hello World
###########

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
prints "hello, world" to the console.

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

Sample Output
=============

.. code-block:: console

    hello, world

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
