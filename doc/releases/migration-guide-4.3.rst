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

Other subsystems
****************

.. zephyr-keep-sorted-start re(^\w)

Modem Modules
=============

* Removed :kconfig:option:`CONFIG_MODEM_CMUX_WORK_BUFFER_SIZE`. The option was being misused to
  determine unrelated buffer sizes and the cmux work buffer itself is no longer needed.

  If the option :kconfig:option:`CONFIG_MODEM_CMUX_WORK_BUFFER_SIZE` was used to determine the
  size of the ``receive_buf`` member of a :c:struct:`modem_cmux_config` instance, replace it
  with ``CONFIG_MODEM_CMUX_MTU + 7``.

  If the option :kconfig:option:`CONFIG_MODEM_CMUX_WORK_BUFFER_SIZE` was used to determine the
  size of the ``receive_buf`` member of a :c:struct:`modem_cmux_dlci_config` instance, replace
  it with ``CONFIG_MODEM_CMUX_MTU``.

.. zephyr-keep-sorted-stop

Modules
*******

Architectures
*************
