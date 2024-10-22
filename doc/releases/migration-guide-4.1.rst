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
