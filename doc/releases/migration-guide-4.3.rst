:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.3:

Migration guide to Zephyr v4.3.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.2.0 to
Zephyr v4.3.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.3>`.

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

Bluetooth
*********

Networking
**********

Other subsystems
****************

Shell
=====

* The MQTT topics related to  :kconfig:option:`SHELL_BACKEND_MQTT`  have been renamed. Renamed
  ``<device_id>_rx`` to ``<device_id>/sh/rx`` and ``<device_id>_tx`` to ``<device_id>/sh/rx``.
  (:github:`92677`).

Modules
*******

Architectures
*************
