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

.. _zephyr_4.2:

Zephyr 4.2.0
############

We are pleased to announce the release of Zephyr version 4.2.0.

Major enhancements with this release include:

**Initial Support for Renesas RX**
  The Renesas RX architecture is now supported, including a QEMU-based
  :zephyr:board:`board target <qemu_rx>`.

**USB Video Class Driver**
  The USB device stack now supports USB Video Class (UVC) allowing camera devices and other
  image/video sources to be exposed as standard USB video devices. See :zephyr:code-sample:`uvc` to
  get started.

**Twister Power Harness**
  A :ref:`new Twister harness <twister_power_harness>` enables measurement of the power consumption
  of the device under test and ensures it remains within a given tolerance.

**MQTT 5.0**
  The networking stack now includes full support for the :ref:`MQTT 5.0 <mqtt_socket_interface>`
  protocol.

**Bluetooth Classic improvements**
  The Bluetooth Classic stack now includes support for **Hands-Free Profile** (HFP) for both Audio
  Gateway (AG) and Hands-Free (HF) roles.

**Zbus**
  The :ref:`Zbus library <zbus>` graduates to stable status with the release of API version v1.0.0.

**Expanded Board Support**
  Support for 96 :ref:`new boards <boards_added_in_zephyr_4_2>` and 22
  :ref:`new shields <shields_added_in_zephyr_4_2>` has been added in this release.

An overview of the changes required or recommended when migrating your application from Zephyr
v4.1.0 to Zephyr v4.2.0 can be found in the separate :ref:`migration guide<migration_4.2>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* :cve:`2025-27809` `TLS clients may unwittingly skip server authentication
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-03-1/>`_
* :cve:`2025-27810` `Potential authentication bypass in TLS handshake
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-03-2/>`_
* :cve:`2025-2962` `Infinite loop in dns_copy_qname
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2qp5-c2vq-g2ww>`_
* :cve:`2025-52496` `Race condition in AESNI support detection
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-06-1/>`_
* :cve:`2025-52497` `Heap buffer under-read when parsing PEM-encrypted material
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-06-2/>`_
* :cve:`2025-49600` `Unchecked return value in LMS verification allows signature bypass
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-06-3/>`_
* :cve:`2025-49601` `Out-of-bounds read in mbedtls_lms_import_public_key()
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-06-4/>`_
* :cve:`2025-49087` `Timing side-channel in block cipher decryption with PKCS#7 padding
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-06-5/>`_
* :cve:`2025-48965` `NULL pointer dereference after using mbedtls_asn1_store_named_data()
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-06-6/>`_
* :cve:`2025-47917` `Misleading memory management in mbedtls_x509_string_to_names()
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-06-7/>`_
* :cve:`2025-7403`: Under embargo until 2025-09-05

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

Removed APIs and options
========================

* Removed the deprecated ``net_buf_put()`` and ``net_buf_get()`` API functions.

* Removed the deprecated ``include/zephyr/net/buf.h`` header file.

* Removed the ``--disable-unrecognized-section-test`` Twister option. Test has been removed and the
  option became the default behavior.

* Removed the deprecated ``kscan`` subsystem.

* Removed :dtcompatible:`meas,ms5837` and replaced with :dtcompatible:`meas,ms5837-30ba`
  and :dtcompatible:`meas,ms5837-02ba`.

* Removed the ``get_ctrl`` driver API from :c:struct:`video_driver_api`.

* Removed ``CONFIG_I3C_USE_GROUP_ADDR`` and support for group addresses for I3C devices.

Deprecated APIs and options
===========================

* The scheduler Kconfig options CONFIG_SCHED_DUMB and CONFIG_WAITQ_DUMB were
  renamed and deprecated. Use :kconfig:option:`CONFIG_SCHED_SIMPLE` and
  :kconfig:option:`CONFIG_WAITQ_SIMPLE` instead.

* The :kconfig:option:`CONFIG_LWM2M_ENGINE_MESSAGE_HEADER_SIZE` Kconfig option has been removed.
  The required header size should be included in the message size, configured using
  :kconfig:option:`CONFIG_LWM2M_COAP_MAX_MSG_SIZE`. Special care should be taken to ensure that
  the CoAP block size used (:kconfig:option:`CONFIG_LWM2M_COAP_BLOCK_SIZE`) can fit the given
  message size with headers. Previous headroom was 48 bytes.

* TLS credential type ``TLS_CREDENTIAL_SERVER_CERTIFICATE`` was renamed and
  deprecated, use :c:enumerator:`TLS_CREDENTIAL_PUBLIC_CERTIFICATE` instead.

* ``arduino_uno_r4_minima`` and ``arduino_uno_r4_wifi`` board targets have been deprecated in favor
  of a new ``arduino_uno_r4`` board with revisions (``arduino_uno_r4@minima`` and
  ``arduino_uno_r4@wifi``).

* ``esp32c6_devkitc`` board target has been deprecated and renamed to
  ``esp32c6_devkitc/esp32c6/hpcore``.

* ``xiao_esp32c6`` board target has been deprecated and renamed to
  ``xiao_esp32c6/esp32c6/hpcore``.

* :kconfig:option:`CONFIG_HAWKBIT_DDI_NO_SECURITY` Kconfig option has been
  deprecated, because support for anonymous authentication had been removed from the
  hawkBit server in version 0.8.0.

* The :kconfig:option:`CONFIG_BT_CONN_TX_MAX` Kconfig option has been deprecated. The number of
  pending TX buffers is now aligned with the :kconfig:option:`CONFIG_BT_BUF_ACL_TX_COUNT` Kconfig
  option.

* The :kconfig:option:`CONFIG_CRYPTO_TINYCRYPT_SHIM` Kconfig option has been removed. It
  was deprecated since Zephyr 4.0, and users were advised to migrate to alternative
  crypto backends.

* The :kconfig:option:`CONFIG_BT_MESH_USES_TINYCRYPT` Kconfig option has been removed. It
  was deprecated since Zephyr 4.0. Users were advised to use
  :kconfig:option:`CONFIG_BT_MESH_USES_MBEDTLS_PSA` or
  :kconfig:option:`CONFIG_BT_MESH_USES_TFM_PSA` instead.

Stable API changes in this release
==================================

* The API signature of ``net_mgmt`` event handler :c:type:`net_mgmt_event_handler_t`
  and request handler :c:type:`net_mgmt_request_handler_t` has changed. The event value
  type is changed from ``uint32_t`` to ``uint64_t``.

New APIs and options
====================

..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w)

* Architectures

  * NIOS2 Architecture was removed from Zephyr.
  * :kconfig:option:`ARCH_HAS_VECTOR_TABLE_RELOCATION`
  * :kconfig:option:`CONFIG_SRAM_VECTOR_TABLE` moved from ``zephyr/Kconfig.zephyr`` to
    ``zephyr/arch/Kconfig`` and added dependencies to it.

* Bluetooth

  * Audio

    * :c:macro:`BT_BAP_ADV_PARAM_CONN_QUICK`
    * :c:macro:`BT_BAP_ADV_PARAM_CONN_REDUCED`
    * :c:macro:`BT_BAP_CONN_PARAM_SHORT_7_5`
    * :c:macro:`BT_BAP_CONN_PARAM_SHORT_10`
    * :c:macro:`BT_BAP_CONN_PARAM_RELAXED`
    * :c:macro:`BT_BAP_ADV_PARAM_BROADCAST_FAST`
    * :c:macro:`BT_BAP_ADV_PARAM_BROADCAST_SLOW`
    * :c:macro:`BT_BAP_PER_ADV_PARAM_BROADCAST_FAST`
    * :c:macro:`BT_BAP_PER_ADV_PARAM_BROADCAST_SLOW`
    * :c:func:`bt_csip_set_member_set_size_and_rank`
    * :c:func:`bt_csip_set_member_get_info`
    * :c:func:`bt_bap_unicast_group_foreach_stream`
    * :c:func:`bt_cap_unicast_group_create`
    * :c:func:`bt_cap_unicast_group_reconfig`
    * :c:func:`bt_cap_unicast_group_add_streams`
    * :c:func:`bt_cap_unicast_group_delete`
    * :c:func:`bt_cap_unicast_group_foreach_stream`

  * Host

    * :c:func:`bt_le_get_local_features`
    * :c:func:`bt_le_bond_exists`
    * :c:func:`bt_br_bond_exists`
    * :c:func:`bt_conn_lookup_addr_br`
    * :c:func:`bt_conn_get_dst_br`
    * LE Connection Subrating is no longer experimental.
    * Remove deletion of the classic bonding information from :c:func:`bt_unpair`, and add
      :c:func:`bt_br_unpair`.
    * Remove query of the classic bonding information from :c:func:`bt_foreach_bond`, and add
      :c:func:`bt_br_foreach_bond`.
    * Add a new parameter ``limited`` to :c:func:`bt_br_set_discoverable` to support limited
      discoverable mode for the classic.
    * Enable retransmission and flow control for the classic L2CAP, including
      :kconfig:option:`CONFIG_BT_L2CAP_RET`, :kconfig:option:`CONFIG_BT_L2CAP_FC`,
      :kconfig:option:`CONFIG_BT_L2CAP_ENH_RET`, and :kconfig:option:`CONFIG_BT_L2CAP_STREAM`.
    * :c:func:`bt_avrcp_get_cap`
    * Improve the classic hands-free unit, including
      :kconfig:option:`CONFIG_BT_HFP_HF_CODEC_NEG`, :kconfig:option:`CONFIG_BT_HFP_HF_ECNR`,
      :kconfig:option:`CONFIG_BT_HFP_HF_3WAY_CALL`, :kconfig:option:`CONFIG_BT_HFP_HF_ECS`,
      :kconfig:option:`CONFIG_BT_HFP_HF_ECC`, :kconfig:option:`CONFIG_BT_HFP_HF_VOICE_RECG_TEXT`,
      :kconfig:option:`CONFIG_BT_HFP_HF_ENH_VOICE_RECG`,
      :kconfig:option:`CONFIG_BT_HFP_HF_VOICE_RECG`,
      :kconfig:option:`CONFIG_BT_HFP_HF_HF_INDICATORS`,
      :kconfig:option:`CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY`, and
      :kconfig:option:`CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY`.
    * Improve the classic hands-free audio gateway, including
      :kconfig:option:`CONFIG_BT_HFP_AG_CODEC_NEG`, :kconfig:option:`CONFIG_BT_HFP_AG_ECNR`,
      :kconfig:option:`CONFIG_BT_HFP_AG_3WAY_CALL`, :kconfig:option:`CONFIG_BT_HFP_AG_ECS`,
      :kconfig:option:`CONFIG_BT_HFP_AG_ECC`, :kconfig:option:`CONFIG_BT_HFP_AG_VOICE_RECG_TEXT`,
      :kconfig:option:`CONFIG_BT_HFP_AG_ENH_VOICE_RECG`,
      :kconfig:option:`CONFIG_BT_HFP_AG_VOICE_TAG`,
      :kconfig:option:`CONFIG_BT_HFP_AG_HF_INDICATORS`,
      :kconfig:option:`CONFIG_BT_HFP_AG_HF_INDICATOR_ENH_SAFETY`,
      :kconfig:option:`CONFIG_BT_HFP_AG_HF_INDICATOR_BATTERY`, and
      :kconfig:option:`CONFIG_BT_HFP_AG_REJECT_CALL`.
    * Add a callback function ``get_ongoing_call()`` to :c:struct:`bt_hfp_ag_cb`.
    * :c:func:`bt_hfp_ag_ongoing_calls`
    * Support the classic L2CAP signaling echo request and response feature, including
      :c:struct:`bt_l2cap_br_echo_cb`, :c:func:`bt_l2cap_br_echo_cb_register`,
      :c:func:`bt_l2cap_br_echo_cb_unregister`, :c:func:`bt_l2cap_br_echo_req`, and
      :c:func:`bt_l2cap_br_echo_rsp`.
    * :c:func:`bt_a2dp_get_conn`
    * :c:func:`bt_rfcomm_send_rpn_cmd`

* Build system

  * Sysbuild

    * Firmware loader image setup/selection support added to sysbuild when using
      :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER` via
      ``SB_CONFIG_FIRMWARE_LOADER`` e.g. :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_IMAGE_SMP_SVR`
      for selecting :zephyr:code-sample:`smp-svr`.
    * Single app RAM load support added to sysbuild using
      :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SINGLE_APP_RAM_LOAD`.

* Counter

  * :c:func:`counter_reset`

* Debug

  * Core Dump

    * :kconfig:option:`CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP`, enabled by default for ARM Cortex M when :kconfig:option:`CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_MIN` is selected.
    * :kconfig:option:`CONFIG_DEBUG_COREDUMP_BACKEND_IN_MEMORY`
    * :kconfig:option:`CONFIG_DEBUG_COREDUMP_BACKEND_IN_MEMORY_SIZE`

* Display

    * Added :c:func:`display_clear` API to allow clearing the display content in a standardized way.
    * Character Frame Buffer (CFB) subsystem now supports drawing circles via :c:func:`cfb_draw_circle`.

* I2C

  * :c:func:`i2c_configure_dt`.
  * :c:macro:`I2C_DEVICE_DT_DEINIT_DEFINE`
  * :c:macro:`I2C_DEVICE_DT_INST_DEINIT_DEFINE`

* I3C

  * :kconfig:option:`CONFIG_I3C_MODE`
  * :kconfig:option:`CONFIG_I3C_CONTROLLER_ROLE_ONLY`
  * :kconfig:option:`CONFIG_I3C_TARGET_ROLE_ONLY`
  * :kconfig:option:`CONFIG_I3C_DUAL_ROLE`
  * :c:func:`i3c_ccc_do_rstdaa`

* Kernel

 * :c:macro:`K_TIMEOUT_ABS_SEC`
 * :c:func:`timespec_add`
 * :c:func:`timespec_compare`
 * :c:func:`timespec_equal`
 * :c:func:`timespec_is_valid`
 * :c:func:`timespec_negate`
 * :c:func:`timespec_normalize`
 * :c:func:`timespec_from_timeout`
 * :c:func:`timespec_to_timeout`
 * :c:func:`k_heap_array_get`

* LVGL (Light and Versatile Graphics Library)

    * The LVGL module was synchronized to v9.3, bringing numerous upstream improvements and new features.
    * LVGL subsystem now supports multiple simultaneous displays, including proper input device-to-display binding.
    * Added L8/Y8 pixel format support for displays such as SSD1327, SSD1320, SSD1322, and ST75256.
    * :kconfig:option:`CONFIG_LV_Z_COLOR_MONO_HW_INVERSION`

* LoRaWAN

   * :c:func:`lorawan_request_link_check`

* Management

  * MCUmgr

    * Firmware loader support added to image mgmt group using
      :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_FIRMWARE_UPDATER`.
    * Optional boot mode (using retention boot mode) added to OS group reset command using
      :kconfig:option:`CONFIG_MCUMGR_GRP_OS_RESET_BOOT_MODE`.

* Networking:

  * CoAP

    * :c:macro:`COAPS_SERVICE_DEFINE`

  * DHCPv4

    * :kconfig:option:`CONFIG_NET_DHCPV4_INIT_REBOOT`

  * DNS

    * :c:func:`dns_resolve_service`
    * :c:func:`dns_resolve_reconfigure_with_interfaces`

  * HTTP

    * :kconfig:option:`CONFIG_HTTP_SERVER_COMPRESSION`

  * IPv4

    * :kconfig:option:`CONFIG_NET_IPV4_MTU`

  * LwM2M

    * :kconfig:option:`CONFIG_LWM2M_SERVER_BOOTSTRAP_ON_FAIL`
    * Implemented Greater Than, Less Than and Step observe attributes handling
      (see :kconfig:option:`CONFIG_LWM2M_MAX_NOTIFIED_NUMERICAL_RES_TRACKED`).

  * Misc

    * :c:func:`net_if_oper_state_change_time`

  * MQTT

    * :kconfig:option:`CONFIG_MQTT_VERSION_5_0`
    * :c:member:`mqtt_transport.if_name`

  * OpenThread

    * Moved OpenThread-related Kconfig options from :zephyr_file:`subsys/net/l2/openthread/Kconfig`
      to :zephyr_file:`modules/openthread/Kconfig`.
    * Refactored OpenThread networking API, see the OpenThread section of the
      :ref:`migration guide <migration_4.2>`.
    * :kconfig:option:`CONFIG_OPENTHREAD_SYS_INIT`
    * :kconfig:option:`CONFIG_OPENTHREAD_SYS_INIT_PRIORITY`

  * SNTP

    * :c:func:`sntp_init_async`
    * :c:func:`sntp_send_async`
    * :c:func:`sntp_read_async`
    * :c:func:`sntp_close_async`

  * Sockets

    * :kconfig:option:`CONFIG_NET_SOCKETS_INET_RAW`
    * :c:func:`socket_offload_dns_enable`
    * Added a new documentation page for :ref:`socket_service_interface` library.
    * New socket options:

      * :c:macro:`IP_MULTICAST_LOOP`
      * :c:macro:`IPV6_MULTICAST_LOOP`
      * :c:macro:`TLS_CERT_VERIFY_RESULT`

  * Wi-Fi

    * :kconfig:option:`CONFIG_WIFI_USAGE_MODE`
    * Added a new section to the Wi-Fi Management documentation
      (``doc/connectivity/networking/api/wifi.rst``) with step-by-step instructions for generating
      test certificates for Wi-Fi using FreeRADIUS scripts. This helps users reproduce the process
      for their own test environments.
    * Changed the hostap IPC mechanism from socketpair to k_fifo. Depending on the enabled Wi-Fi configuration options, this can save up to 6-8 kB memory when using native Wi-Fi stack.

  * zperf

    * :kconfig:option:`CONFIG_ZPERF_SESSION_PER_THREAD`
    * :c:member:`zperf_upload_params.data_loader`
    * :kconfig:option:`CONFIG_NET_ZPERF_SERVER`

* PCIe

   * :kconfig:option:`CONFIG_NVME_PRP_PAGE_SIZE`

* Power management

    * :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_USE_SYSTEM_WQ`
    * :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_USE_DEDICATED_WQ`
    * :kconfig:option:`CONFIG_PM_DEVICE_DRIVER_NEEDS_DEDICATED_WQ`
    * :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_DEDICATED_WQ_STACK_SIZE`
    * :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_DEDICATED_WQ_PRIO`
    * :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_DEDICATED_WQ_INIT_PRIO`
    * :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_ASYNC`

* SPI

  * :c:macro:`SPI_DEVICE_DT_DEINIT_DEFINE`
  * :c:macro:`SPI_DEVICE_DT_INST_DEINIT_DEFINE`

* Sensor

  * :c:func:`sensor_value_to_deci`
  * :c:func:`sensor_value_to_centi`

* Stepper

  * :c:func:`stepper_stop()`

* Storage

  * :c:func:`flash_area_copy()`

* Sys

  * :c:func:`util_eq`
  * :c:func:`util_memeq`
  * :c:func:`sys_clock_gettime`
  * :c:func:`sys_clock_settime`
  * :c:func:`sys_clock_nanosleep`

* USB

  * :c:func:`uvc_set_video_dev`

* UpdateHub

  * :c:func:`updatehub_report_error`

* Video

  * :c:type:`video_api_ctrl_t`
  * :c:func:`video_query_ctrl`
  * :c:func:`video_print_ctrl`
  * :c:type:`video_api_selection_t`
  * :c:func:`video_set_selection`
  * :c:func:`video_get_selection`
  * :ref:`video-sw-generator <snippet-video-sw-generator>`
  * :c:func:`video_get_csi_link_freq`
  * :c:macro:`VIDEO_CID_LINK_FREQ`
  * :c:macro:`VIDEO_CID_AUTO_WHITE_BALANCE` and other controls from the BASE control class.
  * :c:macro:`VIDEO_CID_EXPOSURE_ABSOLUTE` and other controls from the CAMERA control class.
  * :c:macro:`VIDEO_PIX_FMT_Y10` and ``Y12``, ``Y14``, ``Y16`` variants
  * :c:macro:`VIDEO_PIX_FMT_SRGGB10P` and ``12P``, ``14P`` variants, for all 4 bayer variants.
  * :c:member:`video_buffer.index` field
  * :c:member:`video_ctrl_query.int_menu` field
  * :c:macro:`VIDEO_MIPI_CSI2_DT_NULL` and other MIPI standard values

* ZBus

  * Zbus has achieved stable status with the release of API version v1.0.0.
  * Runtime observers can work without heap. Now it is possible to choose between static, dynamic,
    and none allocation for the runtime observers nodes.
  * Runtime observers using :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_NONE` must use
    the new function :c:func:`zbus_chan_add_obs_with_node`.

  * :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_DYNAMIC`
  * :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_STATIC`
  * :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_NONE`
  * :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_POOL_SIZE`

.. zephyr-keep-sorted-stop

.. _boards_added_in_zephyr_4_2:

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

* Adafruit Industries, LLC

   * :zephyr:board:`adafruit_feather_esp32s2` (``adafruit_feather_esp32s2``)
   * :zephyr:board:`adafruit_feather_esp32s2_tft` (``adafruit_feather_esp32s2_tft``)
   * :zephyr:board:`adafruit_feather_esp32s2_tft_reverse` (``adafruit_feather_esp32s2_tft_reverse``)
   * :zephyr:board:`adafruit_feather_esp32s3` (``adafruit_feather_esp32s3``)
   * :zephyr:board:`adafruit_feather_esp32s3_tft` (``adafruit_feather_esp32s3_tft``)
   * :zephyr:board:`adafruit_feather_esp32s3_tft_reverse` (``adafruit_feather_esp32s3_tft_reverse``)

* Advanced Micro Devices (AMD), Inc.

   * :zephyr:board:`versal2_rpu` (``versal2_rpu``)
   * :zephyr:board:`versalnet_rpu` (``versalnet_rpu``)

* Aesc Silicon

   * :zephyr:board:`elemrv` (``elemrv``)

* Ai-Thinker Co., Ltd.

   * :zephyr:board:`ai_wb2_12f` (``ai_wb2_12f``)

* Ambiq Micro, Inc.

   * :zephyr:board:`apollo510_evb` (``apollo510_evb``)

* Analog Devices, Inc.

   * :zephyr:board:`max32657evkit` (``max32657evkit``)

* Arduino

   * :zephyr:board:`arduino_nano_matter` (``arduino_nano_matter``)
   * :zephyr:board:`arduino_portenta_c33` (``arduino_portenta_c33``)

* ARM Ltd.

   * :zephyr:board:`mps4` (``mps4``)

* BeagleBoard.org Foundation

   * :zephyr:board:`pocketbeagle_2` (``pocketbeagle_2``)

* Blues Wireless

   * :zephyr:board:`cygnet` (``cygnet``)

* Bouffalo Lab (Nanjing) Co., Ltd.

   * :zephyr:board:`bl604e_iot_dvk` (``bl604e_iot_dvk``)

* Doctors of Intelligence & Technology

   * :zephyr:board:`dt_bl10_devkit` (``dt_bl10_devkit``)

* ENE Technology, Inc.

   * :zephyr:board:`kb1062_evb` (``kb1062_evb``)

* Espressif Systems

   * :zephyr:board:`esp32_devkitc` (``esp32_devkitc``)

* Ezurio

   * :zephyr:board:`bl54l15_dvk` (``bl54l15_dvk``)
   * :zephyr:board:`bl54l15u_dvk` (``bl54l15u_dvk``)

* FANKE Technology Co., Ltd.

   * :zephyr:board:`fk743m5_xih6` (``fk743m5_xih6``)

* IAR Systems AB

   * :zephyr:board:`stm32f429ii_aca` (``stm32f429ii_aca``)

* Infineon Technologies

   * :zephyr:board:`kit_xmc72_evk` (``kit_xmc72_evk``)

* Intel Corporation

   * :zephyr:board:`intel_btl_s_crb` (``intel_btl_s_crb``)

* ITE Tech. Inc.

   * :zephyr:board:`it515xx_evb` (``it515xx_evb``)

* KWS Computersysteme Gmbh

   * :zephyr:board:`pico2_spe` (``pico2_spe``)
   * :zephyr:board:`pico_spe` (``pico_spe``)

* Lilygo Shenzhen Xinyuan Electronic Technology Co., Ltd

   * :zephyr:board:`tdongle_s3` (``tdongle_s3``)
   * :zephyr:board:`ttgo_tbeam` (``ttgo_tbeam``)
   * :zephyr:board:`ttgo_toiplus` (``ttgo_toiplus``)
   * :zephyr:board:`twatch_s3` (``twatch_s3``)

* M5Stack

   * :zephyr:board:`m5stack_fire` (``m5stack_fire``)

* Microchip Technology Inc.

   * :zephyr:board:`mec_assy6941` (``mec_assy6941``)
   * :zephyr:board:`sama7g54_ek` (``sama7g54_ek``)

* MikroElektronika d.o.o.

   * :zephyr:board:`mikroe_quail` (``mikroe_quail``)

* Nordic Semiconductor

   * :zephyr:board:`nrf54lm20dk` (``nrf54lm20dk``)

* Nuvoton Technology Corporation

   * :zephyr:board:`npck3m8k_evb` (``npck3m8k_evb``)
   * :zephyr:board:`numaker_m55m1` (``numaker_m55m1``)

* NXP Semiconductors

   * :zephyr:board:`frdm_mcxa153` (``frdm_mcxa153``)
   * :zephyr:board:`imx943_evk` (``imx943_evk``)
   * :zephyr:board:`mcx_n9xx_evk` (``mcx_n9xx_evk``)
   * :zephyr:board:`s32k148_evb` (``s32k148_evb``)

* Octavo Systems LLC

   * :zephyr:board:`osd32mp1_brk` (``osd32mp1_brk``)

* OpenHW Group

   * :zephyr:board:`cv32a6_genesys_2` (``cv32a6_genesys_2``)
   * :zephyr:board:`cv64a6_genesys_2` (``cv64a6_genesys_2``)

* Pimoroni Ltd.

   * :zephyr:board:`pico_plus2` (``pico_plus2``)

* QEMU

   * :zephyr:board:`qemu_rx` (``qemu_rx``)

* Raytac Corporation

   * :zephyr:board:`raytac_an54l15q_db` (``raytac_an54l15q_db``)
   * :zephyr:board:`raytac_an7002q_db` (``raytac_an7002q_db``)
   * :zephyr:board:`raytac_mdbt50q_cx_40_dongle` (``raytac_mdbt50q_cx_40_dongle``)

* Renesas Electronics Corporation

   * :zephyr:board:`ek_ra8p1` (``ek_ra8p1``)
   * :zephyr:board:`rsk_rx130` (``rsk_rx130``)
   * :zephyr:board:`rza2m_evk` (``rza2m_evk``)
   * :zephyr:board:`rza3ul_smarc` (``rza3ul_smarc``)
   * :zephyr:board:`rzg2l_smarc` (``rzg2l_smarc``)
   * :zephyr:board:`rzg2lc_smarc` (``rzg2lc_smarc``)
   * :zephyr:board:`rzg2ul_smarc` (``rzg2ul_smarc``)
   * :zephyr:board:`rzn2l_rsk` (``rzn2l_rsk``)
   * :zephyr:board:`rzt2l_rsk` (``rzt2l_rsk``)
   * :zephyr:board:`rzt2m_rsk` (``rzt2m_rsk``)
   * :zephyr:board:`rzv2h_evk` (``rzv2h_evk``)
   * :zephyr:board:`rzv2l_smarc` (``rzv2l_smarc``)
   * :zephyr:board:`rzv2n_evk` (``rzv2n_evk``)

* Seeed Technology Co., Ltd

   * :zephyr:board:`xiao_mg24` (``xiao_mg24``)
   * :zephyr:board:`xiao_ra4m1` (``xiao_ra4m1``)

* sensry.io

   * :zephyr:board:`ganymed_sk` (``ganymed_sk``)

* Shanghai Ruiside Electronic Technology Co., Ltd.

   * :zephyr:board:`art_pi2` (``art_pi2``)
   * :zephyr:board:`ra8d1_vision_board` (``ra8d1_vision_board``)

* Silicon Laboratories

   * :zephyr:board:`siwx917_rb4342a` (``siwx917_rb4342a``)
   * :zephyr:board:`slwrb4180b` (``slwrb4180b``)

* Space Cubics, LLC

   * :zephyr:board:`scobc_a1` (``scobc_a1``)

* STMicroelectronics

   * :zephyr:board:`nucleo_f439zi` (``nucleo_f439zi``)
   * :zephyr:board:`nucleo_u385rg_q` (``nucleo_u385rg_q``)
   * :zephyr:board:`nucleo_wba65ri` (``nucleo_wba65ri``)
   * :zephyr:board:`stm32h757i_eval` (``stm32h757i_eval``)
   * :zephyr:board:`stm32mp135f_dk` (``stm32mp135f_dk``)
   * :zephyr:board:`stm32mp257f_ev1` (``stm32mp257f_ev1``)
   * :zephyr:board:`stm32u5g9j_dk1` (``stm32u5g9j_dk1``)
   * :zephyr:board:`stm32u5g9j_dk2` (``stm32u5g9j_dk2``)

* Texas Instruments

   * :zephyr:board:`am243x_evm` (``am243x_evm``)
   * :zephyr:board:`lp_mspm0g3507` (``lp_mspm0g3507``)
   * :zephyr:board:`sk_am64` (``sk_am64``)

* u-blox

   * :zephyr:board:`ubx_evk_iris_w1` (``ubx_evk_iris_w1``)

* Variscite Ltd.

   * :zephyr:board:`imx8mp_var_dart` (``imx8mp_var_dart``)
   * :zephyr:board:`imx8mp_var_som` (``imx8mp_var_som``)
   * :zephyr:board:`imx93_var_dart` (``imx93_var_dart``)
   * :zephyr:board:`imx93_var_som` (``imx93_var_som``)

* Waveshare Electronics

   * :zephyr:board:`esp32s3_matrix` (``esp32s3_matrix``)
   * :zephyr:board:`rp2040_plus` (``rp2040_plus``)

* WeAct Studio

   * :zephyr:board:`bluepillplus_ch32v203` (``bluepillplus_ch32v203``)
   * :zephyr:board:`weact_stm32f446_core` (``weact_stm32f446_core``)

* WinChipHead

   * :zephyr:board:`ch32v003f4p6_dev_board` (``ch32v003f4p6_dev_board``)
   * :zephyr:board:`ch32v006evt` (``ch32v006evt``)
   * :zephyr:board:`ch32v303vct6_evt` (``ch32v303vct6_evt``)
   * :zephyr:board:`linkw` (``linkw``)

* WIZnet Co., Ltd.

   * :zephyr:board:`w5500_evb_pico2` (``w5500_evb_pico2``)

* Würth Elektronik GmbH.

   * :zephyr:board:`ophelia4ev` (``ophelia4ev``)

.. _shields_added_in_zephyr_4_2:

New shields
===========

* :ref:`Arduino Giga Display Shield <arduino_giga_display_shield>`
* :ref:`Arduino Modulino Buttons <arduino_modulino_buttons>`
* :ref:`Arduino Modulino SmartLEDs <arduino_modulino_smartleds>`
* :ref:`DVP 20-pin OV7670 <dvp_20pin_ov7670>`
* :ref:`EVAL AD4052 ARDZ <eval_ad4052_ardz>`
* :ref:`EVAL ADXL367 ARDZ <eval_adxl367_ardz>`
* :ref:`M5Stack Cardputer <m5stack_cardputer>`
* :ref:`MikroElektronika LTE IoT10 Click <mikroe_lte_iot10_click_shield>`
* :ref:`MikroElektronika Stepper 18 Click <mikroe_stepper_18_click_shield>`
* :ref:`MikroElektronika Stepper 19 Click <mikroe_stepper_19_click_shield>`
* :ref:`NPM2100 Evaluation Kit <npm2100_ek>`
* :ref:`NXP ADTJA1101 <nxp_adtja1101>`
* :ref:`NXP M2 WiFi BT <nxp_m2_wifi_bt>`
* :ref:`OpenThread RCP Arduino <openthread_rcp_arduino_shield>`
* :ref:`RTK7 EKA6M3B00001BU <rtk7eka6m3b00001bu>`
* :ref:`RTKLCDPAR1S00001BE Display <rtklcdpar1s00001be>`
* :ref:`ST B-CAMS-IMX-MB1854 <st_b_cams_imx_mb1854>`
* :ref:`ST MB1897 camera module <st_mb1897_cam>`
* :ref:`ST STM32F4DIS CAM <st_stm32f4dis_cam>`
* :ref:`Waveshare Pico LCD 1.14 <waveshare_pico_lcd_1_14>`
* :ref:`Waveshare Pico OLED 1.3 <waveshare_pico_oled_1_3>`
* :ref:`X-Nucleo-GFX01M2 <x_nucleo_gfx01m2_shield>`

New Drivers
***********

..
  Same as above for boards, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* :abbr:`ADC (Analog to Digital Converter)`

   * :dtcompatible:`adi,ad4050-adc`
   * :dtcompatible:`adi,ad4052-adc`
   * :dtcompatible:`adi,ad4130-adc`
   * :dtcompatible:`ene,kb106x-adc`
   * :dtcompatible:`ite,it51xxx-adc`
   * :dtcompatible:`microchip,mcp356xr`
   * :dtcompatible:`realtek,rts5912-adc`
   * :dtcompatible:`renesas,rz-adc`
   * :dtcompatible:`silabs,siwx91x-adc`
   * :dtcompatible:`ti,am335x-adc`
   * :dtcompatible:`ti,cc23x0-adc`
   * :dtcompatible:`wch,adc`

* Audio

   * :dtcompatible:`ambiq,pdm`
   * :dtcompatible:`maxim,max98091`
   * :dtcompatible:`ti,pcm1681`
   * :dtcompatible:`ti,tlv320aic3110`
   * :dtcompatible:`wolfson,wm8962`

* Auxiliary Display

   * :dtcompatible:`gpio-7-segment`

* :abbr:`CAN (Controller Area Network)`

   * :dtcompatible:`adi,max32-can`
   * :dtcompatible:`renesas,rz-canfd`
   * :dtcompatible:`renesas,rz-canfd-global`

* Charger

   * :dtcompatible:`ti,bq25713`
   * :dtcompatible:`x-powers,axp2101-charger`

* Clock control

   * :dtcompatible:`bflb,bclk`
   * :dtcompatible:`bflb,bl60x-clock-controller`
   * :dtcompatible:`bflb,bl60x-pll`
   * :dtcompatible:`bflb,bl60x-root-clk`
   * :dtcompatible:`bflb,clock-controller`
   * :dtcompatible:`ite,it51xxx-ecpm`
   * :dtcompatible:`microchip,sam-pmc`
   * :dtcompatible:`microchip,sama7g5-sckc`
   * :dtcompatible:`nordic,nrf51-hfxo`
   * :dtcompatible:`nordic,nrf52-hfxo`
   * :dtcompatible:`nordic,nrf54l-hfxo`
   * :dtcompatible:`nordic,nrfs-audiopll`
   * :dtcompatible:`renesas,rx-cgc-pclk`
   * :dtcompatible:`renesas,rx-cgc-pclk-block`
   * :dtcompatible:`renesas,rx-cgc-pll`
   * :dtcompatible:`renesas,rx-cgc-root-clock`
   * :dtcompatible:`renesas,rza2m-cpg`
   * :dtcompatible:`st,stm32mp13-cpu-clock-mux`
   * :dtcompatible:`st,stm32mp13-pll-clock`
   * :dtcompatible:`st,stm32mp2-rcc`
   * :dtcompatible:`st,stm32u3-msi-clock`
   * :dtcompatible:`ti,mspm0-clk`
   * :dtcompatible:`ti,mspm0-osc`
   * :dtcompatible:`ti,mspm0-pll`
   * :dtcompatible:`wch,ch32v20x_30x-pll-clock`

* Comparator

   * :dtcompatible:`ite,it51xxx-vcmp`
   * :dtcompatible:`renesas,ra-acmphs`
   * :dtcompatible:`renesas,ra-acmphs-global`

* Counter

   * :dtcompatible:`adi,max32-wut`
   * :dtcompatible:`espressif,esp32-counter`
   * :dtcompatible:`ite,it51xxx-counter`
   * :dtcompatible:`ite,it8xxx2-counter`
   * :dtcompatible:`neorv32,gptmr`
   * :dtcompatible:`realtek,rts5912-timer`
   * :dtcompatible:`ti,cc23x0-lgpt`
   * :dtcompatible:`ti,cc23x0-rtc`
   * :dtcompatible:`ti,mspm0-timer-counter`
   * :dtcompatible:`wch,gptm`
   * :dtcompatible:`zephyr,native-sim-counter`

* CPU

   * :dtcompatible:`arm,cortex-r8`
   * :dtcompatible:`intel,bartlett-lake`
   * :dtcompatible:`openhwgroup,cva6`
   * :dtcompatible:`renesas,rx`
   * :dtcompatible:`wch,qingke-v4b`
   * :dtcompatible:`wch,qingke-v4c`
   * :dtcompatible:`wch,qingke-v4f`
   * :dtcompatible:`zephyr,native-sim-cpu`

* Cryptographic accelerator

   * :dtcompatible:`ite,it51xxx-sha`
   * :dtcompatible:`realtek,rts5912-sha`
   * :dtcompatible:`ti,cc23x0-aes`

* :abbr:`DAC (Digital to Analog Converter)`

   * :dtcompatible:`nxp,dac12`
   * :dtcompatible:`ti,dac161s997`

* Debug

   * :dtcompatible:`silabs,pti`

* Display

   * :dtcompatible:`sinowealth,sh1122`
   * :dtcompatible:`sitronix,st75256`
   * :dtcompatible:`sitronix,st7567`
   * :dtcompatible:`sitronix,st7701`
   * :dtcompatible:`solomon,ssd1320`
   * :dtcompatible:`solomon,ssd1327fb`
   * :dtcompatible:`solomon,ssd1331`
   * :dtcompatible:`solomon,ssd1351`
   * :dtcompatible:`solomon,ssd1363`
   * :dtcompatible:`zephyr,displays`

* :abbr:`DMA (Direct Memory Access)`

   * :dtcompatible:`renesas,rz-dma`
   * :dtcompatible:`ti,cc23x0-dma`
   * :dtcompatible:`wch,wch-dma`

* :abbr:`EDAC (Error Detection and Correction)`

   * :dtcompatible:`xlnx,zynqmp-ddrc-2.40a`

* :abbr:`eSPI (Enhanced Serial Peripheral Interface)`

   * :dtcompatible:`realtek,rts5912-espi`

* Ethernet

   * :dtcompatible:`ethernet-phy`
   * :dtcompatible:`microchip,vsc8541`
   * :dtcompatible:`nxp,netc-ptp-clock`
   * :dtcompatible:`nxp,tja11xx`
   * :dtcompatible:`st,stm32-ethernet-controller` has been introduced to ease interoperability
     with :dtcompatible:`st,stm32-mdio`.
   * :dtcompatible:`st,stm32n6-ethernet`
   * :dtcompatible:`ti,dp83867`
   * :dtcompatible:`xlnx,axi-ethernet-1.00.a`

* Firmware

   * :dtcompatible:`nxp,scmi-cpu`
   * :dtcompatible:`ti,k2g-sci`

* Flash controller

   * :dtcompatible:`realtek,rts5912-flash-controller`
   * :dtcompatible:`renesas,ra-ospi-b-nor`
   * :dtcompatible:`renesas,rx-flash`
   * :dtcompatible:`silabs,series2-flash-controller`
   * :dtcompatible:`st,stm32u3-flash-controller`

* File system

   * :dtcompatible:`zephyr,fstab,fatfs`

* Fuel gauge

   * :dtcompatible:`onnn,lc709203f`
   * :dtcompatible:`x-powers,axp2101-fuel-gauge`

* :abbr:`GNSS (Global Navigation Satellite System)`

   * :dtcompatible:`u-blox,f9p`

* :abbr:`GPIO (General Purpose Input/Output)`

   * :dtcompatible:`adi,max14915-gpio`
   * :dtcompatible:`adi,max14917-gpio`
   * :dtcompatible:`adi,max22199-gpio`
   * :dtcompatible:`arducam,dvp-20pin-connector`
   * :dtcompatible:`bflb,gpio`
   * :dtcompatible:`ene,kb106x-gpio`
   * :dtcompatible:`espressif,esp32-lpgpio`
   * :dtcompatible:`ite,it51xxx-gpio`
   * :dtcompatible:`nordic,npm1304-gpio`
   * :dtcompatible:`nxp,lcd-pmod`
   * :dtcompatible:`raspberrypi,csi-connector`
   * :dtcompatible:`raspberrypi,pico-gpio-port`
   * :dtcompatible:`renesas,ra-parallel-graphics-header`
   * :dtcompatible:`renesas,rx-gpio`
   * :dtcompatible:`renesas,rza2m-gpio`
   * :dtcompatible:`renesas,rza2m-gpio-int`
   * :dtcompatible:`st,stm32mp2-gpio`
   * :dtcompatible:`ti,mspm0-gpio`

* IEEE 802.15.4 HDLC RCP interface

   * :dtcompatible:`spi,hdlc-rcp-if`

* :abbr:`I2C (Inter-Integrated Circuit)`

   * :dtcompatible:`cdns,i2c`
   * :dtcompatible:`ite,it51xxx-i2c`
   * :dtcompatible:`litex,litei2c`
   * :dtcompatible:`realtek,rts5912-i2c`
   * :dtcompatible:`renesas,ra-i2c-sci-b`
   * :dtcompatible:`renesas,rx-i2c`
   * :dtcompatible:`renesas,rz-riic`
   * :dtcompatible:`sensry,sy1xx-i2c`
   * :dtcompatible:`wch,i2c`

* :abbr:`I2S (Inter-IC Sound)`

   * :dtcompatible:`ambiq,i2s`
   * :dtcompatible:`nordic,nrf-tdm`
   * :dtcompatible:`renesas,ra-i2s-ssie`
   * :dtcompatible:`silabs,siwx91x-i2s`
   * :dtcompatible:`st,stm32-sai`

* :abbr:`I3C (Improved Inter-Integrated Circuit)`

   * :dtcompatible:`ite,it51xxx-i3cm`
   * :dtcompatible:`ite,it51xxx-i3cs`
   * :dtcompatible:`renesas,ra-i3c`

* IEEE 802.15.4

   * :dtcompatible:`espressif,esp32-ieee802154`

* Input

   * :dtcompatible:`arduino,modulino-buttons`
   * :dtcompatible:`ite,it51xxx-kbd`
   * :dtcompatible:`realtek,rts5912-kbd`
   * :dtcompatible:`st,stm32-tsc`
   * :dtcompatible:`tsc-keys`
   * :dtcompatible:`vishay,vs1838b`

* Interrupt controller

   * :dtcompatible:`ite,it51xxx-intc`
   * :dtcompatible:`ite,it51xxx-wuc`
   * :dtcompatible:`ite,it51xxx-wuc-map`
   * :dtcompatible:`renesas,rx-icu`
   * :dtcompatible:`riscv,clic`
   * :dtcompatible:`wch,exti`

* :abbr:`LED (Light Emitting Diode)`

   * :dtcompatible:`arduino,modulino-buttons-leds`
   * :dtcompatible:`dac-leds`
   * :dtcompatible:`nordic,npm1304-led`
   * :dtcompatible:`x-powers,axp192-led`
   * :dtcompatible:`x-powers,axp2101-led`

* :abbr:`LED (Light Emitting Diode)` strip

   * :dtcompatible:`arduino,modulino-smartleds`

* Mailbox

   * :dtcompatible:`arm,mhuv3`
   * :dtcompatible:`renesas,rz-mhu-mbox`
   * :dtcompatible:`ti,secure-proxy`

* :abbr:`MDIO (Management Data Input/Output)`

   * :dtcompatible:`xlnx,axi-ethernet-1.00.a-mdio`

* Memory controller

   * :dtcompatible:`adi,max32-hpb`
   * :dtcompatible:`realtek,rts5912-bbram`
   * :dtcompatible:`silabs,siwx91x-qspi-memory`
   * :dtcompatible:`st,stm32-xspi-psram`

* :abbr:`MFD (Multi-Function Device)`

   * :dtcompatible:`adi,maxq10xx`
   * :dtcompatible:`ambiq,iom`
   * :dtcompatible:`microchip,sam-flexcom`
   * :dtcompatible:`nordic,npm1304`
   * :dtcompatible:`x-powers,axp2101`

* :abbr:`MIPI DBI (Mobile Industry Processor Interface Display Bus Interface)`

   * :dtcompatible:`nxp,mipi-dbi-dcnano-lcdif`

* Miscellaneous

   * :dtcompatible:`ene,kb106x-gcfg`
   * :dtcompatible:`nordic,ironside-call`
   * :dtcompatible:`nordic,nrf-mpc`
   * :dtcompatible:`nxp,rtxxx-dsp-ctrl`
   * :dtcompatible:`renesas,ra-elc`
   * :dtcompatible:`renesas,ra-ulpt`
   * :dtcompatible:`renesas,rx-external-interrupt`
   * :dtcompatible:`renesas,rx-mtu`
   * :dtcompatible:`renesas,rx-sci`
   * :dtcompatible:`renesas,rz-sci`
   * :dtcompatible:`renesas,rz-sci-b`
   * :dtcompatible:`st,stm32n6-ramcfg`

* Modem

   * :dtcompatible:`quectel,eg800q`
   * :dtcompatible:`simcom,a76xx`

* Multi-bit SPI

   * :dtcompatible:`nordic,nrf-exmif`
   * :dtcompatible:`snps,designware-ssi`

* :abbr:`MTD (Memory Technology Device)`

   * :dtcompatible:`fixed-subpartitions`
   * :dtcompatible:`jedec,mspi-nor`
   * :dtcompatible:`mspi-aps-z8`
   * :dtcompatible:`mspi-is25xX0xx`
   * :dtcompatible:`renesas,ra-nv-code-flash`
   * :dtcompatible:`renesas,ra-nv-data-flash`
   * :dtcompatible:`renesas,rx-nv-flash`
   * :dtcompatible:`ti,tmp11x-eeprom`

* Networking

   * :dtcompatible:`nordic,nrf-nfct-v2`
   * :dtcompatible:`silabs,siwx91x-nwp`

* Octal SPI

   * :dtcompatible:`renesas,ra-ospi-b`

* Pin control

   * :dtcompatible:`ambiq,apollo5-pinctrl`
   * :dtcompatible:`arm,mps2-pinctrl`
   * :dtcompatible:`arm,mps3-pinctrl`
   * :dtcompatible:`arm,mps4-pinctrl`
   * :dtcompatible:`arm,v2m_beetle-pinctrl`
   * :dtcompatible:`bflb,pinctrl`
   * :dtcompatible:`ene,kb106x-pinctrl`
   * :dtcompatible:`microchip,sama7g5-pinctrl`
   * :dtcompatible:`nuvoton,npcx-pinctrl-npckn`
   * :dtcompatible:`renesas,rx-pinctrl`
   * :dtcompatible:`renesas,rx-pinmux`
   * :dtcompatible:`renesas,rza-pinctrl`
   * :dtcompatible:`renesas,rza2m-pinctrl`
   * :dtcompatible:`renesas,rzn-pinctrl`
   * :dtcompatible:`renesas,rzt-pinctrl`
   * :dtcompatible:`renesas,rzv-pinctrl`
   * :dtcompatible:`st,stm32n6-pinctrl`
   * :dtcompatible:`ti,mspm0-pinctrl`
   * :dtcompatible:`wch,00x-afio`
   * :dtcompatible:`wch,20x_30x-afio`

* Power management

   * :dtcompatible:`infineon,cat1b-power`
   * :dtcompatible:`realtek,rts5912-ulpm`

* Power domain

   * :dtcompatible:`ti,sci-pm-domain`

* :abbr:`PSI5 (Peripheral Sensor Interface, 5th generation)`

   * :dtcompatible:`nxp,s32-psi5`

* :abbr:`PWM (Pulse Width Modulation)`

   * :dtcompatible:`arduino-header-pwm`
   * :dtcompatible:`ene,kb106x-pwm`
   * :dtcompatible:`ite,it51xxx-pwm`
   * :dtcompatible:`neorv32,pwm`
   * :dtcompatible:`realtek,rts5912-pwm`
   * :dtcompatible:`renesas,rx-mtu-pwm`
   * :dtcompatible:`silabs,letimer-pwm`
   * :dtcompatible:`silabs,siwx91x-pwm`
   * :dtcompatible:`silabs,timer-pwm`
   * :dtcompatible:`ti,mspm0-timer-pwm`
   * :dtcompatible:`wch,gptm-pwm`

* Regulator

   * :dtcompatible:`nordic,npm1304-regulator`
   * :dtcompatible:`x-powers,axp2101-regulator`

* Reset controller

   * :dtcompatible:`microchip,mpfs-reset`
   * :dtcompatible:`reset-mmio`

* :abbr:`RNG (Random Number Generator)`

   * :dtcompatible:`adi,maxq10xx-trng`
   * :dtcompatible:`brcm,iproc-rng200`
   * :dtcompatible:`virtio,device4`
   * :dtcompatible:`zephyr,native-sim-rng`

* :abbr:`RTC (Real Time Clock)`

   * :dtcompatible:`nxp,pcf2123`
   * :dtcompatible:`realtek,rts5912-rtc`
   * :dtcompatible:`silabs,siwx91x-rtc`

* :abbr:`SDHC (Secure Digital Host Controller)`

   * :dtcompatible:`ambiq,sdio`
   * :dtcompatible:`xlnx,versal-8.9a`

* Sensors

   * :dtcompatible:`adi,ad2s1210`
   * :dtcompatible:`bosch,bmm350`
   * :dtcompatible:`brcm,afbr-s50`
   * :dtcompatible:`everlight,als-pt19`
   * :dtcompatible:`invensense,icm40627`
   * :dtcompatible:`invensense,icm45686`
   * :dtcompatible:`invensense,icp201xx`
   * :dtcompatible:`liteon,ltr329`
   * :dtcompatible:`meas,ms5837-02ba`
   * :dtcompatible:`meas,ms5837-30ba`
   * :dtcompatible:`nordic,npm1304-charger`
   * :dtcompatible:`nxp,lpadc-temp40`
   * :dtcompatible:`nxp,tpm-qdec`
   * :dtcompatible:`peacefair,pzem004t`
   * :dtcompatible:`pixart,paa3905`
   * :dtcompatible:`pixart,paj7620`
   * :dtcompatible:`pixart,pat9136`
   * :dtcompatible:`pni,rm3100`
   * :dtcompatible:`rohm,bh1730`
   * :dtcompatible:`rohm,bh1790`
   * :dtcompatible:`st,lsm6dsv32x`
   * :dtcompatible:`st,lsm9ds1_mag`
   * :dtcompatible:`ti,tmp11x`
   * :dtcompatible:`vishay,veml6031`
   * :dtcompatible:`we,wsen-itds-2533020201601`

* :abbr:`SENT (Single Edge Nibble Transmission)`

   * :dtcompatible:`nxp,s32-sent`

* Serial controller

   * :dtcompatible:`aesc,uart`
   * :dtcompatible:`ambiq,pl011-uart`
   * :dtcompatible:`bflb,uart`
   * :dtcompatible:`ene,kb106x-uart`
   * :dtcompatible:`espressif,esp32-lpuart`
   * :dtcompatible:`ite,it51xxx-uart`
   * :dtcompatible:`nuvoton,npcx-uart-npckn`
   * :dtcompatible:`renesas,rx-uart-sci`
   * :dtcompatible:`renesas,rx-uart-sci-qemu`
   * :dtcompatible:`renesas,rz-sci-b-uart`
   * :dtcompatible:`renesas,rz-sci-uart`
   * :dtcompatible:`renesas,rza2m-scif-uart`
   * :dtcompatible:`ti,mspm0-uart`
   * :dtcompatible:`zephyr,native-pty-uart`
   * :dtcompatible:`zephyr,uart-bridge`

* :abbr:`SPI (Serial Peripheral Interface)`

   * :dtcompatible:`cdns,spi`
   * :dtcompatible:`ite,it51xxx-spi`
   * :dtcompatible:`microchip,mec5-qspi`
   * :dtcompatible:`renesas,rx-rspi`
   * :dtcompatible:`renesas,rz-rspi`
   * :dtcompatible:`silabs,gspi`
   * :dtcompatible:`ti,cc23x0-spi`
   * :dtcompatible:`wch,spi`

* Stepper

   * :dtcompatible:`adi,tmc51xx`
   * :dtcompatible:`allegro,a4979`

* System controller

   * :dtcompatible:`bflb,efuse`

* Tachometer

   * :dtcompatible:`ite,it51xxx-tach`
   * :dtcompatible:`realtek,rts5912-tach`

* :abbr:`TCPC (USB Type-C Port Controller)`

   * :dtcompatible:`onnn,fusb307-tcpc`

* Timer

   * :dtcompatible:`infineon,cat1-lp-timer`
   * :dtcompatible:`ite,it51xxx-timer`
   * :dtcompatible:`microchip,sam-pit64b`
   * :dtcompatible:`renesas,ra-ulpt-timer`
   * :dtcompatible:`renesas,rx-timer-cmt`
   * :dtcompatible:`renesas,rx-timer-cmt-start-control`
   * :dtcompatible:`renesas,rz-gtm-os-timer`
   * :dtcompatible:`renesas,rza2m-ostm`
   * :dtcompatible:`silabs,series2-letimer`
   * :dtcompatible:`silabs,series2-timer`
   * :dtcompatible:`ti,mspm0-timer`

* USB

   * :dtcompatible:`adi,max32-usbhs`
   * :dtcompatible:`nxp,uhc-ehci`
   * :dtcompatible:`nxp,uhc-ip3516hs`
   * :dtcompatible:`nxp,uhc-khci`
   * :dtcompatible:`nxp,uhc-ohci`
   * :dtcompatible:`st,stm32n6-otghs`
   * :dtcompatible:`zephyr,uvc-device`

* Video

   * :dtcompatible:`ovti,ov9655`
   * :dtcompatible:`sony,imx335`
   * :dtcompatible:`st,mipid02`
   * :dtcompatible:`st,stm32-dcmipp`
   * :dtcompatible:`zephyr,video-sw-generator`

* Virtio

   * :dtcompatible:`virtio,mmio`
   * :dtcompatible:`virtio,pci`

* Watchdog

   * :dtcompatible:`ene,kb106x-watchdog`
   * :dtcompatible:`ite,it51xxx-watchdog`
   * :dtcompatible:`nordic,npm1304-wdt`
   * :dtcompatible:`nxp,ewm`
   * :dtcompatible:`realtek,rts5912-watchdog`
   * :dtcompatible:`renesas,ra-wdt`
   * :dtcompatible:`silabs,siwx91x-wdt`
   * :dtcompatible:`ti,cc23x0-wdt`
   * :dtcompatible:`wch,iwdg`

* Wi-Fi

   * :dtcompatible:`espressif,esp-hosted`

New Samples
***********

..
  Same as above for boards and drivers, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

* :zephyr:code-sample:`amp_audio_loopback`
* :zephyr:code-sample:`amp_audio_output`
* :zephyr:code-sample:`amp_blinky`
* :zephyr:code-sample:`amp_mbox`
* :zephyr:code-sample:`auxdisplay_digits`
* :zephyr:code-sample:`bmg160`
* :zephyr:code-sample:`debug-ulp`
* :zephyr:code-sample:`distance_polling`
* :zephyr:code-sample:`echo-ulp`
* :zephyr:code-sample:`fatfs-fstab`
* :zephyr:code-sample:`fuel_gauge`
* :zephyr:code-sample:`heart_rate`
* :zephyr:code-sample:`interrupt-ulp`
* :zephyr:code-sample:`light_sensor_polling`
* :zephyr:code-sample:`lvgl-multi-display`
* :zephyr:code-sample:`min-heap`
* :zephyr:code-sample:`mspi-timing-scan`
* :zephyr:code-sample:`net-pkt-filter`
* :zephyr:code-sample:`nrf_ironside_update`
* :zephyr:code-sample:`paj7620_gesture`
* :zephyr:code-sample:`pressure_interrupt`
* :zephyr:code-sample:`pressure_polling`
* :zephyr:code-sample:`psi5`
* :zephyr:code-sample:`renesas-elc`
* :zephyr:code-sample:`renesas_comparator`
* :zephyr:code-sample:`rz-openamp-linux-zephyr`
* :zephyr:code-sample:`sent`
* :zephyr:code-sample:`spis-wakeup`
* :zephyr:code-sample:`stepper`
* :zephyr:code-sample:`stream_drdy`
* :zephyr:code-sample:`uart_async`
* :zephyr:code-sample:`usb-cdc-acm-bridge`
* :zephyr:code-sample:`uuid`
* :zephyr:code-sample:`uvc`
* :zephyr:code-sample:`veml6031`

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?

* Added support for Armv8.1-M MPU's PXN (Privileged Execute Never) attribute.
  With this, the MPU attributes for ``__ramfunc`` and ``__ram_text_reloc`` were modified such that,
  PXN attribute is set for these regions if compiled with ``CONFIG_ARM_MPU_PXN`` and ``CONFIG_USERSPACE``.
  This results in a change in behavior for code being executed from these regions because,
  if these regions have pxn attribute set in them, they cannot be executed in privileged mode.

* Removed support for Nucleo WBA52CG board (``nucleo_wba52cg``) since it is NRND (Not Recommended
  for New Design) and is no longer supported in the STM32CubeWBA from version 1.1.0 (July 2023).
  The migration to :zephyr:board:`nucleo_wba55cg` (``nucleo_wba55cg``) is recommended instead.

* Updated Mbed TLS to version 3.6.4 (from 3.6.2). Release notes for 3.6.3 and
  3.6.4 can be found below:

  * 3.6.3: https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.3
  * 3.6.4: https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.4

* Updated TF-M to version 2.1.2 (from 2.1.1). The release notes can be found at:
  https://trustedfirmware-m.readthedocs.io/en/tf-mv2.1.2/releases/2.1.2.html

* Updated all boards with an external I2C connectors (Qwiic, Stemma, Grove...)
  to use the ``zephyr_i2c`` devicetree label. This allows using the existing
  :ref:`shields` build system feature (``west build --shield``) to interface
  any connectorized i2c module to any board with a compatible i2c port,
  regardless of the specific i2c connector branding.

* Reverted deprecation of receiver option in Nordic UART driver. Receiver mode which is using
  additional TIMER peripheral to count received bytes was previously deprecated
  (e.g. :kconfig:option:`CONFIG_CONFIG_UART_0_NRF_HW_ASYNC`). However, it turned out that this
  previously mode is the only one that is capable of reliably receive data without Hardware
  Flow Control so it should stay in the driver.
