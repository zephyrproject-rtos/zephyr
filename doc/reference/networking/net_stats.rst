.. _net_stats_interface:

Network Statistics
##################

.. contents::
    :local:
    :depth: 2

Overview
********

Network statistics are collected if :option:`CONFIG_NET_STATISTICS` is set.
Individual component statistics for IPv4 or IPv6 can be turned off
if those statistics are not needed. See various options in
:zephyr_file:`subsys/net/ip/Kconfig.stats` file for details.

By default, the system collects network statistics per network interface. This
can be controlled by :option:`CONFIG_NET_STATISTICS_PER_INTERFACE` option.

The :option:`CONFIG_NET_STATISTICS_USER_API` option can be set if the
application wants to collect statistics for further processing. The network
management interface API is used for that. See :ref:`net_mgmt_interface` for
details.

The :option:`CONFIG_NET_STATISTICS_ETHERNET` option can be set to collect
generic Ethernet statistics. If the
:option:`CONFIG_NET_STATISTICS_ETHERNET_VENDOR` option is set, then
Ethernet device driver can collect Ethernet device specific statistics.
These statistics can then be transferred to application for processing.

If the :option:`CONFIG_NET_SHELL` option is set, then network shell can
show statistics information with ``net stats`` command.

API Reference
*************

.. doxygengroup:: net_stats
   :project: Zephyr
