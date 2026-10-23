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

Video
=====

* :c:struct:`video_frmival` is deprecated in favor of passing :c:type:`uint32_t` frame intervals in
  microseconds directly. :c:struct:`video_frmival_enum` now uses :c:type:`uint32_t` for its
  discrete frame interval, and :c:struct:`video_frmival_stepwise` now defines intervals in
  microseconds (:c:type:`uint32_t`) instead of fractions. :c:func:`video_set_frmival` and
  :c:func:`video_get_frmival` now take a pointer to a :c:type:`uint32_t` frame interval in
  microseconds. :c:func:`video_closest_frmival_stepwise` now takes and returns intervals directly
  in microseconds. :c:func:`video_frmival_nsec` is deprecated. (:github:`120731`)

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
