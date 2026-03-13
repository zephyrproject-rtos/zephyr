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

Zephyr 4.4.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.4.0.

Major enhancements with this release include:

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

* :cve:`2026-1678` `Zephyr project bug tracker GHSA-536f-h63g-hj42
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-536f-h63g-hj42>`_

* :cve:`2026-4179` `Zephyr project bug tracker GHSA-9xg7-g3q3-9prf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9xg7-g3q3-9prf>`_

API Changes
***********

..
  Only removed, deprecated and new APIs. Changes go in migration guide.

* Architectures

  * Xtensa

    * Removed as these are architectural features:

      * :kconfig:option:`CONFIG_XTENSA_MMU_DOUBLE_MAP`
      * :kconfig:option:`CONFIG_XTENSA_RPO_CACHE`
      * :kconfig:option:`CONFIG_XTENSA_CACHED_REGION`
      * :kconfig:option:`CONFIG_XTENSA_UNCACHED_REGION`

* Bluetooth

  * Controller

    * :kconfig:option:`CONFIG_BT_CTLR_ADV_AUX_SET`, :kconfig:option:`CONFIG_BT_CTLR_ADV_SYNC_SET`
      and :kconfig:option:`CONFIG_BT_CTLR_ADV_DATA_BUF_MAX` no longer require
      :kconfig:option:`CONFIG_BT_CTLR_ADVANCED_FEATURES`

Removed APIs and options
========================

* Bluetooth

   * ``CONFIG_BT_TBS_SUPPORTED_FEATURES``

   * The deprecated ``bt_hci_cmd_create()`` functon was removed. It has been replaced by
     :c:func:`bt_hci_cmd_alloc`.

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

* POSIX

  * :kconfig:option:`CONFIG_XOPEN_STREAMS` was deprecated. Instead, use :kconfig:option:`CONFIG_XSI_STREAMS`

* Random

  * :kconfig:option:`CONFIG_CTR_DRBG_CSPRNG_GENERATOR` has been deprecrated. Instead, use
    :kconfig:option:`CONFIG_PSA_CSPRNG_GENERATOR`.

* Sensors

  * NXP

    * Deprecated the ``mcux_lpcmp`` driver (:zephyr_file:`drivers/sensor/nxp/mcux_lpcmp/mcux_lpcmp.c`). It is
      currently scheduled to be removed in Zephyr 4.6, along with the ``mcux_lpcmp`` sample. (:github:`100998`).
    * Added new temperature sensor driver (:dtcompatible:`nxp,tempsense`) (:github:`101525`).

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

  * Added new DAC driver (:dtcompatible:`nxp,hpdac`) (:github:`104642`).

* DMA

  * Added new DMA driver (:dtcompatible:`nxp,4ch-dma`) (:github:`97841`).

* Display

  * :kconfig:option:`SSD1325_DEFAULT_CONTRAST`
  * :kconfig:option:`SSD1325_CONV_BUFFER_LINES`

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

* Flash

  * :dtcompatible:`jedec,mspi-nor` now allows MSPI configuration of read, write and
    control commands separately via devicetree.

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

* Modem

  * :kconfig:option:`CONFIG_MODEM_HL78XX_AT_SHELL`
  * :kconfig:option:`CONFIG_MODEM_HL78XX_AIRVANTAGE`

* NVMEM

  * Flash device support

    * :kconfig:option:`CONFIG_NVMEM_FLASH`
    * :kconfig:option:`CONFIG_NVMEM_FLASH_WRITE`

* Networking

  * Wi-Fi

    * Add support for Wi-Fi Direct (P2P) mode.

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

* Sys

  * :c:macro:`COND_CASE_1`

* Timeutil

  * :kconfig:option:`CONFIG_TIMEUTIL_APPLY_SKEW`

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

.. zephyr-keep-sorted-stop

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

* Ai-Thinker Co., Ltd.

   * :zephyr:board:`ai_m61_32s_kit` (``ai_m61_32s_kit``)
   * Rename ai_m62_12f and ai_wb2_12f to ai_m62_12f_kit and ai_wb2_12f_kit

New Shields
***********

..
  Same as above, this will also be recomputed at the time of the release.


* Nordic Semiconductor ASA

  * :ref:`nrf7002eb2 <nrf7002eb2>` (nRF7002 EB II)

New Drivers
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* Comparator

    * Added NXP low power comparator driver (:dtcompatible:`nxp,lpcmp`). (:github:`100998`)

* Radio

  * :dtcompatible:`radio-fem-two-ctrl-pins` (renamed from ``generic-fem-two-ctrl-pins``)
  * :dtcompatible:`radio-gpio-coex` (renamed from ``gpio-radio-coex``)

* Display

  * :dtcompatible:`solomon,ssd1325`

* OTP

  * Added new stm32 BSEC driver that provides means to program and read OTP fuses
    (:dtcompatible:`st,stm32-bsec`). (:github:`102403`)
  * Added new driver that allows reading from OTP/read-only areas of STM32 embedded NVM
    (:dtcompatible:`st,stm32-nvm-otp`) (:github:`102976`)
  * Added SiFli SF32LB eFuse OTP driver (:dtcompatible:`sifli,sf32lb-efuse`).
    (:github:`101926`)
  * :dtcompatible:`nxp,ocotp` (:github:`102567` & :github:`103089`)

* Partitions

  * Added new :dtcompatible:`zephyr,mapped-partition` compatible for memory-mapped partitions
    (:github:`104398`).

New Samples
***********

* :zephyr:code-sample:`ble_peripheral_ans`
* :zephyr:code-sample:`cpu_freq_pressure`
* :zephyr:code-sample:`6dof_fifo_stream` (renamed from ``stream_fifo``)
* :zephyr:code-sample:`accel_stream` (renamed from ``accel_polling``)
* :zephyr:code-sample:`accel_polling` (it uses sensor_read() API)

..
  Same as above, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

DeviceTree
**********

* Migration guide: :ref:`migration_4.4_devicetree`
* Bindings are no longer allowed to specify any default values for the
  ``#address-cells`` and ``#size-cells`` properties.
* :c:macro:`DT_CHILD_BY_UNIT_ADDR_INT`
* :c:macro:`DT_INST_CHILD_BY_UNIT_ADDR_INT`

Kernel
******

* Dropped CONFIG_SCHED_DUMB and CONFIG_WAITQ_DUMB options which were deprecated
  in Zephyr 4.2.0

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

Other notable changes
*********************

* TF-M was updated to version 2.2.2 (from 2.2.0). The release notes can be found at:

  * https://trustedfirmware-m.readthedocs.io/en/tf-mv2.2.2/releases/2.2.1.html
  * https://trustedfirmware-m.readthedocs.io/en/tf-mv2.2.2/releases/2.2.2.html

* NXP SoC DTSI files have been reorganized by moving them into family-specific
  subdirectories under ``dts/arm/nxp``.

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?
