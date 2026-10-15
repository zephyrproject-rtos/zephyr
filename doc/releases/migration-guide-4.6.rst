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

* A devicetree node now inherits the status of its parent node when the parent is not
  ``"okay"``: every node below a ``"disabled"`` (or ``"reserved"``) node is treated as
  ``"disabled"`` (or ``"reserved"``), even if it has ``status = "okay";`` itself. Such nodes
  no longer get a ``struct device``, no longer count toward ``DT_NUM_INST_STATUS_OKAY()`` and
  no longer enable ``CONFIG_DT_HAS_<compat>_ENABLED``. Boards and applications that relied on
  enabled nodes below a disabled parent, for example a device on a bus controller that is
  disabled, must enable the parent node.

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
