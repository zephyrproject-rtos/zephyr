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

* The config option :kconfig:option:`CONFIG_NATIVE_POSIX_SLOWDOWN_TO_REAL_TIME` has been deprecated
  in favor of :kconfig:option:`CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME`.

Device Drivers and Devicetree
*****************************

* Removed Kconfig option ``ETH_STM32_HAL_MII`` (:github:`86074`).
  PHY interface type is now selected via the ``phy-connection-type`` property in the device tree.

Bluetooth
*********

Bluetooth Host
==============

* The symbols ``BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_<NUMBER>`` in
  :zephyr_file:`include/zephyr/bluetooth/conn.h` have been renamed
  to ``BT_LE_CS_TONE_ANTENNA_CONFIGURATION_A<NUMBER>_B<NUMBER>``.

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
