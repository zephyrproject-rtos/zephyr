.. _synchronization_sample:

Synchronization Sample
######################

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

   threed_a: Hello World from cpu(0) on qemu_x86!
   thread_b: Hello World from cpu(0) on qemu_x86!
   thread_a: Hello World from cpu(0) on qemu_x86!
   thread_b: Hello World from cpu(0) on qemu_x86!
   thread_a: Hello World from cpu(0) on qemu_x86!
   thread_b: Hello World from cpu(0) on qemu_x86!
   thread_a: Hello World from cpu(0) on qemu_x86!
   thread_b: Hello World from cpu(0) on qemu_x86!
   thread_a: Hello World from cpu(0) on qemu_x86!
   thread_b: Hello World from cpu(0) on qemu_x86!

   <repeats endlessly>

If the target board supports SMP, for example qemu_x86_64, each thread will run on a CPU with a different id.

.. code-block:: console

   thread_a: Hello World from cpu(0) on qemu_x86_64!
   thread_b: Hello World from cpu(1) on qemu_x86_64!
   thread_a: Hello World from cpu(0) on qemu_x86_64!
   thread_b: Hello World from cpu(1) on qemu_x86_64!
   thread_a: Hello World from cpu(0) on qemu_x86_64!
   thread_b: Hello World from cpu(1) on qemu_x86_64!
   thread_a: Hello World from cpu(0) on qemu_x86_64!
   thread_b: Hello World from cpu(1) on qemu_x86_64!
   thread_a: Hello World from cpu(0) on qemu_x86_64!
   thread_b: Hello World from cpu(1) on qemu_x86_64!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
