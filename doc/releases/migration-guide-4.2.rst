:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.2:

Migration guide to Zephyr v4.2.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.1.0 to
Zephyr v4.2.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.2>`.

.. contents::
    :local:
    :depth: 2

Build System
************

Kernel
******

Boards
******

* ``arduino_uno_r4_minima``, ``arduino_uno_r4_wifi``, and ``mikroe_clicker_ra4m1`` have migrated to
  new FSP-based configurations.
  While there are no major functional changes, the device tree structure has been significantly revised.
  The following device tree bindings are now deprecated:
  ``renesas,ra-gpio``, ``renesas,ra-uart-sci``, ``renesas,ra-pinctrl``,
  ``renesas,ra-clock-generation-circuit``, and ``renesas,ra-interrupt-controller-unit``.
  Instead, use the following replacements:
  - :dtcompatible:`renesas,ra-gpio-ioport`
  - :dtcompatible:`renesas,ra-sci-uart`
  - :dtcompatible:`renesas,ra-pinctrl-pfs`
  - :dtcompatible:`renesas,ra-cgc-pclk-block`

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
