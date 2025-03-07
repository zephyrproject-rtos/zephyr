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

Bluetooth
*********

Bluetooth Audio
===============

* ``CONFIG_BT_CSIP_SET_MEMBER_NOTIFIABLE`` has been renamed to
  :kconfig:option:`CONFIG_BT_CSIP_SET_MEMBER_SIRK_NOTIFIABLE``. (:github:`86763``)

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
