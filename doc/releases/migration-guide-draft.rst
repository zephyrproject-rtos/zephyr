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

Ethernet
========

* :kconfig:option:`CONFIG_ETH_PHY_DRIVER` is no longer enabled by default. It is now selected by
  the Ethernet drivers that need a PHY driver. Out-of-tree Ethernet drivers that use a PHY driver
  must select it as well. (:github:`120871`)

* The GEM nodes of the Xilinx Zynq-7000 and ZynqMP SoCs now use the Cadence MACB/GEM driver,
  which transfers packets without copying. The Xilinx GEM driver with the ``xlnx,gem`` and
  ``xlnx,gem-mdio`` compatibles is deprecated.

  * The GEM nodes are now ``compatible = "xlnx,zynq-gem", "cdns,gem"`` and
    ``compatible = "xlnx,zynqmp-gem", "cdns,gem"`` (:dtcompatible:`xlnx,zynq-gem`,
    :dtcompatible:`xlnx,zynqmp-gem`, :dtcompatible:`cdns,gem`), the MDIO child node is
    :dtcompatible:`cdns,macb-mdio`. Boards only reference the SoC nodes and need no change
    unless they set properties of the ``xlnx,gem`` binding.
  * The hardware configuration properties of the ``xlnx,gem`` binding no longer apply to the
    GEM nodes: ``amba-ahb-burst-length``, ``hw-rx-buffer-size``, ``hw-rx-buffer-offset``,
    ``hw-tx-buffer-size-full``, ``rx-buffer-descriptors``, ``tx-buffer-descriptors``,
    ``rx-buffer-size``, ``tx-buffer-size``, ``handle-rx-in-isr``, ``handle-tx-in-workq``,
    ``disable-rx-checksum-offload``, ``disable-tx-checksum-offload`` and the feature flags
    ``ignore-ipg-rxer``, ``disable-reject-nsp``, ``ipg-stretch``, ``sgmii-mode``,
    ``disable-reject-fcs-crc-errors``, ``rx-halfdup-while-tx``, ``disable-pause-copy``,
    ``discard-rx-fcs``, ``discard-rx-length-errors``, ``pause-frame``, ``tbi``,
    ``ext-address-match``, ``long-frame-rx-support``, ``unicast-hash``, ``multicast-hash``,
    ``reject-broadcast``, ``discard-non-vlan``, ``discard-rx-frame-ahb-unavail``,
    ``ahb-packet-endian-swap`` and ``ahb-md-endian-swap``. The driver derives the
    configuration from the design configuration registers of the core; the PHY interface is
    taken from ``phy-connection-type`` (SGMII mode included) and the ring sizes from
    :kconfig:option:`CONFIG_ETH_CDNS_MACB_NB_TX_DESCS` and
    :kconfig:option:`CONFIG_ETH_CDNS_MACB_NB_RX_DESCS`. Checksum offload is controlled by
    :kconfig:option:`CONFIG_ETH_CDNS_MACB_TX_HW_CHECKSUM` and
    :kconfig:option:`CONFIG_ETH_CDNS_MACB_RX_HW_CHECKSUM`.
  * The ``clock-frequency`` property of the GEM nodes is replaced by the clock control API.
    As in the Linux devicetree, the GEM nodes take their clocks from the clock controllers of
    the SoCs, :dtcompatible:`xlnx,ps7-clkc` on the Zynq-7000, whose ``ps-clk-frequency`` the
    board sets, and :dtcompatible:`xlnx,zynqmp-clk` on the ZynqMP, which derive the rates
    from the registers as the boot firmware left them.
  * :kconfig:option:`CONFIG_ETH_XLNX_GEM` is replaced by
    :kconfig:option:`CONFIG_ETH_XLNX_CDNS_MACB` and :kconfig:option:`CONFIG_MDIO_XLNX_GEM` by
    :kconfig:option:`CONFIG_MDIO_CDNS_MACB`.
  * The receive descriptors point directly at the fragments of the network buffer pool, so
    :kconfig:option:`CONFIG_NET_BUF_FIXED_DATA_SIZE` is required,
    :kconfig:option:`CONFIG_NET_BUF_DATA_SIZE` must be a multiple of 64 and
    :kconfig:option:`CONFIG_NET_BUF_RX_COUNT` should be well above
    :kconfig:option:`CONFIG_ETH_CDNS_MACB_NB_RX_DESCS`. With a data cache,
    :kconfig:option:`CONFIG_NET_BUF_ALIGNMENT` defaults to the cache line size.
  * A board that still needs the Xilinx GEM driver sets the compatibles of its GEM node and
    of the MDIO child back to :dtcompatible:`xlnx,gem` and :dtcompatible:`xlnx,gem-mdio` in
    its devicetree, and restores the ``reg`` property with the second register range of the
    clock control register (``reg-names = "mac", "clkc"``), the ``clock-frequency`` property
    and the configuration properties of the ``xlnx,gem`` binding with the values the SoC
    devicetrees carried before; ``zephyr/dt-bindings/ethernet/xlnx_gem.h`` provides the
    constants.

.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Classic
=================

* The L2CAP BR/EDR ECHO API is now only available with :kconfig:option:`CONFIG_BT_TESTING`:
  :c:struct:`bt_l2cap_br_echo_cb`, :c:func:`bt_l2cap_br_echo_cb_register`,
  :c:func:`bt_l2cap_br_echo_cb_unregister`, :c:func:`bt_l2cap_br_echo_req` and
  :c:func:`bt_l2cap_br_echo_rsp`. The stack answers a received ECHO request with an ECHO response
  without data, unless a registered ``req`` callback handles it.

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
