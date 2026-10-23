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

.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Audio
===============

.. zephyr-keep-sorted-start re(^\* \w)

* MCP

  * The media proxy API has been removed and users will need to use the ``bt_mcs``, ``bt_mcc`` or
    ``bt_mcp_media_control_server`` APIs. Users of the media player API like
    ``media_proxy_pl_init`` can seemlessly switch to using
    :c:func:`bt_mcp_media_control_server_register`.
    Users using the ``bt_mcc`` API can continue as is, but ``struct mpl_cmd`` is now
    ``struct bt_mcs_cmd``, and ``struct mpl_search`` is now ``struct bt_mcs_search``.
    Uses of ``media_proxy_ctrl_`` for local operations can in most places just search-and-replace
    with ``bt_mcp_media_control_``. Uses of ``media_proxy_ctrl_`` for remote operations will need to
    switch to the ``bt_mcc`` API. (:github:`102994`)

.. zephyr-keep-sorted-stop

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
