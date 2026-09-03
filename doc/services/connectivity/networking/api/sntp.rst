.. _sntp_interface:

Simple Network Time Protocol Library
####################################

.. contents::
    :local:
    :depth: 2

Overview
********

The SNTP library implements :rfc:`4330`.

SNTP provides a way to synchronize clocks in computer networks.

The client (:kconfig:option:`CONFIG_SNTP`) queries the time from an SNTP
server. The server (:kconfig:option:`CONFIG_SNTP_SERVER`) answers such queries
on UDP port 123, on every enabled address family. The application is
responsible for setting the system clock, and tells the server where its time
comes from with :c:func:`sntp_server_clock_source`. Until it does so, the
server answers with the leap indicator set to "clock not synchronized" and
stratum 16, so that clients discard its timestamps.

API Reference
*************

.. doxygengroup:: sntp

.. doxygengroup:: sntp_server
