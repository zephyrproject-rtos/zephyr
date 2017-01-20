.. _hello_world:

Hello World
###########

Overview
********
A simple Hello World example that can be used with any supported board and
prints 'Hello World' to the console. This application can be built into modes:

* single thread
* multi threading

Building and Running
********************

This project outputs 'Hello World' to the console.  It can be built and executed
on QEMU as follows:

.. code-block:: console

   $ cd samples/hello_world
   $ make run


To build the single thread version, use the supplied configuration file for
single thread: :file:`prj_single.conf`:

.. code-block:: console

   $ make CONF_FILE=prj_single.conf run

Sample Output
=============

.. code-block:: console

    Hello World! x86
