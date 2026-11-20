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

SD Host Controller
==================

* The Cadence SDHC Kconfig option ``CONFIG_CDNS_SDHC`` has been renamed to
  :kconfig:option:`CONFIG_SDHC_CDNS`.

* The ``reg-names`` property of :dtcompatible:`cdns,sdhc` is now required. The ``reg_base`` and
  ``combo_phy`` register regions are no longer used. Name the slot register region ``slot``.
  Controllers with a separate host register region must name it ``host`` and set the ``has-host``
  property, so ``reg-names = "reg_base", "combo_phy";`` becomes ``reg-names = "host", "slot";``.

* The ``clock-frequency`` property of :dtcompatible:`cdns,sdhc` has been removed. The driver now
  reads the controller clock rate through the clock control API, so reference the clock with the
  ``clocks`` property instead.

* The Cadence SDHC driver no longer controls reset lines and the ``resets`` property has been
  removed from :dtcompatible:`cdns,sdhc`. Reset lines must be released before the driver
  initializes.

* Cadence SD6HC controllers must use the :dtcompatible:`cdns,sd6hc` compatible, listed before
  ``cdns,sdhc``: ``compatible = "cdns,sdhc";`` becomes
  ``compatible = "cdns,sd6hc", "cdns,sdhc";``. The default combo PHY and controller timing
  settings for each speed mode can be overridden with the ``cdns,phy-cfg-*`` and
  ``cdns,ctrl-cfg-*`` properties.

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
