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

Network core helpers
********************

.. doxygengroup:: net_core
   :project: Zephyr
   :content-only:

Network buffers
***************

.. doxygengroup:: net_buf
   :project: Zephyr
   :content-only:

Network packet management
*************************

.. doxygengroup:: net_pkt
   :project: Zephyr
   :content-only:

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

Network layer 2 management
**************************

.. doxygengroup:: net_l2
   :project: Zephyr
   :content-only:

Network link address
********************

.. doxygengroup:: net_linkaddr
   :project: Zephyr
   :content-only:

Application network context
***************************

.. doxygengroup:: net_context
   :project: Zephyr
   :content-only:

BSD Sockets compatible API
**************************

.. doxygengroup:: bsd_sockets
   :project: Zephyr
   :content-only:

Network offloading support
**************************

.. doxygengroup:: net_offload
   :project: Zephyr
   :content-only:

Network statistics
******************

.. doxygengroup:: net_stats
   :project: Zephyr
   :content-only:

Trickle timer support
*********************

.. doxygengroup:: trickle
   :project: Zephyr
   :content-only:

UDP
***

.. doxygengroup:: udp
   :project: Zephyr
   :content-only:

Network technologies
********************

Ethernet
========

.. doxygengroup:: ethernet
   :project: Zephyr
   :content-only:

IEEE 802.15.4
=============

.. doxygengroup:: ieee802154
   :project: Zephyr
   :content-only:

Network and application libraries
*********************************

Network application
===================

.. doxygengroup:: net_app
   :project: Zephyr
   :content-only:

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

CoAP
====

.. doxygengroup:: zoap
   :project: Zephyr
   :content-only:

DNS Resolve
===========

.. doxygengroup:: dns_resolve
   :project: Zephyr
   :content-only:

HTTP
====

.. doxygengroup:: http
   :project: Zephyr
   :content-only:
