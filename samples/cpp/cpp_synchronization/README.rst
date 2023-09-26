.. _cpp_synchronization:

C++ Synchronization
###################

Overview
********
The sample project illustrates usage of pure virtual class, member
functions with different types of arguments, global objects constructor
invocation.

A simple application demonstrates basic sanity of the kernel.  The main thread
and a cooperative thread take turns printing a greeting message to the console,
and use timers and semaphores to control the rate at which messages are
generated. This demonstrates that kernel scheduling, communication, and
timing are operating correctly.

Building and Running
********************

This kernel project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cpp/cpp_synchronization
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

     Create semaphore 0x001042b0
     Create semaphore 0x001042c4
     main: Hello World!
     coop_thread_entry: Hello World!
     main: Hello World!
     coop_thread_entry: Hello World!
     main: Hello World!
     coop_thread_entry: Hello World!
     main: Hello World!
     coop_thread_entry: Hello World!
     main: Hello World!
     coop_thread_entry: Hello World!
     main: Hello World!
     coop_thread_entry: Hello World!
     main: Hello World!
     coop_thread_entry: Hello World!
     main: Hello World!
     coop_thread_entry: Hello World!
     main: Hello World!
     coop_thread_entry: Hello World!
     main: Hello World!

     <repeats endlessly>

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
