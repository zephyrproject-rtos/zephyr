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

OpenThread
==========

* The following Kconfigs options were renamed:

  * ``CONFIG_OPENTHREAD_MBEDTLS_CHOICE`` to
    :kconfig:option:`CONFIG_OPENTHREAD_SECURITY_DEFAULT_CONFIG`
  * ``CONFIG_CUSTOM_OPENTHREAD_SECURITY`` to
    :kconfig:option:`CONFIG_OPENTHREAD_SECURITY_CUSTOM_CONFIG`

* :kconfig:option:`CONFIG_OPENTHREAD_CRYPTO_PSA` no more depends on
  :kconfig:option:`CONFIG_PSA_CRYPTO_CLIENT`, but instead selects
  :kconfig:option:`CONFIG_PSA_CRYPTO`.

* In builds without TF-M, :kconfig:option:`CONFIG_SECURE_STORAGE` is now automatically
  implied if :kconfig:option:`CONFIG_OPENTHREAD_SECURITY_DEFAULT_CONFIG` and
  :kconfig:option:`CONFIG_OPENTHREAD_CRYPTO_PSA` are set. This
  guarantees that a PSA ITS implementation is available and it requires a backend
  for Secure Storage (Settings, ZMS, or a custom one) to be configured.

Trusted Firmware-M
==================

* The ``SECURE_UART1`` TF-M define is now controlled by Zephyr's
  :kconfig:option:`CONFIG_TFM_SECURE_UART`. This option will override any platform values previously
  specified in the TF-M repository.

Architectures
*************
