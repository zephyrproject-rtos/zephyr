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

* Zephyr now officially defaults to C17 (ISO/IEC 9899:2018) as its minimum required
  C standard version.  If your toolchain does not support this standard you will
  need to use one of the existing and now deprecated options:
  :kconfig:option:`CONFIG_STD_C99` or :kconfig:option:`CONFIG_STD_C11`.

Kernel
******

Boards
******

Device Drivers and Devicetree
*****************************

.. zephyr-keep-sorted-start re(^\w)

Ethernet
========

* Driver MAC address configuration support using :c:struct:`net_eth_mac_config` has been introduced
  for the following drivers:

  * :zephyr_file:`drivers/ethernet/eth_test.c` (:github:`96598`)

  * :zephyr_file:`drivers/ethernet/eth_sam_gmac.c` (:github:`96598`)

    * Removed ``CONFIG_ETH_SAM_GMAC_MAC_I2C_EEPROM``
    * Removed ``CONFIG_ETH_SAM_GMAC_MAC_I2C_INT_ADDRESS``
    * Removed ``CONFIG_ETH_SAM_GMAC_MAC_I2C_INT_ADDRESS_SIZE``
    * Removed ``mac-eeprom`` property from :dtcompatible:`atmel,sam-gmac` and
      :dtcompatible:`atmel,sam0-gmac`

MDIO
====

* The ``mdio_bus_enable()`` and ``mdio_bus_disable()`` functions have been removed.
  MDIO bus enabling/disabling is now handled internally by the MDIO drivers.
  (:github:`99690`).

QSPI
====

* :dtcompatible:`st,stm32-qspi` compatible nodes configured with ``dual-flash`` property
  now need to also include the ``ssht-enable`` property to reenable sample shifting.
  Sample shifting is configurable now and disabled by default.
  (:github:`98999`).

Radio
=====

* The following devicetree bindings have been renamed for consistency with the ``radio-`` prefix:

  * :dtcompatible:`generic-fem-two-ctrl-pins` is now :dtcompatible:`radio-fem-two-ctrl-pins`
  * :dtcompatible:`gpio-radio-coex` is now :dtcompatible:`radio-gpio-coex`

* A new :dtcompatible:`radio.yaml` base binding has been introduced for generic radio hardware
  capabilities. The ``tx-high-power-supported`` property has been renamed to
  ``radio-tx-high-power-supported`` for consistency.

* Device trees and overlays using the old compatible strings must be updated to use the new names.

STM32
=====

* STM32 power supply configuration is now performed using Devicetree properties.
  New bindings :dtcompatible:`st,stm32h7-pwr`, :dtcompatible:`st,stm32h7rs-pwr`
  and :dtcompatible:`st,stm32-dualreg-pwr` have been introduced, and all Kconfig
  symbols related to power supply configuration have been removed:

  * ``CONFIG_POWER_SUPPLY_LDO``

  * ``CONFIG_POWER_SUPPLY_DIRECT_SMPS``,

  * ``CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_LDO``

  * ``CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_LDO``,

  * ``CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT_AND_LDO``

  * ``CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT_AND_LDO``

  * ``CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT``

  * ``CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT``

  * ``CONFIG_POWER_SUPPLY_EXTERNAL_SOURCE``

.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Host
==============

* :kconfig:option:`CONFIG_BT_SIGNING` has been deprecated.
* :c:macro:`BT_GATT_CHRC_AUTH` has been deprecated.
* :c:member:`bt_conn_le_info.interval` has been deprecated. Use
  :c:member:`bt_conn_le_info.interval_us` instead. Note that the units have changed: ``interval``
  was in units of 1.25 milliseconds, while ``interval_us`` is in microseconds.
* Legacy Bluetooth LE pairing using the passkey entry method no longer grants authenticated (MITM)
  protection as of the Bluetooth Core Specification v6.2. Stored bonds that were generated using
  this method will be downgraded to unauthenticated when loaded from persistent storage, resulting
  in a lower security level.

Networking
**********

* Networking APIs found in

  * :zephyr_file:`include/zephyr/net/net_ip.h`
  * :zephyr_file:`include/zephyr/net/socket.h`

  and relevant code in ``subsys/net`` etc. is namespaced. This means that either
  ``net_``, ``NET_`` or ``ZSOCK_`` prefix is added to the network API name. This is done in order
  to avoid circular dependency with POSIX or libc that might define the same symbol.
  A compatibility header file :zephyr_file:`include/zephyr/net/net_compat.h`
  is created that provides the old symbols allowing the user to continue use the old symbols.
  External network applications can continue to use POSIX defined network symbols and
  include relevant POSIX header files like ``sys/socket.h`` to get the POSIX symbols as Zephyr
  networking header files will no longer include those. If the application or Zephyr internal
  code cannot use POSIX APIs, then the relevant network API prefix needs to be added to the
  code calling a network API.

Other subsystems
****************

JWT
===

* Previously deprecated ``CONFIG_JWT_SIGN_RSA_LEGACY`` is removed. This removal happens before
  the usual deprecation period of 2 releases because it has been agreed (see :github:`97660`)
  that Mbed TLS is an external module, so normal deprecation rules do not apply in this case.

Modules
*******

Trusted Firmware-M
==================

* The ``SECURE_UART1`` TF-M define is now controlled by Zephyr's
  :kconfig:option:`CONFIG_TFM_SECURE_UART`. This option will override any platform values previously
  specified in the TF-M repository.

Architectures
*************
