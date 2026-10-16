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

* The :dtcompatible:`smsc,lan91c111` node now holds the controller registers itself and the
  :dtcompatible:`smsc,lan91c111-mdio` node is its child, instead of both being children of a node
  with the registers. Move ``compatible``, ``interrupts`` and ``phy-handle`` of the MAC node into
  its parent and make the MDIO node a child of it:

  .. code-block:: devicetree

     eth: ethernet@1a000000 {
             compatible = "smsc,lan91c111";
             reg = <0x1a000000 0x1000>;
             interrupts = <GIC_SPI 15 IRQ_TYPE_LEVEL IRQ_DEFAULT_PRIORITY>;
             phy-handle = <&phy>;

             mdio: mdio {
                     compatible = "smsc,lan91c111-mdio";
                     #address-cells = <1>;
                     #size-cells = <0>;

                     phy: ethernet-phy@0 {
                             compatible = "ethernet-phy";
                             reg = <0>;
                     };
             };
     };

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
