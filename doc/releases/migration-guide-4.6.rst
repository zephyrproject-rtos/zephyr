:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.6:

Migration guide to Zephyr v4.6.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.5.0 to
Zephyr v4.6.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.6>`.

.. contents::
    :local:
    :depth: 2

Common
******

Build System
************

Kernel
******

Boards
******

Device Drivers and Devicetree
*****************************

.. Only place contents common to all device drivers here. Contents specific to one driver subsystem
   goes into its own subsection, below.

.. Group contents in this section by subsystem, e.g.:
..
.. ADC
.. ===
..
.. ...

.. zephyr-keep-sorted-start re(^\w) ignorecase

.. zephyr-keep-sorted-stop

Bluetooth
*********

Networking
**********

Ethernet
========

* Checksum offload capabilities are now advertised with one bit per IP version and protocol.
  ``NET_IF_CHECKSUM_IPV6_HEADER``, ``ETHERNET_CHECKSUM_SUPPORT_IPV6_HEADER``,
  ``ETHERNET_CHECKSUM_SUPPORT_TCP`` and ``ETHERNET_CHECKSUM_SUPPORT_UDP`` have been removed.
  Drivers reporting ``ETHERNET_CONFIG_TYPE_RX_CHECKSUM_SUPPORT`` or
  ``ETHERNET_CONFIG_TYPE_TX_CHECKSUM_SUPPORT`` must set
  :c:enumerator:`ETHERNET_CHECKSUM_SUPPORT_IPV4_TCP`,
  :c:enumerator:`ETHERNET_CHECKSUM_SUPPORT_IPV4_UDP`,
  :c:enumerator:`ETHERNET_CHECKSUM_SUPPORT_IPV6_TCP` and
  :c:enumerator:`ETHERNET_CHECKSUM_SUPPORT_IPV6_UDP` for the combinations they offload. A driver
  that set the IPv6 header bit together with the TCP and UDP bits now sets both the IPv4 and the
  IPv6 TCP and UDP enumerators; a driver that set only the IPv4 header bit together with the TCP
  and UDP bits now sets the IPv4 TCP and UDP enumerators.

Other subsystems
****************

Modules
*******

Architectures
*************
