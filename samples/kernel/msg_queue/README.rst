.. zephyr:code-sample:: msg_queue
   :name: Message Queue

   Implements a basic message queue producer/consumer thread pair.

Overview
********

A simple sample to demonstrate the basic usage of Zephyr message queues.
a producer thread sends both normal and urgent messages.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/msg_queue
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

Every normal message sending implies sending the message to the end of the
queue, and the first message to go is the first to be delivered. Every "urgent"
message, at its turn, implies sending the message to the beginning of the queue.

In this sample, one producer thread sends 1 urgent message for each 2 regular
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
