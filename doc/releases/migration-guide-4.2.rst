:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.2:

Migration guide to Zephyr v4.2.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.1.0 to
Zephyr v4.2.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.2>`.

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

Sensors
=======

  * The :dtcompatible:`we,wsen-itds` driver has been renamed to
    :dtcompatible:`we,wsen-itds-2533020201601`.
    The Device Tree can be configured as follows:

    .. code-block:: devicetree

      &i2c0 {
        itds:itds-2533020201601@19 {
          compatible = "we,wsen-itds-2533020201601";
          reg = <0x19>;
          odr = "400";
          op-mode = "high-perf";
          power-mode = "normal";
          events-interrupt-gpios = <&gpio1 1 GPIO_ACTIVE_HIGH>;
          drdy-interrupt-gpios = < &gpio1 2 GPIO_ACTIVE_HIGH >;
        };
      };

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
