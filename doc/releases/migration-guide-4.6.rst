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

* For ``AF_UNSPEC`` lookups, :c:func:`zsock_getaddrinfo` now returns IPv6 addresses before IPv4
  ones, unless only the IPv4 addresses are reachable, and queries AAAA records before A records.
  Applications that use the first result now connect over IPv6 where they used IPv4. Enable
  :kconfig:option:`CONFIG_NET_SOCKETS_DNS_PREFER_IPV4` to keep IPv4 first, for example with
  offloaded network interfaces, where reachability cannot be determined.

Other subsystems
****************

Modules
*******

Architectures
*************
