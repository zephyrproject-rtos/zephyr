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

* On the Nordic nRF54L and nRF71 Series SoCs, the push area that the Key Management Unit (KMU) of
  CRACEN uses to push keys is now reserved at the start of the SRAM by the ``kmu_push_area``
  devicetree node of the SoC. The ``zephyr,sram`` chosen node of the in-tree boards is now
  ``cpuapp_sram_app``, a child node of ``cpuapp_sram`` that starts after the push area. The nodes
  are not present when ``USE_NON_SECURE_ADDRESS_MAP`` is used. Out-of-tree boards that use the
  CRACEN KMU key push should be updated, as otherwise the application RAM overlaps the push area
  without any diagnostic. Boards that do not use it can keep using the whole RAM:

  * Replace ``zephyr,sram = &cpuapp_sram`` with ``zephyr,sram = &cpuapp_sram_app``.
  * To limit the SRAM available to the application, resize ``cpuapp_sram_app`` and keep its
    offset, for example ``reg = <0x60 (DT_SIZE_K(184) - 0x60)>``. Changing ``reg`` of
    ``cpuapp_sram`` no longer changes the SRAM that ``zephyr,sram`` refers to.
  * If the memory layout deletes and redefines ``cpuapp_sram``, assign the redefined node to
    ``zephyr,sram`` and leave the first ``0x60`` bytes of the SRAM free.
  * Remove custom nodes and linker sections that reserve the push area, such as
    ``nrf_kmu_reserved_push_area``. They would overlap the ``kmu_push_area`` node.

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

ADC
===

* :dtcompatible:`nxp,lpc-lpadc` channels that use ``ADC_GAIN_1_2`` for the 30/64 partial scale
  should use ``ADC_GAIN_15_32``: with ``ADC_GAIN_1_2``, which still selects that scale, the
  conversions of :c:func:`adc_raw_to_millivolts_dt` and :c:func:`adc_raw_to_millivolts` are
  6.25% too low.

Ethernet
========

* :kconfig:option:`CONFIG_ETH_PHY_DRIVER` is no longer enabled by default. It is now selected by
  the Ethernet drivers that need a PHY driver. Out-of-tree Ethernet drivers that use a PHY driver
  must select it as well. (:github:`120871`)

OPAMP
=====

* The NXP OPAMP drivers (:dtcompatible:`nxp,opamp` and :dtcompatible:`nxp,opamp-fast`) hold a
  power state constraint while the device is active, and the OPAMP nodes declare
  ``zephyr,disabling-power-states``. With :kconfig:option:`CONFIG_PM_POLICY_DEVICE_CONSTRAINTS`
  an enabled OPAMP therefore keeps the system out of Power Down and Deep Power Down for as long
  as it is active, which without :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` is from init onwards.
  Enable device runtime PM for the node, or suspend it explicitly, to let the system reach those
  states. (:github:`122135`)

* :dtcompatible:`nxp,opamp-fast` no longer enables an instance marked with
  ``is-reference-source`` from its init function; the instance follows its own power state like
  every other instance. With :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` an application has to hold
  a runtime reference on the reference source while it measures an amplifier fed by it. Builds
  without device runtime PM are unaffected: the device is resumed at init and the instance comes
  up enabled as before. (:github:`122135`)

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
