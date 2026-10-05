.. _dhcpv6_interface:

DHCPv6
######

.. contents::
    :local:
    :depth: 2

Overview
********

The Dynamic Host Configuration Protocol (DHCP) for IPv6 is a network management protocol
used on IPv6 based networks. A DHCPv6 server dynamically assigns an IPv6 address
and other network configuration parameters to each device on a network so they
can communicate with other IP networks.
See this
`DHCPv6 Wikipedia article <https://en.wikipedia.org/wiki/DHCPv6>`_
for a detailed overview of how DHCPv6 works.

Note that Zephyr only supports DHCPv6 client functionality.

Starting the client from Router Advertisements
**********************************************

By default the application starts the client with :c:func:`net_dhcpv6_start`.
When :kconfig:option:`CONFIG_NET_DHCPV6_START_ON_RA` is enabled, the client starts
automatically, based on the M and O flags of a received Router Advertisement
(:rfc:`4861#section-4.2`):

* M flag set: stateful mode. The client requests an address.
* M flag clear, O flag set: stateless mode. The client only requests
  configuration information, for example DNS servers.
* Both flags clear: the client is not started.

If a later Router Advertisement sets the M flag, a stateless client is
restarted in stateful mode. If the M flag is cleared, a stateful client keeps
its leases until they expire, and then follows the flags of the latest Router
Advertisement.

An explicit call to :c:func:`net_dhcpv6_start` replaces the automatic client.
A call to :c:func:`net_dhcpv6_stop` disables the automatic start on that
interface.

API Reference
*************

.. doxygengroup:: dhcpv6
