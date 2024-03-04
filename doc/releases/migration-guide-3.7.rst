:orphan:

.. _migration_3.7:

Migration guide to Zephyr v3.7.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v3.6.0 to
Zephyr v3.7.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_3.7>`.

.. contents::
    :local:
    :depth: 2

Build System
************

* Completely overhauled the way SoCs and boards are defined. This requires all
  out-of-tree SoCs and boards to be ported to the new model. See the
  :ref:`hw_model_v2` for more detailed information.

Kernel
******

Boards
******

Modules
*******

MCUboot
=======

zcbor
=====

Device Drivers and Devicetree
*****************************

Analog-to-Digital Converter (ADC)
=================================

Bluetooth HCI
=============

Controller Area Network (CAN)
=============================

* Removed the following deprecated CAN controller devicetree properties. Out-of-tree boards using
  these properties need to switch to using the ``bus-speed``, ``sample-point``, ``bus-speed-data``,
  and ``sample-point-data`` devicetree properties for specifying the initial CAN bitrate:

  * ``sjw``
  * ``prop-seg``
  * ``phase-seg1``
  * ``phase-seg1``
  * ``sjw-data``
  * ``prop-seg-data``
  * ``phase-seg1-data``
  * ``phase-seg1-data``

* Support for manual bus-off recovery was reworked:

  * Automatic bus recovery will always be enabled upon driver initialization regardless of Kconfig
    options. Since CAN controllers are initialized in "stopped" state, no unwanted bus-off recovery
    will be started at this point.
  * The Kconfig ``CONFIG_CAN_AUTO_BUS_OFF_RECOVERY`` was renamed (and inverted) to
    :kconfig:option:`CONFIG_CAN_MANUAL_RECOVERY_MODE`, which is disabled by default. This Kconfig
    option enables support for the :c:func:`can_recover()` API function and a new manual recovery mode
    (see the next bullet).
  * A new CAN controller operational mode :c:macro:`CAN_MODE_MANUAL_RECOVERY` was added. Support for
    this is only enabled if :kconfig:option:`CONFIG_CAN_MANUAL_RECOVERY_MODE` is enabled. Having
    this as a mode allows applications to inquire whether the CAN controller supports manual
    recovery mode via the :c:func:`can_get_capabilities` API function. The application can then
    either fail initialization or rely on automatic bus-off recovery. Having this as a mode
    furthermore allows CAN controller drivers not supporting manual recovery mode to fail early in
    :c:func:`can_set_mode` during application startup instead of failing when :c:func:`can_recover`
    is called at a later point in time.

Display
=======

Flash
=====

General Purpose I/O (GPIO)
==========================

Input
=====

Interrupt Controller
====================

LED Strip
=========

* The property ``in-gpios`` defined in :dtcompatible:`worldsemi,ws2812-gpio` has been
  renamed to ``gpios``.

Sensors
=======

Serial
======

Timer
=====

Bluetooth
*********

Bluetooth Mesh
==============

Bluetooth Audio
===============

Bluetooth Classic
=================

* The source files of Host BR/EDR have been moved to ``subsys/bluetooth/host/classic``.
  The Header files of Host BR/EDR have been moved to ``include/zephyr/bluetooth/classic``.
  Removed the :kconfig:option:`CONFIG_BT_BREDR`. It is replaced by new option
  :kconfig:option:`CONFIG_BT_CLASSIC`. (:github:`69651`)

Networking
**********

* The zperf zperf_results struct is changed to support 64 bits transferred bytes (total_len)
  and test duration (time_in_us and client_time_in_us), instead of 32 bits. This will make
  the long-duration zperf test show with correct throughput result.
  (:github:`69500`)

* Each IPv4 address assigned to a network interface has an IPv4 netmask
  tied to it instead of being set for the whole interface.
  If there is only one IPv4 address specified for a network interface,
  nothing changes from the user point of view. But, if there is more than
  one IPv4 address / network interface, the netmask must be specified
  for each IPv4 address separately. (:github:`68419`)

Other Subsystems
****************

LoRaWAN
=======

MCUmgr
======

Shell
=====

ZBus
====

Userspace
*********

Architectures
*************

* x86

  * Kconfigs ``CONFIG_DISABLE_SSBD`` and ``CONFIG_ENABLE_EXTENDED_IBRS``
    are deprecated. Use :kconfig:option:`CONFIG_X86_DISABLE_SSBD` and
    :kconfig:option:`CONFIG_X86_ENABLE_EXTENDED_IBRS` instead.

Xtensa
======
