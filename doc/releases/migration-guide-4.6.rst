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

PTP Clock
=========

* :c:func:`ptp_clock_rate_adjust` and the :c:member:`ptp_clock_driver_api.rate_adjust` driver
  operation, which take the clock rate as a floating-point ratio, have been deprecated. Use
  :c:func:`ptp_clock_adjust_rate` and implement :c:member:`ptp_clock_driver_api.adjust_rate`
  instead, which take the rate offset from the nominal frequency as scaled parts per million,
  that is parts per million with a 16-bit binary fractional field:
  ``scaled_ppm = (ratio - 1) * 1000000 * 65536``. :c:macro:`PTP_CLOCK_SCALED_PPM_ONE` is 1 ppm.

  A driver only has to implement one of the two operations. Both functions work with either of
  them and convert the argument if necessary, so applications and drivers can be migrated
  independently.

  Drivers can use :c:func:`ptp_clock_scaled_ppm_to_ppb` if the hardware takes the offset in
  parts per billion, and :c:func:`ptp_clock_adjust_by_scaled_ppm` to compute the value of an
  addend register from its nominal value. All in-tree drivers have been converted.

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
