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

Any other changes (not directly related to migrating applications) will be found in
the release notes.

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

.. Group contents in this section by subsystem, e.g.:
..
.. ADC
.. ===
..
.. ...

.. zephyr-keep-sorted-start re(^\w) ignorecase

Video
=====

* :c:struct:`video_frmival` and :c:struct:`video_frmival_stepwise` now define frame
  intervals in microseconds (:c:type:`uint32_t`) instead of fractions.
  :c:func:`video_closest_frmival_stepwise` now takes and returns intervals directly in
  microseconds. :c:func:`video_frmival_nsec` is deprecated in favor of
  :c:func:`video_frmival_usec`. (:github:`120731`)

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
