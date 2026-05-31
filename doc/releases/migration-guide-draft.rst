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

Serial
======

* The :kconfig:option:`CONFIG_UART_XLNX_PS` Kconfig option has been removed in favour of
  :kconfig:option:`CONFIG_UART_CDNS`. Nodes with :dtcompatible:`xlnx,xuartps` must add
  :dtcompatible:`cdns,uart`; The :dtcompatible:`xlnx,xuartps` remains valid as a hardware identifier;
  the unified driver matches on :dtcompatible:`cdns,uart`. The ``bdiv`` property has been removed;
  baud rate is now computed automatically from clock-frequency and current-speed. (:github:`114729`)

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
