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

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

..
  Only removed, deprecated and new APIs, changes go in migration guide.

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
* Providing a third agument to :c:macro:`SPI_CONFIG_DT`, :c:macro:`SPI_CONFIG_DT_INST`,
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

* Crypto

  * :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS`

* Display

  * :c:enumerator:`PIXEL_FORMAT_AL_88`

  * SDL

    * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_AL_88`
    * :kconfig:option:`CONFIG_SDL_DISPLAY_COLOR_TINT`

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

* Video

  * :c:member:`video_format.size` field
  * :c:func:`video_estimate_fmt_size`

.. zephyr-keep-sorted-stop

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

New Drivers
***********

..
  Same as above for boards, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* Input

   * :dtcompatible:`chipsemi,chsc5x`

* Interrupt controller

   * STM32 EXTI interrupt/event controller (:dtcompatible:`st,stm32-exti`) has a dedicated driver and API now, separate from STM32 GPIO Interrupt Control driver.

* MFD
   * IRQ support has been added for X-Power AXP2101 MFD device. It gets automatically
     enabled as soon as device-tree property ``int-gpios`` is defined on the device node.

   * Support for the power button found on the X-Power AXP2101 MFD is added and can be enabled
     via :kconfig:option:`MFD_AXP2101_POWER_BUTTON`. This feature requires interrupt support to
     be enabled.

* RTC

   * STM32 RTC driver has been updated to use the new STM32 EXTI interrupt controller API

* Sensors

   * :dtcompatible:`we,wsen-isds-2536030320001`

New Samples
***********

..
  Same as above for boards and drivers, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

* Added a new sample :zephyr:code-sample:`opamp_output_measure` showing how to use the opamp device driver.

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
