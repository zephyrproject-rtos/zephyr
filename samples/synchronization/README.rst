.. _synchronization_sample:

Synchronization Sample
######################

Overview
********

A simple application that demonstates basic sanity of the kernel.
Two threads (A and B) take turns printing a greeting message to the console,
and use sleep requests and semaphores to control the rate at which messages
are generated. This demonstrates that kernel scheduling, communication,
and timing are operating correctly.

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. code-block:: console

   $ cd samples/synchronization
   $ make run

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
