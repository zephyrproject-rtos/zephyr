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

   * ARM

      * ``CONFIG_PLATFORM_SPECIFIC_INIT``
      * ``z_arm_platform_init()``

   * RISC-V

      * ``CONFIG_EXTRA_EXCEPTION_INFO``

   * x86

      * ``CONFIG_SSE``
      * ``CONFIG_SSE_FP_MATH``

   * Xtensa

      * ``CONFIG_XTENSA_BACKTRACE_EXCEPTION_DUMP_HOOK``

* Bluetooth

  * Controller

    * ``CONFIG_BT_CTRL_ADV_ADI_IN_SCAN_RSP``

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

    * ``CONFIG_BT_AUTO_PHY_UPDATE``, replaced by the ``BT_AUTO_PHY_CENTRAL`` and
      ``BT_AUTO_PHY_PERIPHERAL`` choices
    * ``_bt_gatt_ccc``
    * ``BT_GATT_CCC_INITIALIZER``
    * ``CONFIG_BT_CONN_TX_MAX``
    * ``CONFIG_BT_FIXED_PASSKEY``
    * ``bt_passkey_set()``
    * ``BT_PASSKEY_INVALID``

  * Mesh

    * ``CONFIG_BT_MESH_BLOB_IO_FLASH_WITH_ERASE``
    * ``CONFIG_BT_MESH_BLOB_IO_FLASH_WITHOUT_ERASE``

  * Services

    * ``CONFIG_BT_DIS_MANUF``
    * ``CONFIG_BT_DIS_MODEL``

* Boards

    * Dropped the following deprecated board aliases:

      * ``arduino_uno_r4_minima``
      * ``arduino_uno_r4_wifi``
      * ``esp32c6_devkitc``
      * ``esp32_devkitc_wroom/esp32/procpu``
      * ``esp32_devkitc_wroom/esp32/appcpu``
      * ``esp32_devkitc_wrover/esp32/procpu``
      * ``esp32_devkitc_wrover/esp32/appcpu``
      * ``neorv32``
      * ``panb511evb``
      * ``raytac_an54l15q_db/nrf54l15/cpuapp``
      * ``scobc_module1``
      * ``xiao_esp32c6``

* Build system

    * ``CONFIG_BUILD_NO_GAP_FILL``
    * ``cmake/app/boilerplate.cmake``
    * Board revision Kconfig fragments named ``<board>_<revision>.conf``, replaced by
      ``<board>_<revision>_defconfig``
    * Pattern expansion in ``zephyr_code_relocate(FILES ...)``, replaced by ``file(GLOB ...)``
    * The CMake ``flash``, ``debug``, ``debugserver``, ``attach`` and ``rtt`` targets,
      replaced by the corresponding ``west`` commands
    * The ``WEST_DIR`` build system variable

* CAN

    * ``bus-speed``
    * ``bus-speed-data``

* Comparator

    * ``nxp,enable-output-pin``, ``nxp,use-unfiltered-output``, ``nxp,high-speed-mode``,
      ``nxp,enable-sample``, ``nxp,filter-count``, ``nxp,filter-period`` and ``nxp,window-mode``
      properties of :dtcompatible:`nxp,kinetis-acmp`

* Counter

    * ``CONFIG_COUNTER_MAXIM_DS3231``
    * ``prescaler`` property of :dtcompatible:`nxp,lptmr`

* hawkBit

    * ``<zephyr/mgmt/hawkbit.h>``

* LLEXT

    * ``llext_get_fn_table``, replaced by ``llext_get_fn_table_entry``

* Mbed TLS

    * ``CONFIG_MBEDTLS_MD``
    * ``CONFIG_MBEDTLS_LMS``
    * ``CONFIG_MBEDTLS_TLS_VERSION_1_2``
    * ``CONFIG_MBEDTLS_DTLS``
    * ``CONFIG_MBEDTLS_TLS_VERSION_1_3``
    * ``CONFIG_MBEDTLS_TLS_SESSION_TICKETS``
    * ``CONFIG_MBEDTLS_CTR_DRBG_ENABLED``
    * ``CONFIG_MBEDTLS_HMAC_DRBG_ENABLED``

* MCUboot

    * ``CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_WITHOUT_SCRATCH``, replaced by
      :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_USING_MOVE`

* MCUmgr

    * ``CONFIG_MCUMGR_GRP_OS_INFO_HARDWARE_INFO_SHORT_HARDWARE_PLATFORM``

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

* Nordic

    * ``owner-id``, ``perm-read``, ``perm-write``, ``perm-execute``, ``perm-secure`` and
      ``non-secure-callable`` properties of :dtcompatible:`nordic,owned-memory` and
      :dtcompatible:`nordic,owned-partitions`

* Random

    * ``CONFIG_CTR_DRBG_CSPRNG_GENERATOR``
    * ``CONFIG_CS_CTR_DRBG_PERSONALIZATION``

* SPI

    * The optional delay argument of :c:macro:`SPI_CONFIG_DT`, :c:macro:`SPI_CONFIG_DT_INST`,
      :c:macro:`SPI_DT_SPEC_GET`, :c:macro:`SPI_DT_SPEC_INST_GET`, :c:macro:`SPI_DT_IODEV_DEFINE`,
      :c:macro:`SPI_DT_INST_IODEV_DEFINE` and :c:macro:`SPI_CS_CONTROL_INIT` has been removed.

* Stream Flash

    * ``stream_flash_erase_page()``

* ZTest

    * ``CONFIG_ZTEST_SHUFFLE_SUITE_REPEAT_COUNT``
    * ``CONFIG_ZTEST_SHUFFLE_TEST_REPEAT_COUNT``

* West sign support for imgtool, which was deprecated in Zephyr 4.0, has been removed.

* The ``scripts/logging/dictionary/log_parser_uart.py`` dictionary logging script, which was
  deprecated in Zephyr 4.3, has been removed. Use
  :zephyr_file:`scripts/logging/dictionary/live_log_parser.py` instead.

* The ``--skip-rebuild`` option of ``west flash``, ``west debug`` and the other commands that
  invoke a runner, which was deprecated in Zephyr 4.3, has been removed. Use ``--no-rebuild``
  instead.

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

* West

  * ``west spdx --init`` is deprecated. A build with
    :kconfig:option:`CONFIG_BUILD_OUTPUT_META` now asks CMake for the file-based API that
    ``west spdx`` reads, so the build directory no longer has to be prepared before it is
    configured. See :ref:`west-spdx`.

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
    * :c:member:`bt_bap_unicast_group_info.c_to_p_interval`
    * :c:member:`bt_bap_unicast_group_info.p_to_c_interval`
    * :c:member:`bt_bap_unicast_group_info.c_to_p_latency`
    * :c:member:`bt_bap_unicast_group_info.p_to_c_latency`
    * :c:member:`bt_bap_unicast_group_info.framing`
    * :c:member:`bt_bap_unicast_group_info.packing`
    * :c:member:`bt_bap_unicast_group_info.has_been_connected`
    * :c:member:`bt_bap_unicast_group_info.c_to_p_ft`
    * :c:member:`bt_bap_unicast_group_info.p_to_c_ft`
    * :c:member:`bt_bap_unicast_group_info.iso_interval`

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
  * :c:func:`k_sleep_ticks`
  * Namespaced equivalents of the interrupt control APIs, preferred for new
    code; the unprefixed names remain fully supported:
    :c:func:`k_irq_lock`, :c:func:`k_irq_unlock`, :c:func:`k_irq_enable`,
    :c:func:`k_irq_disable`, :c:func:`k_irq_is_enabled`,
    :c:func:`k_irq_connect_dynamic` and :c:func:`k_irq_disconnect_dynamic`

* LoRa

  * :c:func:`lora_recv_duty_cycle`
  * :c:func:`lora_recv_duty_cycle_async`
  * :c:func:`lora_energy_detect`
  * :c:func:`lora_rssi`

* Management

  * MCUmgr

    * Added support for SPI MCUmgr SMP transport, which can be enabled with
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_SPI`.

    * Added experimental :ref:`transport management group<mcumgr_smp_group_11>`:
      :kconfig:option:`CONFIG_MCUMGR_GRP_TRANSPORT`,
      :kconfig:option:`CONFIG_MCUMGR_GRP_TRANSPORT_LOCKING`,
      :kconfig:option:`CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES`,
      :kconfig:option:`CONFIG_MCUMGR_GRP_TRANSPORT_GROUP_ID_DEFAULT`,
      :kconfig:option:`CONFIG_MCUMGR_GRP_TRANSPORT_GROUP_ID_CUSTOM_VALUE`,
      :kconfig:option:`CONFIG_MCUMGR_GRP_TRANSPORT_GROUP_ID_CUSTOM_VALUE_GROUP_ID`,
      :kconfig:option:`CONFIG_MCUMGR_GRP_TRANSPORT_GROUP_ID_CUSTOM_FUNCTION` and
      :kconfig:option:`CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS`.

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

* Pulse IO

  * Added the :ref:`Pulse IO <pulse_io_api>` subsystem, a vendor-neutral
    API for hardware that generates and captures timed digital edges on a
    GPIO line.

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

* Adafruit Industries, LLC

  * :zephyr:board:`adafruit_feather_esp32c6` (``adafruit_feather_esp32c6``)
  * :zephyr:board:`adafruit_trrs_trinkey` (``adafruit_trrs_trinkey``)

* Advanced Micro Devices (AMD), Inc.

  * :zephyr:board:`acp_7_0_adsp` (``acp_7_0_adsp``)
  * :zephyr:board:`acp_7_x_adsp` (``acp_7_x_adsp``)
  * :zephyr:board:`zynqmp_apu` (``zynqmp_apu``)
  * :zephyr:board:`zynqmp_rpu` (``zynqmp_rpu``)

* Aesc Silicon

  * :zephyr:board:`elemrv_flask_h` (``elemrv_flask_h``)

* Ai-Thinker Co., Ltd.

  * :zephyr:board:`ai_m64p_32s_kit` (``ai_m64p_32s_kit``)
  * :zephyr:board:`aipi_eyes_s2` (``aipi_eyes_s2``)

* Alif Semiconductor

  * :zephyr:board:`balletto_b1_dk` (``balletto_b1_dk``)
  * :zephyr:board:`ensemble_e8_ak` (``ensemble_e8_ak``)

* Analog Devices, Inc.

  * :zephyr:board:`max32651evkit` (``max32651evkit``)

* Antmicro

  * :zephyr:board:`stm32h7_hdmi_board` (``stm32h7_hdmi_board``)
  * :zephyr:board:`stm32h7_renode_reference_board` (``stm32h7_renode_reference_board``)

* Arduino

  * :zephyr:board:`arduino_nano_connect` (``arduino_nano_connect``)
  * :zephyr:board:`arduino_nesso_n1` (``arduino_nesso_n1``)

* ARM Ltd.

  * :zephyr:board:`fvp_corstone1000` (``fvp_corstone1000``)

* Armfly

  * :zephyr:board:`armfly_stm32h743xih6` (``armfly_stm32h743xih6``)

* Barth Elektronik GmbH

  * :zephyr:board:`stg_800` (``stg_800``)

* BeagleBoard.org Foundation

  * :zephyr:board:`beaglebadge` (``beaglebadge``)
  * :zephyr:board:`beagleconnect_zepto` (``beagleconnect_zepto``)
  * :zephyr:board:`pocketbeagle_2_industrial` (``pocketbeagle_2_industrial``)

* BLIIoT Technology Co., Ltd.

  * :zephyr:board:`am62x_m4_bl350` (``am62x_m4_bl350``)

* Bouffalo Lab (Nanjing) Co., Ltd.

  * :zephyr:board:`bl618g0` (``bl618g0``)

* CAN-module, FOP

  * :zephyr:board:`canbridge_g473` (``canbridge_g473``)
  * :zephyr:board:`usbcan_iso` (``usbcan_iso``)
  * :zephyr:board:`usbcanfd_dual` (``usbcanfd_dual``)
  * :zephyr:board:`usbcanfd_solo` (``usbcanfd_solo``)

* Chengdu Ebyte Electronic Technology

  * :zephyr:board:`e80_900mbl_01` (``e80_900mbl_01``)
  * :zephyr:board:`eora_hub_900tb` (``eora_hub_900tb``)

* Chengdu Heltec Automation Technology Co., Ltd.

  * :zephyr:board:`heltec_t114_v2` (``heltec_t114_v2``)

* Cirrus Logic, Inc.

  * :zephyr:board:`crd40l26` (``crd40l26``)

* emtrion GmbH

  * :zephyr:board:`emsbc_neon_cm7` (``emsbc_neon_cm7``)

* Espressif Systems

  * :zephyr:board:`esp32p4_function_ev_board` (``esp32p4_function_ev_board``)
  * :zephyr:board:`esp32p4x_function_ev_board` (``esp32p4x_function_ev_board``)
  * :zephyr:board:`esp32s3_box3` (``esp32s3_box3``)

* Eurovibes

  * :zephyr:board:`eurovibes_stm32g431_sertest-ng` (``eurovibes_stm32g431_sertest-ng``)

* FlySky

  * :zephyr:board:`fs_i6s` (``fs_i6s``)

* Heimann Sensor GmbH

  * :zephyr:board:`htpa_eval` (``htpa_eval``)

* Intel Corporation

  * :zephyr:board:`intel_nvl_s_rvp` (``intel_nvl_s_rvp``)

* Jhoinrch

  * :zephyr:board:`rh02` (``rh02``)
  * :zephyr:board:`rh02_plus_2026` (``rh02_plus_2026``)

* KAGA FEI Co., Ltd.

  * :zephyr:board:`ec4l15ba1` (``ec4l15ba1``)

* KinCony Electronics Co., Ltd.

  * :zephyr:board:`kincony_kc868_a8` (``kincony_kc868_a8``)

* Lilygo Shenzhen Xinyuan Electronic Technology Co., Ltd

  * :zephyr:board:`t_deck` (``t_deck``)

* M5Stack

  * :zephyr:board:`m5stack_paper_color` (``m5stack_paper_color``)
  * :zephyr:board:`m5stack_sticks3` (``m5stack_sticks3``)
  * :zephyr:board:`m5stack_unitc6l` (``m5stack_unitc6l``)

* Makerfabs

  * :zephyr:board:`matouch_mtro128g` (``matouch_mtro128g``)

* Microchip Technology Inc.

  * :zephyr:board:`m2s010_mkr_kit` (``m2s010_mkr_kit``)
  * :zephyr:board:`m2s_hello_fpga_kit` (``m2s_hello_fpga_kit``)
  * :zephyr:board:`pic32ck_gc01_cult` (``pic32ck_gc01_cult``)
  * :zephyr:board:`pic32cm_gc00_cpro` (``pic32cm_gc00_cpro``)
  * :zephyr:board:`pic32cm_sg00_cpro` (``pic32cm_sg00_cpro``)
  * :zephyr:board:`sama5d27_som1_ek1` (``sama5d27_som1_ek1``)

* MuseLab Electronics

  * :zephyr:board:`nano_ch32v317` (``nano_ch32v317``)
  * :zephyr:board:`nano_ch57x` (``nano_ch57x``)

* Nordic Semiconductor

  * :zephyr:board:`nrf93m1dk` (``nrf93m1dk``)

* Norik Systems

  * :zephyr:board:`dect_nr_plus_usb_dongle` (``dect_nr_plus_usb_dongle``)

* NUCODE Co., Ltd. (nuworks.io)

  * :zephyr:board:`nucode_nu32` (``nucode_nu32``)
  * :zephyr:board:`nucode_nu40` (``nucode_nu40``)

* Nuvoton Technology Corporation

  * :zephyr:board:`numaker_m031ki` (``numaker_m031ki``)
  * :zephyr:board:`numaker_m3351ki` (``numaker_m3351ki``)

* NXP Semiconductors

  * :zephyr:board:`frdm_imxrt1152` (``frdm_imxrt1152``)
  * :zephyr:board:`frdm_imxrt700` (``frdm_imxrt700``)
  * :zephyr:board:`imx952_evk` (``imx952_evk``)
  * :zephyr:board:`lpc845brk` (``lpc845brk``)
  * :zephyr:board:`lpcxpresso54628` (``lpcxpresso54628``)
  * :zephyr:board:`mimxrt685_aud_evk` (``mimxrt685_aud_evk``)
  * :zephyr:board:`mr_navq95b` (``mr_navq95b``)

* OLIMEX Ltd.

  * :zephyr:board:`esp32p4_pc` (``esp32p4_pc``)

* Others

  * :zephyr:board:`bl704l_dvk` (``bl704l_dvk``)
  * :zephyr:board:`esp32h2_supermini` (``esp32h2_supermini``)
  * :zephyr:board:`jz_f407vet6` (``jz_f407vet6``)
  * :zephyr:board:`stm32_debug_probe` (``stm32_debug_probe``)

* Pine64

  * :zephyr:board:`pinecone` (``pinecone``)

* QEMU

  * :zephyr:board:`qemu_cortex_a72` (``qemu_cortex_a72``)

* Radxa

  * :zephyr:board:`rock_3b` (``rock_3b``)
  * :zephyr:board:`rock_5b_plus` (``rock_5b_plus``)

* Raspberry Pi Foundation

  * :zephyr:board:`rpi_zero_2w` (``rpi_zero_2w``)

* Raytac Corporation

  * :zephyr:board:`raytac_an54lv_db_15` (``raytac_an54lv_db_15``)

* Realtek Semiconductor Corp.

  * :zephyr:board:`pke8721daf_c13_f10` (``pke8721daf_c13_f10``)

* Renesas Electronics Corporation

  * :zephyr:board:`rcar_ironhide_x5h` (``rcar_ironhide_x5h``)
  * :zephyr:board:`rza3m_ek` (``rza3m_ek``)

* Seeed Technology Co., Ltd

  * :zephyr:board:`reterminal_e1001` (``reterminal_e1001``)
  * :zephyr:board:`reterminal_e1003` (``reterminal_e1003``)
  * :zephyr:board:`wio_tracker_l1` (``wio_tracker_l1``)
  * :zephyr:board:`xiao_esp32c5` (``xiao_esp32c5``)
  * :zephyr:board:`xiao_nrf54lm20a` (``xiao_nrf54lm20a``)

* SEGGER Microcontroller GmbH

  * :zephyr:board:`nandeval_h743zi` (``nandeval_h743zi``)

* SHAKTI Processor Program

  * :zephyr:board:`nexys_ganga` (``nexys_ganga``)

* Shanghai Ruiside Electronic Technology Co., Ltd.

  * :zephyr:board:`ra8p1_titan` (``ra8p1_titan``)
  * :zephyr:board:`ra8p1_titan_mini` (``ra8p1_titan_mini``)

* Shenzhen JLC Technology Group Co., Ltd.

  * :zephyr:board:`skystar_gd32f407vet6` (``skystar_gd32f407vet6``)

* Shenzhen Luckfox Technology Co., Ltd.

  * :zephyr:board:`pico_ultra` (``pico_ultra``)

* Shenzhen Sipeed Technology Co., Ltd.

  * :zephyr:board:`m0sense` (``m0sense``)
  * :zephyr:board:`m1s_dock` (``m1s_dock``)

* Shenzhen Xunlong Software CO.,Limited

  * :zephyr:board:`opi_zero2w` (``opi_zero2w``)

* Silicon Laboratories

  * :zephyr:board:`kg100s_rb4332a` (``kg100s_rb4332a``)
  * :zephyr:board:`xg26_dk2608a` (``xg26_dk2608a``)
  * :zephyr:board:`xg26_rb4121a` (``xg26_rb4121a``)

* STMicroelectronics

  * :zephyr:board:`nucleo_g491re` (``nucleo_g491re``)
  * :zephyr:board:`nucleo_u545re_q` (``nucleo_u545re_q``)

* Sutajio Ko-Usagi PTE Ltd.

  * :zephyr:board:`tomu` (``tomu``)

* Texas Instruments

  * :zephyr:board:`lp_am13e230` (``lp_am13e230``)
  * :zephyr:board:`lp_am243` (``lp_am243``)
  * :zephyr:board:`lp_mspm33c321a` (``lp_mspm33c321a``)

* Trenz Electronic

  * :zephyr:board:`te0950` (``te0950``)

* Tuya Inc.

  * :zephyr:board:`tyzs3` (``tyzs3``)

* u-blox

  * :zephyr:board:`ubx_evknorab2` (``ubx_evknorab2``)

* Udoo

  * :zephyr:board:`udoo_key` (``udoo_key``)

* Umeta & Ikki Automotive Parts

  * :zephyr:board:`uiapduino_pro_micro_ch32v003` (``uiapduino_pro_micro_ch32v003``)

* VIEWE Display Co., Ltd.

  * :zephyr:board:`uedx24240013_md50e` (``uedx24240013_md50e``)
  * :zephyr:board:`uedx32480035e_wb_a` (``uedx32480035e_wb_a``)

* Waveshare Electronics

  * :zephyr:board:`esp32c6_lcd_1_47` (``esp32c6_lcd_1_47``)
  * :zephyr:board:`esp32p4_wifi6` (``esp32p4_wifi6``)
  * :zephyr:board:`esp32p4_wifi6_dev_kit` (``esp32p4_wifi6_dev_kit``)
  * :zephyr:board:`waveshare_esp32p4_eth` (``waveshare_esp32p4_eth``)

* WeAct Studio

  * :zephyr:board:`ch32v00x_core` (``ch32v00x_core``)
  * :zephyr:board:`usb2canfdv2` (``usb2canfdv2``)
  * :zephyr:board:`weact_ra4m1_core` (``weact_ra4m1_core``)

* WinChipHead

  * :zephyr:board:`ch32h417evt` (``ch32h417evt``)
  * :zephyr:board:`ch32v103evt` (``ch32v103evt``)
  * :zephyr:board:`ch32v203c8t6evt` (``ch32v203c8t6evt``)
  * :zephyr:board:`ch32v305f_evt_r0` (``ch32v305f_evt_r0``)

* WIZnet Co., Ltd.

  * :zephyr:board:`w55rp20_evb_pico` (``w55rp20_evb_pico``)
  * :zephyr:board:`w6100_evb_pico` (``w6100_evb_pico``)
  * :zephyr:board:`w6100_evb_pico2` (``w6100_evb_pico2``)
  * :zephyr:board:`w6300_evb_pico2` (``w6300_evb_pico2``)

New Shields
***********

..
  Same as above, this will also be recomputed at the time of the release.

* :ref:`AD-APARDPFW-SL <ad_apardpfw_sl>`
* :ref:`Adafruit FeatherWing MAX3421E Shield <adafruit_featherwing_max3421e>`
* :ref:`Analog Devices Low-Speed Mixed-Signal Playground <adi_lsmspg>`
* :ref:`ArduCam Mega SPI Camera Shield <arducam_mega>`
* :ref:`DFRobot Gravity TM6605 Haptic Motor Driver Module <dfrobot_gravity_tm6605>`
* :ref:`EVAL-AD5529R-ARDZ <eval_ad5529r_ardz>`
* :ref:`M5Stack Unit Gesture <m5stack_unit_gesture_shield>`
* :ref:`M5Stack Unit Mini OLED <m5stack_unit_minioled_shield>`
* :ref:`MB1280 STMod+ fan-out shield <mb1280_stmod_plus>`
* :ref:`MikroElektronika EERAM 3.3V Click <mikroe_eeram_33v_click_shield>`
* :ref:`MikroElektronika Two Wire ETH Click <mikroe_two_wire_eth_click_shield>`
* :ref:`NXP MX8 DSI OLED1A Panel <nxp_mx8_dsi_oled1a>`
* :ref:`NXP MX9 DSI OLED Panel <nxp_mx9_dsi_oled>`
* :ref:`OD-6010 SLCD Panel Shield <od_6010_shield>`
* :ref:`Seeed Studio COB LED Driver Board for XIAO <seeed_xiao_cob_led>`
* :ref:`ST B-M2MEM-PACK1 M.2 serial memory pack <st_b_m2mem_pack1_shield>`
* :ref:`X-NUCLEO-67W61M1: Wi-Fi 6 expansion board <x_nucleo_67w61m1>`
* :ref:`X-NUCLEO-GNSS1A1: GNSS expansion board based on Teseo-LIV3F <x-nucleo-gnss1a1>`
* :ref:`X-NUCLEO-PGEEZ1 page EEPROM expansion board <x_nucleo_pgeez1_shield>`
* :ref:`X-NUCLEO-WBA25A1: BLE expansion board <x-nucleo-wba25a1>`

New Drivers
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* :abbr:`ADC (Analog to Digital Converter)`

  * :dtcompatible:`adi,ad4190-8-adc` (:github:`111922`)
  * :dtcompatible:`adi,ad4195-8-adc` (:github:`111922`)
  * :dtcompatible:`infineon,autanalog-sar-fifo` (:github:`110289`)
  * :dtcompatible:`infineon,autanalog-sar-fir` (:github:`110289`)
  * :dtcompatible:`m5stack,m5pm1-adc` (:github:`109961`)
  * :dtcompatible:`realtek,ameba-adc` (:github:`106677`)
  * :dtcompatible:`realtek,bee-adc` (:github:`105264`)
  * :dtcompatible:`ti,adc081c021` (:github:`114289`)
  * :dtcompatible:`ti,adc081c027` (:github:`114289`)
  * :dtcompatible:`ti,adc101c021` (:github:`114289`)
  * :dtcompatible:`ti,adc101c027` (:github:`114289`)
  * :dtcompatible:`ti,adc121c021` (:github:`114289`)
  * :dtcompatible:`ti,adc121c027` (:github:`114289`)
  * :dtcompatible:`ti,ads1118` (:github:`94152`)
  * :dtcompatible:`ti,ads1220` (:github:`102479`)
  * :dtcompatible:`ti,ads7828` (:github:`114359`)
  * :dtcompatible:`ti,ads7830` (:github:`114359`)
  * :dtcompatible:`ti,mspm0-adc12` (:github:`94736`)
  * :dtcompatible:`ti,tla2528-adc` (:github:`110722`)

* ARM architecture

  * :dtcompatible:`infineon,edge-npu` (:github:`106826`)
  * :dtcompatible:`nordic,nrf-wicr` (:github:`108141`)
  * :dtcompatible:`nordic,nrf71-uicr` (:github:`106134`)

* Audio

  * :dtcompatible:`st,stm32-dfsdm` (:github:`108302`)
  * :dtcompatible:`st,stm32-dfsdm-dmic` (:github:`108302`)
  * :dtcompatible:`ti,tas2563` (:github:`103148`)
  * :dtcompatible:`ti,tlv320aic26` (:github:`106836`)
  * :dtcompatible:`wolfson,wm8960` (:github:`106212`)
  * :dtcompatible:`zephyr,dummy-codec` (:github:`109891`)
  * :dtcompatible:`zephyr,native-sim-dmic` (:github:`109898`)

* Auxiliary Display

  * :dtcompatible:`nxp,slcd` (:github:`102796`)
  * :dtcompatible:`slcd-panel` (:github:`102796`)

* Bluetooth

  * :dtcompatible:`espressif,esp-hosted-mcu-bt-hci` (:github:`114532`)
  * :dtcompatible:`realtek,ameba-bt-hci` (:github:`109287`)

* Buzzer

  * :dtcompatible:`gpio-buzzer` (:github:`108911`)
  * :dtcompatible:`pwm-buzzer` (:github:`108911`)

* :abbr:`CAN (Controller Area Network)`

  * :dtcompatible:`bflb,bl61x-can` (:github:`110672`)
  * :dtcompatible:`espressif,esp32-twaifd` (:github:`107680`)
  * :dtcompatible:`realtek,bee-can` (:github:`105411`)

* Charger

  * :dtcompatible:`adi,adp5360-charger` (:github:`105258`)
  * :dtcompatible:`silergy,sy6974b` (:github:`107439`)
  * :dtcompatible:`ti,bq24295` (:github:`114650`)
  * :dtcompatible:`ti,bq24296` (:github:`114650`)
  * :dtcompatible:`ti,bq24296m` (:github:`114650`)
  * :dtcompatible:`ti,bq24297` (:github:`114650`)
  * :dtcompatible:`ti,bq24298` (:github:`114650`)

* Clock control

  * :dtcompatible:`bflb,bl616cl-clock-controller` (:github:`112738`)
  * :dtcompatible:`bflb,bl808-clock-controller` (:github:`105580`)
  * :dtcompatible:`bflb,mm-clk` (:github:`105580`)
  * :dtcompatible:`microchip,pic32cm-sg-gc-clock` (:github:`112582`)
  * :dtcompatible:`microchip,pic32cm-sg-gc-dfll48m` (:github:`112582`)
  * :dtcompatible:`microchip,pic32cm-sg-gc-dpll` (:github:`112582`)
  * :dtcompatible:`microchip,pic32cm-sg-gc-gclkgen` (:github:`112582`)
  * :dtcompatible:`microchip,pic32cm-sg-gc-gclkperiph` (:github:`112582`)
  * :dtcompatible:`microchip,pic32cm-sg-gc-mclkdomain` (:github:`112582`)
  * :dtcompatible:`microchip,pic32cm-sg-gc-mclkperiph` (:github:`112582`)
  * :dtcompatible:`microchip,pic32cm-sg-gc-rtc` (:github:`112582`)
  * :dtcompatible:`microchip,pic32cm-sg-gc-xosc` (:github:`112582`)
  * :dtcompatible:`microchip,pic32cm-sg-gc-xosc32k` (:github:`112582`)
  * :dtcompatible:`microchip,smartfusion2-clock` (:github:`106926`)
  * :dtcompatible:`nordic,nrf-clock-hfclk` (:github:`104658`)
  * :dtcompatible:`nordic,nrf-clock-hfclk192m` (:github:`104658`)
  * :dtcompatible:`nordic,nrf-clock-hfclkaudio` (:github:`104658`)
  * :dtcompatible:`nordic,nrf-clock-lfclk` (:github:`104658`)
  * :dtcompatible:`nordic,nrf-clock-xo` (:github:`104658`)
  * :dtcompatible:`nordic,nrf-clock-xo24m` (:github:`104658`)
  * :dtcompatible:`nuvoton,m4-hclk-clock` (:github:`103668`)
  * :dtcompatible:`nuvoton,m4-hxt-clock` (:github:`103668`)
  * :dtcompatible:`nuvoton,m4-lxt-clock` (:github:`103668`)
  * :dtcompatible:`nuvoton,m4-pll-clock` (:github:`103668`)
  * :dtcompatible:`nuvoton,numicro-m4-pcc` (:github:`103668`)
  * :dtcompatible:`nuvoton,numicro-m4-scc` (:github:`103668`)
  * :dtcompatible:`nxp,imxrt118x-arm-pll` (:github:`106881`)
  * :dtcompatible:`nxp,imxrt11xx-arm-pll` (:github:`106881`)
  * :dtcompatible:`nxp,lpc84x-clock` (:github:`105928`)
  * :dtcompatible:`nxp,mcxw7x-clock` (:github:`101937`)
  * :dtcompatible:`silabs,series0-cmu` (:github:`111754`)
  * :dtcompatible:`silabs,series0-hfxo` (:github:`111754`)
  * :dtcompatible:`silabs,series0-lfrco` (:github:`111754`)
  * :dtcompatible:`silabs,series0-lfxo` (:github:`111754`)
  * :dtcompatible:`st,stm32h5-pll-clock` (:github:`110914`)
  * :dtcompatible:`st,stm32n6-msi-clock` (:github:`108997`)
  * :dtcompatible:`wch,ch32h41x-pll-clock` (:github:`111725`)

* Clock monitor

  * :dtcompatible:`nxp,cmu-fc` (:github:`107879`)
  * :dtcompatible:`nxp,cmu-fm` (:github:`107879`)
  * :dtcompatible:`nxp,freqme` (:github:`112403`)

* Comparator

  * :dtcompatible:`espressif,esp32-ana-cmpr` (:github:`113625`)
  * :dtcompatible:`infineon,autanalog-ptcomp-comp` (:github:`107488`)
  * :dtcompatible:`infineon,hppass-csg-comp` (:github:`109879`)
  * :dtcompatible:`infineon,lp-comp` (:github:`104636`)
  * :dtcompatible:`infineon,lp-comp-channel` (:github:`104636`)
  * :dtcompatible:`ti,mspm0-comparator` (:github:`94737`)

* Counter

  * :dtcompatible:`arm,crsas-ma2-counter` (:github:`112815`)
  * :dtcompatible:`arm,crsas-ma2-timer` (:github:`112815`)
  * :dtcompatible:`nxp,irtc-wake-timer` (:github:`111552`)
  * :dtcompatible:`nxp,sysctr` (:github:`106300`)
  * :dtcompatible:`nxp,tstmr` (:github:`112255`)
  * :dtcompatible:`nxp,wake-timer` (:github:`110811`)
  * :dtcompatible:`realtek,ameba-counter` (:github:`106664`)
  * :dtcompatible:`realtek,bee-counter-rtc` (:github:`105193`)
  * :dtcompatible:`ti,k3-rtc-counter` (:github:`104048`)
  * :dtcompatible:`wch,adtm` (:github:`109728`)
  * :dtcompatible:`xlnx,ttc` (:github:`103117`)
  * :dtcompatible:`xlnx,ttc-counter` (:github:`103117`)
  * :dtcompatible:`xlnx,zynqmp-rtc` (:github:`107684`)

* :abbr:`CPU (Central Processing Unit)`

  * :dtcompatible:`adi,max32-m4f-cpu1` (:github:`105310`)
  * :dtcompatible:`arm,armv8` (:github:`114145`)
  * :dtcompatible:`arm,cortex-a32` (:github:`107644`)
  * :dtcompatible:`arm,cortex-a57` (:github:`114109`)
  * :dtcompatible:`arm,cortex-a720` (:github:`113087`)
  * :dtcompatible:`arm,cortex-r8f` (:github:`114145`)
  * :dtcompatible:`intel,nova-lake` (:github:`111818`)
  * :dtcompatible:`intel,x86_64` (:github:`115174`)
  * :dtcompatible:`spinalhdl,vexiiriscv` (:github:`109932`)
  * :dtcompatible:`wch,qingke-v3c` (:github:`111171`)
  * :dtcompatible:`wch,qingke-v3f` (:github:`111725`)
  * :dtcompatible:`wch,qingke-v5f` (:github:`111725`)

* :abbr:`CPU (Central Processing Unit)` frequency scaling

  * :dtcompatible:`zephyr,cpu-freq-thermal-cap` (:github:`108242`)

* :abbr:`CRC (Cyclic Redundancy Check)`

  * :dtcompatible:`ambiq,hw-crc32` (:github:`110366`)

* Cryptographic accelerator

  * :dtcompatible:`infineon,mxcrypto-crypto` (:github:`108439`)
  * :dtcompatible:`infineon,mxcrypto-trng` (:github:`108439`)
  * :dtcompatible:`infineon,mxcryptolite-crypto` (:github:`109693`)
  * :dtcompatible:`infineon,mxcryptolite-trng` (:github:`109693`)
  * :dtcompatible:`realtek,bee-aes` (:github:`114817`)
  * :dtcompatible:`realtek,bee-sha256` (:github:`114817`)
  * :dtcompatible:`ti,mspm0-aes` (:github:`94734`)

* :abbr:`DAC (Digital to Analog Converter)`

  * :dtcompatible:`adi,ad5529r` (:github:`106256`)
  * :dtcompatible:`infineon,autanalog-ctdac` (:github:`107490`)
  * :dtcompatible:`infineon,hppass-csg-dac` (:github:`109967`)
  * :dtcompatible:`microchip,dac-g2` (:github:`109820`)
  * :dtcompatible:`ti,dac43508` (:github:`112038`)
  * :dtcompatible:`ti,dac53508` (:github:`112038`)
  * :dtcompatible:`ti,dac63508` (:github:`112038`)
  * :dtcompatible:`ti,mspm0-dac` (:github:`94725`)

* :abbr:`DAI (Digital Audio Interface)`

  * :dtcompatible:`amd,acp-sdw-dai` (:github:`104450`)
  * :dtcompatible:`amd,tdm-dai` (:github:`108314`)

* :abbr:`DALI (Digital Addressable Lighting Interface)`

  * :dtcompatible:`zephyr,dali-pwm` (:github:`88128`)

* Disk

  * :dtcompatible:`virtio,blk` (:github:`112581`)
  * :dtcompatible:`zephyr,memc-ram-disk` (:github:`111528`)

* Display

  * :dtcompatible:`charlieplex-led-matrix` (:github:`110137`)
  * :dtcompatible:`chipwealth,ch1115` (:github:`107434`)
  * :dtcompatible:`chipwealth,ch1116` (:github:`107434`)
  * :dtcompatible:`eink,ed2208-doa` (:github:`109961`)
  * :dtcompatible:`eink,ed2208-gca` (:github:`107510`)
  * :dtcompatible:`fitipower,ek79007` (:github:`116543`)
  * :dtcompatible:`himax,hx8353e` (:github:`108055`)
  * :dtcompatible:`ite,it8951` (:github:`108591`)
  * :dtcompatible:`levetop,lt7680` (:github:`112389`)
  * :dtcompatible:`raspberrypi,bcm2711-framebuffer` (:github:`109522`)
  * :dtcompatible:`raydium,rm67199` (:github:`98554`)
  * :dtcompatible:`raydium,rm692c9` (:github:`93134`)
  * :dtcompatible:`sinowealth,sh1107` (:github:`107434`)
  * :dtcompatible:`socionext,dpu` (:github:`93134`)
  * :dtcompatible:`solomon,ssd1305` (:github:`107434`)
  * :dtcompatible:`solomon,ssd1306b` (:github:`107434`)
  * :dtcompatible:`solomon,ssd1315` (:github:`107434`)
  * :dtcompatible:`solomon,ssd1683` (:github:`112893`)
  * :dtcompatible:`st,neochrom-gpu2d` (:github:`105970`)
  * :dtcompatible:`st,stm32-dma2d` (:github:`103687`)
  * :dtcompatible:`ultrachip,uc8253` (:github:`113230`)
  * :dtcompatible:`zephyr,panel-color-palette` (:github:`107945`)

* :abbr:`DMA (Direct Memory Access)`

  * :dtcompatible:`amd,acp-host-dma` (:github:`104450`)
  * :dtcompatible:`amd,acp-sdw-dma` (:github:`104450`)
  * :dtcompatible:`amd,acp-tdm-dma` (:github:`108314`)
  * :dtcompatible:`amd,versal2-dma-1.0` (:github:`101685`)
  * :dtcompatible:`infineon,mdma` (:github:`110847`)
  * :dtcompatible:`microchip,dmac-g3-dma` (:github:`109209`)
  * :dtcompatible:`nxp,gdma` (:github:`104868`)
  * :dtcompatible:`realtek,ameba-gdma` (:github:`105366`)
  * :dtcompatible:`realtek,bee-dma` (:github:`104754`)
  * :dtcompatible:`renesas,rza2m-dma` (:github:`107009`)
  * :dtcompatible:`ti,mspm0-dma` (:github:`91502`)
  * :dtcompatible:`xlnx,zynqmp-dma-1.0` (:github:`101685`)

* :abbr:`DSP (Digital Signal Processor)`

  * :dtcompatible:`nxp,powerquad` (:github:`110745`)

* :abbr:`EDAC (Error Detection and Correction)`

  * :dtcompatible:`nxp,mecc` (:github:`105341`)

* :abbr:`ESPI (Enhanced Serial Peripheral Interface)`

  * :dtcompatible:`intel,espi-peci` (:github:`103773`)

* Ethernet

  * :dtcompatible:`brcm,genet` (:github:`113360`)
  * :dtcompatible:`brcm,genet-mdio` (:github:`113360`)
  * :dtcompatible:`microchip,gmac-g1-eth` (:github:`105275`)
  * :dtcompatible:`microchip,gmac-g1-mdio` (:github:`105275`)
  * :dtcompatible:`microchip,lan8840` (:github:`110896`)
  * :dtcompatible:`nxp,imx-netc-vsi` (:github:`114331`)
  * :dtcompatible:`snps,dwmac` (:github:`114760`)
  * :dtcompatible:`snps,dwmac-mdio` (:github:`108046`)
  * :dtcompatible:`snps,dwmac-ptp-clock` (:github:`114242`)
  * :dtcompatible:`wch,ch9120` (:github:`111708`)
  * :dtcompatible:`wiznet,w6300` (:github:`102727`)
  * :dtcompatible:`xlnx,gem-mdio` (:github:`87313`)
  * :dtcompatible:`zephyr,native-ptp-clock` (:github:`109265`)

* Firmware

  * :dtcompatible:`arm,scmi-reset` (:github:`106306`)
  * :dtcompatible:`raspberrypi,bcm283x-firmware` (:github:`107536`)

* Flash controller

  * :dtcompatible:`aesc,spi-flash-controller` (:github:`110957`)
  * :dtcompatible:`infineon,rram-controller` (:github:`108532`)
  * :dtcompatible:`intel,pflash-cfi01` (:github:`112714`)
  * :dtcompatible:`microchip,igloo2-envm-controller` (:github:`111817`)
  * :dtcompatible:`microchip,nvmctrl-g2` (:github:`108440`)
  * :dtcompatible:`microchip,nvmctrl-g3` (:github:`109747`)
  * :dtcompatible:`microchip,smartfusion2-flash-controller` (:github:`106926`)
  * :dtcompatible:`nxp,iap-fmc84x` (:github:`105928`)
  * :dtcompatible:`realtek,ameba-flash-controller` (:github:`106690`)
  * :dtcompatible:`realtek,bee-nor-flash-controller` (:github:`107007`)

* :abbr:`FPGA (Field Programmable Gate Array)`

  * :dtcompatible:`renesas,slg47910` (:github:`107551`)

* Fuel gauge

  * :dtcompatible:`adi,adp5360-fuel-gauge` (:github:`105258`)

* :abbr:`GNSS (Global Navigation Satellite System)`

  * :dtcompatible:`ericsson,f5521gw` (:github:`113055`)
  * :dtcompatible:`u-blox,m10` (:github:`110846`)

* :abbr:`GPIO (General Purpose Input/Output)` & Headers

  * :dtcompatible:`allwinner,sun50i-h618-gpio` (:github:`110502`)
  * :dtcompatible:`allwinner,sunxi-gpio` (:github:`110502`)
  * :dtcompatible:`arduino-mega-header` (:github:`105160`)
  * :dtcompatible:`diodes,pi4ioe5v6408` (:github:`108505`)
  * :dtcompatible:`esp-01-header` (:github:`109705`)
  * :dtcompatible:`gpio-mmio-latch` (:github:`105732`)
  * :dtcompatible:`m5stack,m5pm1-gpio` (:github:`109961`)
  * :dtcompatible:`nordic,npm10xx-gpio` (:github:`108508`)
  * :dtcompatible:`raspberrypi,bcm283x-gpio` (:github:`110788`)
  * :dtcompatible:`realtek,rts5817-gpio` (:github:`105542`)
  * :dtcompatible:`st,m2-memory-connector` (:github:`109004`)
  * :dtcompatible:`st,stmod-plus-connector` (:github:`109705`)
  * :dtcompatible:`st-zio-header` (:github:`115412`)
  * :dtcompatible:`ti,tca9554` (:github:`111041`)
  * :dtcompatible:`ti,tla2528-gpio` (:github:`110722`)
  * :dtcompatible:`virtio,gpio` (:github:`114983`)
  * :dtcompatible:`wch,ch5xx-gpio` (:github:`111171`)

* Haptics

  * :dtcompatible:`cirrus,cs40l26` (:github:`106934`)
  * :dtcompatible:`cirrus,cs40l27` (:github:`106934`)
  * :dtcompatible:`cirrus,cs40l50` (:github:`105683`)
  * :dtcompatible:`cirrus,cs40l51` (:github:`105683`)
  * :dtcompatible:`cirrus,cs40l52` (:github:`105683`)
  * :dtcompatible:`cirrus,cs40l53` (:github:`105683`)
  * :dtcompatible:`titanmec,tm6605` (:github:`109104`)

* Hardware information

  * :dtcompatible:`nxp,lpc-pmc-hwinfo` (:github:`114693`)
  * :dtcompatible:`nxp,mc-rgm` (:github:`111359`)
  * :dtcompatible:`nxp,otp-uid` (:github:`111493`)

* :abbr:`I2C (Inter-Integrated Circuit)`

  * :dtcompatible:`ambiq,ios-i2c` (:github:`96059`)
  * :dtcompatible:`brcm,bcm2711-i2c` (:github:`105601`)
  * :dtcompatible:`ene,kb106x-i2c` (:github:`106693`)
  * :dtcompatible:`realtek,ameba-i2c` (:github:`108235`)
  * :dtcompatible:`realtek,bee-i2c` (:github:`105028`)
  * :dtcompatible:`zephyr,i2c-target-tmp103` (:github:`114727`)

* :abbr:`I2S (Inter-Integrated Circuit Sound)`

  * :dtcompatible:`zephyr,native-sim-i2s` (:github:`109902`)

* :abbr:`I3C (Improved Inter-Integrated Circuit)`

  * :dtcompatible:`microchip,xec-i3c` (:github:`116252`)

* IEEE 802.15.4

  * :dtcompatible:`silabs,efr32-ieee802154` (:github:`108596`)

* Input

  * :dtcompatible:`tbs,crsf` (:github:`106941`)
  * :dtcompatible:`virtio,input` (:github:`111029`)

* Interrupt controller

  * :dtcompatible:`amd,acp-intc` (:github:`104450`)
  * :dtcompatible:`brcm,bcm2835-armctrl-ic` (:github:`110189`)
  * :dtcompatible:`brcm,bcm2836-l1-intc` (:github:`110189`)
  * :dtcompatible:`microchip,smartfusion2-h2f-irqctrl` (:github:`106926`)

* :abbr:`IPC (Inter-Processor Communication)`

  * :dtcompatible:`nxp,ipc-rpmsg-lite` (:github:`104807`)

* :abbr:`LED (Light Emitting Diode)`

  * :dtcompatible:`issi,is31fl3193` (:github:`107555`)
  * :dtcompatible:`nordic,npm10xx-led` (:github:`108756`)
  * :dtcompatible:`nxp,pca9530` (:github:`112203`)
  * :dtcompatible:`nxp,pca9531` (:github:`112203`)
  * :dtcompatible:`nxp,pca9532` (:github:`112203`)
  * :dtcompatible:`ti,lp5860` (:github:`108801`)
  * :dtcompatible:`ti,lp5861` (:github:`108801`)
  * :dtcompatible:`ti,lp5862` (:github:`108801`)
  * :dtcompatible:`ti,lp5864` (:github:`108801`)
  * :dtcompatible:`ti,lp5866` (:github:`108801`)
  * :dtcompatible:`ti,lp5868` (:github:`108801`)
  * :dtcompatible:`zephyr,fake-leds` (:github:`110819`)
  * :dtcompatible:`zephyr,native-linux-leds` (:github:`111189`)

* :abbr:`LED (Light Emitting Diode)` strip

  * :dtcompatible:`worldsemi,ws2812-bflb-wo` (:github:`105325`)
  * :dtcompatible:`worldsemi,ws2812-pulse-io` (:github:`110466`)

* LoRa

  * :dtcompatible:`semtech,lr1121` (:github:`109912`)

* Mailbox

  * :dtcompatible:`arm,mhuv2` (:github:`110686`)
  * :dtcompatible:`brcm,bcm2711-mbox` (:github:`107536`)
  * :dtcompatible:`renesas,rcar-mfis-mbox` (:github:`108868`)

* MCUmgr

  * :dtcompatible:`zephyr,smp-spi` (:github:`106947`)

* Memory controller

  * :dtcompatible:`bflb,bl808-psram-uhs` (:github:`110702`)
  * :dtcompatible:`bflb,bl808-psram-uhs-controller` (:github:`110702`)
  * :dtcompatible:`bflb,sf-bank` (:github:`107223`)
  * :dtcompatible:`bflb,sf-controller` (:github:`107223`)
  * :dtcompatible:`nxp,imx-snvs-gpr` (:github:`109842`)

* Miscellaneous

  * :dtcompatible:`adi,tmc6460` (:github:`113438`)
  * :dtcompatible:`nxp,imx93-video-pll` (:github:`98554`)
  * :dtcompatible:`nxp,mcxw-hw-params` (:github:`108974`)
  * :dtcompatible:`ti,tdp2004` (:github:`111950`)

* Modem

  * :dtcompatible:`fibocom,le250` (:github:`114755`)
  * :dtcompatible:`nordic,nrf91-sm-v2` (:github:`115058`)
  * :dtcompatible:`nordic,nrf93m1` (:github:`106289`)
  * :dtcompatible:`quectel,bc66` (:github:`111279`)
  * :dtcompatible:`quectel,bc660k` (:github:`111279`)
  * :dtcompatible:`quectel,bc66x` (:github:`111279`)
  * :dtcompatible:`quectel,eg21-g` (:github:`115561`)
  * :dtcompatible:`quectel,eg915u` (:github:`95921`)
  * :dtcompatible:`telit,le910c1tx` (:github:`106716`)
  * :dtcompatible:`telit,lex10q1` (:github:`109206`)
  * :dtcompatible:`trasna,lexi-r10` (:github:`107308`)

* :abbr:`MTD (Memory Technology Device)`

  * :dtcompatible:`bflb,sf-device` (:github:`107223`)
  * :dtcompatible:`bflb,sf-flash` (:github:`107223`)
  * :dtcompatible:`is66wv` (:github:`111074`)
  * :dtcompatible:`microchip,flash-g2` (:github:`108440`)
  * :dtcompatible:`microchip,flash-g3` (:github:`109747`)
  * :dtcompatible:`nordic,tz-nonsecure` (:github:`108883`)
  * :dtcompatible:`nordic,tz-secure` (:github:`108883`)
  * :dtcompatible:`nxp,imx-flexspi-nand` (:github:`104870`)
  * :dtcompatible:`nxp,mcxw-ifr` (:github:`108974`)
  * :dtcompatible:`realtek,bee-nor-flash` (:github:`107007`)

* Multi-bit :abbr:`SPI (Serial Peripheral Interface)`

  * :dtcompatible:`microchip,xec-qmspi-controller` (:github:`113243`)
  * :dtcompatible:`microchip,xec-qmspi-device` (:github:`113243`)
  * :dtcompatible:`st,nor` (:github:`113368`)
  * :dtcompatible:`st,psram-device` (:github:`105219`)
  * :dtcompatible:`zephyr,peripheral-device` (:github:`103754`)

* Multi-Function Device

  * :dtcompatible:`ambiq,ios` (:github:`96059`)
  * :dtcompatible:`infineon,autanalog` (:github:`106227`)
  * :dtcompatible:`infineon,autanalog-ac-state` (:github:`106227`)
  * :dtcompatible:`infineon,autanalog-ctb` (:github:`107489`)
  * :dtcompatible:`infineon,autanalog-prb` (:github:`107487`)
  * :dtcompatible:`infineon,autanalog-ptcomp` (:github:`107488`)
  * :dtcompatible:`infineon,hppass-ac-state` (:github:`109196`)
  * :dtcompatible:`infineon,hppass-analog` (:github:`109196`)
  * :dtcompatible:`infineon,hppass-csg` (:github:`109694`)
  * :dtcompatible:`infineon,mxcrypto` (:github:`108439`)
  * :dtcompatible:`infineon,mxcryptolite` (:github:`109693`)
  * :dtcompatible:`m5stack,m5pm1` (:github:`109961`)
  * :dtcompatible:`ti,tla2528` (:github:`110722`)

* :abbr:`MUX (Multiplexer)`

  * :dtcompatible:`adi,adgm3121` (:github:`112088`)
  * :dtcompatible:`adi,adgm3121-gpio` (:github:`112088`)
  * :dtcompatible:`gpio-mux` (:github:`112088`)
  * :dtcompatible:`nxp,inputmux` (:github:`109379`)
  * :dtcompatible:`nxp,trgmux` (:github:`112088`)

* Networking

  * :dtcompatible:`st,stm32wba-radio` (:github:`110546`)

* :abbr:`OPAMP (Operational Amplifier)`

  * :dtcompatible:`infineon,autanalog-ctb-opamp` (:github:`107489`)

* :abbr:`OTP (One-Time Programmable)` memory

  * :dtcompatible:`adi,axi-sysid` (:github:`115280`)
  * :dtcompatible:`nxp,otpc` (:github:`111707`)
  * :dtcompatible:`nxp,rt7xx-ocotp` (:github:`108075`)
  * :dtcompatible:`realtek,rts5817-ocotp` (:github:`111141`)

* :abbr:`PCIe (Peripheral Component Interconnect Express)`

  * :dtcompatible:`brcm,iproc-pcie-ep-v2` (:github:`111490`)

* PHY

  * :dtcompatible:`st,stm32f7-usbphyc` (:github:`114696`)
  * :dtcompatible:`st,stm32n6-usbphyc` (:github:`114696`)

* Pin control

  * :dtcompatible:`aesc,pinctrl` (:github:`108137`)
  * :dtcompatible:`arm,v2m_musca_b1-pinctrl` (:github:`114671`)
  * :dtcompatible:`elan,em32-pinctrl` (:github:`103037`)
  * :dtcompatible:`nxp,lpc84x-iocon` (:github:`105928`)
  * :dtcompatible:`nxp,lpc84x-swm` (:github:`105928`)
  * :dtcompatible:`renesas,rcar-pfc-x5h` (:github:`108871`)
  * :dtcompatible:`wch,ch570-pinctrl` (:github:`111171`)
  * :dtcompatible:`wch,h41x-afio` (:github:`111725`)

* Power domain

  * :dtcompatible:`raspberrypi,bcm283x-power` (:github:`112918`)

* Power management

  * :dtcompatible:`microchip,supc-g1` (:github:`115872`)
  * :dtcompatible:`nxp,smc` (:github:`102228`)
  * :dtcompatible:`sifli,sf32lb52x-pmuc` (:github:`108093`)
  * :dtcompatible:`st,stm32-pwr-wkupctrl` (:github:`114092`)
  * :dtcompatible:`st,stm32f1-pwr-wkupctrl` (:github:`114092`)
  * :dtcompatible:`st,stm32f7-pwr-wkupctrl` (:github:`114092`)

* Pulse IO

  * :dtcompatible:`espressif,esp32-rmt` (:github:`110466`)
  * :dtcompatible:`zephyr,pulse-io-loopback` (:github:`110466`)

* :abbr:`PWM (Pulse Width Modulation)`

  * :dtcompatible:`realtek,ameba-pwm` (:github:`106669`)
  * :dtcompatible:`realtek,bee-pwm` (:github:`105014`)
  * :dtcompatible:`ti,am3352-ecap` (:github:`88860`)
  * :dtcompatible:`ti,am3352-ehrpwm` (:github:`88757`)
  * :dtcompatible:`wch,adtm-pwm` (:github:`109728`)
  * :dtcompatible:`zephyr,pwm-bitbang` (:github:`106536`)

* Regulator

  * :dtcompatible:`gd,gd32-bldo` (:github:`106501`)
  * :dtcompatible:`infineon,autanalog-prb-vref` (:github:`107487`)
  * :dtcompatible:`m5stack,m5pm1-regulator` (:github:`109961`)
  * :dtcompatible:`realtek,rts5817-regulator` (:github:`108545`)
  * :dtcompatible:`sifli,sf32lb52x-ldo` (:github:`108093`)
  * :dtcompatible:`ti,mspm0-vref` (:github:`94732`)

* Reset controller

  * :dtcompatible:`wch,ch32-rcc-rctl` (:github:`115714`)

* Retained memory

  * :dtcompatible:`gd,gd32-backup-sram` (:github:`106501`)

* :abbr:`RNG (Random Number Generator)`

  * :dtcompatible:`brcm,bcm2835-rng` (:github:`110191`)
  * :dtcompatible:`microchip,trng-g2-entropy` (:github:`108155`)
  * :dtcompatible:`realtek,ameba-trng` (:github:`106670`)
  * :dtcompatible:`realtek,bee-trng` (:github:`105335`)

* :abbr:`RTC (Real Time Clock)`

  * :dtcompatible:`ite,it8xxx2-rtc` (:github:`106350`)
  * :dtcompatible:`microchip,rtc-mss` (:github:`110842`)
  * :dtcompatible:`microchip,xec-hibtimer` (:github:`111476`)
  * :dtcompatible:`microchip,xec-rtc` (:github:`106116`)
  * :dtcompatible:`microcrystal,rv3028-rtc` (:github:`112978`)
  * :dtcompatible:`nxp,rtc-analog` (:github:`107196`)
  * :dtcompatible:`realtek,ameba-rtc` (:github:`105376`)

* :abbr:`SDHC (Secure Digital High Capacity)`

  * :dtcompatible:`bflb,sdhc` (:github:`105243`)
  * :dtcompatible:`microchip,sdhc-g1` (:github:`109211`)
  * :dtcompatible:`nuvoton,numaker-sdhc` (:github:`105437`)
  * :dtcompatible:`realtek,ameba-sdhost` (:github:`106687`)
  * :dtcompatible:`ti,am654-sdhci` (:github:`97172`)

* Sensors

  * :dtcompatible:`adi,adis1647x` (:github:`110012`)
  * :dtcompatible:`adi,adxl313` (:github:`114936`)
  * :dtcompatible:`adi,ltc4286` (:github:`105618`)
  * :dtcompatible:`adi,max30009` (:github:`112988`)
  * :dtcompatible:`bflb,tsen` (:github:`107717`)
  * :dtcompatible:`hamamatsu,s9706` (:github:`107607`)
  * :dtcompatible:`invensense,icm56622` (:github:`112362`)
  * :dtcompatible:`invensense,icm56686` (:github:`112362`)
  * :dtcompatible:`invensense,tad2144` (:github:`107994`)
  * :dtcompatible:`maxim,max30102` (:github:`108697`)
  * :dtcompatible:`maxim,max31826` (:github:`112398`)
  * :dtcompatible:`meas,htu21d` (:github:`106318`)
  * :dtcompatible:`meas,htu31d` (:github:`107532`)
  * :dtcompatible:`meas,ms5637` (:github:`106344`)
  * :dtcompatible:`microchip,pac194x` (:github:`105902`)
  * :dtcompatible:`nordic,nrf-vbat` (:github:`106102`)
  * :dtcompatible:`nxp,mcux-eqdc` (:github:`111927`)
  * :dtcompatible:`raspberrypi,bcm283x-vc-thermal` (:github:`110192`)
  * :dtcompatible:`realtek,bee-aon-qdec` (:github:`105129`)
  * :dtcompatible:`realtek,bee-basic-qdec` (:github:`105129`)
  * :dtcompatible:`realtek,bee-qdec` (:github:`105129`)
  * :dtcompatible:`sensylink,cht8315` (:github:`106391`)
  * :dtcompatible:`st,stm32-vddcore` (:github:`108053`)
  * :dtcompatible:`ti,fdc1004` (:github:`107233`)
  * :dtcompatible:`ti,tmp451` (:github:`108384`)
  * :dtcompatible:`zephyr,flow-meter` (:github:`111366`)
  * :dtcompatible:`zephyr,native-linux-temp` (:github:`114563`)

* Serial controller

  * :dtcompatible:`elan,em32-uart` (:github:`103037`)
  * :dtcompatible:`microchip,uart-g1` (:github:`114034`)
  * :dtcompatible:`nxp,lpc84x-uart` (:github:`105928`)
  * :dtcompatible:`shakti,uart` (:github:`113000`)
  * :dtcompatible:`wch,ch5xx-uart` (:github:`111171`)
  * :dtcompatible:`wch,sdi-console` (:github:`109777`)

* :abbr:`SMbus (System Management Bus)`

  * :dtcompatible:`ite,it51xxx-smbus` (:github:`114832`)

* :abbr:`SPI (Serial Peripheral Interface)`

  * :dtcompatible:`microchip,flexcom-g1-spi` (:github:`107467`)
  * :dtcompatible:`nuvoton,numaker-usci-spi` (:github:`109123`)
  * :dtcompatible:`realtek,ameba-spi` (:github:`108234`)
  * :dtcompatible:`realtek,bee-spi` (:github:`104958`)
  * :dtcompatible:`realtek,rts5817-spi` (:github:`106346`)
  * :dtcompatible:`renesas,rz-spi-b` (:github:`107073`)
  * :dtcompatible:`ti,mspm0-spi` (:github:`94726`)
  * :dtcompatible:`xlnx,zynqmp-qspi-1.0` (:github:`88466`)

* Tachometer

  * :dtcompatible:`ene,kb106x-tach` (:github:`106739`)

* Timer

  * :dtcompatible:`microchip,pit-g1-timer` (:github:`114034`)
  * :dtcompatible:`st,stm32u5-lptim` (:github:`112400`)
  * :dtcompatible:`ti,am26-rtitimer` (:github:`102545`)

* :abbr:`USB (Universal Serial Bus)`

  * :dtcompatible:`espressif,esp32-usb-otg-fs` (:github:`111508`)
  * :dtcompatible:`espressif,esp32-usb-otg-hs` (:github:`111508`)
  * :dtcompatible:`infineon,usbhs` (:github:`106841`)
  * :dtcompatible:`microchip,udphs-g1-udc` (:github:`99620`)
  * :dtcompatible:`nordic,nrf-usbhs-bc12` (:github:`106759`)

* Wakeup Controller

  * :dtcompatible:`nxp,sleepcon-wuc` (:github:`113447`)
  * :dtcompatible:`nxp,wuc-wuu` (:github:`100866`)

* Watchdog

  * :dtcompatible:`arm,crsas-ma2-watchdog` (:github:`112700`)
  * :dtcompatible:`m5stack,m5pm1-wdt` (:github:`109961`)
  * :dtcompatible:`nordic,npm10xx-wdt` (:github:`109381`)
  * :dtcompatible:`nordic,nrf-gswdt` (:github:`110067`)
  * :dtcompatible:`nuvoton,numaker-wdt` (:github:`105247`)
  * :dtcompatible:`realtek,ameba-watchdog` (:github:`106672`)
  * :dtcompatible:`realtek,bee-core-wdt` (:github:`107021`)
  * :dtcompatible:`ti,mspm0-watchdog` (:github:`95304`)

* Wi-Fi

  * :dtcompatible:`bflb,wifi6` (:github:`113078`)
  * :dtcompatible:`espressif,esp-hosted-mcu` (:github:`114532`)
  * :dtcompatible:`espressif,esp-hosted-mcu-wifi` (:github:`114532`)
  * :dtcompatible:`realtek,ameba-wifi` (:github:`105614`)
  * :dtcompatible:`st,st67w611m1` (:github:`111583`)
  * :dtcompatible:`zephyr,wifi-hwsim` (:github:`111236`)

New Samples
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the sample, further details go in the sample documentation itself.

* :zephyr:code-sample:`adi-gpio-wakeup`
* :zephyr:code-sample:`adi-pm`
* :zephyr:code-sample:`autanalog_fir_fifo`
* :zephyr:code-sample:`bluetooth_cap_handover`
* :zephyr:code-sample:`buzzer-tone`
* :zephyr:code-sample:`coap-client-tcp`
* :zephyr:code-sample:`color-palette`
* :zephyr:code-sample:`coredump-udp-demo-shell`
* :zephyr:code-sample:`coresight_stm_shell`
* :zephyr:code-sample:`cpu_freq_thermal_cap`
* :zephyr:code-sample:`cs40l26`
* :zephyr:code-sample:`dali`
* :zephyr:code-sample:`dhcpv6-pd`
* :zephyr:code-sample:`esp32-qdec-trigger`
* :zephyr:code-sample:`espnow`
* :zephyr:code-sample:`fido2`
* :zephyr:code-sample:`flow-meter`
* :zephyr:code-sample:`frdm-mcxe31b-system-off`
* :zephyr:code-sample:`i2c-tiny-usb`
* :zephyr:code-sample:`logging_multidomain`
* :zephyr:code-sample:`lora-duty-cycle`
* :zephyr:code-sample:`lp586x`
* :zephyr:code-sample:`mcp-server-hello-world`
* :zephyr:code-sample:`mfd_charger`
* :zephyr:code-sample:`mspi-throughput`
* :zephyr:code-sample:`net-rtp`
* :zephyr:code-sample:`nrf-sys-event`
* :zephyr:code-sample:`nxp_mcx_s2ram`
* :zephyr:code-sample:`nxp_mcx_system_off`
* :zephyr:code-sample:`nxp_smartdma_mem_to_mem`
* :zephyr:code-sample:`pm-latency`
* :zephyr:code-sample:`pulse_io_byte_transfer`
* :zephyr:code-sample:`qdec_multi`
* :zephyr:code-sample:`quic-client-echo`
* :zephyr:code-sample:`quic-service-echo`
* :zephyr:code-sample:`riscv-aia-smp-uart-echo`
* :zephyr:code-sample:`riscv-aia-uart-echo`
* :zephyr:code-sample:`rpi-board-info`
* :zephyr:code-sample:`rpmsg-lite`
* :zephyr:code-sample:`rw612_pm_flash_check`
* :zephyr:code-sample:`spi-rtio-loopback`
* :zephyr:code-sample:`ssh-server-client`
* :zephyr:code-sample:`sx9500`
* :zephyr:code-sample:`tad2144`
* :zephyr:code-sample:`tflite-neutron`
* :zephyr:code-sample:`tfm_fwu`
* :zephyr:code-sample:`tm6605`
* :zephyr:code-sample:`tmc6460`
* :zephyr:code-sample:`tracing-pipeline`
* :zephyr:code-sample:`tsn-switch`
* :zephyr:code-sample:`wifi-ble-provisioning`
* :zephyr:code-sample:`wifi-mesh`
* :zephyr:code-sample:`wifi-mesh-ip`
* :zephyr:code-sample:`zms-cycle-count`
* :zephyr_file:`samples/drivers/clock_monitor/check_freq`
* :zephyr_file:`samples/drivers/clock_monitor/measure_freq`

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

  * TF-M was updated from version 2.2.2 to version 2.3.1. Release notes can be
    found at:

    * https://trustedfirmware-m.readthedocs.io/en/latest/releases/2.3.0.html
    * https://trustedfirmware-m.readthedocs.io/en/tf-mv2.3.1/releases/2.3.1.html

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

* __assert

   * ``__ASSERT_ON`` define has been removed.


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

  * The Nordic nRF7000/nRF7001/nRF7002 devicetree compatibles
    ``"nordic,nrf7000-spi"``, ``"nordic,nrf7000-qspi"``,
    ``"nordic,nrf7001-spi"``, ``"nordic,nrf7001-qspi"``,
    ``"nordic,nrf7002-spi"``, and ``"nordic,nrf7002-qspi"`` have been replaced
    by the bare :dtcompatible:`nordic,nrf7000`, :dtcompatible:`nordic,nrf7001`,
    and :dtcompatible:`nordic,nrf7002` compatibles, disambiguated by
    ``on-bus``, matching the pattern already used for the MSPI variant and the
    convention used elsewhere in the tree for multi-bus devices. Out-of-tree
    boards, shields, and overlays instantiating an nRF70 node over SPI or QSPI
    must update their ``compatible`` property accordingly.

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
