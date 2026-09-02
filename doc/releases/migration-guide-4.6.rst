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

Clock Control
=============

* Drivers had been returning inconsistent result codes for :c:func:`clock_control_on()` and
  :c:func:`clock_control_off()` for clocks that aren't stoppable. In addition, some drivers had
  implemented only the former function. Both have been marked as mandatory, with new helper
  functions added to :file:`include/zephyr/drivers/clock_control/clock_control_common.h`. Drivers
  are expected to use them instead of writing their own, whenever feasible. See :github:`118560`
  for examples of how in-tree drivers have been updated.

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
