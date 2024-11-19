:orphan:

.. _migration_4.1:

Migration guide to Zephyr v4.1.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.0.0 to
Zephyr v4.1.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.1>`.

.. contents::
    :local:
    :depth: 2

Build System
************

Kernel
******

Boards
******

Modules
*******

Mbed TLS
========

* If a platform has a CSPRNG source available (i.e. :kconfig:option:`CONFIG_CSPRNG_ENABLED`
  is set), then the Kconfig option :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG`
  is the default choice for random number source instead of
  :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_LEGACY_RNG`. This helps in reducing
  ROM/RAM footprint of the Mbed TLS library.

Trusted Firmware-M
==================

LVGL
====

* The config option :kconfig:option:`CONFIG_LV_Z_FLUSH_THREAD_PRIO` is now called
  :kconfig:option:`CONFIG_LV_Z_FLUSH_THREAD_PRIORITY` and its value is now interpreted as an
  absolute priority instead of a cooperative one.

Device Drivers and Devicetree
*****************************

Controller Area Network (CAN)
=============================

Display
=======

Enhanced Serial Peripheral Interface (eSPI)
===========================================

GNSS
====

Input
=====

Interrupt Controller
====================

LED Strip
=========

Sensors
=======

Serial
======

Stepper
=======

  * Renamed the ``compatible`` from ``zephyr,gpio-steppers`` to :dtcompatible:`zephyr,gpio-stepper`.

Regulator
=========

Bluetooth
*********

Bluetooth HCI
=============

Bluetooth Mesh
==============

Bluetooth Audio
===============

* The following Kconfig options are not longer automatically enabled by the LE Audio Kconfig
  options and may need to be enabled manually (:github:`81328`):

    * :kconfig:option:`CONFIG_BT_GATT_CLIENT`
    * :kconfig:option:`CONFIG_BT_GATT_AUTO_DISCOVER_CCC`
    * :kconfig:option:`CONFIG_BT_GATT_AUTO_UPDATE_MTU`
    * :kconfig:option:`CONFIG_BT_EXT_ADV`
    * :kconfig:option:`CONFIG_BT_PER_ADV_SYNC`
    * :kconfig:option:`CONFIG_BT_ISO_BROADCASTER`
    * :kconfig:option:`CONFIG_BT_ISO_SYNC_RECEIVER`
    * :kconfig:option:`CONFIG_BT_PAC_SNK`
    * :kconfig:option:`CONFIG_BT_PAC_SRC`

Bluetooth Classic
=================

Bluetooth Host
==============

Bluetooth Crypto
================

Networking
**********

Other Subsystems
****************

Flash map
=========

hawkBit
=======

MCUmgr
======

Modem
=====

Architectures
*************

* Common

  * ``_current`` is deprecated, used :c:func:`arch_current_thread` instead.

* native/POSIX

  * :kconfig:option:`CONFIG_NATIVE_APPLICATION` has been deprecated. Out-of-tree boards using this
    option should migrate to the native_simulator runner (:github:`81232`).
    For an example of how this was done with a board in-tree check :github:`61481`.
  * For the native_sim target :kconfig:option:`CONFIG_NATIVE_SIM_NATIVE_POSIX_COMPAT` has been
    switched to ``n`` by default, and this option has been deprecated. Ensure your code does not
    use the :kconfig:option:`CONFIG_BOARD_NATIVE_POSIX` option anymore (:github:`81232`).
