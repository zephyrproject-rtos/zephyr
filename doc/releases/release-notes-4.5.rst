:orphan:

..
  What goes here: removed/deprecated apis, new boards, new drivers, notable
  features. If you feel like something new can be useful to a user, put it
  under "Other Enhancements" in the first paragraph, if you feel like something
  is worth mentioning in the project media (release blog post, release
  livestream) put it under "Major enhancement".
..
  If you are describing a feature or functionality, consider adding it to the
  actual project documentation rather than the release notes, so that the
  information does not get lost in time.
..
  No list of bugfixes, minor changes, those are already in the git log, this is
  not a changelog.
..
  Does the entry have a link that contains the details? Just add the link, if
  you think it needs more details, put them in the content that shows up on the
  link.
..
  Are you thinking about generating this? Don't put anything at all.
..
  Does the thing require the user to change their application? Put it on the
  migration guide instead. (TODO: move the removed APIs section in the
  migration guide)

.. _zephyr_4.5:

Zephyr 4.5.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.5.0.

Major enhancements with this release include:

**New driver classes**

  Zephyr 4.5 adds several new driver APIs, including:

  - :ref:`Clock Monitor <clock_monitor_api>` for runtime observation of clock frequency

**New subsystems**

  Zephyr 4.5 adds several new subsystem APIs, including:

  - :ref:`Video <video_api>` for controlling video drivers

An overview of the changes required or recommended when migrating your application from Zephyr
v4.4.0 to Zephyr v4.5.0 can be found in the separate :ref:`migration guide<migration_4.5>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* :cve:`2026-8718` Under embargo until 2026-08-08

* :cve:`2026-9263` Under embargo until 2026-06-28

API Changes
***********

..
  Only removed, deprecated and new APIs. Changes go in migration guide.

Removed APIs and options
========================

* Architectures

   * Xtensa

      * ``CONFIG_XTENSA_BACKTRACE_EXCEPTION_DUMP_HOOK``

* Bluetooth

  * Host

    * The ``CONFIG_BT_RECV_CONTEXT`` choice and its options ``CONFIG_BT_RECV_WORKQ_SYS``
      and ``CONFIG_BT_RECV_WORKQ_BT`` have been removed. The host now always
      processes low-priority HCI packets on the dedicated Bluetooth RX workqueue
      (the former ``CONFIG_BT_RECV_WORKQ_BT`` behavior). See the migration guide.

    * Selected Host work items have moved from the system workqueue to the
      dedicated Bluetooth RX workqueue. Application callbacks reached from
      those work items now run in the Bluetooth RX thread. See the migration
      guide for affected callback families.

    * The ``CONFIG_BT_HCI_RAW_H4`` and ``CONFIG_BT_HCI_RAW_H4_ENABLE`` Kconfig
      options have been removed. They have had no effect since Zephyr 4.2,
      where the HCI raw layer switched to using H:4 packet encoding for all
      buffers unconditionally. Applications still setting these options can
      simply drop them.

* Counter

    * ``CONFIG_COUNTER_MAXIM_DS3231``
    * ``prescaler`` property of :dtcompatible:`nxp,lptmr`

* LLEXT

    * ``llext_get_fn_table``, replaced by ``llext_get_fn_table_entry``

* Networking

    * ``CONFIG_NET_TC_SKIP_FOR_HIGH_PRIO``
    * ``CONFIG_NET_SOCKETS_POLL_MAX``
    * ``CONFIG_NET_GPTP_CLOCK_ACCURACY_*``
    * ``net_ipv6_set_hop_limit()``
    * ``net_if_ipv4_get_netmask()``
    * ``net_if_ipv4_set_netmask()``
    * ``net_if_ipv4_set_netmask_by_index()``
    * ``openthread_state_changed_cb_register()``
    * ``openthread_state_changed_cb_unregister()``
    * ``openthread_start()``
    * ``openthread_api_mutex_lock()``
    * ``openthread_api_mutex_try_lock()``
    * ``openthread_api_mutex_unlock()``
    * ``struct openthread_state_changed_cb``
    * ``TLS_CREDENTIAL_SERVER_CERTIFICATE``
    * ``start_11r_roaming``

* Random

    * ``CONFIG_CTR_DRBG_CSPRNG_GENERATOR``
    * ``CONFIG_CS_CTR_DRBG_PERSONALIZATION``

* West sign support for imgtool, which was deprecated in Zephyr 4.0, has been removed.

Deprecated APIs and options
===========================

* Audio Codec

  * The :c:struct:`audio_codec_api` struct has been deprecated. Audio codec drivers are now
    expected to use the :c:macro:`DEVICE_API` macro to declare their driver API.

* Build system

  * The ``zephyr_file_copy()`` CMake function has been deprecated. Use the native
    ``file(COPY_FILE ...)`` CMake command instead.

* CPU Load

  * :kconfig:option:`CONFIG_CPU_LOAD_METRIC` and :c:func:`cpu_load_metric_get` are deprecated. The
    CPU load metric module has been merged into the unified :ref:`cpu_load` module; use
    :kconfig:option:`CONFIG_CPU_LOAD` with the
    :kconfig:option:`CONFIG_CPU_LOAD_BACKEND_RUNTIME_STATS` backend and :c:func:`cpu_load_get_cpu`.

* :abbr:`DMIC (Digital Microphone Interface)`

  * The :c:struct:`_dmic_ops` struct has been deprecated. DMIC drivers are now expected to use the
    :c:macro:`DEVICE_API` macro to declare their driver API.

* Fuel Gauge

  * Deprecated various fuel gauge property enums and union fields in favor of
    new versions with explicit unit suffixes.

* LoRa

  * Renamed :c:func:`lora_recv_duty_cycle` to :c:func:`lora_recv_duty_cycle_async`
    to be consistent with the existing sync/async naming convention.

* Nordic

  * The internal SoC platform Kconfig symbols ``NRF_PLATFORM_HALTIUM`` and
    ``NRF_PLATFORM_LUMOS`` have been deprecated. Use specific SOC_SERIES_* Kconfig options instead.

  * The sysbuild Kconfig option ``SB_CONFIG_NRF_HALTIUM_GENERATE_UICR`` has
    been renamed to :kconfig:option:`SB_CONFIG_NRF_GENERATE_UICR`.

  * The Nordic SoC headers :file:`<haltium_power.h>` and :file:`<haltium_pm_s2ram.h>`
    have been renamed to :file:`<soc_power.h>` and :file:`<soc_pm_s2ram.h>` respectively.

* Ring buffer

  * The ring buffer item API (:c:func:`ring_buf_item_init`, :c:func:`ring_buf_item_put`,
    :c:func:`ring_buf_item_get`, :c:func:`ring_buf_item_space_get`) has been deprecated in favor of
    :c:struct:`sys_ringq` (see :ref:`fixed_size_ringq_api`).

* Networking

  * Deprecated LLMNR support (:kconfig:option:`CONFIG_LLMNR_RESOLVER` and
    :kconfig:option:`CONFIG_LLMNR_RESPONDER`). LLMNR is being phased out; use
    mDNS (:kconfig:option:`CONFIG_MDNS_RESOLVER` /
    :kconfig:option:`CONFIG_MDNS_RESPONDER`) instead.

* Networking Link layer

  * Deprecated :kconfig:option:`CONFIG_NET_L2_PTP`.
    Used :kconfig:option:`CONFIG_NET_L2_PTP_TIMESTAMPING` instead.

* Timer

  * New :c:func:`sys_clock_no_timeout` hook for handling of
    :kconfig:option:`CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE`, replacing the call to
    :c:func:`sys_clock_set_timeout` with ``ticks=K_TICKS_FOREVER``.
  * New :c:func:`sys_clock_idle_enter` hook for handling of entry in low-power state,
    replacing the call to :c:func:`sys_clock_set_timeout` with ``idle=true``.

* Video

  * All functions in the video driver API (``<zephyr/drivers/video.h>``) have moved to the video
    subsystem (``<zephyr/video/video.h>``). Application only need to rename the ``#include``.

* Work queue

  * :c:member:`k_work_q.thread` has been deprecated. Use :c:member:`k_work_q.thread_id` instead.

New APIs and options
====================
..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w) ignorecase

* ADC

  * Optional :c:member:`adc_driver_api.ref_get` callback and
    :c:func:`adc_ref_get` so applications and
    :c:func:`adc_raw_to_millivolts_dt` can use a driver-owned runtime
    millivolt scale for any :c:enum:`adc_reference`. Static
    :c:member:`adc_driver_api.ref_internal` remains the fallback for
    :c:enumerator:`ADC_REF_INTERNAL` when the callback is NULL.
    :c:func:`adc_raw_to_millivolts_dt` falls back to channel DT
    ``zephyr,vref-mv`` when :c:func:`adc_ref_get` fails.

* Architectures

  * :kconfig:option:`CONFIG_ARM_MPU_CM7_UNMAPPED_REGION` (Arm Cortex-M7 catch-all MPU region
    for unmapped addresses, erratum 1013783 workaround)
  * :kconfig:option:`CONFIG_EXCEPTION_DUMP` (enabled by default, can be disabled to compile
    out the fault handler output on size constrained builds)

* Audio

  * :c:member:`pcm_stream_cfg.gain_db`
  * :c:struct:`audio_codec_eq_cfg`

* Bluetooth

  * Audio

    * :c:func:`bt_ascs_register`
    * :c:func:`bt_ascs_unregister`
    * :c:func:`bt_bap_unicast_client_qos_from_group`
    * :c:func:`bt_bap_qos_cfg_eq`

  * Host

    * :c:func:`bt_conn_take`
    * :c:func:`bt_conn_drop`
    * :c:func:`bt_le_per_adv_update_did`
    * :c:member:`bt_le_adv_param.tx_power` and :c:enumerator:`BT_LE_ADV_OPT_TX_POWER`
      to request a specific TX power level per extended advertising set.
    * :c:member:`bt_conn_cb.le_param_update_rejected`
    * ``BT_HCI_QUIRK_NO_FLOW_CONTROL`` HCI device quirk for controllers that
      advertise but reject the controller to host flow control commands.

  * Mesh

    * :c:struct:`bt_mesh_lpn_timing`
    * :c:func:`bt_mesh_stat_lpn_timing_get`
    * :c:func:`bt_mesh_stat_lpn_timing_reset`
    * :kconfig:option:`CONFIG_BT_MESH_LPN_OFFER_WAIT_TIMEOUT`

* Crypto

  * :c:enumerator:`CRYPTO_CIPHER_MODE_CFB`
  * :c:enumerator:`CRYPTO_CIPHER_MODE_OFB`
  * :c:func:`cipher_cfb_op`
  * :c:func:`cipher_ofb_op`

* Devicetree

  * :c:macro:`DT_IRQN_BY_NAME`
  * :c:macro:`DT_INST_IRQN_BY_NAME`

* Haptics

  * :c:enumerator:`haptics_monitor`
  * :c:enumerator:`haptics_monitor_type`
  * :c:enumerator:`haptics_source`
  * :c:union:`haptics_config`
  * :c:func:`haptics_calibrate`
  * :c:func:`haptics_monitor_get`
  * :c:func:`haptics_monitor_set`
  * :c:func:`haptics_select_source`
  * :c:func:`haptics_set_level`
  * :c:func:`haptics_stream_samples`

* HWSPINLOCK

  * :c:macro:`HWSPINLOCK_SPINLOCK_ARRAY_DT_DEFINE`
  * :c:macro:`HWSPINLOCK_SPINLOCK_ARRAY_DT_INST_DEFINE`
  * :c:macro:`HWSPINLOCK_COMMON_CONFIG_FROM_DT_NODE`
  * :c:macro:`HWSPINLOCK_COMMON_CONFIG_FROM_DT_INST`

* Kconfig

  * Add ``dt_partition_mtd`` preprocessor function (:github:`111599`)

* Kernel

  * :c:func:`k_thread_runtime_stats_is_enabled`
  * :c:func:`atomic_test_and_set_bit_to`
  * :c:macro:`K_MSGQ_DEFINE_STATIC`
  * :c:macro:`K_MSGQ_DEFINE_TYPE`
  * :c:macro:`K_MSGQ_DEFINE_STATIC_TYPE`

* LoRa

  * :c:func:`lora_recv_duty_cycle`
  * :c:func:`lora_recv_duty_cycle_async`

* Management

  * MCUmgr

    * Added support for SPI MCUmgr SMP transport, which can be enabled with
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_SPI`.

* Network

  * Add :c:func:`net_eth_set_if_type_wifi` to set the ethernet interface type to Wi-Fi.
  * Add :c:func:`net_dhcpv4_set_reboot_hint` to seed the DHCPv4 client with a
    previously leased address for INIT-REBOOT.
  * Add an mDNS responder interface policy
    (:kconfig:option:`CONFIG_MDNS_RESPONDER_IFACE_POLICY_ALLOWLIST`,
    :kconfig:option:`CONFIG_MDNS_RESPONDER_IFACE_POLICY_DENYLIST`) together with
    :kconfig:option:`CONFIG_MDNS_RESPONDER_IFACE_LIST` to control on which
    network interfaces the mDNS responder operates.
  * Add :c:func:`mdns_responder_enable_iface` and
    :c:func:`mdns_responder_disable_iface`
    (:kconfig:option:`CONFIG_MDNS_RESPONDER_RUNTIME_IFACE_CONTROL`) to enable or
    disable the mDNS responder on a network interface at runtime.
  * Add a DHCPv6 server with IPv6 prefix delegation support
    (:kconfig:option:`CONFIG_NET_DHCPV6_SERVER`):
    :c:func:`net_dhcpv6_server_start`, :c:func:`net_dhcpv6_server_stop` and
    :c:func:`net_dhcpv6_server_foreach_lease`.
  * Add IPv6 router role, that is, transmission of Router Advertisements
    (:kconfig:option:`CONFIG_NET_IPV6_ND_RA_TX`):
    :c:func:`net_if_ipv6_router_start`, :c:func:`net_if_ipv6_router_stop` and
    :c:func:`net_if_ipv6_prefix_set_advertise`.
  * Add requesting router support to the DHCPv6 client, a delegated prefix can
    be sub-delegated onto downstream links via
    :c:member:`net_dhcpv6_params.downstream_ifaces`.
  * Add :c:func:`net_eth_mcast_addr_add`, :c:func:`net_eth_mcast_addr_rm` and
    :c:func:`net_eth_mcast_addr_foreach`. The Ethernet L2 now keeps track of the
    link layer multicast addresses that an interface listens to, so an Ethernet
    driver is asked to change its receive filter only when a group is joined by
    its first user or left by its last one. Previously the IP level joins and
    the packet socket memberships were forwarded to the driver separately, and
    leaving one group could stop the device from listening to another group that
    needs the same link layer address. This happens easily as IPv4 multicast
    addresses map 32:1 to link layer addresses. A driver can also treat
    ``ETHERNET_CONFIG_TYPE_FILTER`` as a hint that the addresses changed and
    reprogram its filter by iterating them with
    :c:func:`net_eth_mcast_addr_foreach`, which suits devices that filter by a
    hash of the address. How many addresses an interface can track is the sum
    of what the enabled subsystems ask for, and
    :kconfig:option:`CONFIG_NET_L2_ETHERNET_MCAST_FILTER_COUNT` can raise it if
    an application needs more. A multicast destination
    address given to :c:func:`net_eth_mac_filter` or to
    ``NET_REQUEST_ETHERNET_SET_MAC_FILTER`` is counted the same way, so each
    such filter that an application sets must now also be unset by it, and
    unsetting one that was never set fails with ``-ENOENT``.
  * Add ``ZSOCK_PACKET_ADD_MEMBERSHIP`` and ``ZSOCK_PACKET_DROP_MEMBERSHIP``
    socket options at the ``ZSOCK_SOL_PACKET`` level
    (:kconfig:option:`CONFIG_NET_SOCKETS_PACKET_MCAST_MEMBERSHIP`), so that a
    packet socket can ask the network interface to start or stop listening to
    an extra L2 multicast address. On Ethernet the address is programmed to the
    receive filter of the device if it supports filtering, and a device that
    does not filter passes the group up anyway. A join that the interface
    cannot serve is reported to the application, ``ENOMEM`` if the interface
    cannot track another address and ``ENOTSUP`` if it is not an Ethernet
    interface. The membership changes are reported by the
    :c:macro:`NET_EVENT_PACKET_MCAST_MEMBERSHIP_ADD` and
    :c:macro:`NET_EVENT_PACKET_MCAST_MEMBERSHIP_DROP` network management events.
    Memberships still held when the socket is closed are dropped automatically,
    and :kconfig:option:`CONFIG_NET_SOCKETS_PACKET_MCAST_MEMBERSHIP_COUNT` sets
    how many memberships can be active at the same time.

* Power Management

  * :c:macro:`LOG_DBG_PM_DEVICE_RUNTIME_GET`
  * :c:macro:`LOG_WRN_PM_DEVICE_RUNTIME_GET`
  * :c:macro:`LOG_ERR_PM_DEVICE_RUNTIME_GET`
  * :c:macro:`LOG_DBG_PM_DEVICE_RUNTIME_PUT`
  * :c:macro:`LOG_WRN_PM_DEVICE_RUNTIME_PUT`
  * :c:macro:`LOG_ERR_PM_DEVICE_RUNTIME_PUT`
  * :c:macro:`LOG_INST_DBG_PM_DEVICE_RUNTIME_GET`
  * :c:macro:`LOG_INST_WRN_PM_DEVICE_RUNTIME_GET`
  * :c:macro:`LOG_INST_ERR_PM_DEVICE_RUNTIME_GET`
  * :c:macro:`LOG_INST_DBG_PM_DEVICE_RUNTIME_PUT`
  * :c:macro:`LOG_INST_WRN_PM_DEVICE_RUNTIME_PUT`
  * :c:macro:`LOG_INST_ERR_PM_DEVICE_RUNTIME_PUT`

* Ring buffer

  * :c:struct:`sys_ringq` (see :ref:`fixed_size_ringq_api`)

* Zbus

  * :kconfig:option:`CONFIG_ZBUS_RUNTIME_CHANNEL_REGISTRATION`
  * :c:func:`zbus_runtime_channel_init`
  * :c:func:`zbus_runtime_channel_register`
  * :c:func:`zbus_runtime_channel_unregister`

.. zephyr-keep-sorted-stop

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

* Arduino

  * :zephyr:board:`Arduino Nesso N1 <arduino_nesso_n1>` (``arduino_nesso_n1``)

* Seeed

  * :zephyr:board:`Seeed Wio Tracker L1 <wio_tracker_l1>` (``wio_tracker_l1``)

* WCH

  * :zephyr:board:`WCH CH32V103EVT <ch32v103evt>` (``ch32v103evt``)

New Shields
***********

..
  Same as above, this will also be recomputed at the time of the release.


New Drivers
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* ADC

  * Analog Devices AD4190-8 and AD4195-8 Sigma-Delta ADCs
    (:dtcompatible:`adi,ad4190-8-adc`, :dtcompatible:`adi,ad4195-8-adc`).

* GPIO

  * Diodes/Pericom PI4IOE5V6408 8-bit I2C-bus I/O expander
    (:dtcompatible:`diodes,pi4ioe5v6408`).
  * ST Zio connector for STM32 Nucleo-144 boards
    (:dtcompatible:`st-zio-header`).

* Input

  * VIRTIO input device (:dtcompatible:`virtio,input`).

* Sensors

  * Analog Devices ADXL313 3-axis accelerometer (:dtcompatible:`adi,adxl313`).

* Clock Monitor

  * :dtcompatible:`nxp,cmu-fc` — NXP Clock Monitoring Unit (Frequency Check)
    back-end for the new :ref:`clock_monitor_api` subsystem.
  * :dtcompatible:`nxp,cmu-fm` — NXP Clock Monitoring Unit (Frequency Meter)
    back-end for the new :ref:`clock_monitor_api` subsystem.

* USB

  * :dtcompatible:`espressif,esp32-usb-otg-fs` - Espressif USB-OTG full-speed
    controller with internal FS/LS PHY.
  * :dtcompatible:`espressif,esp32-usb-otg-hs` - Espressif USB-OTG high-speed
    controller with internal UTMI PHY.

New Samples
***********

..
  Same as above, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

* :zephyr:code-sample:`mctp_i2c_bus_host` (renamed from ``mctp_i2c_bus_owner``)
* :zephyr:code-sample:`mctp_i3c_bus_host` (renamed from ``mctp_i3c_bus_owner``)
* ``samples/drivers/clock_monitor/check_freq`` — demonstrates WINDOW-mode
  out-of-window frequency checking on the new :ref:`clock_monitor_api`.
* ``samples/drivers/clock_monitor/measure_freq`` — demonstrates MEASURE-mode
  one-shot frequency measurement on the new :ref:`clock_monitor_api`.

Libraries / Subsystems
**********************

* Crypto

  * Added AES CFB and OFB cipher mode support.

* Mbed TLS

  * Mbed TLS was updated to version 4.1.1. Release notes can be found
    `here <https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-4.1.1>`_.

  * TF-PSA-Crypto was updated to version 1.1.1. Release notes can be found
    `here <https://github.com/Mbed-TLS/TF-PSA-Crypto/releases/tag/tf-psa-crypto-1.1.1>`_.

  * Added :kconfig:option:`CONFIG_TF_PSA_CRYPTO_DISPATCH_DIR`, which enables TF-PSA-Crypto to use
    custom implementations of crypto operation dispatch. This makes hardware acceleration of
    cryptographic operations possible by using an accelerator-aware dispatch implementation.

* TF-M

  * TF-M was updated from version 2.2.2 to version 2.3.0. Release notes can be
    found `here <https://trustedfirmware-m.readthedocs.io/en/tf-mv2.3.0/releases/2.3.0.htm>`_.

  * TF-M can now be compiled using LLVM by setting ``ZEPHYR_TOOLCHAIN_VARIANT``
    to ``zephyr/llvm``.

* DFU

  * Added :kconfig:option:`CONFIG_IMG_CUSTOM_SECTOR_SIZE` to allow MCUboot to use a different
    sector size for reducing the swap-using-offset status area size.

* LoRa / LoRaWAN

  * Added a native LoRaWAN backend
    (:kconfig:option:`CONFIG_LORA_MODULE_BACKEND_NATIVE`) that implements
    LoRaWAN 1.0.x Class A directly on top of the LoRa radio driver, without
    the Semtech LoRaMac-node dependency.  Currently supports the EU868 region.
  * :c:member:`lora_modem_config.sync_word`

* Video

  * Introducing a video subsystem that inherits all the function names previously in
    video drivers.

* Zbus

  * :kconfig:option:`CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_ISOLATION` now works without requiring
    a dedicated pool on every channel (channels fall back to the shared pool until
    :c:func:`zbus_chan_set_msg_sub_pool` is called)

Devicetree
**********
* Nodes can now use phandles to refer to their children without causing a cycle in the
  dependency graph and a build error. See :ref:`dt-bindings-dependency-mode` how to
  use this new feature. (:github:`108892`)

  * :c:macro:`DT_NODELABEL_C_TOKEN`
  * :c:macro:`DT_NODELABEL_C_TOKEN_BY_IDX`

Other notable changes
*********************

* Build system

  * The minimum required CMake version has been raised to 3.28.0, a version satisfied by the CMake package in the
    Ubuntu 24.04 LTS package repositories. See the :ref:`migration guide <migration_4.5>` for
    options if your distribution ships an older version.

* Kernel

  * :kconfig:option:`CONFIG_SCHED_CPU_MASK` no longer depends on
    :kconfig:option:`CONFIG_SCHED_SIMPLE`.  CPU affinity masks are now
    supported on all three scheduler backends: ``SCHED_SIMPLE`` (O(N) list
    scan), ``SCHED_SCALABLE`` (O(N) red/black tree walk), and ``SCHED_MULTIQ``
    (O(P·N) per-priority bucket scan).  See the updated
    :ref:`SMP documentation<smp_cpu_mask>` for per-backend performance notes.

  * :kconfig:option:`CONFIG_SCHED_CPU_MASK_PIN_ONLY` now enforces the
    one-CPU-bit invariant at both the API boundary (``cpu_mask_mod()``) and at
    queue time (``thread_runq()``).  Calling :c:func:`k_thread_cpu_mask_clear`,
    :c:func:`k_thread_cpu_mask_enable_all`, or
    :c:func:`k_thread_cpu_mask_disable` in PIN_ONLY mode triggers an assertion
    failure.  Use :c:func:`k_thread_cpu_pin` to reassign a thread to a
    different CPU.

* Timer

  * With :kconfig:option:`CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE` enabled, a driver may no
    longer stop its time base as soon as no timeout is pending, if that breaks
    :c:func:`sys_clock_cycle_get_32` / :c:func:`sys_clock_cycle_get_64`. Those must
    keep counting while the CPU runs. Stopping the time base is permitted only from
    :c:func:`sys_clock_idle_enter`, where :c:func:`sys_clock_idle_exit` is
    guaranteed to follow.

  * Tickless system-timer drivers can now be built on a shared implementation
    header, :file:`drivers/timer/system_timer_generic.h`, which owns the tick
    accounting each driver previously open-coded: the cycle-to-tick conversion,
    the announce baseline, the tick-aligned deadline and the counter wrap and
    range handling. A driver reduces to a few cycle-domain primitives, a
    cycle-counter read plus an absolute-compare arm. See the
    :ref:`migration guide <migration_4.5>` for how to use it (:github:`115844`).

* Wi-Fi

  * Removed the ``samples/net/wifi/test_certs/rsa2k`` enterprise test
    certificates (DES-encrypted private keys). Use ``rsa2k_no_des`` instead.

  * The transmit power ceiling properties in ``wifi-tx-power-2g.yaml`` and
    ``wifi-tx-power-5g.yaml`` are no longer ``required`` and now carry
    conservative defaults, so a board that has not been characterised errs on
    the side of transmitting too little rather than exceeding a regulatory
    limit. Boards that have measured their own limits continue to state them
    explicitly, so no board changes behaviour.

* MCUboot

  * :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` now accepts a comma-separated list of
    key files, embedding the public half of each in the MCUboot bootloader. When more
    than one key is given, MCUboot accepts an image signed with any of them -- the
    typical use is a development bootloader that boots both development- and
    production-signed images, while production bootloaders embed only the production
    key. The first entry is the key the application is signed with and the rest are
    verification-only public keys. See :ref:`build-signing`.

* NXP

  * The NXP LPC DTSI files have been reorganized from the flat
    ``dts/arm/nxp/lpc/`` directory into per-series subdirectories
    (``lpc11u6x/``, ``lpc51u68/``, ``lpc54xxx/``, ``lpc55xxx/``, ``lpc84x/``).
    See the :ref:`migration guide <migration_4.5>` for how to update out-of-tree
    board includes.

  * The NXP Kinetis DTSI files have been reorganized from the flat
    ``dts/arm/nxp/kinetis/`` directory into per-series subdirectories
    (``k2x/``, ``k32lx/``, ``k6x/``, ``k8x/``, ``ke1xf/``, ``ke1xz/``,
    ``kl2x/``, ``kv5x/``, ``kwx/``).
    See the :ref:`migration guide <migration_4.5>` for how to update
    out-of-tree board includes.

  * The NXP MCX DTSI files have been reorganized from the flat
    ``dts/arm/nxp/mcx/`` directory into per-series subdirectories
    (``mcxa/``, ``mcxc/``, ``mcxe/``, ``mcxl/``, ``mcxn/``, ``mcxw/``).
    See the :ref:`migration guide <migration_4.5>` for how to update
    out-of-tree board includes.

  * The NXP i.MX RT DTSI files have been reorganized from the flat
    ``dts/arm/nxp/imxrt/`` directory into per-series subdirectories
    (``imxrt10xx/``, ``imxrt11xx/``, ``imxrt5xx/``, ``imxrt6xx/``,
    ``imxrt7xx/``, ``imxrt118x/``).
    See the :ref:`migration guide <migration_4.5>` for how to update
    out-of-tree board includes.

* Arm

  * The non-secure variant of
      :zephyr:board:`Arm Musca-S1 <v2m_musca_s1>` (``v2m_musca_s1/musca_s1/ns``)
      has been removed due to TF-M removing platform support for this board.

  * As a consequence of the above, the secure variant of
    :zephyr:board:`Arm Musca-S1 <v2m_musca_s1>` (``v2m_musca_s1``) has been deprecated.
    This is to avoid a confusing state of partial support.

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?

Trusted Firmware-A
******************

* ``CONFIG_TFA_BUILD_FIP`` is introduced to configure FIP (Firmware Image Package) generation.
  FIP generation is by default disabled, but can be enabled by setting ``CONFIG_TFA_BUILD_FIP=y``
  in ``prj.conf`` or for custom boards, in the board's ``<board>_defconfig`` file.
