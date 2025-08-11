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

* mimxrt11x0: renamed lpadc1 to lpadc2 and renamed lpadc0 to lpadc1.

Device Drivers and Devicetree
*****************************

.. zephyr-keep-sorted-start re(^\w)

Sensors
=======

* Nodes with compatible property :dtcompatible:`invensense,icm42688` now additionally need to also
  include :dtcompatible:`invensense,icm4268x` in order to work.

Stepper
=======

* :dtcompatible:`zephyr,gpio-stepper` has been replaced by :dtcompatible:`zephyr,h-bridge-stepper`.

.. zephyr-keep-sorted-stop

Bluetooth
*********

.. zephyr-keep-sorted-start re(^\w)

.. zephyr-keep-sorted-stop

Ethernet
========

* The :dtcompatible:`microchip,vsc8541` PHY driver now expects the reset-gpios entry to specify
  the GPIO_ACTIVE_LOW flag when the reset is being used as active low. Previously the active-low
  nature was hard-coded into the driver. (:github:`91726`).

Networking
**********

* The HTTP server now respects the configured ``_config`` value. Check that
  you provide applicable value to :c:macro:`HTTP_SERVICE_DEFINE_EMPTY`,
  :c:macro:`HTTPS_SERVICE_DEFINE_EMPTY`, :c:macro:`HTTP_SERVICE_DEFINE` and
  :c:macro:`HTTPS_SERVICE_DEFINE`.

.. zephyr-keep-sorted-start re(^\w)

.. zephyr-keep-sorted-stop

Display
*******

* The RGB565 and BGR565 pixel formats were used interchangeably in the display sample.
  This has now been fixed. Boards and applications that were tested or developed based on the
  previous sample may be affected by this change (see :github:`79996` for more information).

PTP Clock
*********

* The doc of :c:func:`ptp_clock_rate_adjust` API didn't provide proper and clear function description.
  Drivers implemented it to adjust rate ratio relatively based on current frequency.
  Now PI servo is introduced in both PTP and gPTP, and this API function is changed to use for rate
  ratio adjusting based on nominal frequency. Drivers implementing :c:func:`ptp_clock_rate_adjust`
  should be adjusted to account for the new behavior.

Other subsystems
****************

.. zephyr-keep-sorted-start re(^\w)

.. zephyr-keep-sorted-stop

Modules
*******

* The TinyCrypt library was removed as the upstream version is no longer maintained.
  PSA Crypto API is now the recommended cryptographic library for Zephyr.

Silabs
======

* Aligned the name of the Rail options with the other SiSDK related options:

   * :kconfig:option:`CONFIG_RAIL_PA_CURVE_HEADER` to
     :kconfig:option:`CONFIG_SILABS_SISDK_RAIL_PA_CURVE_HEADER`
   * :kconfig:option:`CONFIG_RAIL_PA_CURVE_TYPES_HEADER` to
     :kconfig:option:`CONFIG_SILABS_SISDK_RAIL_PA_CURVE_TYPES_HEADER`
   * :kconfig:option:`CONFIG_RAIL_PA_ENABLE_CALIBRATION` to
     :kconfig:option:`CONFIG_SILABS_SISDK_RAIL_PA_ENABLE_CALIBRATION`

* Fixed name of the :kconfig:option:`CONFIG_SOC_*`. These option contained PART_NUMBER in their
  while they shouldn't.

Architectures
*************
