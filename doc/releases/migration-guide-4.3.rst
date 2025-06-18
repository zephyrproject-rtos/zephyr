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

.. zephyr-keep-sorted-start re(^\w)

.. zephyr-keep-sorted-stop

Bluetooth
*********

.. zephyr-keep-sorted-start re(^\w)

.. zephyr-keep-sorted-stop

Networking
**********

.. zephyr-keep-sorted-start re(^\w)

.. zephyr-keep-sorted-stop

Display
*******

* The RGB565 and BGR565 pixel formats were used interchangeably in the display sample.
  This has now been fixed. Boards and applications that were tested or developed based on the
  previous sample may be affected by this change (see :github:`79996` for more information).

Other subsystems
****************

.. zephyr-keep-sorted-start re(^\w)

.. zephyr-keep-sorted-stop

Modules
*******

Architectures
*************
