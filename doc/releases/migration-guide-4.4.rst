:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.4:

Migration guide to Zephyr v4.4.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.3.0 to
Zephyr v4.4.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.4>`.

.. contents::
    :local:
    :depth: 2

Build System
************

Kernel
******

Boards
******

Device Drivers and Devicetree
*****************************

.. zephyr-keep-sorted-start re(^\w)

SD Host Controller
==================

* Moved extra fields ``bus_4_bit_support``, ``hs200_support`` and ``hs400_support`` from
  :c:struct:`sdhc_host_caps` to :c:struct:`sdhc_host_props` as per the
  `SD Host Controller Specification <https://www.sdcard.org/downloads/pls/pdf/?p=PartA2_SD%20Host_Controller_Simplified_Specification_Ver4.20.jpg>`_.
  (:github:`91701`)

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
