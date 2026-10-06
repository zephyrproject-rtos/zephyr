:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.6:

Migration guide to Zephyr v4.6.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.5.0 to
Zephyr v4.6.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.6>`.

.. contents::
    :local:
    :depth: 2

Common
******

Build System
************

Kernel
******

Boards
******

Device Drivers and Devicetree
*****************************

.. Only place contents common to all device drivers here. Contents specific to one driver subsystem
   goes into its own subsection, below.

.. Group contents in this section by subsystem, e.g.:
..
.. ADC
.. ===
..
.. ...

.. zephyr-keep-sorted-start re(^\w) ignorecase

Ethernet
========

* The STM32 HAL PTP clock rate-adjustment bounds now have Kconfig ranges.
  :kconfig:option:`CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MIN_PCT` must be between 1 and 99, and
  :kconfig:option:`CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MAX_PCT` must be between 100 and 2147483647,
  inclusive. The lower bound must be below the nominal rate of 100% so the clock can be slowed down.
  Update out-of-tree configurations that assign values outside these ranges, or remove the
  assignments to use the unchanged defaults of 90 and 110, respectively. These constraints
  apply whenever :kconfig:option:`CONFIG_PTP_CLOCK_STM32_HAL` is enabled, even if
  :kconfig:option:`CONFIG_PTP_CLOCK_STM32_HAL_OUTPUT` is disabled. (:github:`121471`)

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
