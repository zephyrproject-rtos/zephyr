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

QSPI
===

* :dtcompatible:`st,stm32-qspi` compatible nodes configured with ``dual-flash`` property
  now need to also include the ``ssht-enable`` property to reenable sample shifting.
  Sample shifting is configurable now and disabled by default.
  (:github:`98999`).

Bluetooth
*********

Networking
**********

Other subsystems
****************

Modules
*******

Trusted Firmware-M
==================

* The ``SECURE_UART1`` TF-M define is now controlled by Zephyr's
  :kconfig:option:`CONFIG_TFM_SECURE_UART`. This option will override any platform values previously
  specified in the TF-M repository.

Architectures
*************
