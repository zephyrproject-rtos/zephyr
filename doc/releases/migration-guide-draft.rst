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

.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Audio
===============

.. zephyr-keep-sorted-start re(^\* \w)

* BAP

  * :c:member:`bt_bap_unicast_client_cb.start` and :c:member:`bt_bap_unicast_client_cb.stop` are
    only available if :kconfig:option:`CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT` is non-zero, as
    the receiver start ready and receiver stop ready operations are only valid for
    :c:enumerator:`BT_AUDIO_DIR_SOURCE` endpoints. Applications that support Unicast Client
    configurations without any source ASEs shall guard the assignment of these callbacks.
  * :c:func:`bt_bap_unicast_client_discover` now returns ``-ENOTSUP`` if the requested direction is
    not supported by the device, rather than starting a discovery procedure that would always fail.
    Discovering :c:enumerator:`BT_AUDIO_DIR_SINK` requires
    :kconfig:option:`CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT` to be non-zero and discovering
    :c:enumerator:`BT_AUDIO_DIR_SOURCE` requires
    :kconfig:option:`CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT` to be non-zero.

.. zephyr-keep-sorted-stop

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
