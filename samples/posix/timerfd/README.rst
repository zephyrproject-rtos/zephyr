.. zephyr:code-sample:: posix-timerfd
   :name: timerfd_create()

   Use ``timerfd_create()`` to create a file descriptor for timer notification.

Overview
********

This sample application demonstrates using the POSIX timerfd_create() functions to create a file
descriptor, which can be used as a timer. The returned file descriptor is used with read calls
and the timeout count are printed to the console.

Building and Running
********************

This project outputs to the console. It can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/posix/timerfd
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

For comparison, to build directly for your host OS if it is POSIX compliant (for ex. Linux):

.. code-block:: console

   cd samples/posix/timerfd
   make -f Makefile.host

The make output file will be located in samples/posix/timerfd/build.

Sample Output
=============

.. code-block:: console

    Timer expired 1 times
    Timer expired 3 times
    Timer closed
