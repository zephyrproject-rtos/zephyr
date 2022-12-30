.. _coop_synchronization_sample:

Cooperative Synchronization Sample
##################################

Overview
********

A simple application that demonstrates even more basic sanity of the kernel.
Two threads (A and B) take turns printing a greeting message to the console.
They use yields and semaphores to control the whose turn it is to print.
The rate is only controlled via busy waits. This demonstrates kernel communication only.
This is a good example to test new platforms that may not have a working clock.
I.e. this will still work when :kconfig:option:`CONFIG_SYS_CLOCK_EXISTS` =n
It's also a very simplistic demo allowing users to follow the execution trace in
a no-timer (i.e. nonexistent, disabled, deferred) scenario and experience predictability.
To that end, sample should ideally be built with no interrupt-driven-UART to avoid spurious
IRQ fires which may trigger unpredictable context switch. However, some platforms do not
support polling api for UART. For those, only the regular variant is built.

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/coop_synchronization
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

   thread_a: Hello World from cpu 0 on qemu_x86! counter = 1
   thread_b: Hello World from cpu 0 on qemu_x86! counter = -1
   thread_a: Hello World from cpu 0 on qemu_x86! counter = 2
   thread_b: Hello World from cpu 0 on qemu_x86! counter = -2
   thread_a: Hello World from cpu 0 on qemu_x86! counter = 3
   thread_b: Hello World from cpu 0 on qemu_x86! counter = -3
   thread_a: Hello World from cpu 0 on qemu_x86! counter = 4
   thread_b: Hello World from cpu 0 on qemu_x86! counter = -4
   thread_a: Hello World from cpu 0 on qemu_x86! counter = 5
   thread_b: Hello World from cpu 0 on qemu_x86! counter = -5

   <repeats endlessly>

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
