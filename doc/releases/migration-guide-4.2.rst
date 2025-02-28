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

Device Drivers and Devicetree
*****************************

GPIO
====

* To support the RP2350B, which has many pins, the RaspberryPi-GPIO configuration has
  been changed. The previous role of :dtcompatible:`raspberrypi,rpi-gpio` has been migrated to
  :dtcompatible:`raspberrypi,rpi-gpio-port`, and :dtcompatible:`raspberrypi,rpi-gpio` is
  now left as a placeholder and mapper.
  The labels have also been changed along, so no changes are necessary for regular use.

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
