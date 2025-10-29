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

* Node label ``sram0`` is deprecated in STM32U0xx SoCs in favor to label ``sram1`` that better
  match the wording used to refer to this internal memory in the SoCs reference manuals and
  litterature. The label is still defined but will be removed in the future.

Device Drivers and Devicetree
*****************************

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
