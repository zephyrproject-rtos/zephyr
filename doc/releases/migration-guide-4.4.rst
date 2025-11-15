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

Display
=======

* For ILI9XXX controllers, the usage of ``ILI9XXX_PIXEL_FORMAT_x`` in devicetrees for panel color format selection
  has been updated to ``PANEL_PIXEL_FORMAT_x``. Out-of-tree boards and shields should be updated accordingly.
  (:github:`99267`).
* For ILI9341 controller, display mirroring configuration has been updated to conform with the described behavior
  of the sample ``samples/drivers/display``. (:github:`99267`).

QSPI
===

* :dtcompatible:`st,stm32-qspi` compatible nodes configured with ``dual-flash`` property
  now need to also include the ``ssht-enable`` property to reenable sample shifting.
  Sample shifting is configurable now and disabled by default.
  (:github:`98999`).

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
