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

DAI
===

* Renamed the devicetree property ``dai_id`` to ``dai-id``.
* Renamed the devicetree property ``afe_name`` to ``afe-name``.
* Renamed the devicetree property ``agent_disable`` to ``agent-disable``.
* Renamed the devicetree property ``ch_num`` to ``ch-num``.
* Renamed the devicetree property ``mono_invert`` to ``mono-invert``.
* Renamed the devicetree property ``quad_ch`` to ``quad-ch``.
* Renamed the devicetree property ``int_odd`` to ``int-odd``.

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
