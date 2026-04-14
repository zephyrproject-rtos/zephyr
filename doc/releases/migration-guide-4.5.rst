:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.5:

Migration guide to Zephyr v4.5.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.4.0 to
Zephyr v4.5.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.5>`.

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

.. Group contents in this section by subsystem, e.g.:
..
.. ADC
.. ===
..
.. ...

.. zephyr-keep-sorted-start re(^\w) ignorecase

USB
===

* The ``remote-mac-address`` devicetree property on ``zephyr,cdc-ecm-ethernet``
  and ``zephyr,cdc-ncm-ethernet`` nodes is deprecated. New designs should omit
  the property; the host-side MAC advertised via the ``iMACAddress`` string
  descriptor is then derived at runtime from the local MAC by flipping the U/L
  bit, and tracks subsequent runtime changes to the local MAC (for example via
  :c:macro:`NET_REQUEST_ETHERNET_SET_MAC_ADDRESS`). Overlays that continue to
  set the property keep the previous build-time-baked behaviour for backward
  compatibility but emit a devicetree deprecation warning.

.. zephyr-keep-sorted-stop

Bluetooth
*********


Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
