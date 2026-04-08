:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.5:

Migration guide to Zephyr v4.5.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.4.0 to
Zephyr v4.5.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.5>`.

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

.. Group contents in this section by subsystem, e.g.:
..
.. ADC
.. ===
..
.. ...

.. zephyr-keep-sorted-start re(^\w) ignorecase

Clock Control
=============

* The :dtcompatible:`nxp,imxrt11xx-arm-pll` binding now uses ``loop-div`` and
  ``post-div`` for ARM PLL configuration. The legacy ``clock-mult`` and
  ``clock-div`` properties remain supported but are deprecated. Existing
  RT11xx overlays should be updated using the mapping
  ``loop-div = clock-mult * 2`` and ``post-div = clock-div``.


.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Audio
===============

* :c:member:`bt_bap_stream.codec_cfg` is now ``const``, to better reflect that it is a read-only
  value. Any non-read uses of it will need to be updated with the appropriate operations such as
  :c:func:`bt_bap_stream_config`, :c:func:`bt_bap_stream_reconfig`, :c:func:`bt_bap_stream_enable`
  or :c:func:`bt_bap_stream_metadata`. (:github:`104219`)
* Almost all API uses of ``struct bt_audio_codec_cfg *`` is now const, which means that once the
  ``codec_cfg`` has been stored in a parameter struct like
  :c:struct:`bt_cap_initiator_broadcast_subgroup_param` or
  :c:struct:`bt_cap_unicast_audio_start_stream_param`, then the parameter's pointer cannot be used
  to modify the ``codec_cfg``, and the actual definition of the struct should be modified instead.
  (:github:`104219`)

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
