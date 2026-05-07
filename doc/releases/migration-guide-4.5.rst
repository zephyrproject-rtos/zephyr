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

* ``_k_neg_eagain`` has been renamed to ``_errno_neg_egain`` as ``errno`` has been migrated out of
  kernel into ``lib/libc/common``.

Boards
******

Device Drivers and Devicetree
*****************************

Haptics
=======

* The ``cirrus,cs40l5x`` compatible has been replaced by variant-specific compatibles
  :dtcompatible:`cirrus,cs40l50`, :dtcompatible:`cirrus,cs40l51`, :dtcompatible:`cirrus,cs40l52`,
  and :dtcompatible:`cirrus,cs40l53`. Applications using the old compatible must update their
  devicetree nodes accordingly.

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

Digital Microphone
==================

* The DMIC driver backend API now uses :c:struct:`dmic_driver_api` instead of ``struct _dmic_ops``.

  Out-of-tree DMIC drivers must rename their backend API struct definitions and switch their API
  instances to ``DEVICE_API(dmic, ...)``. See :github:`107695` for examples of how in-tree drivers
  have been updated. Application code using :c:func:`dmic_configure`, :c:func:`dmic_trigger`, and
  :c:func:`dmic_read` is not impacted.

Ethernet
========

* ``ETHERNET_CONFIG_TYPE_T1S_PARAM`` and the related ``NET_REQUEST_ETHERNET_SET_T1S_PARAM`` has
  been removed. :c:func:`phy_set_plca_cfg` together with :c:func:`net_eth_get_phy` should be
  used instead to set these parameters (:github:`108136`).

* In the functions implemented by the :c:struct:`ethernet_api` a additional argument was added for
  a pointer to :c:struct:`net_if`. This api is not directly exposed to the application, so only
  out-of-tree drivers need to be updated. (:github:`106086`)

* The ``pinctrl-0`` and ``pinctrl-names`` devicetree properties for the
  :dtcompatible:`nxp,enet-mac` need to be moved from the MAC node to the parent Ethernet controller
  node. (:github:`107352`)

Flash
=====
* :dtcompatible:`jedec,spi-nand` now requires a ``plane-bytes`` property, which indicates the size
  of each plane in the flash device. For devices with a single plane, this should be set to the
  same value as ``size-bytes``.

GPIO
====

* The STM32 GPIO driver now returns ``-EINVAL`` when attempting to configure a GPIO pin in disabled
  state with a pull-up/pull-down resistor using :c:func:`gpio_pin_configure`. The driver would
  previously return ``0`` without actually honoring those flags (no PU/PD resistor was enabled).
  Applications encountering this error should remove :c:macro:`GPIO_PULL_UP`/ :c:macro:`GPIO_PULL_DOWN`
  from the ``flags`` they provide to :c:func:`gpio_pin_configure`; this will result in the same
  behavior as before since these flags were effectively ignored. (:github:`104690`)

* On STM32F1 series, GPIO output pins now use 50 MHz max. speed instead of 10 MHz. (:github:`104690`)

NXP
===

* :kconfig:option:`CONFIG_MCUX_LPTMR_TIMER` no longer defaults to ``y`` based on the
  ``/chosen/zephyr,system-timer`` chosen node being compatible with
  :dtcompatible:`nxp,lptmr`. Out-of-tree SoCs and boards that rely on the LPTMR
  as the system timer must now explicitly default the symbol in their
  ``Kconfig.defconfig`` (for example ``default y if PM``).

* Kinetis KE1xF no longer requires a board overlay to designate the system
  timer when :kconfig:option:`CONFIG_PM` is enabled. The SoC DTSI now sets the
  ``zephyr,system-timer`` chosen property, so boards that added the overlay
  described in the Zephyr 4.4 migration guide can remove it.

STM32
=====

* SoC DTSI files now consistently use interrupt priority zero for all peripherals.
  Applications must now explicitly configure interrupt priorities using Devicetree
  if they previously relied on the values found in SoC DTSI files. (:github:`106188`)

Syscon
======

* The syscon API functions :c:func:`syscon_read_reg` and :c:func:`syscon_write_reg` now use
  ``uint32_t`` for the register offset parameter instead of ``uint16_t``. This allows for
  larger register offsets. Code that explicitly declares ``uint16_t`` variables for the
  register parameter or implements the syscon driver API functions may need to be updated.

WiFi
====

* In the functions implemented by the :c:struct:`net_wifi_mgmt_offload`, internally
  :c:struct:`ethernet_api` and :c:struct:`wifi_mgmt_ops`, a additional argument was added for
  a pointer to :c:struct:`net_if`. This api is not directly exposed to the application, so only
  out-of-tree drivers need to be updated. (:github:`106086`)

.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Audio
===============

.. zephyr-keep-sorted-start re(^\* \w)

* BAP

  * :c:member:`bt_bap_stream.codec_cfg` is now ``const``, to better reflect that it is a read-only
    value. Any non-read uses of it will need to be updated with the appropriate operations such as
    :c:func:`bt_bap_stream_config`, :c:func:`bt_bap_stream_reconfig`, :c:func:`bt_bap_stream_enable`
    or :c:func:`bt_bap_stream_metadata`. (:github:`104219`)
  * :c:member:`bt_bap_stream.qos` is now ``const``, to better reflect that it is a read-only
    value. Any non-read uses of it will need to be set with the appropriate operations such as
    :c:func:`bt_bap_unicast_group_create`, :c:func:`bt_bap_unicast_group_reconfig`,
    :c:func:`bt_bap_broadcast_source_create` or :c:func:`bt_bap_broadcast_source_reconfig`.
    (:github:`104887`)
  * Almost all API uses of ``struct bt_bap_qos_cfg *`` are now ``const``, which means that once the
    ``qos`` has been stored in a parameter struct like
    :c:struct:`bt_bap_broadcast_source_param` or
    :c:struct:`bt_bap_unicast_group_stream_param`, then the parameter's pointer cannot be used
    to modify the ``qos``, and the actual definition of the struct should be modified instead.
    (:github:`104219`)
  * :c:member:`bt_bap_unicast_group_info.sink_pd` and :c:member:`bt_bap_unicast_group_info.source_pd`
    now reflect the local values defined for the group, and not the values configured for any remote
    ASEs. (:github:`104887`)
  * :c:func:`bt_bap_unicast_client_discover` and :c:func:`bt_bap_broadcast_assistant_discover` now
    require that the connection has already gone through the pairing process and meets the security
    requirements of BAP before doing any discovery. In most cases this requires a call to
    :c:func:`bt_conn_set_security` for new devices. Bonded devices that reconnect should not require
    anything.
  * Almost all API uses of ``struct bt_audio_codec_cfg *`` are now ``const``, which means that once
    the ``codec_cfg`` has been stored in a parameter struct like
    :c:struct:`bt_bap_stream` or
    :c:struct:`bt_bap_broadcast_source_subgroup_param`, then the parameter's pointer cannot be used
    to modify the ``codec_cfg``, and the actual definition of the struct should be modified instead.
    (:github:`104219`)

* CAP

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

* CCP

  * :c:member:`bt_tbs_client_cb.technology` has changed the ``value`` parameter from ``uint32_t``
    to ``enum bt_bearer_tech``. Applications using this application should switch the type.
    (:github:`102430`)
  * All ``BT_TBS_TECHNOLOGY_*`` values like ``BT_TBS_TECHNOLOGY_3G`` are renamed to
    ``BT_BEARER_TECH_*`` like ``BT_BEARER_TECH_3G``. Applications can do search-and-replace from
    ``BT_TBS_TECHNOLOGY`` to ``BT_BEARER_TECH``. Additionally the values are now defined in
    :zephyr_file:`include/zephyr/bluetooth/assigned_numbers.h` instead of
    :zephyr_file:`include/zephyr/bluetooth/audio/tbs.h`. (:github:`102430`)

* CSIP

  * Optional CSIS characteristics have been made configurable via Kconfig and must be enabled
    explicitly:

    * Coordinated Set Size → :kconfig:option:`CONFIG_BT_CSIP_SET_MEMBER_SIZE_SUPPORT`
    * Set Member Lock → :kconfig:option:`CONFIG_BT_CSIP_SET_MEMBER_LOCK_SUPPORT`
    * Set Member Rank → :kconfig:option:`CONFIG_BT_CSIP_SET_MEMBER_RANK_SUPPORT`

.. zephyr-keep-sorted-stop

Bluetooth Classic
=================

* The BR/EDR specific callbacks ``role_changed`` and ``br_mode_changed`` in
  :c:struct:`bt_conn_cb` have been moved into a new sub-struct
  :c:struct:`bt_conn_br_cb`, accessible via the ``br`` member. Application code
  using these callbacks must update the designated initializers:

  * ``.role_changed`` → ``.br.role_changed``
  * ``.br_mode_changed`` → ``.br.mode_changed``

  (:github:`108022`)

Bluetooth HCI
=============

* The devicetree compatible ``bflb,bl70x-bt-hci`` has been renamed to
  :dtcompatible:`bflb,bt-hci`, now that a single binding covers all Bouffalo Lab
  on-chip BLE controllers (BL60x/BL70x/BL70XL). Out-of-tree boards and shields
  must update their devicetree nodes accordingly.

Networking
**********

Ethernet
========

* :kconfig:option:`CONFIG_NET_DEFAULT_IF_ETHERNET` now allows to get the first ethernet interface,
  instead of the first between ethernet and wifi.

Other subsystems
****************

* Demand paging (``subsys/demand_paging``) is moved under Memory Management
  into ``subsys/mem_mgmt/demand_paging``. Custom backing store and eviction algorithm code need
  to be moved there.


* The ring buffer "item" API in ``<zephyr/sys/ring_buffer.h>`` has been deprecated in favor of the new
  fixed-size queue API in ``<zephyr/sys/ringq.h>``.

  Code storing fixed-size items should migrate to :c:struct:`sys_ringq` (see
  :ref:`fixed_size_ringq_api`). Code that only used the item API at the byte level should switch to
  the byte-mode functions :c:func:`ring_buf_put` / :c:func:`ring_buf_get` calls on the same
  :c:struct:`ring_buf`. (:github:`98255`)

Modules
*******

* Support for the `CANopenNode <https://github.com/CANopenNode/CANopenNode>`_ protocol stack was
  moved to an :ref:`external module<external_module_canopennode>`.

hal_nxp
=======

* S32K344: The pinmux header file for this SoC was renamed from ``S32K344-172MQFP-pinctrl.h`` to
  ``S32K344_K324_K314_172HDQFP-pinctrl.h``. Out-of-tree boards must update their include directive accordingly::

    #include <nxp/s32/S32K344_K324_K314_172HDQFP-pinctrl.h>

Mbed TLS
========

* :kconfig:option:`CONFIG_MBEDTLS_SSL_EARLY_DATA` is now an explicit opt-in and is no longer
  implicitly enabled by :kconfig:option:`CONFIG_MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED`.
  Out-of-tree applications or board configurations that rely on TLS 1.3 PSK early data (0-RTT)
  must now explicitly enable :kconfig:option:`CONFIG_MBEDTLS_SSL_EARLY_DATA`.

Architectures
*************

* A new architecture primitive, ``arch_cpu_irqs_are_enabled()``, has been added.
  It returns the current interrupt-enable state of the calling CPU without
  modifying it, complementing ``arch_irq_unlocked()`` which inspects a saved
  key.  Out-of-tree architecture ports must provide an implementation.
