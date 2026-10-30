.. zephyr:code-sample:: workq
   :name: Work Queue Sample

   Shows how to use Zephyr work queues to offload work to the workq structure.

Overview
********

A sample demonstrating the use of workqueues in Zephyr.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/workq
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board target, replace "qemu_x86" above with it.

Sample Output
=============

.. code-block:: console


Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
