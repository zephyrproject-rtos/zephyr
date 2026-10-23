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

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
