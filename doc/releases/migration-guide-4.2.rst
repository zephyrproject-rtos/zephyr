:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.2:

Migration guide to Zephyr v4.2.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.1.0 to
Zephyr v4.2.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.2>`.

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


Enhanced Serial Peripheral Interface (eSPI)
===========================================

* Renamed the devicetree property ``io_girq`` to ``io-girq``.
* Renamed the devicetree property ``vw_girqs`` to ``vw-girqs``.
* Renamed the devicetree property ``pc_girq`` to ``pc-girq``.
* Renamed the devicetree property ``poll_timeout`` to ``poll-timeout``.
* Renamed the devicetree property ``poll_interval`` to ``poll-interval``.
* Renamed the devicetree property ``consec_rd_timeout`` to ``consec-rd-timeout``.
* Renamed the devicetree property ``sus_chk_delay`` to ``sus-chk-delay``.
* Renamed the devicetree property ``sus_rsm_interval`` to ``sus-rsm-interval``.

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
