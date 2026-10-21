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

MFD
===

* The nPM13xx MFD event callback API no longer reuses :c:struct:`gpio_callback`. Register with
  :c:struct:`mfd_npm13xx_event_callback`, setting ``event_mask`` (a bitwise-OR of
  ``BIT(NPM13XX_EVENT_*)``) and ``handler`` directly instead of calling ``gpio_init_callback()``.
  The handler signature is now ``void handler(const struct device *dev,
  struct mfd_npm13xx_event_callback *cb, npm13xx_event_t events)``. A handler is invoked once
  per dispatch with the fired events it subscribed to rather than once per event bit.
  (:github:`110454`)

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
