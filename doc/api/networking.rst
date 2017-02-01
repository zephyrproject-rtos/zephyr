.. _networking_api:

Networking API
##############

.. contents::
   :depth: 1
   :local:
   :backlinks: top

This is the full set of networking public APIs. Their exposure
depends on relevant Kconfig options. For instance IPv6 related
APIs will not be present if :option:`CONFIG_NET_IPV6` has not
been selected.

IPv4/IPv6 primitives and helpers
********************************

.. doxygengroup:: ip_4_6
   :project: Zephyr
   :content-only:

Network interface
*****************

.. doxygengroup:: net_if
   :project: Zephyr
   :content-only:

Network Management
******************

.. doxygengroup:: net_mgmt
   :project: Zephyr
   :content-only:

Application network context
***************************

.. doxygengroup:: net_context
   :project: Zephyr
   :content-only:

Network and application libraries
*********************************

DHCPv4
======

.. doxygengroup:: dhcpv4
   :project: Zephyr
   :content-only:

MQTT 3.1.1
==========

.. doxygengroup:: mqtt
   :project: Zephyr
   :content-only:

DNS Client
==========

.. doxygengroup:: dns_client
   :project: Zephyr
   :content-only:
