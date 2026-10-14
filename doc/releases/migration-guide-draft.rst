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

Bluetooth Classic
=================

* The L2CAP BR/EDR ECHO API is now only available with :kconfig:option:`CONFIG_BT_TESTING`:
  :c:struct:`bt_l2cap_br_echo_cb`, :c:func:`bt_l2cap_br_echo_cb_register`,
  :c:func:`bt_l2cap_br_echo_cb_unregister`, :c:func:`bt_l2cap_br_echo_req` and
  :c:func:`bt_l2cap_br_echo_rsp`. The stack answers a received ECHO request with an ECHO response
  without data, unless a registered ``req`` callback handles it.

Haptics
*******

* Vendor prefixes have been added to hardware-specific devicetree properties. Applications using the
  following devicetree properties must update accordingly:

  * :dtcompatible:`cirrus,cs40l26`, :dtcompatible:`cirrus,cs40l27`, :dtcompatible:`cirrus,cs40l50`,
    :dtcompatible:`cirrus,cs40l51`, :dtcompatible:`cirrus,cs40l52`, :dtcompatible:`cirrus,cs40l53`

    * ``trigger-mapping`` changed to ``cirrus,trigger-mapping``
    * ``flash-storage`` changed to ``cirrus,flash-storage``
    * ``flash-offset`` changed to ``cirrus,flash-offset``

  * :dtcompatible:`ti,drv2605`

    * ``actuator-mode`` changed to ``ti,actuator-mode``
    * ``feedback-brake-factor`` changed to ``ti,feedback-brake-factor``
    * ``loop-gain`` changed to ``ti,loop-gain``
    * ``vib-rated-mv`` changed to ``ti,vib-rated-mv``
    * ``vib-overdrive-mv`` changed to ``ti,vib-overdrive-mv``

  * :dtcompatible:`titanmec,tm6605`

    * ``default-effect`` changed to ``titanmec,default-effect``

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
