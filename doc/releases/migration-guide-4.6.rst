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

.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Host
==============

* ``CONFIG_BT_ISO_TX_MTU`` has been removed. Applications should define their own SDU buffer sizes.
  The ISO HCI data buffer size can be configured for the Controller with
  :kconfig:option:`CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE`, and the SDU size with
  :kconfig:option:`CONFIG_BT_CTLR_ISO_TX_SDU_LEN_MAX`.
  In most cases :kconfig:option:`CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE` should be set to 4
  (if timestamps are not used) or 8 (if timestamps are used) above
  :kconfig:option:`CONFIG_BT_CTLR_ISO_TX_SDU_LEN_MAX`. For larger values of
  :kconfig:option:`CONFIG_BT_CTLR_ISO_TX_SDU_LEN_MAX` it may be better to increase
  :kconfig:option:`CONFIG_BT_CTLR_ISO_TX_BUFFERS` instead of
  :kconfig:option:`CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE`.

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
