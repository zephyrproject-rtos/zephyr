.. _posix-gettimeofday-sample:

POSIX gettimeofday() with clock initialization over SNTP
########################################################

Overview
********

This sample application demonstrates using the POSIX gettimeofday()
function to display the absolute wall clock time and local time every
second. At system startup, the current time is queried using the SNTP
networking protocol, enabled by setting the
:option:`CONFIG_NET_CONFIG_CLOCK_SNTP_INIT` and
:option:`CONFIG_NET_CONFIG_SNTP_INIT_SERVER` options.

Requirements
************

- :ref:`networking_with_host`
- or, a board with hardware networking
- NAT/routing should be set up to allow connections to the Internet
- DNS server should be available on the host to resolve domain names

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/posix/gettimeofday
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

For comparison, a version for native POSIX operating systems (e.g. Linux)
can be built using:

.. code-block:: console

   make -f Makefile.posix
