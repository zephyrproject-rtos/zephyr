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

* The ``CONFIG_UART_SIFIVE_PORT_0``, ``CONFIG_UART_SIFIVE_PORT_1`` and the related
  ``CONFIG_UART_SIFIVE_PORT_*_RXCNT_IRQ`` and ``CONFIG_UART_SIFIVE_PORT_*_TXCNT_IRQ`` Kconfig
  options have been removed. The :dtcompatible:`sifive,uart0` driver now instantiates a device for
  every enabled devicetree node. The TX watermark interrupt fires when the TX FIFO is empty and
  the RX watermark interrupt fires as soon as the RX FIFO holds a character.

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
