.. _dhcpv4_interface:

DHCPv4
######

.. contents::
    :local:
    :depth: 2

Overview
********

The Dynamic Host Configuration Protocol (DHCP) is a network management protocol
used on IPv4 networks. A DHCPv4 server dynamically assigns an IPv4 address
and other network configuration parameters to each device on a network so they
can communicate with other IP networks.
See this
`DHCP Wikipedia article <https://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol>`_
for a detailed overview of how DHCP works.

Note that Zephyr only supports DHCP client functionality.

Sample usage
************

See :zephyr:code-sample:`dhcpv4-client` sample application for details.

API Reference
*************

.. doxygengroup:: dhcpv4
