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

* :kconfig:option:`CONFIG_ETH_PHY_DRIVER` is no longer enabled by default. It is now selected by
  the Ethernet drivers that need a PHY driver. Out-of-tree Ethernet drivers that use a PHY driver
  must select it as well. (:github:`120871`)

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

Bluetooth Classic
=================

* The L2CAP BR/EDR ECHO API is now only available with :kconfig:option:`CONFIG_BT_TESTING`:
  :c:struct:`bt_l2cap_br_echo_cb`, :c:func:`bt_l2cap_br_echo_cb_register`,
  :c:func:`bt_l2cap_br_echo_cb_unregister`, :c:func:`bt_l2cap_br_echo_req` and
  :c:func:`bt_l2cap_br_echo_rsp`. The stack answers a received ECHO request with an ECHO response
  without data, unless a registered ``req`` callback handles it.

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
