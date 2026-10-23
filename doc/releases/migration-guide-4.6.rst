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

Pin Control
===========

* i.MX RT266x: the ``<pad>_hsp_xbar<n>_inout<m>`` pinmux nodes were replaced by
  ``<pad>_hsp_xbar<n>_in<m>`` (the pad feeds the crossbar) and ``<pad>_hsp_xbar<n>_out<m>`` (the
  crossbar drives the pad). The new nodes also set the XBAR direction bit of the pad, which resets
  to output. Use the node that matches the signal direction, for example
  ``main__iomuxc_pio3_0_hsp_xbar0_in00`` instead of ``main__iomuxc_pio3_0_hsp_xbar0_inout00``.

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
