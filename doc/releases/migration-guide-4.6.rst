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

* The default Wi-Fi credentials backend is now
  :kconfig:option:`CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_PS` for non-secure targets with the TF-M
  Protected Storage partition and for targets that enable :kconfig:option:`CONFIG_SECURE_STORAGE`.
  Previously, these targets used :kconfig:option:`CONFIG_WIFI_CREDENTIALS_BACKEND_PSA` and
  :kconfig:option:`CONFIG_WIFI_CREDENTIALS_BACKEND_SETTINGS`, respectively. The new backend does
  not read credentials stored by these backends. To keep using credentials stored before the
  update, enable the previously used backend explicitly.

Other subsystems
****************

Modules
*******

Architectures
*************
