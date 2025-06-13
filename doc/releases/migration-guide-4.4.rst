:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.4:

Migration guide to Zephyr v4.4.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.3.0 to
Zephyr v4.4.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.4>`.

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

Bluetooth Audio
===============

* :c:func:`bt_bap_broadcast_assistant_discover` will now no longer perform reads of the remote BASS
  receive states at the end of the procedure. Users will have to manually call
  :c:func:`bt_bap_broadcast_assistant_read_recv_state` to read the existing receive states, if any,
  prior to performing any operations. (:github:`91587``)

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
