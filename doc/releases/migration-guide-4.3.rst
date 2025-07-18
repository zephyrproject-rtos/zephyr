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

Logging
=======

* The UART dictionary log parsing script
  :zephyr_file:`scripts/logging/dictionary/log_parser_uart.py` has been deprecated. Instead, the
  more generic script of :zephyr_file:`scripts/logging/dictionary/live_log_parser.py` should be
  used. The new script supports the same functionality (and more), but requires different command
  line arguments when invoked.

Modules
*******

Architectures
*************
