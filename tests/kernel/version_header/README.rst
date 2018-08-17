.. _version_header:

Version Header
##############

Overview
********
This program tests that the version header is added to the elf binary
when the :option:`CONFIG_VERSION_HEADER`  option is enabled:

vesion header members can be modified with script add_binary_version_header.py.
which can be found under script folder. This script can be used to fill
appropriate major, minor and patchlevel information to the version header
members.

Building and Running
********************

This project validates major, minor and patchlevel version.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: kernel/version_header
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

    KERNEL VERSION is ${KERNELVERSION}
