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

.. _zephyr_4.3:

Zephyr 4.3.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.3.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.2.0 to Zephyr v4.3.0 can be found in the separate :ref:`migration guide<migration_4.3>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

* :cve:`2025-12035`: Under embargo until 2025-12-13

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

..
  Only removed, deprecated and new APIs, changes go in migration guide.

* Crypto

  * The input buffer in :c:struct:`hash_pkt` is now constant

Removed APIs and options
========================

* The TinyCrypt library was removed as the upstream version is no longer maintained.
  PSA Crypto API is now the recommended cryptographic library for Zephyr.
* The legacy pipe object API was removed. Use the new pipe API instead.
* ``bt_le_set_auto_conn``
* ``CONFIG_BT_BUF_ACL_RX_COUNT``
* ``ok`` enum value has now been removed completely from ``base.yaml`` binding ``status`` property in devicetree.
* STM32 LPTIM clock source selection through Kconfig was removed. Device Tree must now be used instead.
  Affected Kconfig symbols: ``CONFIG_STM32_LPTIM_CLOCK_LSI`` / ``CONFIG_STM32_LPTIM_CLOCK_LSI``

Deprecated APIs and options
===========================

* :dtcompatible:`maxim,ds3231` is deprecated in favor of :dtcompatible:`maxim,ds3231-rtc`.
* Providing a third argument to :c:macro:`SPI_CONFIG_DT`, :c:macro:`SPI_CONFIG_DT_INST`,
  :c:macro:`SPI_DT_SPEC_GET`, :c:macro:`SPI_DT_SPEC_INST_GET` is deprecated. Providing a
  second argument to :c:macro:`SPI_CS_CONTROL_INIT` is deprecated. Use new DT properties
  ``spi-cs-setup-delay-ns`` and ``spi-cs-hold-delay-ns`` to specify delay instead.

* :c:enum:`bt_hci_bus` was deprecated as it was not used. :c:macro:`BT_DT_HCI_BUS_GET` should be
  used instead.

* :kconfig:option:`CONFIG_BT_AUTO_PHY_UPDATE` was deprecated and has been replaced with
  role-specific (central vs peripheral) options that allow specifying exactly which PHY is
  preferred for automatic updates.

* :kconfig:option:`CONFIG_POSIX_READER_WRITER_LOCKS` is deprecated. Use :kconfig:option:`CONFIG_POSIX_RW_LOCKS` instead.

* :c:func:`bt_ctlr_set_public_addr` is deprecated in favor of using
  :c:struct:`bt_hci_cp_vs_write_bd_addr` for setting the public Bluetooth device address.

* :kconfig:option:`CONFIG_JWT_SIGN_RSA_LEGACY` is deprecated. Please switch to the
  PSA Crypto API based alternative (i.e. :kconfig:option:`CONFIG_JWT_SIGN_RSA_PSA`).

* RISCV's :kconfig:option:`CONFIG_EXTRA_EXCEPTION_INFO` is deprecated. Use :kconfig:option:`CONFIG_EXCEPTION_DEBUG` instead.

New APIs and options
====================

..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w)

* Architectures

  * :kconfig:option:`CONFIG_ARCH_HAS_HW_SHADOW_STACK`
  * :kconfig:option:`CONFIG_SRAM_SW_ISR_TABLE`

  * x86 Intel CET support

    * :kconfig:option:`CONFIG_X86_CET`
    * :kconfig:option:`CONFIG_X86_CET_IBT`
    * :kconfig:option:`CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT`
    * :kconfig:option:`CONFIG_X86_CET_SOC_PREPARE_SHADOW_STACK_SWITCH`
    * :kconfig:option:`CONFIG_X86_CET_VERIFY_KERNEL_SHADOW_STACK`

  * ARM (Cortex-M) system state save/restore primitives

    * :c:func:`z_arm_save_scb_context` / :c:func:`z_arm_restore_scb_context`
    * :c:func:`z_arm_save_mpu_context` / :c:func:`z_arm_restore_mpu_context`
    * Existing :c:func:`z_arm_save_fp_context` and :c:func:`z_arm_save_fp_context` have also been updated

* Bluetooth

  * Audio

    * :c:struct:`bt_audio_codec_cfg` now contains a target_latency and a target_phy option
    * :c:func:`bt_bap_broadcast_source_foreach_stream`
    * :c:func:`bt_cap_initiator_broadcast_foreach_stream`
    * :c:struct:`bt_bap_stream` now contains an ``iso`` field as a reference to the ISO channel
    * :c:func:`bt_bap_unicast_group_get_info`
    * :c:func:`bt_cap_unicast_group_get_info`
    * :c:func:`bt_bap_unicast_client_unregister_cb`

  * Host

    * :c:struct:`bt_iso_unicast_info` now contains a ``cig_id`` and a ``cis_id`` field
    * :c:struct:`bt_iso_broadcaster_info` now contains a ``big_handle`` and a ``bis_number`` field
    * :c:struct:`bt_iso_sync_receiver_info` now contains a ``big_handle`` and a ``bis_number`` field
    * :c:struct:`bt_le_ext_adv_info` now contains an ``sid`` field with the Advertising Set ID.
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_PERIPHERAL_NONE`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_PERIPHERAL_1M`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_PERIPHERAL_2M`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_PERIPHERAL_CODED`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_CENTRAL_NONE`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_CENTRAL_1M`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_CENTRAL_2M`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_CENTRAL_CODED`

* CPUFreq

  * Introduced experimental dynamic CPU frequency scaling subsystem

    * :kconfig:option:`CONFIG_CPU_FREQ`

* Cellular

  * :c:enumerator:`CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT`

* Crypto

  * :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS`

* Display

  * :c:enumerator:`PIXEL_FORMAT_AL_88`

  * SDL

    * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_AL_88`
    * :kconfig:option:`CONFIG_SDL_DISPLAY_COLOR_TINT`

* Haptics

  * :kconfig:option:`CONFIG_HAPTICS_SHELL`

* Instrumentation subsystem

  * Introduced instrumentation subsystem

    * :kconfig:option:`CONFIG_INSTRUMENTATION`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_CALLGRAPH`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_BUFFER_SIZE`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_BUFFER_OVERWRITE`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_STATISTICAL`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_STATISTICAL_MAX_NUM_FUNC`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_STATISTICAL_MAX_CALL_DEPTH`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_TRIGGER_FUNCTION`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_STOPPER_FUNCTION`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_EXCLUDE_FUNCTION_LIST`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_EXCLUDE_FILE_LIST`
    * :c:struct:`instr_header`
    * :c:struct:`instr_event_context`
    * :c:struct:`instr_record`
    * :c:func:`instr_tracing_supported`
    * :c:func:`instr_profiling_supported`
    * :c:func:`instr_fundamentals_initialized`
    * :c:func:`instr_init`
    * :c:func:`instr_initialized`
    * :c:func:`instr_enabled`
    * :c:func:`instr_enable`
    * :c:func:`instr_disable`
    * :c:func:`instr_turn_on`
    * :c:func:`instr_turn_off`
    * :c:func:`instr_turned_on`
    * :c:func:`instr_trace_enabled`
    * :c:func:`instr_profile_enabled`
    * :c:func:`instr_dump_buffer_uart`
    * :c:func:`instr_dump_deltas_uart`
    * :c:func:`instr_event_handler`
    * :c:func:`instr_set_trigger_func`
    * :c:func:`instr_set_stop_func`
    * :c:func:`instr_get_trigger_func`
    * :c:func:`instr_get_stop_func`

* Kernel

  * :kconfig:option:`CONFIG_HW_SHADOW_STACK`
  * :kconfig:option:`CONFIG_HW_SHADOW_STACK_ALLOW_REUSE`
  * :kconfig:option:`CONFIG_HW_SHADOW_STACK_MIN_SIZE`
  * :kconfig:option:`CONFIG_HW_SHADOW_STACK_PERCENTAGE_SIZE`
  * :c:macro:`K_THREAD_HW_SHADOW_STACK_SIZE`
  * :c:macro:`K_KERNEL_HW_SHADOW_STACK_DECLARE`
  * :c:macro:`K_KERNEL_HW_SHADOW_STACK_ARRAY_DECLARE`
  * :c:macro:`K_THREAD_HW_SHADOW_STACK_DEFINE`
  * :c:macro:`K_THREAD_HW_SHADOW_STACK_ARRAY_DEFINE`
  * :c:macro:`K_THREAD_HW_SHADOW_STACK_ATTACH`
  * :c:macro:`k_thread_hw_shadow_stack_attach`

* LVGL (Light and Versatile Graphics Library)

  * :kconfig:option:`CONFIG_LV_Z_MEMORY_POOL_ZEPHYR_REGION`
  * :kconfig:option:`CONFIG_LV_Z_MEMORY_POOL_ZEPHYR_REGION_NAME`
  * :kconfig:option:`CONFIG_LV_Z_VDB_ZEPHYR_REGION`
  * :kconfig:option:`CONFIG_LV_Z_VDB_ZEPHYR_REGION_NAME`

* Logging:

  * :kconfig:option:`CONFIG_LOG_BACKEND_SWO_SYNC_PACKETS`

  * Added options to skip timestamp and level in log backends.

    * :kconfig:option:`CONFIG_LOG_BACKEND_SHOW_TIMESTAMP`
    * :kconfig:option:`CONFIG_LOG_BACKEND_SHOW_LEVEL`

  * Added rate-limited logging macros to prevent log flooding when messages are generated frequently.

    * :c:macro:`LOG_ERR_RATELIMIT` - Rate-limited error logging macro (convenience)
    * :c:macro:`LOG_WRN_RATELIMIT` - Rate-limited warning logging macro (convenience)
    * :c:macro:`LOG_INF_RATELIMIT` - Rate-limited info logging macro (convenience)
    * :c:macro:`LOG_DBG_RATELIMIT` - Rate-limited debug logging macro (convenience)
    * :c:macro:`LOG_HEXDUMP_ERR_RATELIMIT` - Rate-limited error hexdump macro (convenience)
    * :c:macro:`LOG_HEXDUMP_WRN_RATELIMIT` - Rate-limited warning hexdump macro (convenience)
    * :c:macro:`LOG_HEXDUMP_INF_RATELIMIT` - Rate-limited info hexdump macro (convenience)
    * :c:macro:`LOG_HEXDUMP_DBG_RATELIMIT` - Rate-limited debug hexdump macro (convenience)
    * :c:macro:`LOG_ERR_RATELIMIT_RATE` - Rate-limited error logging macro (explicit rate)
    * :c:macro:`LOG_WRN_RATELIMIT_RATE` - Rate-limited warning logging macro (explicit rate)
    * :c:macro:`LOG_INF_RATELIMIT_RATE` - Rate-limited info logging macro (explicit rate)
    * :c:macro:`LOG_DBG_RATELIMIT_RATE` - Rate-limited debug logging macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_ERR_RATELIMIT_RATE` - Rate-limited error hexdump macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_WRN_RATELIMIT_RATE` - Rate-limited warning hexdump macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_INF_RATELIMIT_RATE` - Rate-limited info hexdump macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_DBG_RATELIMIT_RATE` - Rate-limited debug hexdump macro (explicit rate)

* Management

  * hawkBit

    * :kconfig:option:`CONFIG_HAWKBIT_REBOOT_NONE`
    * :kconfig:option:`CONFIG_HAWKBIT_CONFIRM_IMG_ON_INIT`
    * :kconfig:option:`CONFIG_HAWKBIT_ERASE_SECOND_SLOT_ON_CONFIRM`

* Modem

  * :kconfig:option:`CONFIG_MODEM_DEDICATED_WORKQUEUE`

* NVMEM

  * Introduced :ref:`Non-Volatile Memory (NVMEM)<nvmem>` subsystem

    * :kconfig:option:`CONFIG_NVMEM`
    * :kconfig:option:`CONFIG_NVMEM_EEPROM`
    * :c:struct:`nvmem_cell`
    * :c:func:`nvmem_cell_read`
    * :c:func:`nvmem_cell_write`
    * :c:func:`nvmem_cell_is_ready`
    * :c:macro:`NVMEM_CELL_GET_BY_NAME` - and variants
    * :c:macro:`NVMEM_CELL_GET_BY_IDX` - and variants

* Networking

  * CoAP

    * :c:struct:`coap_client_response_data`
    * :c:member:`coap_client_request.payload_cb`

  * Sockets

    * :c:func:`zsock_listen` now implements the ``backlog`` parameter support. The TCP server
      socket will limit the number of pending incoming connections to that value.

* Newlib

  * :kconfig:option:`CONFIG_NEWLIB_LIBC_USE_POSIX_LIMITS_H`

* Opamp

  * Introduced opamp device driver APIs selected with :kconfig:option:`CONFIG_OPAMP`. It supports
    initial configuration through Devicetree and runtime configuration through vendor specific APIs.
  * Added support for NXP OPAMP :dtcompatible:`nxp,opamp`.
  * Added support for NXP OPAMP_FAST :dtcompatible:`nxp,opamp_fast`.

* Power management

   * :c:func:`pm_device_driver_deinit`
   * :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_DEFAULT_ENABLE`
   * :kconfig:option:`CONFIG_PM_S2RAM` has been refactored to be promptless. The application now
     only needs to enable any "suspend-to-ram" power state in the devicetree.
   * The :kconfig:option:`PM_S2RAM_CUSTOM_MARKING` has been renamed to
     :kconfig:option:`HAS_PM_S2RAM_CUSTOM_MARKING` and refactored to be promptless. This option
     is now selected by SoCs if they need it for their "suspend-to-ram" implementations.

* Settings

   * :kconfig:option:`CONFIG_SETTINGS_TFM_ITS`

* Shell

   * MQTT backend

      * :kconfig:option:`CONFIG_SHELL_MQTT_TOPIC_RX_ID`
      * :kconfig:option:`CONFIG_SHELL_MQTT_TOPIC_TX_ID`
      * :kconfig:option:`CONFIG_SHELL_MQTT_CONNECT_TIMEOUT_MS`
      * :kconfig:option:`CONFIG_SHELL_MQTT_WORK_DELAY_MS`
      * :kconfig:option:`CONFIG_SHELL_MQTT_LISTEN_TIMEOUT_MS`

* State Machine Framework

  * :c:func:`smf_get_current_leaf_state`
  * :c:func:`smf_get_current_executing_state`

* Storage

    * :kconfig:option:`CONFIG_FILE_SYSTEM_SHELL_LS_SIZE`

* Sys

  * :c:func:`sys_count_bits`

* Task Watchdog

  * :kconfig:option:`CONFIG_TASK_WDT_DUMMY`

* Toolchain

  * :c:macro:`__deprecated_version`

* USB

  * Video

    * :c:func:`uvc_add_format`

* Video

  * :c:member:`video_format.size` field
  * :c:func:`video_estimate_fmt_size`
  * :c:func:`video_transfer_buffer`

.. zephyr-keep-sorted-stop

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

* Adafruit Industries, LLC

   * :zephyr:board:`adafruit_feather_adalogger_rp2040` (``adafruit_feather_adalogger_rp2040``)
   * :zephyr:board:`adafruit_feather_canbus_rp2040` (``adafruit_feather_canbus_rp2040``)
   * :zephyr:board:`adafruit_feather_esp32` (``adafruit_feather_esp32``)
   * :zephyr:board:`adafruit_feather_rfm95_rp2040` (``adafruit_feather_rfm95_rp2040``)
   * :zephyr:board:`adafruit_feather_rp2040` (``adafruit_feather_rp2040``)
   * :zephyr:board:`adafruit_itsybitsy_rp2040` (``adafruit_itsybitsy_rp2040``)
   * :zephyr:board:`adafruit_metro_rp2040` (``adafruit_metro_rp2040``)
   * :zephyr:board:`adafruit_metro_rp2350` (``adafruit_metro_rp2350``)
   * :zephyr:board:`adafruit_trinkey_qt2040` (``adafruit_trinkey_qt2040``)

* Advanced Micro Devices (AMD), Inc.

   * :zephyr:board:`versalnet_apu` (``versalnet_apu``)

* Ai-Thinker Co., Ltd.

   * :zephyr:board:`ai_m62_12f` (``ai_m62_12f``)
   * :zephyr:board:`esp32_cam` (``esp32_cam``)

* Ambiq Micro, Inc.

   * :zephyr:board:`apollo2_evb` (``apollo2_evb``)

* Arduino

   * :zephyr:board:`arduino_uno_q` (``arduino_uno_q``)

* DFRobot

   * :zephyr:board:`beetle_rp2040` (``beetle_rp2040``)

* Doctors of Intelligence & Technology

   * :zephyr:board:`dt_xt_zb1_devkit` (``dt_xt_zb1_devkit``)

* Egis Technology Inc

   * :zephyr:board:`egis_et171` (``egis_et171``)

* Espressif Systems

   * :zephyr:board:`esp32h2_devkitm` (``esp32h2_devkitm``)

* FANKE Technology Co., Ltd.

   * :zephyr:board:`fk723m1_zgt6` (``fk723m1_zgt6``)

* Firefly

   * :zephyr:board:`roc_rk3588_pc` (``roc_rk3588_pc``)

* FoBE Studio

   * :zephyr:board:`quill_nrf52840_mesh` (``quill_nrf52840_mesh``)

* Guangdong Embedsky Technology Co., Ltd.

   * :zephyr:board:`tq_h503a` (``tq_h503a``)

* Infineon Technologies

   * :zephyr:board:`kit_psc3m5_evk` (``kit_psc3m5_evk``)
   * :zephyr:board:`kit_pse84_eval` (``kit_pse84_eval``)

* Intel Corporation

   * :zephyr:board:`intel_ptl_h_crb` (``intel_ptl_h_crb``)

* Microchip Technology Inc.

   * :zephyr:board:`pic32cm_jh01_cnano` (``pic32cm_jh01_cnano``)
   * :zephyr:board:`pic32cm_jh01_cpro` (``pic32cm_jh01_cpro``)
   * :zephyr:board:`sam_e54_xpro` (``sam_e54_xpro``)

* Nuvoton Technology Corporation

   * :zephyr:board:`numaker_m5531` (``numaker_m5531``)

* NXP Semiconductors

   * :zephyr:board:`frdm_imx91` (``frdm_imx91``)
   * :zephyr:board:`frdm_imx93` (``frdm_imx93``)
   * :zephyr:board:`frdm_k32l2b3` (``frdm_k32l2b3``)
   * :zephyr:board:`frdm_mcxa266` (``frdm_mcxa266``)
   * :zephyr:board:`frdm_mcxa346` (``frdm_mcxa346``)
   * :zephyr:board:`frdm_mcxa366` (``frdm_mcxa366``)
   * :zephyr:board:`frdm_mcxw23` (``frdm_mcxw23``)
   * :zephyr:board:`imx91_qsb` (``imx91_qsb``)
   * :zephyr:board:`imx95_evk_15x15` (``imx95_evk_15x15``)
   * :zephyr:board:`mcx_n9xx_evk` (``mcx_n9xx_evk``)
   * :zephyr:board:`mcx_n5xx_evk` (``mcx_n5xx_evk``)
   * :zephyr:board:`mcxw23_evk` (``mcxw23_evk``)

* Panasonic Corporation

   * :zephyr:board:`panb611evb` (``panb611evb``)

* Raspberry Pi Foundation

   * :zephyr:board:`rpi_debug_probe` (``rpi_debug_probe``)

* Renesas Electronics Corporation

   * :zephyr:board:`ek_ra4c1` (``ek_ra4c1``)
   * :zephyr:board:`ek_rx261` (``ek_rx261``)
   * :zephyr:board:`fpb_rx261` (``fpb_rx261``)
   * :zephyr:board:`mcb_rx26t` (``mcb_rx26t``)
   * :zephyr:board:`mck_ra8t2` (``mck_ra8t2``)
   * :zephyr:board:`rssk_ra2l1` (``rssk_ra2l1``)

* Seeed Technology Co., Ltd

   * :zephyr:board:`wio_wm1110_dev_kit` (``wio_wm1110_dev_kit``)
   * :zephyr:board:`xiao_nrf54l15` (``xiao_nrf54l15``)

* Shanghai Ruiside Electronic Technology Co., Ltd.

   * :zephyr:board:`art_pi` (``art_pi``)

* Shenzhen Holyiot Technology Co., Ltd.

   * :zephyr:board:`holyiot_yj17095` (``holyiot_yj17095``)

* SiFli Technologies(Nanjing) Co., Ltd

   * :zephyr:board:`sf32lb52_devkit_lcd` (``sf32lb52_devkit_lcd``)

* Silicon Laboratories

   * :zephyr:board:`bgm220_ek4314a` (``bgm220_ek4314a``)
   * :zephyr:board:`pg23_pk2504a` (``pg23_pk2504a``)
   * :zephyr:board:`pg28_pk2506a` (``pg28_pk2506a``)
   * :zephyr:board:`siwx917_dk2605a` (``siwx917_dk2605a``)
   * :zephyr:board:`xg22_ek2710a` (``xg22_ek2710a``)
   * :zephyr:board:`bg29_rb4420a` (``bg29_rb4420a``)
   * :zephyr:board:`xg24_rb4186c` (``xg24_rb4186c``)
   * :zephyr:board:`xg24_rb4187c` (``xg24_rb4187c``)
   * :zephyr:board:`xgm240_rb4316a` (``xgm240_rb4316a``)
   * :zephyr:board:`xgm240_rb4317a` (``xgm240_rb4317a``)
   * :zephyr:board:`bg27_rb4110b` (``bg27_rb4110b``)
   * :zephyr:board:`bg27_rb4111b` (``bg27_rb4111b``)
   * :zephyr:board:`xg27_rb4194a` (``xg27_rb4194a``)
   * :zephyr:board:`xg28_rb4401c` (``xg28_rb4401c``)

* SteelSeries

   * :zephyr:board:`apex_pro_mini` (``apex_pro_mini``)

* STMicroelectronics

   * :zephyr:board:`nucleo_c092rc` (``nucleo_c092rc``)
   * :zephyr:board:`stm32mp257f_dk` (``stm32mp257f_dk``)
   * :zephyr:board:`stm32wba65i_dk1` (``stm32wba65i_dk1``)

* Texas Instruments

   * :zephyr:board:`lp_mspm0g3519` (``lp_mspm0g3519``)
   * :zephyr:board:`lp_mspm0l2228` (``lp_mspm0l2228``)

* Toradex AG

   * :zephyr:board:`verdin_am62` (``verdin_am62``)

* Waveshare Electronics

   * :zephyr:board:`rp2040_geek` (``rp2040_geek``)
   * :zephyr:board:`rp2040_keyboard_3` (``rp2040_keyboard_3``)
   * :zephyr:board:`rp2040_matrix` (``rp2040_matrix``)

* WeAct Studio

   * :zephyr:board:`blackpill_h523ce` (``blackpill_h523ce``)
   * :zephyr:board:`blackpill_u585ci` (``blackpill_u585ci``)
   * :zephyr:board:`weact_stm32wb55_core` (``weact_stm32wb55_core``)
   * :zephyr:board:`weact_esp32s3_b` (``weact_esp32s3_b``)

New Shields
***********

  * :ref:`Adafruit 24LC32 EEPROM Shield <adafruit_24lc32>`
  * :ref:`Adafruit AHT20 Shield <adafruit_aht20>`
  * :ref:`Adafruit APDS9960 Shield <adafruit_apds9960>`
  * :ref:`Adafruit DPS310 Shield <adafruit_dps310>`
  * :ref:`Adafruit DRV2605L Shield <adafruit_drv2605l>`
  * :ref:`Adafruit FeatherWing 128x32 OLED Shield <adafruit_featherwing_128x32_oled>`
  * :ref:`Adafruit HT16K33 LED Matrix Shield <adafruit_ht16k33>`
  * :ref:`Adafruit I2C to 8 Channel Solenoid Driver Shield <adafruit_8chan_solenoid>`
  * :ref:`Adafruit INA219 Shield <adafruit_ina219>`
  * :ref:`Adafruit INA237 Shield <adafruit_ina237>`
  * :ref:`Adafruit LIS2MDL Shield <adafruit_lis2mdl>`
  * :ref:`Adafruit LIS3DH Shield <adafruit_lis3dh>`
  * :ref:`Adafruit LTR-329 Shield <adafruit_ltr329>`
  * :ref:`Adafruit MCP9808 Shield <adafruit_mcp9808>`
  * :ref:`Adafruit PCF8523 Shield <adafruit_pcf8523>`
  * :ref:`Adafruit TSL2591 Shield <adafruit_tsl2591>`
  * :ref:`Adafruit VCNL4040 Shield <adafruit_vcnl4040>`
  * :ref:`Adafruit VEML7700 Shield <adafruit_veml7700>`
  * :ref:`ArduCam CU450 OV5640 Camera Module <arducam_cu450_ov5640>`
  * :ref:`Arduino Modulino Movement <arduino_modulino_movement>`
  * :ref:`Arduino Modulino Thermo <arduino_modulino_thermo>`
  * :ref:`MikroElektronika 3D Hall 3 Click <mikroe_3d_hall_3_click_shield>`
  * :ref:`MikroElektronika Air Quality 3 Click <mikroe_air_quality_3_click_shield>`
  * :ref:`MikroElektronika Ambient 2 Click <mikroe_ambient_2_click_shield>`
  * :ref:`MikroElektronika H Bridge 4 Click <mikroe_h_bridge_4_click_shield>`
  * :ref:`MikroElektronika Illuminance Click <mikroe_illuminance_click_shield>`
  * :ref:`MikroElektronika IR Gesture Click <mikroe_ir_gesture_click_shield>`
  * :ref:`MikroElektronika LSM6DSL Click <mikroe_lsm6dsl_click_shield>`
  * :ref:`MikroElektronika Pressure 3 Click <mikroe_pressure_3_click_shield>`
  * :ref:`MikroElektronika Proximity 9 Click <mikroe_proximity_9_click_shield>`
  * :ref:`MikroElektronika RTC 18 Click <mikroe_rtc_18_click_shield>`
  * :ref:`Nordic nPM1304 EK <npm1304_ek>`
  * :ref:`Olimex SHIELD-MIDI <olimex_shield_midi>`
  * :ref:`Renesas EK-RA8D1 to RTK7EKA6M3B00001BU Display Adapter <ek_ra8d1_rtk7eka6m3b00001bu>`
  * :ref:`Renesas RTK0EG0019B01002BJ Capacitive Touch Application Shield <rtk0eg0019b01002bj>`
  * :ref:`Sierra Wireless HL/RC Module Evaluation Kit Shield <swir_hl78xx_ev_kit>`
  * :ref:`Sparkfun Environmental Combo Shield with ENS160 and BME280 <sparkfun_environmental_combo>`
  * :ref:`Sparkfun RV8803 Shield <sparkfun_rv8803>`
  * :ref:`Sparkfun SHTC3 Shield <sparkfun_shtc3>`

New Drivers
***********

..
  Same as above for boards, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description


* :abbr:`ADC (Analog to Digital Converter)`

   * :dtcompatible:`adi,max32-adc-b-me18`
   * :dtcompatible:`nxp,sar-adc`
   * :dtcompatible:`renesas,rx-adc`
   * :dtcompatible:`renesas,rz-adc-c`
   * :dtcompatible:`silabs,iadc`

* ARM architecture

   * :dtcompatible:`microchip,sercom-g1`
   * :dtcompatible:`nuvoton,numaker-npu`

* Cache

   * :dtcompatible:`bflb,l1c`

* Charger

   * :dtcompatible:`nxp,pca9422-charger`

* Clock control

   * :dtcompatible:`bflb,bl61x-clock-controller`
   * :dtcompatible:`bflb,bl70x-clock-controller`
   * :dtcompatible:`infineon,fixed-clock`
   * :dtcompatible:`infineon,fixed-factor-clock`
   * :dtcompatible:`infineon,peri-div`
   * :dtcompatible:`mediatek,mt818x_cpuclk`
   * :dtcompatible:`microchip,sam-d5x-e5x-clock`
   * :dtcompatible:`nordic,nrf-iron-hsfll-local`
   * :dtcompatible:`renesas,ra-cgc-utasel`
   * :dtcompatible:`renesas,rz-cgc`
   * :dtcompatible:`sifli,sf32lb-rcc-clk`
   * :dtcompatible:`st,stm32f4-rcc`
   * :dtcompatible:`st,stm32fx-pllsai-clock`
   * :dtcompatible:`st,stm32h5-rcc`
   * :dtcompatible:`st,stm32l0-hsi-clock`
   * :dtcompatible:`st,stm32l4-pllsai-clock`

* Comparator

   * :dtcompatible:`nxp,cmp`
   * :dtcompatible:`renesas,rx-lvd`
   * :dtcompatible:`st,stm32-comp`
   * :dtcompatible:`st,stm32g4-comp`
   * :dtcompatible:`st,stm32h7-comp`

* Counter

   * :dtcompatible:`infineon,tcpwm-counter`
   * :dtcompatible:`nxp,imx-snvs-rtc`
   * :dtcompatible:`nxp,lpit`
   * :dtcompatible:`nxp,lpit-channel`
   * :dtcompatible:`renesas,rz-cmtw-counter`

* CPU

   * :dtcompatible:`arm,cortex-a78`
   * :dtcompatible:`arm,cortex-m52`
   * :dtcompatible:`arm,cortex-m52f`
   * :dtcompatible:`intel,panther-lake`
   * :dtcompatible:`renesas,rxv1`
   * :dtcompatible:`renesas,rxv2`
   * :dtcompatible:`renesas,rxv3`
   * :dtcompatible:`snps,av5rhx`
   * :dtcompatible:`snps,av5rmx`
   * :dtcompatible:`xuantie,e907`

* :abbr:`CRC (Cyclic Redundancy Check)`

   * :dtcompatible:`renesas,ra-crc`

* Cryptographic accelerator

   * :dtcompatible:`nxp,els`
   * :dtcompatible:`st,stm32-hash`

* :abbr:`DAC (Digital to Analog Converter)`

   * :dtcompatible:`atmel,samd5x-dac`
   * :dtcompatible:`silabs,vdac`

* Debug

   * :dtcompatible:`nordic,coresight-nrf`

* Display

   * :dtcompatible:`jdi,lpm013m126`
   * :dtcompatible:`sitronix,st7305`
   * :dtcompatible:`sitronix,st7306`
   * :dtcompatible:`sitronix,st7567`
   * :dtcompatible:`solomon,ssd1357`
   * :dtcompatible:`ultrachip,uc8151d`

* :abbr:`DMA (Direct Memory Access)`

   * :dtcompatible:`andestech,atcdmacx00`
   * :dtcompatible:`nuvoton,npcx-gdma`
   * :dtcompatible:`renesas,ra-dma`
   * :dtcompatible:`sifli,sf32lb-dmac`
   * :dtcompatible:`silabs,gpdma`

* Ethernet

   * :dtcompatible:`intel,eth-plat`
   * :dtcompatible:`intel,igc-mac`
   * :dtcompatible:`microchip,sam-ethernet-controller`
   * :dtcompatible:`nxp,imx-netc`
   * :dtcompatible:`nxp,imx-netc-blk-ctrl`
   * :dtcompatible:`virtio,net`

* Flash controller

   * :dtcompatible:`adi,max32-spixf-nor`
   * :dtcompatible:`bflb,flash-controller`
   * :dtcompatible:`ite,it51xxx-manual-flash-1k`
   * :dtcompatible:`nordic,nrf-mramc`
   * :dtcompatible:`renesas,ra-mram-controller`

* File system

   * :dtcompatible:`zephyr,fstab,ext2`

* Fuel gauge

   * :dtcompatible:`silergy,sy24561`
   * :dtcompatible:`ti,bq40z50`

* :abbr:`GPIO (General Purpose Input/Output)`

   * :dtcompatible:`aesc,gpio`
   * :dtcompatible:`arducam,ffc-40pin-connector`
   * :dtcompatible:`bflb,bl60x_70x-gpio`
   * :dtcompatible:`bflb,bl61x-gpio`
   * :dtcompatible:`fobe,quill-header`
   * :dtcompatible:`microchip,port-g1-gpio`
   * :dtcompatible:`microchip,sam-pio4`
   * :dtcompatible:`nordic,nrf-gpio-pad-group`
   * :dtcompatible:`nxp,pca6408`
   * :dtcompatible:`nxp,pcal6408`
   * :dtcompatible:`nxp,pcal6416`
   * :dtcompatible:`nxp,pcal9538`
   * :dtcompatible:`nxp,pcal9539`
   * :dtcompatible:`nxp,pcal9722`
   * :dtcompatible:`sifli,sf32lb-gpio`
   * :dtcompatible:`sifli,sf32lb-gpio-parent`
   * :dtcompatible:`silabs,exp-header`
   * :dtcompatible:`silabs,gpio`
   * :dtcompatible:`silabs,gpio-port`

* Hardware information

   * :dtcompatible:`nxp,cmc-reset-cause`
   * :dtcompatible:`nxp,imx-src-rev2`
   * :dtcompatible:`nxp,rstctl-hwinfo`

* :abbr:`I2C (Inter-Integrated Circuit)`

   * :dtcompatible:`infineon,cat1-i2c-pdl`
   * :dtcompatible:`renesas,rz-iic`
   * :dtcompatible:`silabs,i2c`
   * :dtcompatible:`ti,cc23x0-i2c`

* :abbr:`I3C (Improved Inter-Integrated Circuit)`

   * :dtcompatible:`adi,max32-i3c`

* IEEE 802.15.4

   * :dtcompatible:`st,stm32wba-ieee802154`

* Input

   * :dtcompatible:`chipsemi,chsc5x`
   * :dtcompatible:`nxp,mcux-kpp`
   * :dtcompatible:`renesas,ra-ctsu`
   * :dtcompatible:`renesas,rx-ctsu`

* Interrupt controller

   * :dtcompatible:`hazard3,hazard3-intc`
   * :dtcompatible:`microchip,dmec-ecia-girq`
   * :dtcompatible:`renesas,rz-icu`
   * :dtcompatible:`renesas,rz-intc`
   * :dtcompatible:`sifive,clic-draft`
   * :dtcompatible:`st,stm32-exti`

* :abbr:`LED (Light Emitting Diode)`

   * :dtcompatible:`leds-group-multicolor`
   * :dtcompatible:`nxp,pca9533`

* :abbr:`LED (Light Emitting Diode)` strip

   * :dtcompatible:`worldsemi,ws2812-uart`

* Mailbox

   * :dtcompatible:`renesas,ra-ipc-mbox`

* :abbr:`MDIO (Management Data Input/Output)`

   * :dtcompatible:`intel,igc-mdio`

* Memory controller

   * :dtcompatible:`motorola,mc146818-bbram`
   * :dtcompatible:`sifli,sf32lb-mpi`
   * :dtcompatible:`st,stm32-ospi-psram`

* :abbr:`MFD (Multi-Function Device)`

   * :dtcompatible:`motorola,mc146818-mfd`
   * :dtcompatible:`nxp,pca9422`
   * :dtcompatible:`sifli,sf32lb-rcc`

* Miscellaneous

   * :dtcompatible:`nxp,rt600-dsp-ctrl`
   * :dtcompatible:`nxp,rt700-dsp-ctrl-hifi4`
   * :dtcompatible:`renesas,rx-dtc`
   * :dtcompatible:`st,stm32-npu`

* Modem

   * :dtcompatible:`quectel,bg96`
   * :dtcompatible:`swir,hl7812`
   * :dtcompatible:`swir,hl78xx`

* :abbr:`MTD (Memory Technology Device)`

   * :dtcompatible:`andestech,qspi-nor-xip`
   * :dtcompatible:`atmel,at25xv021a`
   * :dtcompatible:`renesas,ra-nv-mram`
   * :dtcompatible:`renesas,ra-qspi-nor`

* :abbr:`OPAMP (Operational Amplifier)`

   * :dtcompatible:`nxp,opamp`
   * :dtcompatible:`nxp,opamp-fast`

   * :dtcompatible:`zephyr,generic-pstate`
   * :dtcompatible:`zephyr,native-sim-pstate`
   * :dtcompatible:`zephyr,pstate`

* Pin control

   * :dtcompatible:`ambiq,apollo2-pinctrl`
   * :dtcompatible:`microchip,port-g1-pinctrl`
   * :dtcompatible:`nxp,imx-blkctrl-ns-aon`
   * :dtcompatible:`nxp,imx-blkctrl-wakeup`
   * :dtcompatible:`sifli,sf32lb52x-pinmux`

* Power management

   * :dtcompatible:`ite,it8xxx2-power-elpm`
   * :dtcompatible:`sifli,sf32lb-aon`
   * :dtcompatible:`sifli,sf32lb-pmuc`

* Power domain

   * :dtcompatible:`nordic,nrfs-gdpwr`
   * :dtcompatible:`nordic,nrfs-swext`
   * :dtcompatible:`silabs,siwx91x-power-domain`

* :abbr:`PWM (Pulse Width Modulation)`

   * :dtcompatible:`ambiq,ctimer-pwm`
   * :dtcompatible:`ambiq,timer-pwm`
   * :dtcompatible:`infineon,tcpwm-pwm`
   * :dtcompatible:`renesas,rz-mtu-pwm`
   * :dtcompatible:`ti,cc23x0-lgpt-pwm`

* Quad SPI

   * :dtcompatible:`adi,max32-spixf`
   * :dtcompatible:`renesas,ra-qspi`

* Regulator

   * :dtcompatible:`nxp,pca9422-regulator`
   * :dtcompatible:`nxp,vrefv1`

* Reserved memory

   * :dtcompatible:`renesas,ofs-memory`

* Reset controller

   * :dtcompatible:`nxp,mrcc-reset`
   * :dtcompatible:`sifli,sf32lb-rcc-rctl`

* Retained memory

   * :dtcompatible:`sifli,sf32lb-rtc-backup`

* :abbr:`RNG (Random Number Generator)`

   * :dtcompatible:`ambiq,puf-trng`
   * :dtcompatible:`nxp,els-trng`

* :abbr:`RTC (Real Time Clock)`

   * :dtcompatible:`microcrystal,rv3032`
   * :dtcompatible:`nxp,pcf85063a`
   * :dtcompatible:`sifli,sf32lb-rtc`
   * :dtcompatible:`ti,mspm0-rtc`
   * :dtcompatible:`zephyr,rtc-counter`

* :abbr:`SDHC (Secure Digital Host Controller)`

   * :dtcompatible:`microchip,sama7g5-sdmmc`
   * :dtcompatible:`st,stm32-sdio`

* Sensors

   * :dtcompatible:`invensense,icm42686`
   * :dtcompatible:`invensense,icm4268x`
   * :dtcompatible:`maxbotix,mb7040`
   * :dtcompatible:`nxp,tmpsns`
   * :dtcompatible:`omron,d6f-p0001`
   * :dtcompatible:`omron,d6f-p0010`
   * :dtcompatible:`pni,rm3100`
   * :dtcompatible:`st,iis3dwb`
   * :dtcompatible:`ti,ina228`
   * :dtcompatible:`ti,ina7xx`
   * :dtcompatible:`we,wsen-isds-2536030320001`

* Serial controller

   * :dtcompatible:`infineon,cat1-uart-pdl`
   * :dtcompatible:`microchip,sercom-g1-uart`
   * :dtcompatible:`sifli,sf32lb-usart`
   * :dtcompatible:`virtio,console`
   * :dtcompatible:`zephyr,uart-bitbang`

* :abbr:`SPI (Serial Peripheral Interface)`

   * :dtcompatible:`egis,et171-spi`
   * :dtcompatible:`ti,omap-mcspi`

* System controller

   * :dtcompatible:`sifli,sf32lb-cfg`

* Timer

   * :dtcompatible:`ambiq,ctimer`
   * :dtcompatible:`ambiq,timer`
   * :dtcompatible:`infineon,tcpwm`
   * :dtcompatible:`microchip,xec-basic-timer`
   * :dtcompatible:`renesas,rz-cmtw`
   * :dtcompatible:`renesas,rz-mtu`
   * :dtcompatible:`st,stm32wb0-radio-timer`

* USB

   * :dtcompatible:`espressif,esp32-usb-otg`

* Video

   * :dtcompatible:`renesas,ra-ceu`
   * :dtcompatible:`st,stm32-venc`

* Watchdog

   * :dtcompatible:`nxp,cop`
   * :dtcompatible:`renesas,rx-iwdt`
   * :dtcompatible:`renesas,rz-wdt`
   * :dtcompatible:`sifli,sf32lb-wdt`
   * :dtcompatible:`ti,j7-rti-wdt`
   * :dtcompatible:`xlnx,versal-wwdt`

* Wi-Fi

   * :dtcompatible:`nordic,wlan`

New Samples
***********

..
  Same as above for boards and drivers, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

* :zephyr:code-sample:`adc_stream`
* :zephyr:code-sample:`capture`
* :zephyr:code-sample:`coap-upload`
* :zephyr:code-sample:`cpu_freq_on_demand`
* :zephyr:code-sample:`crc_drivers`
* :zephyr:code-sample:`crc_subsys`
* :zephyr:code-sample:`ext2-fstab`
* :zephyr:code-sample:`frdm_mcxa156_lpdac_opamp_lpadc`
* :zephyr:code-sample:`hello_hl78xx`
* :zephyr:code-sample:`instrumentation`
* :zephyr:code-sample:`latmon-client`
* :zephyr:code-sample:`mctp_i2c_bus_endpoint`
* :zephyr:code-sample:`mctp_i2c_bus_owner`
* :zephyr:code-sample:`msg_queue`
* :zephyr:code-sample:`netmidi2`
* :zephyr:code-sample:`ocpp`
* :zephyr:code-sample:`opamp_output_measure`
* :zephyr:code-sample:`openthread-border-router`
* :zephyr:code-sample:`producer_consumer`
* :zephyr:code-sample:`red-black-tree`
* :zephyr:code-sample:`renesas_lvd`
* :zephyr:code-sample:`rtk0eg0019b01002bj`
* :zephyr:code-sample:`virtiofs`

Libraries / Subsystems
**********************

* Logging:

  * Added hybrid rate-limited logging macros to prevent log flooding when messages are generated frequently.
    The system provides both convenience macros (using default rate from :kconfig:option:`CONFIG_LOG_RATELIMIT_INTERVAL_MS`)
    and explicit rate macros (with custom rate parameter). This follows Linux's ``printk_ratelimited`` pattern
    while providing more flexibility. The rate limiting is per-macro-call-site, meaning that each unique call
    to a rate-limited macro has its own independent rate limit. Rate-limited logging can be globally enabled/disabled
    via :kconfig:option:`CONFIG_LOG_RATELIMIT`. When rate limiting is disabled, the behavior can be controlled
    via :kconfig:option:`CONFIG_LOG_RATELIMIT_FALLBACK` to either log all messages or drop them completely.
    For more details, see :ref:`logging_ratelimited`.

* Mbed TLS

  * Kconfig :kconfig:option:`CONFIG_PSA_CRYPTO` is added to simplify the enablement of a PSA
    Crypto API provider. This is TF-M if :kconfig:option:`CONFIG_BUILD_WITH_TFM` is enabled,
    or Mbed TLS otherwise. :kconfig:option:`CONFIG_PSA_CRYPTO_PROVIDER_TFM` is set in the former
    case while :kconfig:option:`CONFIG_PSA_CRYPTO_PROVIDER_MBEDTLS` is set in the latter.
    :kconfig:option:`CONFIG_PSA_CRYPTO_PROVIDER_CUSTOM` is also added to allow end users to
    provide a custom solution.

* Secure storage

  * The experimental status has been removed. (:github:`96483`)

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?

* Nordic Semiconductor nRF54L09 PDK (``nrf54l09pdk``), which only targeted an emulator, has been removed
  from the tree. It will be replaced with a proper board definition as soon as it's available.

* Removed support for Nordic Semiconductor nRF54L20 PDK (``nrf54l20pdk``) since it is
  replaced with :zephyr:board:`nrf54lm20dk` (``nrf54lm20dk``).
