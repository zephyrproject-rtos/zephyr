.. _cpp_exceptions:

C++ Exceptions
##############

Overview
********
This is a buildable and runnable example of using C++ exceptions in zephyr apps.

Building and Running
********************

This kernel project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cpp_exceptions
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

   Exception caught: foobar
