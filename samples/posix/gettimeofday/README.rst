.. _posix-gettimeofday-sample:

POSIX gettimeofday() with clock initialization over SNTP
########################################################

Overview
********

This sample application demonstrates using the POSIX `gettimeofday()`_ function to display the
absolute wall clock time and local time every second. At system startup, the current time is
queried using the SNTP networking protocol, enabled by setting the
:kconfig:option:`CONFIG_NET_CONFIG_CLOCK_SNTP_INIT` and
:kconfig:option:`CONFIG_NET_CONFIG_SNTP_INIT_SERVER` options.

Requirements
************

- :ref:`networking_with_host`
- or, a board with hardware networking
- NAT/routing should be set up to allow connections to the Internet
- DNS server should be available on the host to resolve domain names

Building and Running
********************

This project outputs to the console. It can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/posix/gettimeofday
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

For comparison, to build directly for your host OS if it is POSIX compliant (for ex. Linux):

.. code-block:: console

   cd samples/posix/gettimeofday
   make -f Makefile.host

The make output file will be located in samples/posix/gettimeofday/build.

.. _gettimeofday(): https://pubs.opengroup.org/onlinepubs/009604599/functions/gettimeofday.html
