.. zephyr:code-sample:: msg_queue
   :name: Message Queue

   Implements a basic message queue producer/consumer thread pair.

Overview
********

A sample demonstrating the basic usage of Zephyr message queues.
A producer thread sends both normal and urgent messages to be retrieved
by a consumer thread.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/msg_queue
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board target, replace "qemu_x86" above with it.

Sample Output
=============

Every normal message is put at the end of the queue, and they are delivered
in FIFO order. Every "urgent" message is put at the beginning of the queue,
and it is delivered first as long as no other "urgent" message comes in after
it.

In this sample, one producer thread sends 1 urgent message for each 2 normal
ones. Note that message C is the first retrieved because it was the last one
sent as "urgent".

.. code-block:: console

   [producer] sending: 0
   [producer] sending: 1
   [producer] sending: A (urgent)
   [producer] sending: 2
   [producer] sending: 3
   [producer] sending: B (urgent)
   [producer] sending: 4
   [producer] sending: 5
   [producer] sending: C (urgent)
   [consumer] got sequence: CBA012345

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
