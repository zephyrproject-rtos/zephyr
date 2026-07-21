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

Zephyr supports both DHCPv6 client and server functionality, including IPv6
prefix delegation (IA_PD) as specified in
`RFC 8415 <https://www.rfc-editor.org/rfc/rfc8415>`_.

Prefix delegation
*****************

The DHCPv6 client can act as a *requesting router*: in addition to requesting a
non-temporary address (IA_NA), it can request a delegated prefix (IA_PD) by
setting :c:member:`net_dhcpv6_params.request_prefix`. The delegated prefix is
installed on the requesting interface.

To turn the node into a full requesting router that sub-delegates the prefix to
one or more downstream links, list the downstream (LAN) interface indices in
:c:member:`net_dhcpv6_params.downstream_ifaces` and set
:c:member:`net_dhcpv6_params.downstream_count` to the number of entries. Each
downstream interface is assigned a distinct ``/64`` carved from the delegated
prefix (the Nth interface gets the Nth ``/64``), which is then advertised on
that interface via Router Advertisements, allowing downstream hosts to
auto-configure addresses using SLAAC. The array holds at most
:kconfig:option:`CONFIG_NET_DHCPV6_MAX_DOWNSTREAM` entries; a
:c:member:`net_dhcpv6_params.downstream_count` of 0 means the delegated prefix
is only installed on the requesting interface. This requires
:kconfig:option:`CONFIG_NET_IPV6_RA` (Router Advertisement transmit / router
role) and, for traffic to be forwarded between the upstream and downstream
interfaces, :kconfig:option:`CONFIG_NET_IPV6_FORWARDING`.

The DHCPv6 server (:kconfig:option:`CONFIG_NET_DHCPV6_SERVER`) implements the
*delegating router* role, handing out addresses and delegated prefixes from
configured pools. See :zephyr:code-sample:`dhcpv6-pd` for a complete example of
both roles.

API Reference
*************

.. doxygengroup:: dhcpv6

.. doxygengroup:: dhcpv6_server
