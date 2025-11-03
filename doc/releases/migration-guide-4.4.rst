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

Radio
=====

* The following devicetree bindings have been renamed for consistency with the ``radio-`` prefix:

  * :dtcompatible:`generic-fem-two-ctrl-pins` is now :dtcompatible:`radio-fem-two-ctrl-pins`
  * :dtcompatible:`gpio-radio-coex` is now :dtcompatible:`radio-gpio-coex`

* A new :dtcompatible:`radio.yaml` base binding has been introduced for generic radio hardware
  capabilities. The ``tx-high-power-supported`` property has been renamed to
  ``radio-tx-high-power-supported`` for consistency.

* Device trees and overlays using the old compatible strings must be updated to use the new names.

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
