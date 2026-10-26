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

* Infineon PSOC Edge: the ``pse84`` SoC series has been renamed to ``pse8x``, because it covers the
  E81, E82, E83 and E84 parts. Out-of-tree boards and applications have to be updated:

  * The SoC directory moved from ``soc/infineon/edge/pse84/`` to ``soc/infineon/edge/pse8x/`` and
    the devicetree include path from ``<infineon/edge/pse84/...>`` to ``<infineon/edge/pse8x/...>``.
    The SoC dtsi files are renamed accordingly, for example ``pse84.cm33.dtsi`` is now
    ``pse8x.cm33.dtsi``.

  * Kconfig symbols are renamed:

    * ``CONFIG_SOC_SERIES_PSE84`` to :kconfig:option:`CONFIG_SOC_SERIES_PSE8X`
    * ``CONFIG_SOC_SERIES_PSE84_M33`` to :kconfig:option:`CONFIG_SOC_SERIES_PSE8X_M33`
    * ``CONFIG_SOC_SERIES_PSE84_M55`` to :kconfig:option:`CONFIG_SOC_SERIES_PSE8X_M55`
    * ``CONFIG_SOC_PSE84_M55_ENABLE`` to :kconfig:option:`CONFIG_SOC_PSE8X_M55_ENABLE`
    * ``CONFIG_SOC_PSE84_LPM_TIMER`` to :kconfig:option:`CONFIG_SOC_PSE8X_LPM_TIMER`
    * ``CONFIG_SOC_PACKAGE_PSE84_*`` to ``CONFIG_SOC_PACKAGE_PSE8X_*``

  * SoC names now match the orderable part numbers exactly. The 52 parts that carried an invented
    ``b`` suffix lost it, for example ``pse846gps4dbzc4b`` is now ``pse846gps4dbzc4``. This changes
    the board target, the ``CONFIG_SOC_<MPN>`` symbols and the ``<infineon/edge/mpns/<mpn>.dtsi>``
    include. Parts whose part number ends in ``A`` (such as ``pse846gps2dbzc4a``, used by
    ``kit_pse84_eval`` and ``kit_pse84_ai``) are unchanged.

  * The per-part dtsi files now describe the actual part. E81 and E82 parts no longer have the
    ``npu0`` node, the 4 MB and 2 MB SOCMEM parts have a smaller ``socmem`` node, and WLB-154 and
    eWLB-235 parts no longer have the nodes for peripherals the package does not bond out. Boards
    referencing these nodes have to be updated.

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
