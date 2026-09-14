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

GPIO
====

* The ``CONFIG_GPIO_NCT38XX_PORT_INIT_PRIORITY`` Kconfig option has been removed. The NCT38xx
  GPIO port driver now uses :kconfig:option:`CONFIG_GPIO_NCT38XX_INIT_PRIORITY`.
  (:github:`115448`)

MFD
===

* The default of :kconfig:option:`CONFIG_MFD_INIT_PRIORITY` changed from ``80`` to
  :kconfig:option:`CONFIG_KERNEL_INIT_PRIORITY_DEVICE`, the default init priority of I2C and SPI
  controllers. All in-tree multi-function device drivers now use this option, and the init
  priorities of their child drivers default to it. Devices sharing an init priority are
  initialized in devicetree dependency order, so a parent MFD device is still initialized before
  its children. Out-of-tree boards and applications that choose init priorities relative to the
  old default of ``80``, for example to initialize a device between an MFD and its child devices,
  must review them. (:github:`115448`)

* The following driver-specific init priority Kconfig options have been removed. The drivers now
  use :kconfig:option:`CONFIG_MFD_INIT_PRIORITY` instead. (:github:`115448`)

  * ``CONFIG_MFD_ADP5360_INIT_PRIORITY``
  * ``CONFIG_MFD_ADP5585_INIT_PRIORITY``
  * ``CONFIG_MFD_MAXQ10XX_INIT_PRIORITY``
  * ``CONFIG_MFD_MICROCRYSTAL_RV3032_INIT_PRIORITY``
  * ``CONFIG_MFD_MOTOROLA_MC146818_INIT_PRIORITY``
  * ``CONFIG_MFD_NPM10XX_INIT_PRIORITY``
  * ``CONFIG_MFD_NPM13XX_INIT_PRIORITY``
  * ``CONFIG_MFD_NPM6001_INIT_PRIORITY``
  * ``CONFIG_MFD_RV3028_INIT_PRIORITY``
  * ``CONFIG_MFD_TLA2528_INIT_PRIO``

* The ``CONFIG_REGULATOR_MAXIM_MAX20335_COMMON_INIT_PRIORITY``,
  ``CONFIG_REGULATOR_NPM10XX_COMMON_INIT_PRIORITY`` and
  ``CONFIG_REGULATOR_PCA9422_COMMON_INIT_PRIORITY`` Kconfig options have been removed. The common
  part of these regulator drivers now uses the same init priority as the regulator devices,
  :kconfig:option:`CONFIG_REGULATOR_MAXIM_MAX20335_INIT_PRIORITY`,
  :kconfig:option:`CONFIG_REGULATOR_NPM10XX_INIT_PRIORITY` and
  :kconfig:option:`CONFIG_REGULATOR_PCA9422_INIT_PRIORITY` respectively. (:github:`115448`)

* The following init priority Kconfig options of MFD child drivers now default to
  :kconfig:option:`CONFIG_MFD_INIT_PRIORITY` instead of a fixed value. (:github:`115448`)

  * :kconfig:option:`CONFIG_ADC_M5PM1_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_ADC_TLA2528_INIT_PRIO`
  * :kconfig:option:`CONFIG_GPIO_ADP5585_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_GPIO_AXP192_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_GPIO_BD8LB600FS_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_GPIO_M5PM1_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_GPIO_MAX22017_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_GPIO_NCT38XX_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_GPIO_NPM10XX_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_GPIO_NPM13XX_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_GPIO_NPM2100_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_GPIO_NPM6001_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_GPIO_TLE9104_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_REGULATOR_AXP192_AXP2101_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_REGULATOR_M5PM1_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_REGULATOR_MAXIM_MAX20335_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_REGULATOR_NPM10XX_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_REGULATOR_PCA9422_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_SENSOR_DS3231_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_WDT_M5PM1_INIT_PRIORITY`
  * :kconfig:option:`CONFIG_WDT_NPM10XX_INIT_PRIORITY`

* The :dtcompatible:`motorola,mc146818` RTC and x86 CMOS counter drivers are no longer initialized
  at a fixed offset from the MC146818 MFD init priority. They now use
  :kconfig:option:`CONFIG_RTC_INIT_PRIORITY` and :kconfig:option:`CONFIG_COUNTER_INIT_PRIORITY`
  respectively. (:github:`115448`)

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
