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
* :c:func:`bt_cap_commander_broadcast_reception_start` now waits for the CAP acceptors to sync to
  the broadcast before completing. This means that if the broadcast source is offline,
  including colocated broadcast sources like the ones created by
  :c:func:`bt_cap_handover_unicast_to_broadcast`, shall be active and have the periodic advertising
  enabled with a configured BASE. For :c:func:`bt_cap_handover_unicast_to_broadcast` the newly
  added :c:member:`bt_cap_handover_cb.unicast_to_broadcast_created` can be used to configure the
  BASE. This also means that any current checks implemented by an application to wait for receive
  state updates indicating successful sync can be removed,
  as :c:func:`bt_cap_commander_broadcast_reception_start` now ensures this when
  :c:member:`bt_cap_commander_cb.broadcast_reception_start` is called. This also applies for
  :c:func:`bt_cap_commander_broadcast_reception_stop` in a similar manner. (:github:`101070`)

Bluetooth HCI
=============

* The devicetree compatible ``bflb,bl70x-bt-hci`` has been renamed to
  :dtcompatible:`bflb,bt-hci`, now that a single binding covers all Bouffalo Lab
  on-chip BLE controllers (BL60x/BL70x/BL70XL). Out-of-tree boards and shields
  must update their devicetree nodes accordingly.

Networking
**********

Other subsystems
****************

Modules
*******

hal_nxp
=======

* S32K344: The pinmux header file for this SoC was renamed from ``S32K344-172MQFP-pinctrl.h`` to
  ``S32K344_K324_K314_172HDQFP-pinctrl.h``. Out-of-tree boards must update their include directive accordingly::

    #include <nxp/s32/S32K344_K324_K314_172HDQFP-pinctrl.h>

Architectures
*************
