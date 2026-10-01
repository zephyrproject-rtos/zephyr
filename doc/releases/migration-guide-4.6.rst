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

Networking
**********

Other subsystems
****************

Secure Storage
==============

* The default PS implementation changed from
  :kconfig:option:`CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ITS` to the newly added
  :kconfig:option:`CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ZEPHYR`. The latter stores PS entries
  in a way that is not compatible with the former, and existing entries are not migrated.
  On devices that already have PS entries stored, explicitly enable
  :kconfig:option:`CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ITS` to keep the previous behavior.
  Otherwise these entries can no longer be read and may become impossible to overwrite or remove.
  See :ref:`secure_storage` for details.

Modules
*******

Architectures
*************
