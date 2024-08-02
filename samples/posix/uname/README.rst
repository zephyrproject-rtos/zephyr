.. _posix-uname-sample:

POSIX uname()
#############

Overview
********

In this sample application, the POSIX `uname()`_ function is used to acquire system information and
it is output to the console. Additionally, uname is added as a shell command and system information
is displayed according to the option(s) provided for the command.

Building and Running
********************

This project outputs to the console. It can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/posix/uname
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

For comparison, to build directly for your host OS if it is POSIX compliant (for ex. Linux):

.. code-block:: console

   cd samples/posix/uname
   make -f Makefile.host

The make output file will be located in samples/posix/uname/build.

Sample Output
=============

.. code-block:: console

    Printing everything in utsname...
    sysname[7]: Zephyr
    nodename[7]: zephyr
    release[13]: 3.5.99
    version[61]: zephyr-v3.5.0-3515-g10156f5f1d9c Jan  9 2024 22:23:04
    machine[4]: x86


    uart:~$ uname -a
    Zephyr zephyr 3.5.99 zephyr-v3.5.0-3515-g10156f5f1d9c Jan  9 2024 22:23:04 x86 qemu_x86
    uart:~$ uname -smi
    Zephyr x86 qemu_x86

.. _uname(): https://pubs.opengroup.org/onlinepubs/9699919799/functions/uname.html
