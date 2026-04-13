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

.. _zephyr_4.4:

Zephyr 4.4.0
############

We are pleased to announce the release of Zephyr version 4.4.0.

Major enhancements with this release include:

**OpenRISC support**
  Zephyr now supports the :zephyr:board-catalog:`OpenRISC architecture <#arch=openrisc>`.

**Toolchain updates: Zephyr SDK 1.0 and C17**
  Zephyr 4.4 is the first release to support :ref:`Zephyr SDK 1.0 <toolchain_zephyr_sdk>`, with an
  upgraded GNU toolchain, experimental Clang/LLVM support, and multi-platform QEMU and OpenOCD
  host tools.

  Zephyr now defaults to C17 as its minimum required C standard version.

**Networking enhancements**
  The Wi-Fi management stack now supports :ref:`wifi_mgmt_p2p`, allowing devices to discover and
  connect directly without a traditional access point.

  The networking stack also adds support for :zephyr:code-sample:`WireGuard VPN <wireguard-vpn>`,
  enabling secure, low-overhead tunneling.

**USB host**
  Experimental USB host support has been significantly expanded with a new host-class driver
  framework and support for :abbr:`UVC (USB Video Class)` cameras on Zephyr devices acting as USB
  hosts.

**New driver classes**
  Zephyr 4.4 adds several new driver APIs, including:

  - :ref:`One-Time Programmable (OTP) memory devices <otp>` for provisioning and reading permanent
    device data,

  - A :ref:`biometrics API <biometrics_api>` for integrating biometric sensors such as fingerprint
    scanners or facial recognition systems, and

  - A :ref:`Wake-up Controller (WUC) API <wuc_api>` for managing wake-up sources that can bring the
    system out of low-power states.

**Zbus async listeners and proxy agents**
  Zbus async listeners enable non-blocking observer callbacks via workqueues.

  :ref:`Zbus proxy agents <zbus_proxy_agent>` extend publish-subscribe messaging across CPU and
  domain boundaries over IPC.

**Pressure-based CPU frequency scaling**
  The experimental :ref:`CPU frequency scaling <cpu_freq>` subsystem now includes a
  :ref:`pressure-based policy <pressure_policy>` that adjusts CPU frequency according to scheduler
  load.

**ARM Cortex-M context switching performance improvements**
  A new context switch implementation for ARM Cortex-M, enabled via
  :kconfig:option:`CONFIG_USE_SWITCH`, delivers significant performance improvements.

**NAND flash support**
  A new Flash Translation Layer (FTL) disk driver (:dtcompatible:`zephyr,ftl-dhara`) provides wear
  leveling and bad block management and enables NAND flash memories to be utilized as standard disk
  devices.

**Developer experience improvements**
  This release adds several new tools and improvements to development and testing workflows:

  - A new :ref:`dashboard <dashboard>` consolidates build information such as RAM and ROM footprint,
    Devicetree configuration, subsystem initialization levels, and more in a single report.

  - A new display driver for QEMU targets simplifies development of display-based applications in
    environments where the native simulator is unavailable.

  - A new heap hardening mechanism (:kconfig:option:`CONFIG_SYS_HEAP_HARDENING`) provides multiple
    levels of runtime protection against heap corruption.

  - New :ref:`scope-based cleanup helpers <cleanup_api>` provide :abbr:`RAII (Resource Acquisition
    Is Initialization)`/defer-style automatic cleanup in C when leaving scope.

  - The new :ref:`ztest benchmarking framework <ztest_benchmarking>` provides a standardized way to
    create cycle-accurate benchmarks, with automated data collection, overhead compensation, and
    statistical reporting.

**Expanded board support**
  This release adds support for 121 :ref:`new boards <boards_added_in_zephyr_4_4>` and 31
  :ref:`new shields <shields_added_in_zephyr_4_4>`.

An overview of the changes required or recommended when migrating your application from Zephyr
v4.3.0 to Zephyr v4.4.0 can be found in the separate :ref:`migration guide<migration_4.4>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* :cve:`2025-53022` `(TF-M) FWU does not check the length of the TLV’s payload
  <https://trustedfirmware-m.readthedocs.io/en/latest/security/security_advisories/fwu_tlv_payload_out_of_bounds_vulnerability.html>`_

* :cve:`2026-0849` `Zephyr project bug tracker GHSA-ff4p-3ggg-prp6
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-ff4p-3ggg-prp6>`_

* :cve:`2026-1677` Under embargo until 2026-04-15

* :cve:`2026-1678` `Zephyr project bug tracker GHSA-536f-h63g-hj42
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-536f-h63g-hj42>`_

* :cve:`2026-1679` `Zephyr project bug tracker GHSA-qx3g-5g22-fq5w
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-qx3g-5g22-fq5w>`_

* :cve:`2026-1681` Under embargo until 2026-04-15

* :cve:`2026-4179` `Zephyr project bug tracker GHSA-9xg7-g3q3-9prf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9xg7-g3q3-9prf>`_

* :cve:`2026-5066` Under embargo until 2026-06-01

* :cve:`2026-5067` Under embargo until 2026-05-23

* :cve:`2026-5068` Under embargo until 2026-05-21

* :cve:`2026-5071` Under embargo until 2026-05-18

* :cve:`2026-5072` Under embargo until 2026-05-18

* :cve:`2026-5589` Under embargo until 2026-06-03

* :cve:`2026-5590` `Zephyr project bug tracker GHSA-4vqm-pw24-g9jp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4vqm-pw24-g9jp>`_

API Changes
***********

..
  Only removed, deprecated and new APIs. Changes go in migration guide.

Removed APIs and options
========================

* Architectures

  * Xtensa

    * Removed as these are architectural features:

      * :kconfig:option:`CONFIG_XTENSA_MMU_DOUBLE_MAP`
      * :kconfig:option:`CONFIG_XTENSA_RPO_CACHE`
      * :kconfig:option:`CONFIG_XTENSA_CACHED_REGION`
      * :kconfig:option:`CONFIG_XTENSA_UNCACHED_REGION`

* Bluetooth

  * ``CONFIG_BT_TBS_SUPPORTED_FEATURES``

  * The deprecated ``bt_hci_cmd_create()`` function was removed. It has been replaced by
    :c:func:`bt_hci_cmd_alloc`.

  * Controller

    * :kconfig:option:`CONFIG_BT_CTLR_ADV_AUX_SET`, :kconfig:option:`CONFIG_BT_CTLR_ADV_SYNC_SET`
      and :kconfig:option:`CONFIG_BT_CTLR_ADV_DATA_BUF_MAX` no longer require
      :kconfig:option:`CONFIG_BT_CTLR_ADVANCED_FEATURES`

* Mbed TLS

  * ``CONFIG_PSA_WANT_KEY_TYPE_DES``
  * ``CONFIG_PSA_WANT_ECC_SECP_K1_192``
  * ``CONFIG_PSA_WANT_ECC_SECP_R1_192``
  * ``CONFIG_PSA_WANT_ECC_SECP_R1_224``
  * ``CONFIG_CUSTOM_MBEDTLS_CFG_FILE``
  * ``CONFIG_MBEDTLS_CHACHAPOLY_AEAD_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_AES_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_CAMELLIA_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_CCM_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_CHACHA20_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_DES_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_GCM_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_MODE_CBC_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_MODE_CTR_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_MODE_XTS_ENABLED``
  * ``CONFIG_MBEDTLS_CMAC``
  * ``CONFIG_MBEDTLS_DHM_C``
  * ``CONFIG_MBEDTLS_ECDH_C``
  * ``CONFIG_MBEDTLS_ECDSA_C``
  * ``CONFIG_MBEDTLS_ECDSA_DETERMINISTIC``
  * ``CONFIG_MBEDTLS_ECJPAKE_C``
  * ``CONFIG_MBEDTLS_ECP_ALL_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_C``
  * ``CONFIG_MBEDTLS_ECP_DP_BP256R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_BP384R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_BP512R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_CURVE25519_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_CURVE448_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP192K1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP192R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP224K1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP224R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP256K1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP256R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP384R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP521R1_ENABLED``
  * ``CONFIG_MBEDTLS_GENPRIME_ENABLED``
  * ``CONFIG_MBEDTLS_HKDF_C``
  * ``CONFIG_MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED``
  * ``CONFIG_MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED``
  * ``CONFIG_MBEDTLS_KEY_EXCHANGE_RSA_ENABLED``
  * ``CONFIG_MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED``
  * ``CONFIG_MBEDTLS_MD5``
  * ``CONFIG_MBEDTLS_PKCS1_V15``
  * ``CONFIG_MBEDTLS_PKCS1_V21``
  * ``CONFIG_MBEDTLS_POLY1305``
  * ``CONFIG_MBEDTLS_RSA_C``
  * ``CONFIG_MBEDTLS_SHA1``
  * ``CONFIG_MBEDTLS_SHA224``
  * ``CONFIG_MBEDTLS_SHA256``
  * ``CONFIG_MBEDTLS_SHA384``
  * ``CONFIG_MBEDTLS_SHA512``
  * ``CONFIG_MBEDTLS_USE_PSA_CRYPTO``

  * ``CONFIG_MBEDTLS_ENTROPY_POLL_ZEPHYR`` has been renamed to
    :kconfig:option:`CONFIG_MBEDTLS_PSA_DRIVER_GET_ENTROPY`.

  * ``CONFIG_MBEDTLS_PEM_CERTIFICATE_FORMAT`` has been replaced by the underlying options it used
    to enable: :kconfig:option:`CONFIG_MBEDTLS_PEM_PARSE_C`,
    :kconfig:option:`CONFIG_MBEDTLS_PEM_WRITE_C` and :kconfig:option:`CONFIG_MBEDTLS_BASE64_C`.

  * ``CONFIG_MBEDTLS_SERVER_NAME_INDICATION`` has been renamed to
    :kconfig:option:`CONFIG_MBEDTLS_SSL_SERVER_NAME_INDICATION`.

  * ``CONFIG_MBEDTLS_TEST`` has been renamed to :kconfig:option:`CONFIG_MBEDTLS_DEBUG_C`.

* Random

  * ``CONFIG_CSPRNG_AVAILABLE`` has been renamed to :kconfig:option:`CONFIG_ENTROPY_NODE_ENABLED`.

Deprecated APIs and options
===========================

* Bluetooth

  * Mesh

    * The function :c:func:`bt_mesh_input_number` was deprecated. Applications should use
      :c:func:`bt_mesh_input_numeric` instead.
    * The callback :c:member:`output_number` in :c:struct:`bt_mesh_prov` structure was deprecated.
      Applications should use :c:member:`output_numeric` callback instead.
    * The :kconfig:option:`CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE` option has been deprecated.

  * Host

    * :c:member:`bt_conn_le_info.interval` has been deprecated. Use
      :c:member:`bt_conn_le_info.interval_us` instead. Note that the units have changed:
      ``interval`` was in units of 1.25 milliseconds, while ``interval_us`` is in microseconds.
    * The :kconfig:option:`CONFIG_DEVICE_NAME_GATT_WRITABLE_NONE` option has been deprecated.
      Applications should use :kconfig:option:`CONFIG_BT_DEVICE_NAME_GATT_WRITABLE_NONE`
      option instead.
    * The :kconfig:option:`CONFIG_DEVICE_NAME_GATT_WRITABLE_ENCRYPT` option has been deprecated.
      Applications should use :kconfig:option:`CONFIG_BT_DEVICE_NAME_GATT_WRITABLE_ENCRYPT`
      option instead.
    * The :kconfig:option:`CONFIG_DEVICE_NAME_GATT_WRITABLE_AUTHEN` option has been deprecated.
      Applications should use :kconfig:option:`CONFIG_BT_DEVICE_NAME_GATT_WRITABLE_AUTHEN`
      option instead.
    * The :kconfig:option:`CONFIG_DEVICE_APPEARANCE_GATT_WRITABLE_AUTHEN` option has been
      deprecated.
      Applications should use :kconfig:option:`CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE_AUTHEN`
      option instead.

  * HCI

    * :c:macro:`BT_HCI_LE_SUPERVISON_TIMEOUT_MIN` and :c:macro:`BT_HCI_LE_SUPERVISON_TIMEOUT_MAX` have been deprecated.
      Use :c:macro:`BT_HCI_LE_SUPERVISION_TIMEOUT_MIN` and :c:macro:`BT_HCI_LE_SUPERVISION_TIMEOUT_MAX` instead.

* Entropy

   * :kconfig:option:`CONFIG_ENTROPY_PSA_CRYPTO_RNG` has been deprecated.

* I2S

  * The following macros have been deprecated and are replaced with equivalent macros whose names
    are aligned with the `latest revision of the I2S specification`_.

    * :c:macro:`I2S_OPT_BIT_CLK_MASTER` -> :c:macro:`I2S_OPT_BIT_CLK_CONTROLLER`
    * :c:macro:`I2S_OPT_FRAME_CLK_MASTER` -> :c:macro:`I2S_OPT_FRAME_CLK_CONTROLLER`
    * :c:macro:`I2S_OPT_BIT_CLK_SLAVE` -> :c:macro:`I2S_OPT_BIT_CLK_TARGET`
    * :c:macro:`I2S_OPT_FRAME_CLK_SLAVE` -> :c:macro:`I2S_OPT_FRAME_CLK_TARGET`

.. _latest revision of the I2S specification: https://www.nxp.com/docs/en/user-manual/UM11732.pdf

* Mbed TLS

  * :kconfig:option:`CONFIG_MBEDTLS_USER_CONFIG_ENABLE` and
    :kconfig:option:`CONFIG_MBEDTLS_CFG_FILE` were deprecated. Instead, use
    :kconfig:option:`CONFIG_MBEDTLS_CONFIG_FILE`.

  * :kconfig:option:`CONFIG_MBEDTLS_LIBRARY` was deprecated. Instead, use
    :kconfig:option:`CONFIG_MBEDTLS_CUSTOM`.


* POSIX

  * :kconfig:option:`CONFIG_XOPEN_STREAMS` was deprecated. Instead, use :kconfig:option:`CONFIG_XSI_STREAMS`

* Random

  * :kconfig:option:`CONFIG_CTR_DRBG_CSPRNG_GENERATOR` has been deprecrated. Instead, use
    :kconfig:option:`CONFIG_PSA_CSPRNG_GENERATOR`.

* Sensors

  * NXP

    * Deprecated the ``mcux_lpcmp`` driver (:zephyr_file:`drivers/sensor/nxp/mcux_lpcmp/mcux_lpcmp.c`). It is
      currently scheduled to be removed in Zephyr 4.6, along with the ``mcux_lpcmp`` sample. (:github:`100998`).

* Timer

  * The legacy Cortex-M SysTick low-power companion compatibility macros
    :c:macro:`z_cms_lptim_hook_on_lpm_entry` and :c:macro:`z_cms_lptim_hook_on_lpm_exit`,
    along with the compatibility header :zephyr_file:`drivers/timer/cortex_m_systick.h`,
    have been deprecated. Out-of-tree SoC/platform code should migrate to
    :c:func:`z_sys_clock_lpm_enter`, :c:func:`z_sys_clock_lpm_exit`, and
    :zephyr_file:`include/zephyr/drivers/timer/system_timer_lpm.h`.
    The legacy Kconfig options:
    :kconfig:option:`CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_NONE`,
    :kconfig:option:`CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_COUNTER`,
    :kconfig:option:`CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_HOOKS`, and
    :kconfig:option:`CONFIG_CORTEX_M_SYSTICK_RESET_BY_LPM` are deprecated in favor of
    :kconfig:option:`CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE`,
    :kconfig:option:`CONFIG_SYSTEM_TIMER_LPM_COMPANION_COUNTER`,
    :kconfig:option:`CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS`, and
    :kconfig:option:`CONFIG_SYSTEM_TIMER_RESET_BY_LPM`.
    The chosen property ``/chosen/zephyr,cortex-m-idle-timer`` is deprecated in
    favor of ``/chosen/zephyr,system-timer-companion``.
    The compatibility shim is currently scheduled to be removed in Zephyr 4.6.0.

New APIs and options
====================
..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w)

* ADC

  * :c:macro:`ADC_DT_SPEC_GET_BY_IDX_OR`
  * :c:macro:`ADC_DT_SPEC_GET_BY_NAME_OR`
  * :c:macro:`ADC_DT_SPEC_GET_OR`
  * :c:macro:`ADC_DT_SPEC_INST_GET_BY_IDX_OR`
  * :c:macro:`ADC_DT_SPEC_INST_GET_BY_NAME_OR`
  * :c:macro:`ADC_DT_SPEC_INST_GET_OR`
  * :c:member:`adc_sequence.priority`
  * :kconfig:option:`CONFIG_ADC_SEQUENCE_PRIORITY`

* Architectures

  * Xtensa

    * :kconfig:option:`CONFIG_XTENSA_MMU_USE_DEFAULT_MAPPINGS`

* Audio

  * :c:macro:`PDM_DT_IO_CFG_GET`
  * :c:macro:`PDM_DT_HAS_LEFT_CHANNEL`
  * :c:macro:`PDM_DT_HAS_RIGHT_CHANNEL`

* Bluetooth

  * Audio

    * :c:func:`bt_bap_ep_get_conn`
    * :c:member:`bt_ccp_call_control_client_cb.user_data`
    * :kconfig:option:`CONFIG_BT_TBS_MAX_FRIENDLY_NAME_LENGTH`
    * :c:member:`bt_cap_handover_cb.unicast_to_broadcast_created`
    * :c:func:`bt_tbs_client_get_by_index`
    * :c:member:`bt_bap_unicast_client_cb.supported_contexts`

  * Host

    * :c:func:`bt_gatt_cb_unregister` Added an API to unregister GATT callback handlers.
    * :c:func:`bt_le_per_adv_sync_cb_unregister`

  * ISO

    * :c:member:`bt_iso_chan_ops.disconnected` will now always be called before
      :c:member:`bt_conn_cb.disconnected` for unicast (CIS) channels,
      to provide a more deterministic order of callback events. (:github:`104695`).

  * Mesh

    * :c:func:`bt_mesh_input_numeric` to provide provisioning numeric input OOB value.
    * :c:member:`output_numeric` callback in :c:struct:`bt_mesh_prov` structure to
      output numeric values during provisioning.
    * :kconfig:option:`CONFIG_BT_MESH_CDB_KEY_SYNC` to enable key synchronization between
      the Configuration Database (CDB) and the local Subnet and AppKey storages when keys are
      added, deleted, or updated during key refresh procedure.
      The option is enabled by default.

  * Services

    * Introduced Alert Notification Service (ANS) :kconfig:option:`CONFIG_BT_ANS`

* Build system

  * Added ``zephyr_constants_library()`` CMake function for generating
    headers with build-time constants derived from C struct layouts
    (:github:`104861`).

  * Added :ref:`slot1-partition <snippet-slot1-partition>` snippet.

  * Sysbuild

    * Added :kconfig:option:`SB_CONFIG_MERGED_HEX_FILES` which allows generating
      :ref:`merged hex files <sysbuild_merged_hex_files>`.

    * Added experimental ``ExternalZephyrVariantProject_Add()`` sysbuild CMake function which
      allows for adding :ref:`variant images<sysbuild_zephyr_application>` to projects which are
      based on existing images in a build.

    * Added :kconfig:option:`SB_CONFIG_MCUBOOT_DIRECT_XIP_GENERATE_VARIANT` which allows for
      generating slot 1 images automatically in sysbuild projects when using MCUboot in
      direct-xip mode.

* CPUFreq

  * :kconfig:option:`CONFIG_CPU_FREQ_POLICY_PRESSURE`

* DAC

  * Added new DAC driver APIs (:github:`104630`)

    * :c:struct:`dac_dt_spec`
    * :c:macro:`DAC_CHANNEL_CFG_DT`
    * :c:macro:`DAC_DT_SPEC_GET_BY_NAME`
    * :c:macro:`DAC_DT_SPEC_GET_BY_NAME_OR`
    * :c:macro:`DAC_DT_SPEC_INST_GET_BY_NAME`
    * :c:macro:`DAC_DT_SPEC_INST_GET_BY_NAME_OR`
    * :c:macro:`DAC_DT_SPEC_GET_BY_IDX`
    * :c:macro:`DAC_DT_SPEC_GET_BY_IDX_OR`
    * :c:macro:`DAC_DT_SPEC_INST_GET_BY_IDX`
    * :c:macro:`DAC_DT_SPEC_INST_GET_BY_IDX_OR`
    * :c:macro:`DAC_DT_SPEC_GET`
    * :c:macro:`DAC_DT_SPEC_GET_OR`
    * :c:macro:`DAC_DT_SPEC_INST_GET`
    * :c:macro:`DAC_DT_SPEC_INST_GET_OR`
    * :c:func:`dac_channel_setup_dt`
    * :c:func:`dac_write_value_dt`
    * :c:func:`dac_millivolts_to_raw`
    * :c:func:`dac_microvolts_to_raw`
    * :c:func:`dac_x_to_raw_dt_chan`
    * :c:func:`dac_millivolts_to_raw_dt`
    * :c:func:`dac_microvolts_to_raw_dt`
    * :c:func:`dac_is_ready_dt`

* DMA

  * Added new DMA driver (:dtcompatible:`nxp,4ch-dma`) (:github:`97841`).

* Display

  * :c:func:`display_register_event_cb` and :c:func:`display_unregister_event_cb`.
  * :kconfig:option:`CONFIG_SSD1325_DEFAULT_CONTRAST`
  * :kconfig:option:`CONFIG_SSD1325_CONV_BUFFER_LINES`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_XRGB_8888`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_BGR_888`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_ABGR_8888`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_RGBA_8888`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_BGRA_8888`
  * :c:enumerator:`PIXEL_FORMAT_XRGB_8888`
  * :c:enumerator:`PIXEL_FORMAT_BGR_888`
  * :c:enumerator:`PIXEL_FORMAT_ABGR_8888`
  * :c:enumerator:`PIXEL_FORMAT_RGBA_8888`
  * :c:enumerator:`PIXEL_FORMAT_BGRA_8888`
  * :c:macro:`PANEL_PIXEL_FORMAT_XRGB_8888`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_ROUNDED_MASK`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_ROUNDED_MASK_COLOR`
  * ``serial-vcom-inversion`` and ``serial-vcom-interval`` properties of :dtcompatible:`sharp,ls0xx`.
  * :kconfig:option:`CONFIG_LS0XX_VCOM_THREAD_PRIO`

* Ethernet

  * Driver MAC address configuration with support for NVMEM cell.

    * :c:func:`net_eth_mac_load`
    * :c:struct:`net_eth_mac_config`
    * :c:macro:`NET_ETH_MAC_DT_CONFIG_INIT` and :c:macro:`NET_ETH_MAC_DT_INST_CONFIG_INIT`

  * Added :c:enum:`ethernet_stats_type` and optional ``get_stats_type`` callback in
    :c:struct:`ethernet_api` to allow filtering of ethernet statistics by type
    (common, vendor, or all). Drivers that support vendor-specific statistics can
    implement ``get_stats_type`` to skip expensive FW queries when only common stats
    are requested. The existing ``get_stats`` API remains unchanged for backward
    compatibility.

* Exception

  * :kconfig:option:`CONFIG_EXCEPTION_DUMP_HOOK_ONLY`

* Flash

  * :dtcompatible:`jedec,mspi-nor` now allows MSPI configuration of read, write and
    control commands separately via devicetree.

  * Added extended operations to the flash API to support marking blocks as bad
    (:c:enum:`FLASH_EX_OP_MARK_BAD_BLOCK`) and checking if a block is bad
    (:c:enum:`FLASH_EX_OP_IS_BAD_BLOCK`).

* Haptics

  * Added error callback to API

    * :c:enum:`haptics_error_type` to enumerate common fault conditions in haptics devices.
    * :c:type:`haptics_error_callback_t` to provide function prototype for error callbacks.
    * :c:func:`haptics_register_error_callback` to register an error callback with a driver.

* Hwspinlock

  * Added new hwspinlock driver (:dtcompatible:`nxp,sema42`) (:github:`101499`).

* IPM

  * IPM callbacks for the mailbox backend now correctly handle signal-only mailbox
    mailbox usage. Applications should be prepared to receive a NULL payload pointer
    in IPM callbacks when no data buffer is provided by the mailbox.

* Management

  * MCUmgr

    * :kconfig:option:`CONFIG_UART_MCUMGR_RAW_PROTOCOL`,
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_RAW_UART`,
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT`,
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS` see
      :ref:`raw UART MCUmgr SMP transport <mcumgr_smp_transport_raw_uart>` for details.

* Mbed TLS

  * :kconfig:option:`CONFIG_TF_PSA_CRYPTO_USER_CONFIG_FILE`
  * :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128`
  * :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256`
  * :kconfig:option:`CONFIG_MBEDTLS_BASE64_C`
  * :kconfig:option:`CONFIG_MBEDTLS_PEM_PARSE_C`
  * :kconfig:option:`CONFIG_MBEDTLS_PEM_WRITE_C`
  * :kconfig:option:`CONFIG_MBEDTLS_PK_PARSE_C`
  * :kconfig:option:`CONFIG_MBEDTLS_SSL_KEYING_MATERIAL_EXPORT`
  * :kconfig:option:`CONFIG_MBEDTLS_VERSION_C`
  * :kconfig:option:`CONFIG_MBEDTLS_X509_CRT_PARSE_C`
  * :kconfig:option:`CONFIG_MBEDTLS_X509_RSASSA_PSS_SUPPORT`
  * :kconfig:option:`CONFIG_MBEDTLS_X509_USE_C`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_PSK_WITH_AES_256_CBC_SHA384`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_PSK_WITH_AES_128_GCM_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_ECJPAKE_WITH_AES_128_CCM_8`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_256_GCM_SHA384`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_128_GCM_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_128_CCM_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_CHACHA20_POLY1305_SHA256`

* Modem

  * :kconfig:option:`CONFIG_MODEM_HL78XX_AT_SHELL`
  * :kconfig:option:`CONFIG_MODEM_HL78XX_AIRVANTAGE`

* NVMEM

  * Flash device support

    * :kconfig:option:`CONFIG_NVMEM_FLASH`
    * :kconfig:option:`CONFIG_NVMEM_FLASH_WRITE`

* Networking

  * CoAP

    * :kconfig:option:`CONFIG_COAP_CLIENT_MULTICAST`

  * DHCP

    * :c:func:`net_dhcpv4_server_set_address_validator_cb`

  * LwM2M

    * :kconfig:option:`CONFIG_LWM2M_SEND_SCHEDULER`
    * :kconfig:option:`CONFIG_LWM2M_IPSO_MAGNETOMETER`
    * :c:func:`lwm2m_cache_free_slots_get`

  * Misc

    * :kconfig:option:`CONFIG_WIREGUARD`
    * :kconfig:option:`CONFIG_NET_ZPERF_RAW_TX`
    * :kconfig:option:`CONFIG_FTP_CLIENT`

  * OpenThread

    * :kconfig:option:`CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR`

  * Sockets

    * DTLS server socket now supports multiple parallel client sessions on a
      single socket.
    * :kconfig:option:`CONFIG_NET_SOCKETS_TLS_CONNECT_TIMEOUT`

  * Wi-Fi

    * Add support for Wi-Fi Direct (P2P) mode.
    * Add support for WEP (Wired Equivalent Privacy) security. This is disabled by default
      but can be enabled by :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_WEP`
    * Use PSA crypto by default instead of legacy one. This can be controlled by
      :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_MBEDTLS_PSA` option.
    * :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CLEANUP_INTERVAL`
    * :kconfig:option:`CONFIG_WIFI_NM_HOSTAPD_CLEANUP_INTERVAL`

* OTP

  * New OTP driver API providing means to provision (:c:func:`otp_program()`) and
    read (:c:func:`otp_read()`) :abbr:`OTP(One Time Programmable)` memory devices
    (:github:`101292`). OTP devices can also be accessed through the
    :ref:`Non-Volatile Memory (NVMEM)<nvmem>` subsystem. Available options are:

    * :kconfig:option:`CONFIG_OTP`
    * :kconfig:option:`CONFIG_OTP_PROGRAM`
    * :kconfig:option:`CONFIG_OTP_INIT_PRIORITY`

* PWM

  * Extended API with PWM events

    * :c:struct:`pwm_event_callback` to hold a pwm event callback
    * :c:func:`pwm_init_event_callback` to help initialize a :c:struct:`pwm_event_callback` object
    * :c:func:`pwm_add_event_callback` to add a callback
    * :c:func:`pwm_remove_event_callback` to remove a callback
    * :c:member:`manage_event_callback` in :c:struct:`pwm_driver_api` to manage pwm events
    * :kconfig:option:`CONFIG_PWM_EVENT`

* Power

  * The new ``voltage-scale`` property of :dtcompatible:`st,stm32u5-pwr` can be used to
    select the voltage scale manually on STM32U5 series via Devicetree. This notably
    enables usage of the USB controller at lower system clock frequencies.

* Random

  * :kconfig:option:`CONFIG_PSA_CSPRNG_GENERATOR`

* Settings

  * :kconfig:option:`CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION`
  * :kconfig:option:`CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION_VALUE_SIZE`

* Shell

  * :c:func:`shell_readline` for :ref:`user input <shell-readline>`

* Stepper

  * :c:func:`stepper_ctrl_configure_ramp`

* Sys

  * :c:macro:`COND_CASE_1`

* Timeutil

  * :kconfig:option:`CONFIG_TIMEUTIL_APPLY_SKEW`

* Userspace

  * :c:func:`k_object_access_check`
  * :c:func:`k_mem_domain_deinit`

* Utilities

  * :abbr:`COBS (Consistent Overhead Byte Stuffing)` streaming support

    * :c:struct:`cobs_decoder`
    * :c:func:`cobs_decoder_init`
    * :c:func:`cobs_decoder_write`
    * :c:func:`cobs_decoder_close`
    * :c:struct:`cobs_encoder`
    * :c:func:`cobs_encoder_init`
    * :c:func:`cobs_encoder_write`
    * :c:func:`cobs_encoder_close`

  * Disjoint-set support
    * :c:struct:`sys_set_node`
    * :c:func:`sys_set_makeset`
    * :c:func:`sys_set_find`
    * :c:func:`sys_set_union`

* Video

  * :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE`
  * :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_ZEPHYR_REGION`
  * :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_ZEPHYR_REGION_NAME`
  * :c:func:`video_transform_cap`
  * :c:macro:`VIDEO_PIX_FMT_SBGGR8P16`
  * :c:macro:`VIDEO_PIX_FMT_SGBRG8P16`
  * :c:macro:`VIDEO_PIX_FMT_SGRBG8P16`
  * :c:macro:`VIDEO_PIX_FMT_SRGGB8P16`
  * :c:macro:`VIDEO_PIX_FMT_Y8P16`
  * :c:macro:`VIDEO_FMT_IS_SEMI_PLANAR`
  * :c:macro:`VIDEO_FMT_IS_PLANAR`
  * :c:macro:`VIDEO_FMT_IS_GRAYSCALE`
  * :c:macro:`VIDEO_FMT_IS_BAYER`
  * :c:macro:`VIDEO_FMT_IS_RGB`
  * :c:macro:`VIDEO_FMT_IS_YUV`
  * :c:macro:`VIDEO_FMT_IS_MIPI_PACKED`
  * :c:macro:`VIDEO_FMT_IS_PADDED`
  * :c:macro:`VIDEO_FMT_IS_SEMI_PLANAR`
  * :c:macro:`VIDEO_FMT_IS_FULL_PLANAR`
  * :c:macro:`VIDEO_FOREACH_BAYER`
  * :c:macro:`VIDEO_FOREACH_BAYER_PADDED`
  * :c:macro:`VIDEO_FOREACH_BAYER_MIPI_PACKED`
  * :c:macro:`VIDEO_FOREACH_BAYER_NON_PACKED`
  * :c:macro:`VIDEO_FOREACH_GRAYSCALE`
  * :c:macro:`VIDEO_FOREACH_GRAYSCALE_NON_PACKED`
  * :c:macro:`VIDEO_FOREACH_GRAYSCALE_PADDED`
  * :c:macro:`VIDEO_FOREACH_GRAYSCALE_MIPI_PACKED`
  * :c:macro:`VIDEO_FOREACH_RGB`
  * :c:macro:`VIDEO_FOREACH_RGB_PACKED`
  * :c:macro:`VIDEO_FOREACH_RGB_NON_PACKED`
  * :c:macro:`VIDEO_FOREACH_RGB_ALPHA`
  * :c:macro:`VIDEO_FOREACH_RGB_PADDED`
  * :c:macro:`VIDEO_FOREACH_YUV`
  * :c:macro:`VIDEO_FOREACH_YUV_NON_PLANAR`
  * :c:macro:`VIDEO_FOREACH_YUV_SEMI_PLANAR`
  * :c:macro:`VIDEO_FOREACH_YUV_FULL_PLANAR`
  * :c:macro:`VIDEO_FOREACH_COMPRESSED`
  * :c:func:`video_import_buffer`

* Zbus

   * :kconfig:option:`CONFIG_ZBUS_ASYNC_LISTENER`
   * :kconfig:option:`CONFIG_ZBUS_ASYNC_LISTENER_EXEC_TIMEOUT`
   * :kconfig:option:`CONFIG_ZBUS_PROXY_AGENT`
   * :kconfig:option:`CONFIG_ZBUS_PROXY_AGENT_IPC`
   * :c:macro:`ZBUS_ASYNC_LISTENER_DEFINE`
   * :c:macro:`ZBUS_ASYNC_LISTENER_DEFINE_WITH_ENABLE`
   * :c:macro:`ZBUS_PROXY_AGENT_DEFINE`
   * :c:macro:`ZBUS_PROXY_ADD_CHAN`
   * :c:macro:`ZBUS_SHADOW_CHAN_DEFINE`
   * :c:macro:`ZBUS_SHADOW_CHAN_DEFINE_WITH_ID`
   * :c:func:`zbus_chan_from_name`. Retrieve a zbus channel reference by its name string.
   * :c:func:`zbus_async_listener_set_work_queue`. Set the work queue for an async listener
     observer.
   * :c:func:`zbus_chan_pub_stats_msg_age`. Get the message age in milliseconds since the last
     publish.

.. zephyr-keep-sorted-stop

.. _boards_added_in_zephyr_4_4:

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.


* Adafruit Industries, LLC

   * :zephyr:board:`adafruit_feather_propmaker_rp2040` (``adafruit_feather_propmaker_rp2040``)
   * :zephyr:board:`adafruit_feather_scorpio_rp2040` (``adafruit_feather_scorpio_rp2040``)

* Advanced Micro Devices (AMD), Inc.

   * :zephyr:board:`versal2_apu` (``versal2_apu``)
   * :zephyr:board:`versal_apu` (``versal_apu``)
   * :zephyr:board:`versal_rpu` (``versal_rpu``)

* Ai-Thinker Co., Ltd.

   * :zephyr:board:`ai_m61_32s_kit` (``ai_m61_32s_kit``)

* Alientek

   * :zephyr:board:`dnesp32s3b` (``dnesp32s3b``)

* Alif Semiconductor

   * :zephyr:board:`ensemble_e1c_dk` (``ensemble_e1c_dk``)
   * :zephyr:board:`ensemble_e8_dk` (``ensemble_e8_dk``)

* Analog Devices, Inc.

   * :zephyr:board:`adi_eval_adin2111d1z` (``adi_eval_adin2111d1z``)

* ARM Ltd.

   * :zephyr:board:`fvp_base_revc_2xaem` (``fvp_base_revc_2xaem``)

* BlackBerry Limited

   * :zephyr:board:`qnxhv_vm` (``qnxhv_vm``)

* Bouffalo Lab (Nanjing) Co., Ltd.

   * :zephyr:board:`bl706_iot_dvk` (``bl706_iot_dvk``)

* Cadence Design Systems Inc.

   * Cadence SweRV (``cdns_swerv``)

* Chengdu Heltec Automation Technology Co., Ltd.

   * :zephyr:board:`heltec_wifi_lora32_v3` (``heltec_wifi_lora32_v3``)
   * :zephyr:board:`heltec_wireless_tracker` (``heltec_wireless_tracker``)

* Cirrus Logic, Inc.

   * :zephyr:board:`crd40l50` (``crd40l50``)

* Cytron Technologies

   * :zephyr:board:`maker_nano_rp2040` (``maker_nano_rp2040``)
   * :zephyr:board:`maker_pi_rp2040` (``maker_pi_rp2040``)
   * :zephyr:board:`maker_uno_rp2040` (``maker_uno_rp2040``)
   * :zephyr:board:`motion_2350_pro` (``motion_2350_pro``)

* DFRobot

   * :zephyr:board:`beetle_esp32c3` (``beetle_esp32c3``)
   * :zephyr:board:`beetle_rp2350` (``beetle_rp2350``)

* Elan Microelectronic Corp.

   * :zephyr:board:`32f967_dv` (``32f967_dv``)

* Espressif Systems

   * :zephyr:board:`esp32c5_devkitc` (``esp32c5_devkitc``)
   * :zephyr:board:`esp_threadbr` (``esp_threadbr``)

* Ezurio

   * :zephyr:board:`lyra_24_dvk_p10` (``lyra_24_dvk_p10``)
   * :zephyr:board:`lyra_24_dvk_p20` (``lyra_24_dvk_p20``)
   * :zephyr:board:`lyra_24_dvk_p20rf` (``lyra_24_dvk_p20rf``)
   * :zephyr:board:`lyra_24_dvk_s10` (``lyra_24_dvk_s10``)
   * :zephyr:board:`lyra_dvk_p` (``lyra_dvk_p``)
   * :zephyr:board:`lyra_dvk_s` (``lyra_dvk_s``)
   * :zephyr:board:`rm126x_dvk_rm1261` (``rm126x_dvk_rm1261``)
   * :zephyr:board:`rm126x_dvk_rm1262` (``rm126x_dvk_rm1262``)

* FocalTech Systems Co.,Ltd

   * :zephyr:board:`ft9001_eval` (``ft9001_eval``)

* Framework Computer, Inc.

   * :zephyr:board:`framework_ledmatrix` (``framework_ledmatrix``)
   * :zephyr:board:`framework_laptop16_keyboard` (``framework_laptop16_keyboard``)

* Infineon Technologies

   * :zephyr:board:`cy8ckit_041s_max` (``cy8ckit_041s_max``)
   * :zephyr:board:`cy8cproto_041tp` (``cy8cproto_041tp``)
   * :zephyr:board:`kit_t2g_b_h_evk` (``kit_t2g_b_h_evk``)
   * :zephyr:board:`kit_t2g_b_h_lite` (``kit_t2g_b_h_lite``)

* Intel Corporation

   * :zephyr:board:`intel_wcl_crb` (``intel_wcl_crb``)

* Longan Labs (Shenzhen Longan Technology Co., Ltd.)

   * :zephyr:board:`canbed_rp2040` (``canbed_rp2040``)

* M5Stack

   * :zephyr:board:`m5stack_nanoc6` (``m5stack_nanoc6``)

* Makerbase Co., Ltd.

   * :zephyr:board:`mks_canable_v10` (``mks_canable_v10``)

* MediaTek Inc.

   * MT8365 ADSP (``mt8365``)

* Microchip Technology Inc.

   * :zephyr:board:`pic32cm_pl10_cnano` (``pic32cm_pl10_cnano``)
   * :zephyr:board:`pic32cx_sg41_cult` (``pic32cx_sg41_cult``)
   * :zephyr:board:`pic32cz_ca90_cult` (``pic32cz_ca90_cult``)
   * :zephyr:board:`pic64gx_curiosity_kit` (``pic64gx_curiosity_kit``)
   * :zephyr:board:`sam_e54_cult` (``sam_e54_cult``)

* Nordic Semiconductor

   * :zephyr:board:`nrf54l15tag` (``nrf54l15tag``)
   * :zephyr:board:`nrf7120dk` (``nrf7120dk``)

* Nuvoton Technology Corporation

   * :zephyr:board:`numaker_gai_m55m1` (``numaker_gai_m55m1``)

* NXP Semiconductors

   * :zephyr:board:`frdm_imxrt1186` (``frdm_imxrt1186``)
   * :zephyr:board:`frdm_ke16z` (``frdm_ke16z``)
   * :zephyr:board:`frdm_mcxa577` (``frdm_mcxa577``)
   * :zephyr:board:`frdm_mcxl255` (``frdm_mcxl255``)
   * :zephyr:board:`frdm_mcxw70` (``frdm_mcxw70``)
   * :zephyr:board:`s32k5xxcvb` (``s32k5xxcvb``)

* Others

   * :zephyr:board:`doit_esp32_devkit_v1` (``doit_esp32_devkit_v1``)
   * :zephyr:board:`esp32c3_lckfb` (``esp32c3_lckfb``)

* PCB Cupid

   * :zephyr:board:`glyph_c3` (``glyph_c3``)
   * :zephyr:board:`glyph_h2` (``glyph_h2``)

* PHYTEC

   * :zephyr:board:`phyboard_atlas` (``phyboard_atlas``)

* Pimoroni Ltd.

   * :zephyr:board:`tiny2040` (``tiny2040``)

* QEMU

   * :zephyr:board:`qemu_or1k` (``qemu_or1k``)

* Qualcomm Technologies, Inc

   * :zephyr:board:`qcc744m_evk` (``qcc744m_evk``)

* RAKwireless Technology Limited

   * :zephyr:board:`rak11160` (``rak11160``)

* Realtek Semiconductor Corp.

   * :zephyr:board:`rtl8721f_evb` (``rtl8721f_evb``)
   * :zephyr:board:`rtl872xd_evb` (``rtl872xd_evb``)
   * :zephyr:board:`rtl872xda_evb` (``rtl872xda_evb``)
   * :zephyr:board:`rtl8752h_evb` (``rtl8752h_evb``)
   * :zephyr:board:`rtl87x2g_evb_a` (``rtl87x2g_evb_a``)
   * :zephyr:board:`rts5817_maa_evb` (``rts5817_maa_evb``)

* Renesas Electronics Corporation

   * :zephyr:board:`aik_ra8d1` (``aik_ra8d1``)
   * :zephyr:board:`cpkcor_ra8d1b` (``cpkcor_ra8d1b``)
   * :zephyr:board:`ek_ra8t2` (``ek_ra8t2``)
   * :zephyr:board:`fpb_ra0e1` (``fpb_ra0e1``)
   * :zephyr:board:`fpb_ra8e1` (``fpb_ra8e1``)
   * :zephyr:board:`fpb_rx140` (``fpb_rx140``)
   * :zephyr:board:`fpb_rx14t` (``fpb_rx14t``)
   * :zephyr:board:`mcb_rx14t` (``mcb_rx14t``)
   * :zephyr:board:`mck_ra4t1` (``mck_ra4t1``)
   * :zephyr:board:`rsk_rx140` (``rsk_rx140``)
   * :zephyr:board:`rzg3e_smarc` (``rzg3e_smarc``)
   * :zephyr:board:`rzn2h_evb` (``rzn2h_evb``)
   * :zephyr:board:`rzt2h_evb` (``rzt2h_evb``)

* Retronix Technology Inc.

   * :zephyr:board:`sparrowhawk_rcar_v4h` (``sparrowhawk_rcar_v4h``)

* Seeed Technology Co., Ltd

   * :zephyr:board:`reterminal_e1002` (``reterminal_e1002``)
   * :zephyr:board:`xiao_rp2350` (``xiao_rp2350``)

* Shenzhen Holyiot Technology Co., Ltd.

   * :zephyr:board:`holyiot_21014` (``holyiot_21014``)
   * :zephyr:board:`holyiot_25008` (``holyiot_25008``)

* Shenzhen Sipeed Technology Co., Ltd.

   * :zephyr:board:`maix_m0s_dock` (``maix_m0s_dock``)

* Shenzhen Xunlong Software CO.,Limited

   * :zephyr:board:`opi_zero` (``opi_zero``)
   * :zephyr:board:`orangepi_5_ultra_rk3588` (``orangepi_5_ultra_rk3588``)

* Silicon Laboratories

   * :zephyr:board:`efm32tg_stk3300` (``efm32tg_stk3300``)
   * :zephyr:board:`xg28_ek2705a` (``xg28_ek2705a``)
   * :zephyr:board:`siwx917_rb4338a` (``siwx917_rb4338a``)
   * :zephyr:board:`siwx917_rb4342a` (``siwx917_rb4342a``)

* Soldered Electronics

   * :zephyr:board:`inkplate_6color` (``inkplate_6color``)

* Space Cubics Inc.

   * :zephyr:board:`scobc_v1` (``scobc_v1``)

* SparkFun Electronics

   * :zephyr:board:`sparkfun_rp2040_mikrobus` (``sparkfun_rp2040_mikrobus``)

* STMicroelectronics

   * :zephyr:board:`nucleo_c542rc` (``nucleo_c542rc``)
   * :zephyr:board:`nucleo_c562re` (``nucleo_c562re``)
   * :zephyr:board:`nucleo_c5a3zg` (``nucleo_c5a3zg``)
   * :zephyr:board:`nucleo_u3c5zi_q` (``nucleo_u3c5zi_q``)
   * :zephyr:board:`nucleo_wba25ce1` (``nucleo_wba25ce1``)
   * :zephyr:board:`stm32h5f5j_dk` (``stm32h5f5j_dk``)
   * :zephyr:board:`stm32mp215f_dk` (``stm32mp215f_dk``)

* Synaptics

   * :zephyr:board:`sr100_rdk` (``sr100_rdk``)

* Texas Instruments

   * :zephyr:board:`am62l_evm` (``am62l_evm``)
   * :zephyr:board:`cc1312r1_launchxl` (``cc1312r1_launchxl``)

* Third Reality, Inc.

   * :zephyr:board:`3r_tnh_sensor_lite` (``3r_tnh_sensor_lite``)

* u-blox

   * :zephyr:board:`ubx_evkninab5` (``ubx_evkninab5``)

* Vicharak

   * :zephyr:board:`shrike_lite` (``shrike_lite``)

* VIEWE Display Co., Ltd.

   * :zephyr:board:`uedx24320028e_wb_a` (``uedx24320028e_wb_a``)

* Waveshare Electronics

   * :zephyr:board:`esp32s3_geek` (``esp32s3_geek``)
   * :zephyr:board:`esp32s3_rlcd_4_2` (``esp32s3_rlcd_4_2``)
   * :zephyr:board:`rp2350_zero` (``rp2350_zero``)

* WeAct Studio

   * :zephyr:board:`can485dbv1` (``can485dbv1``)
   * :zephyr:board:`rp2350b_core` (``rp2350b_core``)
   * :zephyr:board:`weact_stm32g0b1_core` (``weact_stm32g0b1_core``)

* WEMOS Electronics

   * :zephyr:board:`lolin32_lite` (``lolin32_lite``)

* WinChipHead

   * :zephyr:board:`ch32v307v_evt_r1` (``ch32v307v_evt_r1``)

.. _shields_added_in_zephyr_4_4:

New Shields
***********

..
  Same as above, this will also be recomputed at the time of the release.

* :ref:`Adafruit FeatherWing 128x64 OLED Shield <adafruit_featherwing_128x64_oled>`
* :ref:`Adafruit HTS221 Shield <adafruit_hts221>`
* :ref:`Adafruit INA3221 Shield <adafruit_ina3221>`
* :ref:`Adafruit MAX17048 Shield <adafruit_max17048>`
* :ref:`Adafruit MCP4728 Quad DAC Shield <adafruit_mcp4728>`
* :ref:`Analog Devices EVAL-CN0391-ARDZ <eval_cn0391_ardz>`
* :ref:`Arduino Modulino Latch Relay <arduino_modulino_latch_relay>`
* :ref:`ESP Thread BR / Zigbee GW Ethernet <esp_threadbr_ethernet>`
* :ref:`Microchip RNBD451 Add-on Board <rnbd451_add_on_shield>`
* :ref:`MikroElektronika 3 axis Accel 4 Click <mikroe_accel4_click_shield>`
* :ref:`MikroElektronika CAN FD 6 Click <mikroe_can_fd_6_click_shield>`
* :ref:`MikroElektronika EEPROM 13 Click <mikroe_eeprom_13_click_shield>`
* :ref:`MikroElektronika Flash 5 Click <mikroe_flash_5_click_shield>`
* :ref:`MikroElektronika Flash 6 Click <mikroe_flash_6_click_shield>`
* :ref:`MikroElektronika Flash 8 Click <mikroe_flash_8_click_shield>`
* :ref:`MikroElektronika LTE IoT 7 Click <mikroe_lte_iot7_click_shield>`
* :ref:`MikroElektronika MCP251x Click shields <mikroe_mcp251x_click_shield>`
* :ref:`MikroElektronika MCP251xFD Click shields <mikroe_mcp251xfd_click_shield>`
* :ref:`MikroElektronika RS485 Isolator 5 Click <mikroe_rs485_isolator_5_click_shield>`
* :ref:`MikroElektronika Temp&Hum Click <mikroe_temp_hum_click_shield>`
* :ref:`Nordic Semiconductor nRF7002 EB II <nrf7002eb2>`
* :ref:`NXP S32K5XX-MB Shield <nxp_s32k5xx_mb>`
* :ref:`Raspberry Pi Camera Module 2 <raspberry_pi_camera_module_2>`
* :ref:`Renesas AIK OV2640 Camera Shield <renesas_aik_ov2640_cam>`
* :ref:`Seeed Studio 24GHz mmWave Sensor for XIAO <seeed_xiao_hsp24>`
* :ref:`Semtech SX1261MB2BAS LoRa Shield <semtech_sx1261mb2bas>`
* :ref:`ST Microelectronics B-DSI-MB1314 <st_b_dsi_mb1314>`
* :ref:`ST Microelectronics ST87MXX shield <st87mxx_generic>`
* :ref:`ST Microelectronics X-NUCLEO-IKS5A1: MEMS Inertial and Environmental Multi sensor shield <x-nucleo-iks5a1>`
* :ref:`WIZnet W5500 Ethernet Shield <wiznet_w5500>`
* :ref:`ZHAW Luma Matrix Shield <zhaw_lumamatrix>`

New Drivers
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* :abbr:`ADC (Analog to Digital Converter)`

   * :dtcompatible:`bflb,adc` (:github:`98624`)
   * :dtcompatible:`infineon,sar-adc` (:github:`103453`)
   * :dtcompatible:`maxim,max2253x` (:github:`102115`)
   * :dtcompatible:`microchip,adc-g1` (:github:`99966`)
   * :dtcompatible:`microchip,mcp3221` (:github:`105751`)
   * :dtcompatible:`renesas,ra-adc12` (:github:`95710`)
   * :dtcompatible:`renesas,ra-adc16` (:github:`95710`)
   * :dtcompatible:`renesas,rz-adc-e` (:github:`100575`)
   * :dtcompatible:`renesas,rza2m-adc` (:github:`100637`)
   * :dtcompatible:`sifli,sf32lb-gpadc` (:github:`99460`)
   * :dtcompatible:`ti,ads7950` (:github:`101660`)
   * :dtcompatible:`ti,ads7951` (:github:`101660`)
   * :dtcompatible:`ti,ads7952` (:github:`101660`)
   * :dtcompatible:`ti,ads7953` (:github:`101660`)
   * :dtcompatible:`ti,ads7954` (:github:`101660`)
   * :dtcompatible:`ti,ads7955` (:github:`101660`)
   * :dtcompatible:`ti,ads7956` (:github:`101660`)
   * :dtcompatible:`ti,ads7957` (:github:`101660`)
   * :dtcompatible:`ti,ads7958` (:github:`101660`)
   * :dtcompatible:`ti,ads7959` (:github:`101660`)
   * :dtcompatible:`ti,ads7960` (:github:`101660`)
   * :dtcompatible:`ti,ads7961` (:github:`101660`)

* ARM architecture

   * :dtcompatible:`arm,axi-timing-adapter` (:github:`100356`)
   * :dtcompatible:`nordic,nrf-pwr-antswc` (:github:`101199`)
   * :dtcompatible:`nordic,nrf-tampc` (:github:`99295`)

* Audio

   * :dtcompatible:`awinic,aw88298` (:github:`97006`)
   * :dtcompatible:`infineon,pdm` (:github:`104698`)
   * :dtcompatible:`sifli,sf32lb-audcodec` (:github:`98701`)
   * :dtcompatible:`zephyr,pdm-dmic` (:github:`99351`)

* Biometrics

   * :dtcompatible:`adh-tech,gt5x` (:github:`100139`)
   * :dtcompatible:`zephyr,biometrics-emul` (:github:`100139`)
   * :dtcompatible:`zhiantec,zfm-x0` (:github:`100139`)

* Bluetooth

   * :dtcompatible:`bflb,bl70x-bt-hci` (:github:`104346`)
   * :dtcompatible:`infineon,bt-hci-uart` (:github:`103871`)
   * :dtcompatible:`realtek,bee-bt-hci` (:github:`104580`)
   * :dtcompatible:`sifli,sf32lb-mailbox` (:github:`96692`)

* :abbr:`CAN (Controller Area Network)`

   * :dtcompatible:`infineon,can` (:github:`105471`)
   * :dtcompatible:`infineon,canfd-controller` (:github:`105471`)

* Charger

   * :dtcompatible:`nordic,npm10xx-charger` (:github:`105564`)
   * :dtcompatible:`ti,bq25186` (:github:`97157`)
   * :dtcompatible:`ti,bq25188` (:github:`97157`)
   * :dtcompatible:`zephyr,charger-gpio` (:github:`103112`)

* Clock control

   * :dtcompatible:`alif,clockctrl` (:github:`101244`)
   * :dtcompatible:`bflb,bl70x_l-clock-controller` (:github:`104625`)
   * :dtcompatible:`bflb,bl70x_l-dll` (:github:`104625`)
   * :dtcompatible:`bflb,f32k` (:github:`104738`)
   * :dtcompatible:`bflb,pll` (:github:`104738`)
   * :dtcompatible:`bflb,root-clk` (:github:`104738`)
   * :dtcompatible:`elan,em32-ahb` (:github:`97843`)
   * :dtcompatible:`elan,em32-apb` (:github:`97843`)
   * :dtcompatible:`focaltech,ft9001-cpm` (:github:`95959`)
   * :dtcompatible:`microchip,pic32cm-jh-clock` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-fdpll` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-gclkgen` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-gclkperiph` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-mclkcpu` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-mclkperiph` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-osc32k` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-osc48m` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-rtc` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-xosc` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-xosc32k` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-pl-clock` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-gclkgen` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-gclkperiph` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-mclkcpu` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-mclkperiph` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-osc32k` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-oschf` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-rtc` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-xosc32k` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cz-ca-clock` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-dfll48m` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-dpll` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-gclkgen` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-gclkperiph` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-mclkdomain` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-mclkperiph` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-rtc` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-xosc` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-xosc32k` (:github:`101934`)
   * :dtcompatible:`nordic,nrf71-hfxo` (:github:`103349`)
   * :dtcompatible:`nordic,nrf71-lfxo` (:github:`101199`)
   * :dtcompatible:`realtek,ameba-rcc` (:github:`104843`)
   * :dtcompatible:`realtek,bee-cctl` (:github:`102691`)
   * :dtcompatible:`realtek,rts5817-clock` (:github:`91486`)
   * :dtcompatible:`renesas,r8a779g0-cpg-mssr` (:github:`97783`)
   * :dtcompatible:`st,stm32c5-rcc` (:github:`105577`)
   * :dtcompatible:`st,stm32c5-xsik-clock` (:github:`105577`)
   * :dtcompatible:`st,stm32fx-pll-clock` (:github:`100757`)
   * :dtcompatible:`syna,sr100-clock` (:github:`100172`)
   * :dtcompatible:`ti,k2g-sci-clk` (:github:`90216`)

* Comparator

   * :dtcompatible:`microchip,ac-g1-comparator` (:github:`99155`)
   * :dtcompatible:`nxp,acomp` (:github:`100818`)
   * :dtcompatible:`nxp,hscmp` (:github:`100629`)
   * :dtcompatible:`nxp,lpcmp` (:github:`100998`)

* Counter

   * :dtcompatible:`bflb,rtc` (:github:`104739`)
   * :dtcompatible:`bflb,timer` (:github:`104739`)
   * :dtcompatible:`bflb,timer-channel` (:github:`104739`)
   * :dtcompatible:`microchip,rtc-g1-counter` (:github:`102163`)
   * :dtcompatible:`microchip,sam-pit64b-counter` (:github:`93806`)
   * :dtcompatible:`microchip,tc-g1` (:github:`100070`)
   * :dtcompatible:`microchip,tc-g1-counter` (:github:`101941`)
   * :dtcompatible:`microchip,tc-g2-counter` (:github:`93401`)
   * :dtcompatible:`microchip,tcc-g1-counter` (:github:`100745`)
   * :dtcompatible:`microcrystal,rv3032-counter` (:github:`98918`)
   * :dtcompatible:`nuvoton,npck-lct` (:github:`98548`)
   * :dtcompatible:`nuvoton,npcx-lct-base` (:github:`98548`)
   * :dtcompatible:`nuvoton,npcx-lct-v1` (:github:`98548`)
   * :dtcompatible:`nuvoton,npcx-lct-v2` (:github:`98548`)
   * :dtcompatible:`nxp,imx-gpt` (:github:`101040`)
   * :dtcompatible:`raspberrypi,pico-pit` (:github:`85618`)
   * :dtcompatible:`raspberrypi,pico-pit-channel` (:github:`105006`)
   * :dtcompatible:`realtek,bee-counter-timer` (:github:`104805`)
   * :dtcompatible:`renesas,rza2m-ostm-counter` (:github:`100934`)
   * :dtcompatible:`silabs,burtc-counter` (:github:`102272`)
   * :dtcompatible:`silabs,protimer` (:github:`103428`)
   * :dtcompatible:`silabs,timer-counter` (:github:`103885`)

* CPU

   * :dtcompatible:`adi,max32-rv32` (:github:`97309`)
   * :dtcompatible:`arm,cortex-a320` (:github:`96852`)
   * :dtcompatible:`arm,cortex-a510` (:github:`96852`)
   * :dtcompatible:`arm,cortex-a7` (:github:`101582`)
   * :dtcompatible:`arm,cortex-a9` (:github:`101582`)
   * :dtcompatible:`cdns,swerv,s400` (:github:`102288`)
   * :dtcompatible:`cdns,swerv,s420` (:github:`102288`)
   * :dtcompatible:`intel,wildcat-lake` (:github:`99205`)
   * :dtcompatible:`riscv` (:github:`105006`)
   * :dtcompatible:`spinalhdl,vexriscv` (:github:`97925`)

* :abbr:`CRC (Cyclic Redundancy Check)`

   * :dtcompatible:`nxp,crc` (:github:`100875`)
   * :dtcompatible:`nxp,lpc-crc` (:github:`101528`)
   * :dtcompatible:`sifli,sf32lb-crc` (:github:`98997`)
   * :dtcompatible:`silabs,gpcrc` (:github:`104471`)
   * :dtcompatible:`st,stm32-crc` (:github:`105302`)

* Cryptographic accelerator

   * :dtcompatible:`bflb,sec-eng-aes` (:github:`104371`)
   * :dtcompatible:`bflb,sec-eng-sha` (:github:`104371`)
   * :dtcompatible:`bflb,sec-eng-trng` (:github:`104349`)
   * :dtcompatible:`microchip,aes-g1` (:github:`105389`)
   * :dtcompatible:`microchip,sha-g1-crypto` (:github:`98894`)
   * :dtcompatible:`nxp,s32-crypto-hse-mu` (:github:`79351`)
   * :dtcompatible:`raspberrypi,pico-sha256` (:github:`85036`)
   * :dtcompatible:`sifli,sf32lb-crypto` (:github:`100583`)

* :abbr:`DAC (Digital to Analog Converter)`

   * :dtcompatible:`microchip,dac-g1` (:github:`101431`)
   * :dtcompatible:`nxp,hpdac` (:github:`104642`)
   * :dtcompatible:`ti,dac5311` (:github:`90811`)
   * :dtcompatible:`ti,dac6311` (:github:`90811`)
   * :dtcompatible:`ti,dac7311` (:github:`90811`)
   * :dtcompatible:`ti,dac8311` (:github:`90811`)
   * :dtcompatible:`ti,dac8411` (:github:`90811`)
   * :dtcompatible:`zephyr,dac-emul` (:github:`100306`)

* Disk

   * :dtcompatible:`zephyr,ftl-dhara` (:github:`100858`)

* Display

   * :dtcompatible:`eink,ac057tc1` (:github:`104142`)
   * :dtcompatible:`ilitek,ili9163c` (:github:`104071`)
   * :dtcompatible:`nxp,imx-lcdifv2` (:github:`103646`)
   * :dtcompatible:`qemu,ramfb` (:github:`103887`)
   * :dtcompatible:`sifli,sf32lb-lcdc` (:github:`99549`)
   * :dtcompatible:`sitronix,st7586s` (:github:`103296`)
   * :dtcompatible:`solomon,ssd1325` (:github:`102128`)
   * :dtcompatible:`waveshare,dsi2dpi` (:github:`100140`)

* :abbr:`DMA (Direct Memory Access)`

   * :dtcompatible:`infineon,dmac` (:github:`101583`)
   * :dtcompatible:`microchip,dmac-g1-dma` (:github:`96300`)
   * :dtcompatible:`microchip,dmac-g2-dma` (:github:`104404`)
   * :dtcompatible:`nxp,4ch-dma` (:github:`97841`)

* :abbr:`EDAC (Error Detection and Correction)`

   * :dtcompatible:`nxp,eim` (:github:`94111`)
   * :dtcompatible:`nxp,erm` (:github:`94111`)

* Ethernet

   * :dtcompatible:`davicom,dm9051` (:github:`104715`)
   * :dtcompatible:`ethernet-phy-fixed-link` (:github:`100454`)
   * :dtcompatible:`maxlinear,gpy111` (:github:`100995`)
   * :dtcompatible:`microchip,lan8742` (:github:`96134`)
   * :dtcompatible:`motorcomm,yt8521` (:github:`97535`)
   * :dtcompatible:`motorcomm,yt8531` (:github:`104945`)
   * :dtcompatible:`nxp,t1s-phy` (:github:`105033`)
   * :dtcompatible:`renesas,ra-eswm` (:github:`100995`)
   * :dtcompatible:`renesas,ra-ethernet-rmac` (:github:`100995`)
   * :dtcompatible:`renesas,ra-mdio-rmac` (:github:`100995`)
   * :dtcompatible:`st,stm32h5-ethernet` (:github:`100910`)
   * :dtcompatible:`st,stm32mp13-ethernet` (:github:`96134`)
   * :dtcompatible:`wch,ethernet` (:github:`101390`)
   * :dtcompatible:`wch,ethernet-controller` (:github:`101390`)
   * :dtcompatible:`wch,mdio` (:github:`101390`)
   * :dtcompatible:`wiznet,w6100` (:github:`101753`)
   * :dtcompatible:`xlnx,xps-ethernetlite-1.00.a` (:github:`95073`)
   * :dtcompatible:`xlnx,xps-ethernetlite-1.00.a-mac` (:github:`95073`)
   * :dtcompatible:`xlnx,xps-ethernetlite-1.00.a-mdio` (:github:`103944`)
   * :dtcompatible:`xlnx,xps-ethernetlite-3.00.a` (:github:`95073`)
   * :dtcompatible:`xlnx,xps-ethernetlite-3.00.a-mac` (:github:`95073`)
   * :dtcompatible:`xlnx,xps-ethernetlite-3.00.a-mdio` (:github:`103944`)

* Firmware

   * :dtcompatible:`arm,scmi-smc` (:github:`103584`)
   * :dtcompatible:`arm,scmi-system` (:github:`99037`)
   * :dtcompatible:`qemu,fw-cfg-ioport` (:github:`103717`)
   * :dtcompatible:`qemu,fw-cfg-mmio` (:github:`103717`)

* Flash controller

   * :dtcompatible:`nxp,c40-flash-controller` (:github:`97401`)
   * :dtcompatible:`renesas,rza2m-qspi-spibsc` (:github:`102175`)
   * :dtcompatible:`st,stm32c5-flash-controller` (:github:`105577`)

* Fuel gauge

   * :dtcompatible:`hycon,hy4245` (:github:`105006`)

* :abbr:`GNSS (Global Navigation Satellite System)`

   * :dtcompatible:`globaltop,pa6h` (:github:`104789`)

* :abbr:`GPIO (General Purpose Input/Output)`

   * :dtcompatible:`elan,em32-gpio` (:github:`97843`)
   * :dtcompatible:`espressif,esp-threadbr-header` (:github:`99704`)
   * :dtcompatible:`infineon,cyw43-gpio` (:github:`104728`)
   * :dtcompatible:`infineon,shared-gpio` (:github:`105081`)
   * :dtcompatible:`microchip,xpro-header` (:github:`98043`)
   * :dtcompatible:`nordic,expansion-board-header` (:github:`104138`)
   * :dtcompatible:`nxp,sc18is606-gpio` (:github:`100743`)
   * :dtcompatible:`realtek,ameba-gpio` (:github:`78036`)
   * :dtcompatible:`realtek,bee-gpio` (:github:`102691`)
   * :dtcompatible:`renesas,rz-gpio-common` (:github:`101256`)
   * :dtcompatible:`renesas,rz-gpio-common-v2` (:github:`101256`)
   * :dtcompatible:`renesas,rz-gpio-common-v3` (:github:`104804`)
   * :dtcompatible:`solderedelectronics,easyc-connector` (:github:`104919`)

* Haptics

   * :dtcompatible:`cirrus,cs40l5x` (:github:`100042`)

* Hardware information

   * :dtcompatible:`microchip,hwinfo-g1` (:github:`100147`)
   * :dtcompatible:`nxp,rcm-hwinfo` (:github:`102490`)
   * :dtcompatible:`nxp,sim-uuid` (:github:`102490`)

* Hardware spinlock

   * :dtcompatible:`nxp,sema42` (:github:`101499`)

* :abbr:`I2C (Inter-Integrated Circuit)`

   * :dtcompatible:`bflb,i2c` (:github:`98364`)
   * :dtcompatible:`microchip,sercom-g1-i2c` (:github:`98385`)
   * :dtcompatible:`renesas,rza2m-riic` (:github:`100513`)
   * :dtcompatible:`sifli,sf32lb-i2c` (:github:`96316`)

* :abbr:`I2S (Inter-IC Sound)`

   * :dtcompatible:`adi,max32-i2s` (:github:`91508`)
   * :dtcompatible:`infineon,i2s` (:github:`100606`)

* Input

   * :dtcompatible:`adafruit,seesaw-gamepad` (:github:`105508`)
   * :dtcompatible:`bflb,irx` (:github:`100600`)
   * :dtcompatible:`chipsemi,chsc6540` (:github:`104710`)
   * :dtcompatible:`focaltech,ft6146` (:github:`96330`)
   * :dtcompatible:`hynitron,cst8xx` (:github:`105348`)
   * :dtcompatible:`nxp,tsi-input` (:github:`103116`)
   * :dtcompatible:`parade,tma525b` (:github:`101254`)
   * :dtcompatible:`realtek,bee-keyscan` (:github:`105110`)
   * :dtcompatible:`wch,ch9350l` (:github:`101976`)

* Interrupt controller

   * :dtcompatible:`adi,max32-rv32-intc` (:github:`97309`)
   * :dtcompatible:`cdns,swerv-pic` (:github:`102288`)
   * :dtcompatible:`microchip,aic-g1-intc` (:github:`101016`)
   * :dtcompatible:`microchip,eic-g1-intc` (:github:`100928`)
   * :dtcompatible:`nxp,gint` (:github:`100240`)
   * :dtcompatible:`opencores,or1k-pic-level` (:github:`98160`)
   * :dtcompatible:`renesas,rx-grp-intc` (:github:`96451`)
   * :dtcompatible:`renesas,rz-icu-v2` (:github:`104804`)
   * :dtcompatible:`renesas,rz-intc-v2` (:github:`101256`)
   * :dtcompatible:`renesas,rz-tint` (:github:`101256`)
   * :dtcompatible:`riscv,aplic` (:github:`104730`)
   * :dtcompatible:`riscv,imsic` (:github:`102055`)

* :abbr:`LED (Light Emitting Diode)`

   * :dtcompatible:`issi,is31fl3197` (:github:`96821`)
   * :dtcompatible:`sct,sct2024` (:github:`98698`)

* LoRa

   * :dtcompatible:`semtech,llcc68` (:github:`100705`)
   * :dtcompatible:`semtech,sx1268` (:github:`100705`)
   * :dtcompatible:`semtech,sx1278` (:github:`100705`)

* Mailbox

   * :dtcompatible:`adi,mbox-max32-sema` (:github:`104547`)
   * :dtcompatible:`raspberrypi,pico-mbox` (:github:`94502`)
   * :dtcompatible:`xlnx,mbox-versal-ipi-mailbox` (:github:`92768`)

* :abbr:`MCTP (Management Component Transport Protocol)`

   * :dtcompatible:`zephyr,mctp-i3c-controller` (:github:`105006`)
   * :dtcompatible:`zephyr,mctp-i3c-endpoint` (:github:`105006`)
   * :dtcompatible:`zephyr,mctp-i3c-target` (:github:`105006`)

* Memory controller

   * :dtcompatible:`adi,max32-backup-sram` (:github:`104528`)

* :abbr:`MFD (Multi-Function Device)`

   * :dtcompatible:`adi,max2221x` (:github:`97584`)
   * :dtcompatible:`microcrystal,rv3032-mfd` (:github:`98918`)
   * :dtcompatible:`nordic,npm10xx` (:github:`105447`)

* :abbr:`MIPI DBI (Mobile Industry Processor Interface Display Bus Interface)`

   * :dtcompatible:`bflb,dbi` (:github:`98752`)
   * :dtcompatible:`espressif,esp32-lcd-cam-mipi-dbi` (:github:`99863`)
   * :dtcompatible:`raspberrypi,pico-mipi-dbi-pio` (:github:`91350`)
   * :dtcompatible:`sifli,sf32lb-lcdc-mipi-dbi` (:github:`99549`)

* Miscellaneous

   * :dtcompatible:`adi,max2221x-misc` (:github:`97584`)
   * :dtcompatible:`espressif,esp32-lcd-cam` (:github:`99863`)
   * :dtcompatible:`nordic,axon` (:github:`102160`)
   * :dtcompatible:`raspberrypi,pico-sio` (:github:`94502`)
   * :dtcompatible:`renesas,ra-drw` (:github:`97163`)
   * :dtcompatible:`renesas,ra-sau` (:github:`102379`)
   * :dtcompatible:`renesas,ra-sau-channel` (:github:`102379`)
   * :dtcompatible:`skyworks,sky13348` (:github:`102321`)
   * :dtcompatible:`st,stm32-npu-cache` (:github:`102232`)

* Modem

   * :dtcompatible:`st,st87mxx` (:github:`100366`)

* Multi-bit SPI

   * :dtcompatible:`st,stm32-ospi-controller` (:github:`96670`)
   * :dtcompatible:`st,stm32-qspi-controller` (:github:`96670`)
   * :dtcompatible:`st,stm32-xspi-controller` (:github:`96670`)

* :abbr:`MTD (Memory Technology Device)`

   * :dtcompatible:`jedec,spi-nand` (:github:`100845`)
   * :dtcompatible:`mxicy,mx25u` (:github:`104357`)
   * :dtcompatible:`netsol,s3axx04` (:github:`97867`)
   * :dtcompatible:`nxp,c40-flash` (:github:`97401`)
   * :dtcompatible:`nxp,imx-flexspi-is66wvs8m8` (:github:`100976`)
   * :dtcompatible:`nxp,s32-xspi-device` (:github:`101487`)
   * :dtcompatible:`nxp,s32-xspi-hyperram` (:github:`101487`)
   * :dtcompatible:`zephyr,mapped-partition` (:github:`104398`)

* :abbr:`OPAMP (Operational Amplifier)`

   * :dtcompatible:`st,stm32-opamp` (:github:`99181`)
   * :dtcompatible:`st,stm32g4-opamp` (:github:`99181`)

* :abbr:`OTP (One Time Programmable)` Memory

   * :dtcompatible:`nxp,ocotp` (:github:`103089`)
   * :dtcompatible:`sifli,sf32lb-efuse` (:github:`101926`)
   * :dtcompatible:`st,stm32-bsec` (:github:`102403`)
   * :dtcompatible:`st,stm32-nvm-otp` (:github:`102976`)
   * :dtcompatible:`zephyr,otp-emul` (:github:`101292`)

* :abbr:`P-state (Performance State)`

   * :dtcompatible:`nxp,mcxn-pstate` (:github:`105006`)

* Pin control

   * :dtcompatible:`alif,pinctrl` (:github:`101244`)
   * :dtcompatible:`brcm,bcm2711-pinctrl` (:github:`101008`)
   * :dtcompatible:`nxp,s32k5-pinctrl` (:github:`100803`)
   * :dtcompatible:`realtek,ameba-pinctrl` (:github:`78036`)
   * :dtcompatible:`realtek,bee-pinctrl` (:github:`102691`)
   * :dtcompatible:`realtek,rts5817-pinctrl` (:github:`91486`)
   * :dtcompatible:`renesas,ra0-pinctrl-pfs` (:github:`102379`)
   * :dtcompatible:`st,stm32h5-pinctrl` (:github:`105856`)
   * :dtcompatible:`syna,sr100-pinctrl` (:github:`100172`)

* Power management CPU operations

   * :dtcompatible:`arm,fvp-pwrc` (:github:`96852`)

* Power management

   * :dtcompatible:`bflb,power-controller` (:github:`102063`)
   * :dtcompatible:`st,stm32-dualreg-pwr` (:github:`99171`)
   * :dtcompatible:`st,stm32-iocell` (:github:`100539`)
   * :dtcompatible:`st,stm32h5-iocell` (:github:`104599`)
   * :dtcompatible:`st,stm32h7-pwr` (:github:`99171`)
   * :dtcompatible:`st,stm32h7rs-pwr` (:github:`99171`)
   * :dtcompatible:`st,stm32u5-pwr` (:github:`100319`)
   * :dtcompatible:`st,stm32wba-pwr` (:github:`105279`)

* Power domain

   * :dtcompatible:`arm,scmi-power-domain` (:github:`102370`)

* :abbr:`PS/2 (Personal System/2)`

   * :dtcompatible:`ite,it51xxx-ps2` (:github:`102790`)

* :abbr:`PWM (Pulse Width Modulation)`

   * :dtcompatible:`adi,max2221x-pwm` (:github:`97584`)
   * :dtcompatible:`bflb,pwm-1` (:github:`99195`)
   * :dtcompatible:`bflb,pwm-2` (:github:`99195`)
   * :dtcompatible:`elan,em32-pwm` (:github:`97843`)
   * :dtcompatible:`microchip,tc-g1-pwm` (:github:`100070`)
   * :dtcompatible:`renesas,rza2m-gpt-pwm` (:github:`100932`)
   * :dtcompatible:`sifli,sf32lb-atim-pwm` (:github:`100137`)
   * :dtcompatible:`sifli,sf32lb-gpt-pwm` (:github:`99362`)

* Regulator

   * :dtcompatible:`arduino,modulino-latch-relay` (:github:`104466`)
   * :dtcompatible:`bflb,aon-regulator` (:github:`102063`)
   * :dtcompatible:`bflb,rt-regulator` (:github:`102063`)
   * :dtcompatible:`bflb,soc-regulator` (:github:`102063`)
   * :dtcompatible:`espressif,esp32-regulator` (:github:`105076`)
   * :dtcompatible:`nordic,npm10xx-regulator` (:github:`105562`)
   * :dtcompatible:`nordic,vregusb-regulator` (:github:`97642`)
   * :dtcompatible:`st,stm32-vrefbuf` (:github:`99304`)
   * :dtcompatible:`ti,tps55287` (:github:`98662`)

* Reset controller

   * :dtcompatible:`focaltech,ft9001-cpm-rctl` (:github:`95959`)
   * :dtcompatible:`realtek,rts5817-reset` (:github:`91486`)
   * :dtcompatible:`syna,sr100-reset` (:github:`100172`)

* :abbr:`RNG (Random Number Generator)`

   * :dtcompatible:`gd,gd32-trng` (:github:`101559`)
   * :dtcompatible:`microchip,trng-g1-entropy` (:github:`99183`)
   * :dtcompatible:`raspberrypi,pico-rng` (:github:`83346`)
   * :dtcompatible:`renesas,ra-rsip-e50d-trng` (:github:`100995`)
   * :dtcompatible:`sifli,sf32lb-trng` (:github:`98467`)
   * :dtcompatible:`ti,mspm0-trng` (:github:`94733`)
   * :dtcompatible:`wch,rng` (:github:`101390`)

* :abbr:`RTC (Real Time Clock)`

   * :dtcompatible:`adi,max31331` (:github:`100508`)
   * :dtcompatible:`maxim,ds1302` (:github:`103964`)
   * :dtcompatible:`microchip,rtc-g1` (:github:`99144`)
   * :dtcompatible:`microchip,rtc-g2` (:github:`99889`)
   * :dtcompatible:`nxp,rtc-jdp` (:github:`98114`)

* :abbr:`SDHC (Secure Digital Host Controller)`

   * :dtcompatible:`infineon,sdhc-sdio` (:github:`100644`)
   * :dtcompatible:`litex,mmc` (:github:`93816`)

* Sensors

   * :dtcompatible:`adi,ade7978` (:github:`104030`)
   * :dtcompatible:`adi,adt7410` (:github:`105009`)
   * :dtcompatible:`adi,adt7422` (:github:`105009`)
   * :dtcompatible:`adi,adxl355` (:github:`103387`)
   * :dtcompatible:`adi,max30210` (:github:`100511`)
   * :dtcompatible:`ams,as5048` (:github:`100382`)
   * :dtcompatible:`ams,as6221` (:github:`94899`)
   * :dtcompatible:`avia,hx711-spi` (:github:`104416`)
   * :dtcompatible:`iclegend,s3km1110` (:github:`104279`)
   * :dtcompatible:`invensense,icm45605` (:github:`101061`)
   * :dtcompatible:`invensense,icm45605s` (:github:`101061`)
   * :dtcompatible:`invensense,icm45686s` (:github:`101061`)
   * :dtcompatible:`invensense,icm45688p` (:github:`101061`)
   * :dtcompatible:`liteon,ltr553` (:github:`101669`)
   * :dtcompatible:`microcrystal,rv3032-temp` (:github:`98918`)
   * :dtcompatible:`nordic,npm10xx-adc` (:github:`105597`)
   * :dtcompatible:`nuvoton,npcx-adc-v2t` (:github:`105006`)
   * :dtcompatible:`nxp,mcux-qdc` (:github:`104880`)
   * :dtcompatible:`nxp,tempsense` (:github:`101525`)
   * :dtcompatible:`qst,qmi8658a` (:github:`104345`)
   * :dtcompatible:`sensirion,stcc4` (:github:`104929`)
   * :dtcompatible:`sifli,sf32lb-tsen` (:github:`99463`)
   * :dtcompatible:`st,ism6hg256x` (:github:`95802`)
   * :dtcompatible:`st,lsm6dsv320x` (:github:`95802`)
   * :dtcompatible:`st,lsm6dsv80x` (:github:`95802`)
   * :dtcompatible:`ti,ina232` (:github:`98791`)
   * :dtcompatible:`ti,opt3004` (:github:`99387`)

* Serial controller

   * :dtcompatible:`focaltech,ft9001-usart` (:github:`95959`)
   * :dtcompatible:`microchip,dbgu-g1-uart` (:github:`101016`)
   * :dtcompatible:`realtek,ameba-loguart` (:github:`78036`)
   * :dtcompatible:`realtek,bee-uart` (:github:`102691`)
   * :dtcompatible:`renesas,ra-uart-sau` (:github:`102379`)
   * :dtcompatible:`rpmsg-uart` (:github:`98463`)

* :abbr:`SPI (Serial Peripheral Interface)`

   * :dtcompatible:`bflb,spi` (:github:`94752`)
   * :dtcompatible:`infineon,spi` (:github:`100644`)
   * :dtcompatible:`microchip,sercom-g1-spi` (:github:`101864`)
   * :dtcompatible:`realtek,rts5912-spi` (:github:`96006`)
   * :dtcompatible:`renesas,ra-spi-sci` (:github:`97339`)
   * :dtcompatible:`renesas,ra-spi-sci-b` (:github:`95014`)
   * :dtcompatible:`sensry,sy1xx-spi` (:github:`102323`)
   * :dtcompatible:`sifli,sf32lb-spi` (:github:`97626`)

* Stepper

   * :dtcompatible:`adi,tmc50xx-stepper-ctrl` (:github:`101001`)
   * :dtcompatible:`adi,tmc50xx-stepper-driver` (:github:`101001`)
   * :dtcompatible:`adi,tmc51xx-stepper-ctrl` (:github:`101001`)
   * :dtcompatible:`adi,tmc51xx-stepper-driver` (:github:`101001`)
   * :dtcompatible:`adi,tmcm3216` (:github:`104508`)
   * :dtcompatible:`adi,tmcm3216-stepper-ctrl` (:github:`104508`)
   * :dtcompatible:`adi,tmcm3216-stepper-driver` (:github:`104508`)
   * :dtcompatible:`zephyr,fake-stepper-ctrl` (:github:`101001`)
   * :dtcompatible:`zephyr,fake-stepper-driver` (:github:`101001`)
   * :dtcompatible:`zephyr,gpio-step-dir-stepper-ctrl` (:github:`101001`)
   * :dtcompatible:`zephyr,h-bridge-stepper-ctrl` (:github:`101001`)

* System controller

   * :dtcompatible:`ti,control-module` (:github:`103330`)

* Timer

   * :dtcompatible:`adi,max32-rv32-sys-timer` (:github:`97309`)
   * :dtcompatible:`adi,max32-wut-timer` (:github:`104687`)
   * :dtcompatible:`arm,armv7-timer` (:github:`99675`)
   * :dtcompatible:`infineon,cat1-lp-timer-pdl` (:github:`97831`)
   * :dtcompatible:`infineon,lp-timer` (:github:`100644`)
   * :dtcompatible:`realtek,bee-basic-timer` (:github:`104805`)
   * :dtcompatible:`realtek,bee-enhanced-timer` (:github:`104805`)
   * :dtcompatible:`realtek,bee-timer` (:github:`104805`)
   * :dtcompatible:`renesas,rza2m-gpt` (:github:`100932`)
   * :dtcompatible:`renesas,rza2m-ostm-timer` (:github:`100934`)
   * :dtcompatible:`sifli,sf32lb-atim` (:github:`100137`)
   * :dtcompatible:`sifli,sf32lb-gptim` (:github:`99362`)

* :abbr:`UAOL (USB Audio Offload Link)`

   * :dtcompatible:`intel,adsp-uaol` (:github:`104137`)
   * :dtcompatible:`intel,uaol-dai` (:github:`104137`)

* USB

   * :dtcompatible:`atmel,sam-udp` (:github:`102041`)
   * :dtcompatible:`bflb,udc-1` (:github:`104244`)
   * :dtcompatible:`nordic,nrf-usbhs-wrapper` (:github:`97642`)
   * :dtcompatible:`nuvoton,numaker-hsusbd` (:github:`95709`)

* USB Type-C

   * :dtcompatible:`zephyr,usb-c-pwrctrl` (:github:`103883`)

* Video

   * :dtcompatible:`arducam,mega` (:github:`96234`)
   * :dtcompatible:`himax,hm0360` (:github:`94904`)
   * :dtcompatible:`ovti,ov5642` (:github:`97106`)
   * :dtcompatible:`ovti,ov7675` (:github:`96319`)
   * :dtcompatible:`sony,imx219` (:github:`101754`)

* Wakeup Controller

   * :dtcompatible:`nxp,llwu` (:github:`100559`)

* Watchdog

   * :dtcompatible:`adi,max42500-watchdog` (:github:`102929`)
   * :dtcompatible:`bflb,wdt` (:github:`104243`)
   * :dtcompatible:`microchip,wdt-g1` (:github:`101335`)
   * :dtcompatible:`realtek,rts5817-watchdog` (:github:`91486`)

* Wi-Fi

   * :dtcompatible:`nordic,nrf7120-wifi` (:github:`104055`)

* :abbr:`XSPI (Expanded Serial Peripheral Interface)`

   * :dtcompatible:`nxp,s32-xspi` (:github:`101487`)
   * :dtcompatible:`nxp,s32-xspi-sfp-frad` (:github:`101487`)
   * :dtcompatible:`nxp,s32-xspi-sfp-mdad` (:github:`101487`)
   * :dtcompatible:`st,stm32-xspim` (:github:`104943`)

New Samples
***********

* :zephyr:code-sample:`6dof_fifo_stream` (renamed from ``stream_fifo``)
* :zephyr:code-sample:`accel_stream` (renamed from ``accel_polling``)
* :zephyr:code-sample:`adc_stream`
* :zephyr:code-sample:`amp_talk`
* :zephyr:code-sample:`at_client`
* :zephyr:code-sample:`bflb-bl61x-wo-uart`
* :zephyr:code-sample:`ble_peripheral_ans`
* :zephyr:code-sample:`ble_peripheral_ets`
* :zephyr:code-sample:`ble_peripheral_gap_svc`
* :zephyr:code-sample:`bluetooth_a2dp_sink`
* :zephyr:code-sample:`bluetooth_a2dp_source`
* :zephyr:code-sample:`bluetooth_l2cap_coc_acceptor`
* :zephyr:code-sample:`bluetooth_l2cap_coc_initiator`
* :zephyr:code-sample:`bridge`
* :zephyr:code-sample:`button_interrupt`
* :zephyr:code-sample:`capture`
* :zephyr:code-sample:`coap-upload`
* :zephyr:code-sample:`codec`
* :zephyr:code-sample:`cpu_freq_on_demand`
* :zephyr:code-sample:`cpu_freq_pressure`
* :zephyr:code-sample:`crc_drivers`
* :zephyr:code-sample:`crc_subsys`
* :zephyr:code-sample:`cs40l5x`
* :zephyr:code-sample:`device_pm`
* :zephyr:code-sample:`dsa`
* :zephyr:code-sample:`event`
* :zephyr:code-sample:`ext2-fstab`
* :zephyr:code-sample:`fingerprint-sensor`
* :zephyr:code-sample:`flash-ipm`
* :zephyr:code-sample:`frdm_mcxa156_lpdac_opamp_lpadc`
* :zephyr:code-sample:`ftp-client`
* :zephyr:code-sample:`hello_hl78xx`
* :zephyr:code-sample:`hwspinlock`
* :zephyr:code-sample:`instrumentation`
* :zephyr:code-sample:`is31fl319x`
* :zephyr:code-sample:`latmon-client`
* :zephyr:code-sample:`lp-gpio-wakeup`
* :zephyr:code-sample:`lp-timer-wakeup`
* :zephyr:code-sample:`max32664c`
* :zephyr:code-sample:`mctp_i2c_bus_endpoint`
* :zephyr:code-sample:`mctp_i2c_bus_owner`
* :zephyr:code-sample:`mctp_i3c_bus_endpoint`
* :zephyr:code-sample:`mctp_i3c_bus_owner`
* :zephyr:code-sample:`mctp-usb-endpoint`
* :zephyr:code-sample:`msg_queue`
* :zephyr:code-sample:`mtch9010`
* :zephyr:code-sample:`netmidi2`
* :zephyr:code-sample:`nrf_clock_control`
* :zephyr:code-sample:`ocpp`
* :zephyr:code-sample:`opamp_output_measure`
* :zephyr:code-sample:`openthread-border-router`
* :zephyr:code-sample:`pico-w-wifi-led`
* :zephyr:code-sample:`producer_consumer`
* :zephyr:code-sample:`quality-of-service`
* :zephyr:code-sample:`red-black-tree`
* :zephyr:code-sample:`regulator_shell`
* :zephyr:code-sample:`renesas_lvd`
* :zephyr:code-sample:`rtk0eg0019b01002bj`
* :zephyr:code-sample:`s3km1110`
* :zephyr:code-sample:`scmi`
* :zephyr:code-sample:`sct2024`
* :zephyr:code-sample:`shell-devmem-load`
* :zephyr:code-sample:`stm32_pwm_mastermode`
* :zephyr:code-sample:`t1s`
* :zephyr:code-sample:`tmcm3216`
* :zephyr:code-sample:`usb-c-drp`
* :zephyr:code-sample:`usb-host-uvc`
* :zephyr:code-sample:`veml6046`
* :zephyr:code-sample:`virtiofs`
* :zephyr:code-sample:`wireguard-vpn`
* :zephyr:code-sample:`zbus-async-listeners`
* :zephyr:code-sample:`zbus-proxy-agent-ipc`
* :zephyr:code-sample:`ztest_benchmark`

..
  Same as above, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

Devicetree
**********

* Migration guide: :ref:`migration_4.4_devicetree`

* New macros for reg property iteration (:github:`104223`)

  * :c:macro:`DT_FOREACH_REG`
  * :c:macro:`DT_FOREACH_REG_SEP`
  * :c:macro:`DT_FOREACH_REG_VARGS`
  * :c:macro:`DT_FOREACH_REG_SEP_VARGS`
  * Instance number based variants of each, e.g. :c:macro:`DT_INST_FOREACH_REG`

* Definitions for ``*-map`` related properties (:github:`87595`)
  provide first-class support for nexus nodes and specifier mappings.
  See Devicetree Specification v0.4 section 2.5 for more details
  on these properties.

* New :dtcompatible:`zephyr,mapped-partition` binding and associated
  APIs for memory-mapped flash partitions. This is a successor to the
  existing :dtcompatible:`fixed-partitions` binding

* Bindings are no longer allowed to specify any default values for the
  ``status``, ``#address-cells`` and ``#size-cells`` properties.

* :c:macro:`DT_CHILD_BY_UNIT_ADDR_INT`

* :c:macro:`DT_INST_CHILD_BY_UNIT_ADDR_INT`

Kconfig
*******

* Added new preprocessor function ``dt_highest_controller_irq_number`` (:github:`104819`)

Kernel
******

* Dropped CONFIG_SCHED_DUMB and CONFIG_WAITQ_DUMB options which were deprecated
  in Zephyr 4.2.0

* Added tiered heap hardening with :kconfig:option:`CONFIG_SYS_HEAP_HARDENING`
  (Basic, Moderate, Full, Extreme) providing progressive levels of runtime
  corruption detection for :c:func:`sys_heap_alloc` and :c:func:`sys_heap_free`,
  including double-free detection, neighbor consistency checks, and optional
  per-chunk canaries (:github:`104999`).

* :ref:`cleanup_api`

  * :c:macro:`SCOPE_VAR_DEFINE`
  * :c:macro:`SCOPE_GUARD_DEFINE`
  * :c:macro:`SCOPE_DEFER_DEFINE`
  * :c:macro:`scope_var`
  * :c:macro:`scope_var_init`
  * :c:macro:`scope_guard`
  * :c:macro:`scope_defer`

Libraries / Subsystems
**********************

* LoRa/LoRaWAN

   * :c:func:`lora_airtime`
   * Added Channel Activity Detection (CAD) support to the LoRa API:
     :c:func:`lora_cad`, :c:func:`lora_cad_async`.
     CAD parameters and LBT mode are configured via
     :c:struct:`lora_modem_config`.
   * Added :c:func:`lora_recv_duty_cycle` for hardware-driven
     wake-on-radio (RX duty cycling).

* Mbed TLS

  * Added :kconfig:option:`CONFIG_MBEDTLS_VERSION_C` to simplify the
    export of version information from Mbed TLS. If enabled, the
    :c:func:`mbedtls_version_get_number()` function will be available.

  * Mbed TLS has been upgraded to version 4.1.0. From now on this repo will only include TLS
    and X.509, while crypto support was moved to TF-PSA-Crypto. A new west module
    has been introduced for the latter and it's based on upstream release 1.1.0.
    Release notes for both projects can be found here:

    * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-4.1.0
    * https://github.com/Mbed-TLS/TF-PSA-Crypto/releases/tag/tf-psa-crypto-1.1.0

* Zbus

   * Added async listener support. Async listeners execute in a workqueue context instead of the
     publisher's thread, enabling non-blocking operations without requiring a dedicated subscriber
     thread.
   * Added :zephyr:code-sample:`zbus-async-listeners`.
   * Added experimental proxy-agent communication with IPC backend support for
     forwarding channel data across domains.
   * Added :zephyr:code-sample:`zbus-proxy-agent-ipc`.
   * Added the :c:func:`zbus_chan_from_name` function. Retrieve a zbus channel from its name string.
   * Added the :c:func:`zbus_async_listener_set_work_queue` function. Set the work queue for an
     async listener.
   * Added the :c:func:`zbus_chan_pub_stats_msg_age` function. Get the message age in milliseconds
     since the last publish.
   * Clarified observer priority documentation and fixed spelling and grammar.
   * Updated observer types image in documentation.
   * Filtered out tests that are not SMP aware.


Other notable changes
*********************

* TF-M was updated to version 2.2.2 (from 2.2.0). The release notes can be found at:

  * https://trustedfirmware-m.readthedocs.io/en/tf-mv2.2.2/releases/2.2.1.html
  * https://trustedfirmware-m.readthedocs.io/en/tf-mv2.2.2/releases/2.2.2.html

* TF-M NS interface headers are now automatically available to non-secure applications via the
  ``zephyr_interface`` CMake library, removing the need to explicitly link against ``tfm_api``.

* NXP SoC DTSI files have been reorganized by moving them into family-specific
  subdirectories under ``dts/arm/nxp``.

* :zephyr:board:`native_sim` based targets can now be :ref:`cross-compiled<posix_arch_cross_compile>`
  (:github:`100182`)

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?
