.. _networking_api:

Networking API
##############

.. contents::
   :depth: 2
   :local:
   :backlinks: top

This is the full set of networking public APIs. Their exposure
depends on relevant Kconfig options. For instance IPv6 related
APIs will not be present if :option:`CONFIG_NET_IPV6` has not
been selected.

.. comment
   not documenting
   .. doxygengroup:: networking
   .. doxygengroup:: arp

Network core helpers
********************

.. doxygengroup:: net_core
   :project: Zephyr

Network buffers
***************

.. doxygengroup:: net_buf
   :project: Zephyr

Network packet management
*************************

.. doxygengroup:: net_pkt
   :project: Zephyr

IPv4/IPv6 primitives and helpers
********************************

.. doxygengroup:: ip_4_6
   :project: Zephyr

Network interface
*****************

.. doxygengroup:: net_if
   :project: Zephyr

Network Management
******************

.. doxygengroup:: net_mgmt
   :project: Zephyr

Network layer 2 management
**************************

.. doxygengroup:: net_l2
   :project: Zephyr

Network link address
********************

.. doxygengroup:: net_linkaddr
   :project: Zephyr

Application network context
***************************

.. doxygengroup:: net_context
   :project: Zephyr

BSD Sockets compatible API
**************************

.. doxygengroup:: bsd_sockets
   :project: Zephyr

Network offloading support
**************************

.. doxygengroup:: net_offload
   :project: Zephyr

Network statistics
******************

.. doxygengroup:: net_stats
   :project: Zephyr

Trickle timer support
*********************

.. doxygengroup:: trickle
   :project: Zephyr

UDP
***

.. doxygengroup:: udp
   :project: Zephyr

Hostname Configuration Library
******************************

.. doxygengroup:: net_hostname
   :project: Zephyr

generic Precision Time Protocol (gPTP)
**************************************

.. doxygengroup:: gptp
   :project: Zephyr

Network technologies
********************

Ethernet
========

.. doxygengroup:: ethernet
   :project: Zephyr

Ethernet Management
===================

.. doxygengroup:: ethernet_mgmt
   :project: Zephyr

Virtual LAN definitions and helpers
===================================

.. doxygengroup:: vlan
   :project: Zephyr

Link Layer Discovery Protocol definitions and helpers
=====================================================

.. doxygengroup:: lldp
   :project: Zephyr

IEEE 802.15.4
=============

.. doxygengroup:: ieee802154
   :project: Zephyr

IEEE 802.15.4 Management
========================

.. doxygengroup:: ieee802154_mgmt
   :project: Zephyr

Network and application libraries
*********************************

Network application
===================

.. doxygengroup:: net_app
   :project: Zephyr

DHCPv4
======

.. doxygengroup:: dhcpv4
   :project: Zephyr

MQTT 3.1.1
==========

.. doxygengroup:: mqtt
   :project: Zephyr

CoAP
====

.. doxygengroup:: coap
   :project: Zephyr

DNS Resolve
===========

.. doxygengroup:: dns_resolve
   :project: Zephyr

HTTP
====

.. doxygengroup:: http
   :project: Zephyr

Websocket
=========

.. doxygengroup:: websocket
   :project: Zephyr

Websocket console
=================

.. doxygengroup:: websocket_console
   :project: Zephyr
