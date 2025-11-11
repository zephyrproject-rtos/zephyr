.. zephyr:code-sample:: posix-eventfd
   :name: eventfd()

   Use ``eventfd()`` to create a file descriptor for event notification.

Overview
********

This sample application demonstrates using the POSIX eventfd() function to create a file descriptor,
which can be used for event notification. The returned file descriptor is used with write/read calls
and write/read values are output to the console.

Building and Running
********************

This project outputs to the console. It can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/posix/eventfd
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

For comparison, to build directly for your host OS if it is POSIX compliant (for ex. Linux):

.. code-block:: console

   cd samples/posix/eventfd
   make -f Makefile.host

The make output file will be located in samples/posix/eventfd/build.

Sample Output
=============

.. code-block:: console

    Writing 1 to efd
    Writing 2 to efd
    Writing 3 to efd
    Writing 4 to efd
    Completed write loop
    About to read
    Read 10 (0xa) from efd
    Finished
