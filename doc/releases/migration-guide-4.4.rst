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

Ethernet
========

* The Xilinx GEM Ethernet driver (:dtcompatible:`xlnx,gem`) has been switched over to use the
  current MDIO and PHY facilities, splitting up the driver's implementation into separate
  MDIO and Ethernet MAC drivers. The driver's custom PHY management code has been removed.
  The types of Ethernet PHYs supported by the removed custom code, the Marvell Alaska GBit
  PHY family and the TI TLK105/DP83822 100 MBit PHYs are both covered by the standard
  (:dtcompatible:`ethernet-phy`) driver. The QEMU targets which emulate the Xilinx GEM have
  been updated accordingly, as have been the device trees of the Zynq-7000 and ZynqMP /
  UltraScale+ SoC families.

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
