.. zephyr:code-sample:: synchronization
   :name: Basic Synchronization
   :relevant-api: thread_apis semaphore_apis

   Manipulate basic kernel synchronization primitives.

Overview
********

A simple application that demonstrates basic sanity of the kernel.
Two threads (A and B) take turns printing a greeting message to the console,
and use sleep requests and semaphores to control the rate at which messages
are generated. This demonstrates that kernel scheduling, communication,
and timing are operating correctly.

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

   threadA: Hello World!
   threadB: Hello World!
   threadA: Hello World!
   threadB: Hello World!
   threadA: Hello World!
   threadB: Hello World!
   threadA: Hello World!
   threadB: Hello World!
   threadA: Hello World!
   threadB: Hello World!

   <repeats endlessly>

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
