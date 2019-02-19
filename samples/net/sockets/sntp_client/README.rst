.. _sntp-client-sample:

SNTP client sample
##################

Overview
********

This sample is a simple SNTP client showing how to retrieve the current
time in seconds since 1st January 1970.

This demo assumes that the platform of choice has networking support,
some adjustments to the configuration may be needed. It also assumes
SNTP server is running on the host.

Building and Running
********************

When the application runs, it issues an SNTP request to the host and waits
for a response. When the response is received, the current epoch time, in
seconds, as well as the status code of the response (0 on success), is
printed.

See the `net-tools`_ project for more details.

This sample can be built and executed on QEMU or native_posix board as
described in :ref:`networking_with_qemu`.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
