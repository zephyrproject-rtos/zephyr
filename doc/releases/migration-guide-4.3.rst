:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.3:

Migration guide to Zephyr v4.3.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.2.0 to
Zephyr v4.3.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.3>`.

.. contents::
    :local:
    :depth: 2

Build System
************

Kernel
******

* :c:func:`device_init` Earlier releases returned a positive +errno value in case
  of device init failure due to a bug. This is now fixed to return the correct
  negative -errno value. Applications that implemented workarounds for this
  issue should now update their code accordingly.

Base Libraries
**************

* UTF-8 utils declarations (:c:func:`utf8_trunc`, :c:func:`utf8_lcpy`) have
  been moved from ``util.h`` to a separate
  :zephyr_file:`include/zephyr/sys/util_utf8.h` file.

* ``Z_MIN``, ``Z_MAX`` and ``Z_CLAMP`` macros have been renamed to
  :c:macro:`min` :c:macro:`max` and :c:macro:`clamp`.

* The header files ``<zephyr/posix/time.h>``, ``<zephyr/posix/signal.h>`` should no longer be used.
  Include them in the standard path as ``<time.h>``, and ``<signal.h>``, provided by the C library.
  Non-POSIX C library maintainers may include :zephyr_file:`include/zephyr/posix/posix_time.h`
  and :zephyr_file:`include/zephyr/posix/posix_signal.h` to portably provide POSIX definitions.

* POSIX limits are no longer defined in ``<zephyr/posix/posix_features.h>``. Similarly, include them
  in the standard path via ``<limits.h>``, provided by the C library. Non-POSIX C library maintainers
  may include :zephyr_file:`include/zephyr/posix/posix_limits.h` for Zephyr's definitions. Some
  runtime-invariant values may need to be queried via :c:func:`sysconf`.

* The number of file descriptor table size and its availability is now determined by
  a ``ZVFS_OPEN_SIZE`` define instead of the :kconfig:option:`CONFIG_ZVFS_OPEN_MAX`
  Kconfig option. Subsystems can specify their own custom file descriptor table size
  requirements by specifying Kconfig options with the prefix ``CONFIG_ZVFS_OPEN_ADD_SIZE_``.
  The old Kconfig option still exists, but will be overridden if the custom requirements
  are larger. To force the old Kconfig option to be used, even when its value is less
  than the indicated custom requirements, a new :kconfig:option:`CONFIG_ZVFS_OPEN_IGNORE_MIN`
  option has been introduced (which defaults being disabled).

Boards
******

* b_u585i_iot02a/ns: The flash layout was changed to be in sync with the upstream TF-M 2.2.1 board
  configurations. The new layout expands the flash partitions, moving the secondary ones to the
  external NOR flash. This change currently prevents upgrade from older Zephyr release images to
  Zephyr 4.3 release images. More details in the TF-M migration and release notes.

* nucleo_h753zi: the flash layout was updated and firmware upgrade may fail due to layout
  incompatibility with the previous layout. The new layout includes storage partition enlarged to
  2 sectors, scratch partition removed and all flash partitions reordered for better structure.

* mimxrt11x0: renamed lpadc1 to lpadc2 and renamed lpadc0 to lpadc1.

* NXP ``frdm_mcxa166`` is renamed to ``frdm_mcxa346``.
* NXP ``frdm_mcxa276`` is renamed to ``frdm_mcxa266``.

* Panasonic ``panb511evb`` is renamed to ``panb611evb``.

* STM32 boards OpenOCD configuration files have been changed to support latest OpenOCD versions
  (> v0.12.0) in which the HLA/SWD transport has been deprecated (see
  https://review.openocd.org/c/openocd/+/8523 and commit
  https://sourceforge.net/p/openocd/code/ci/34ec5536c0ba3315bc5a841244bbf70141ccfbb4/).
  Issues may be encountered when connecting to an ST-Link adapter running firmware prior
  v2j24 which do not support the new transport. In this case, the ST-Link firmware should
  be upgraded or, if not possible, the OpenOCD configuration script should be changed to
  source "interface/stlink-hla.cfg" and select the "hla_swd" interface explicitly.
  Backward compatibility with OpenOCD v0.12.0 or older is maintained.

Device Drivers and Devicetree
*****************************

.. zephyr-keep-sorted-start re(^\w)

ADC
===

* ``iadc_gecko.c`` driver is replaced by ``adc_silabs_iadc.c``.
  :dtcompatible:`silabs,gecko-iadc` is replaced by :dtcompatible:`silabs,iadc`.

* :dtcompatible:`st,stm32-adc` and its derivatives now require the ``clock-names`` property to be
  defined and to match the number of clocks in the ``clocks`` property. The expected clock names are
  ``adcx`` for the register clock, ``adc-ker`` for the kernel source clock, and ``adc-pre`` to set
  the ADC prescaler (for series where it is located in the RCC registers).

Clock Control
=============

* :kconfig:option:`CONFIG_CLOCK_STM32_HSE_CLOCK` is no longer user-configurable. Its value is now
  always taken from the ``clock-frequency`` property of ``&clk_hse`` DT node, but only if the node
  is enabled (otherwise, the symbol is not defined). This change should only affect STM32 MPU-based
  platforms and aligns them with existing practice from STM32 MCU platforms.

* :dtcompatible:`st,stm32f1-rcc` and :dtcompatible:`st,stm32f3-rcc` do not exist anymore. Therefore
  ``adc-prescaler``, ``adc12-prescaler`` and ``adc34-prescaler`` properties are no longer defined
  either. They are replaced by adding the prescaler as an additional clock in the ADC ``clocks``
  property.

Comparator
==========

* :dtcompatible:`nordic,nrf-comp` and :dtcompatible:`nordic,nrf-lpcomp` ``psel`` and ``extrefsel``
  properties type has been changed to integer. The value of these properties is in the range
  of :c:macro:`NRF_COMP_AIN0` to :c:macro:`NRF_COMP_AIN_VDDH_DIV5`, where :c:macro:`NRF_COMP_AIN0`
  to :c:macro:`NRF_COMP_AIN7` represent the external inputs AIN0 to AIN7,
  :c:macro:`NRF_COMP_AIN_VDD_DIV2` represents internal reference VDD/2,
  and :c:macro:`NRF_COMP_AIN_VDDH_DIV5` represents VDDH/5.
  The old ``string`` properties type is deprecated.

DMA
===

* DMA no longer implements user mode syscalls as part of its API. The syscalls were determined to be
  too broadly defined in access and impossible to implement the syscall parameter verification step
  in another.

MFD
===

* Driver support for AXP2101 has been separated from the AXP192 one. As a consequence the
  kconfig symbol ``MFD_AXP192_AXP2101`` is removed. :kconfig:option:`MFD_AXP192` is now to be
  used for AXP192 device while :kconfig:option:`MFD_AXP2101` for the AXP2101 one.

MISC
====

* The nrf_etr driver has been migrated to drivers/debug. As a consequence the related Kconfig
  symbol was renamed from ``NRF_ETR`` to :kconfig:option:`DEBUG_NRF_ETR`, along with the rest of
  the ``NRF_ETR`` symbols. Also the driver needs to be explicitly enabled via
  :kconfig:option:`DEBUG_DRIVER` as it is no longer built by default.

PWM
===

* :dtcompatible:`nxp,pca9685` ``invert`` property has been removed and you can now use the
  :c:macro:`PWM_POLARITY_INVERTED` or :c:macro:`PWM_POLARITY_NORMAL` flags as specifier cells for
  space "pwm" are now named: ``['channel', 'period', 'flags']`` (old value:
  ``['channel', 'period']``) and ``#pwm-cells`` const value changed from 2 to 3.

Phy
===

* Nodes with compatible property :dtcompatible:`st,stm32u5-otghs-phy` now need to select the
  CLKSEL (phy reference clock) in the SYSCFG_OTGHSPHYCR register using the new property
  clock-reference. The selection directly depends on the value on OTGHSSEL (OTG_HS PHY kernel
  clock source selection) located in the RCC_CCIPR2 register.

SPI
===

* The macros :c:macro:`SPI_CS_CONTROL_INIT` :c:macro:`SPI_CS_CONTROL_INIT_INST`,
  :c:macro:`SPI_CONFIG_DT`, :c:macro:`SPI_CONFIG_DT_INST`, :c:macro:`SPI_DT_SPEC_GET`,
  and :c:macro:`SPI_DT_SPEC_INST_GET` have been changed so that they do not need to be
  provided a delay parameter anymore. This is because the timing parameters of a SPI peripheral
  chip select should now be specified in DT with the
  ``spi-cs-setup-delay-ns`` and ``spi-cs-hold-delay-ns`` properties.
  (:github:`87427`).

Sensors
=======

* Nodes with compatible property :dtcompatible:`invensense,icm42688` now additionally need to also
  include :dtcompatible:`invensense,icm4268x` in order to work.

Stepper
=======

* :dtcompatible:`zephyr,gpio-stepper` has been replaced by :dtcompatible:`zephyr,h-bridge-stepper`.

USB
===

* The USB Video Class was configuring the framerate and format of the source video device.
  This is now to be done by the application after the host selected the format (:github:`93192`).

.. zephyr-keep-sorted-stop

Bluetooth
*********

* :c:struct:`bt_le_cs_test_param` and :c:struct:`bt_le_cs_create_config_params` now require
  providing both the main and sub mode as a single parameter.
* :c:struct:`bt_conn_le_cs_config` now reports both the main and sub mode as a single parameter.
* :c:struct:`bt_conn_le_cs_main_mode` and :c:struct:`bt_conn_le_cs_sub_mode` have been replaced
  with :c:struct:`bt_conn_le_cs_mode`.

Bluetooth Controller
====================

* The following have been renamed:

    * :kconfig:option:`CONFIG_BT_CTRL_ADV_ADI_IN_SCAN_RSP` to
      :kconfig:option:`CONFIG_BT_CTLR_ADV_ADI_IN_SCAN_RSP`
    * :c:struct:`bt_hci_vs_fata_error_cpu_data_cortex_m` to
      :c:struct:`bt_hci_vs_fatal_error_cpu_data_cortex_m` and now contains the program counter
      value.

.. zephyr-keep-sorted-start re(^\w)

Bluetooth Audio
===============

* :c:struct:`bt_audio_codec_cfg` now requires setting the target latency and target PHY explicitly,
  rather than always setting the target latency to "Balanced" and the target PHY to LE 2M.
  To keep current functionality, set the ``target_latency`` to
  :c:enumerator:`BT_AUDIO_CODEC_CFG_TARGET_LATENCY_BALANCED` and ``target_phy`` to
  :c:enumerator:`BT_AUDIO_CODEC_CFG_TARGET_PHY_2M`.
  The :c:macro:`BT_AUDIO_CODEC_CFG` macro defaults to these values.
  (:github:`93825`)
* Setting the BGS role for GMAP now requires also supporting and implementing the
  :kconfig:option:`CONFIG_BT_BAP_BROADCAST_ASSISTANT`.
  See the :zephyr:code-sample:`bluetooth_bap_broadcast_assistant` sample as a reference.
* The BAP Scan Delegator will no longer automatically update the PA sync state, and
  :c:func:`bt_bap_scan_delegator_set_pa_state` must be used to update the state. If the
  BAP Scan Delegator is used together with the BAP Broadcast Sink, then the PA state of the
  receive state of a  :c:struct:`bt_bap_broadcast_sink` will still be automatically updated when the
  PA state changes. (:github:`95453`)


.. zephyr-keep-sorted-stop

Bluetooth HCI
=============

* The deprecated ``ipm`` value was removed from ``bt-hci-bus`` devicetree property.
  ``ipc`` should be used instead.

Bluetooth Mesh
==============

* Kconfigs ``CONFIG_BT_MESH_USES_MBEDTLS_PSA`` and ``CONFIG_BT_MESH_USES_TFM_PSA`` have
  been removed. The selection of the PSA Crypto provider is now automatically controlled
  by Kconfig :kconfig:option:`CONFIG_PSA_CRYPTO`.

Bluetooth Host
==============

* :kconfig:option:`CONFIG_BT_FIXED_PASSKEY` has been deprecated. Instead, the application can
  provide passkeys for pairing using the :c:member:`bt_conn_auth_cb.app_passkey` callback, which is
  available when :kconfig:option:`CONFIG_BT_APP_PASSKEY` is enabled. The application can return the
  passkey for pairing, or :c:macro:`BT_PASSKEY_RAND` for the Host to generate a random passkey
  instead.

Ethernet
========

* The :dtcompatible:`microchip,vsc8541` PHY driver now expects the reset-gpios entry to specify
  the GPIO_ACTIVE_LOW flag when the reset is being used as active low. Previously the active-low
  nature was hard-coded into the driver. (:github:`91726`).

* CRC checksum generation offloading to hardware is now explicitly disabled rather then explicitly
  enabled in the Xilinx GEM Ethernet driver (:dtcompatible:`xlnx,gem`). By default, offloading is
  now enabled by default to improve performance, however, offloading is always disabled for QEMU
  targets due to the checksum generation in hardware not being emulated regardless of whether it
  is explicitly disabled via the devicetree or not. (:github:`95435`)

    * Replaced devicetree property ``rx-checksum-offload`` which enabled RX checksum offloading
      ``disable-rx-checksum-offload`` which now actively disables it.
    * Replaced devicetree property ``tx-checksum-offload`` which enabled TX checksum offloading
      ``disable-tx-checksum-offload`` which now actively disables it.

* The Xilinx GEM Ethernet driver (:dtcompatible:`xlnx,gem`) now obtains the AMBA AHB data bus
  width matching the current target SoC (either Zynq-7000 or ZynqMP) from a design configuration
  register at run-time, making the devicetree property ``amba-ahb-dbus-width`` obsolete, which
  has therefore been removed.

Power management
****************

* :kconfig:option:`CONFIG_PM_S2RAM` and :kconfig:option:`PM_S2RAM_CUSTOM_MARKING` have been
  refactored to be automatically managed by SoCs and the devicetree. Applications shall no
  longer enable them directly, instead, enable or disable the "suspend-to-ram" power states
  in the devicetree.

* For the NXP RW61x, the devicetree property ``exit-latency-us`` has been updated to reflect more
  accurate, measured wake-up times. For applications utilizing Standby mode (PM3), this update and
  an increase to the ``min-residency-us`` devicetree property may influence how the system
  transitions between power modes. In some cases, this could lead to changes in power consumption.

Networking
**********

* The HTTP server now respects the configured ``_config`` value. Check that
  you provide applicable value to :c:macro:`HTTP_SERVICE_DEFINE_EMPTY`,
  :c:macro:`HTTPS_SERVICE_DEFINE_EMPTY`, :c:macro:`HTTP_SERVICE_DEFINE` and
  :c:macro:`HTTPS_SERVICE_DEFINE`.

* The size of socket address length type :c:type:`socklen_t` has changed. It is now defined to
  be always 32 bit ``uint32_t`` in order to be aligned with Linux. Previously it was defined as
  ``size_t`` which meant that the size could be either 32 bit or 64 bit depending on system
  configuration.

* :c:func:`net_icmp_init_ctx` API has changed, it now accepts an additional ``family`` argument
  to indicate what packet family the context should work with. For ICMPv4 contexts, use ``AF_INET``,
  for ICMPv6 contexts use ``AF_INET6``.

.. zephyr-keep-sorted-start re(^\w)

CoAP
====

* The :c:type:`coap_client_response_cb_t` signature has changed. The list of arguments
  is passed as a :c:struct:`coap_client_response_data` pointer instead.

* The :c:struct:`coap_client_request` has changed to improve the library's resilience against
  misconfiguration (i.e. using transient pointers within the struct):

  * The :c:member:`coap_client_request.path` is now a ``char`` array instead of a pointer.
    The array size is configurable with :kconfig:option:`CONFIG_COAP_CLIENT_MAX_PATH_LENGTH`.
  * The :c:member:`coap_client_request.options` is now a :c:struct:`coap_client_option` array
    instead of a pointer. The array size is configurable with
    :kconfig:option:`CONFIG_COAP_CLIENT_MAX_EXTRA_OPTIONS`.

.. zephyr-keep-sorted-stop

Modem
*****

* ``CONFIG_MODEM_AT_SHELL_USER_PIPE`` has been renamed to :kconfig:option:`CONFIG_MODEM_AT_USER_PIPE`.
* ``CONFIG_MODEM_CMUX_WORK_BUFFER_SIZE`` has been updated to :kconfig:option:`CONFIG_MODEM_CMUX_WORK_BUFFER_SIZE_EXTRA`,
  which only takes the number of extra bytes desired over the default of (:kconfig:option:`CONFIG_MODEM_CMUX_MTU` + 7).

Display
*******

* The RGB565 and BGR565 pixel formats were used interchangeably in the display sample.
  This has now been fixed. Boards and applications that were tested or developed based on the
  previous sample may be affected by this change (see :github:`79996` for more information).

* SSD1363's properties using 'greyscale' now use 'grayscale'.

PTP Clock
*********

* The doc of :c:func:`ptp_clock_rate_adjust` API didn't provide proper and clear function description.
  Drivers implemented it to adjust rate ratio relatively based on current frequency.
  Now PI servo is introduced in both PTP and gPTP, and this API function is changed to use for rate
  ratio adjusting based on nominal frequency. Drivers implementing :c:func:`ptp_clock_rate_adjust`
  should be adjusted to account for the new behavior.

Video
*****

* The ``min_line_count`` and ``max_line_count`` fields have been removed from :c:struct:`video_caps`.
  Application should base on the new :c:member:`video_format.size` to allocate buffers.

Other subsystems
****************

.. zephyr-keep-sorted-start re(^\w)

Cellular
========

 * :c:enum:`cellular_access_technology` values have been redefined to align with 3GPP TS 27.007.
 * :c:enum:`cellular_registration_status` values have been extended to align with 3GPP TS 27.007.

Crypto
======

* Hashing operations now require a constant input in the :c:struct:`hash_pkt`.
  This shouldn't affect any existing code, unless an out-of-tree hashing backend actually
  performs that operation in-place (see :github:`94218`)

Flash Map
=========

* With the long-term goal of transitioning to PSA Crypto API as the only crypto support in Zephyr,
  :kconfig:option:`FLASH_AREA_CHECK_INTEGRITY_MBEDTLS` is deprecated.
  :kconfig:option:`FLASH_AREA_CHECK_INTEGRITY_PSA` is now the default choice: if TF-M is not
  enabled or not supported by the platform, Mbed TLS will be used as PSA Crypto API provider.

Logging
=======

* The UART dictionary log parsing script
  :zephyr_file:`scripts/logging/dictionary/log_parser_uart.py` has been deprecated. Instead, the
  more generic script of :zephyr_file:`scripts/logging/dictionary/live_log_parser.py` should be
  used. The new script supports the same functionality (and more), but requires different command
  line arguments when invoked.

MCUmgr
======

* The :ref:`OS mgmt<mcumgr_smp_group_0>` :ref:`mcumgr_os_application_info` command's response for
  hardware platform has been updated to output the board target instead of the board and board
  revision, which now includes the SoC and board variant. The old behaviour has been deprecated,
  but can still be used by enabling
  :kconfig:option:`CONFIG_MCUMGR_GRP_OS_INFO_HARDWARE_INFO_SHORT_HARDWARE_PLATFORM`.

* Support for legacy Mbed TLS hash crypto is removed and only PSA Crypto API is used.
  :kconfig:option:`CONFIG_MCUMGR_GRP_FS_HASH_SHA256` automatically enables Mbed TLS and its
  PSA Crypto implementation if TF-M is not enabled in the build.

Mbed TLS
========

* In order to improve the 1:1 matching between Zephyr Kconfig and Mbed TLS build symbols, the
  following Kconfigs were renamed:

  * :kconfig:option:`CONFIG_MBEDTLS_MD` -> :kconfig:option:`CONFIG_MBEDTLS_MD_C`
  * :kconfig:option:`CONFIG_MBEDTLS_LMS` -> :kconfig:option:`CONFIG_MBEDTLS_LMS_C`
  * :kconfig:option:`CONFIG_MBEDTLS_TLS_VERSION_1_2` -> :kconfig:option:`CONFIG_MBEDTLS_SSL_PROTO_TLS1_2`
  * :kconfig:option:`CONFIG_MBEDTLS_DTLS` -> :kconfig:option:`CONFIG_MBEDTLS_SSL_PROTO_DTLS`
  * :kconfig:option:`CONFIG_MBEDTLS_TLS_VERSION_1_3` -> :kconfig:option:`CONFIG_MBEDTLS_SSL_PROTO_TLS1_3`
  * :kconfig:option:`CONFIG_MBEDTLS_TLS_SESSION_TICKETS` -> :kconfig:option:`CONFIG_MBEDTLS_SSL_SESSION_TICKETS`
  * :kconfig:option:`CONFIG_MBEDTLS_CTR_DRBG_ENABLED` -> :kconfig:option:`CONFIG_MBEDTLS_CTR_DRBG_C`
  * :kconfig:option:`CONFIG_MBEDTLS_HMAC_DRBG_ENABLED` -> :kconfig:option:`CONFIG_MBEDTLS_HMAC_DRBG_C`

RTIO
====

* Callback operations now take an additional argument corresponding to the result code of the first
  error in the chain.
* Callback operations are always called regardless of success/error status of previous submissions
  in the chain.

Secure storage
==============

* The size of :c:type:`psa_storage_uid_t`, used to identify storage entries, was changed from 64 to
  30 bits.
  This change breaks backward compatibility with previously stored entries for which authentication
  will start failing.
  Enable :kconfig:option:`CONFIG_SECURE_STORAGE_64_BIT_UID` if you are updating an existing
  installation from an earlier version of Zephyr and want to keep the pre-existing entries.
  (:github:`94171`)

Shell
=====

* The MQTT topics related to :kconfig:option:`SHELL_BACKEND_MQTT` have been renamed. Renamed
  ``<device_id>_rx`` to ``<device_id>/sh/rx`` and ``<device_id>_tx`` to ``<device_id>/sh/rx``. The
  part after the ``<device_id>`` is now configurable via :kconfig:option:`SHELL_MQTT_TOPIC_RX_ID`
  and :kconfig:option:`SHELL_MQTT_TOPIC_TX_ID`. This allows keeping the previous topics for backward
  compatibility.
  (:github:`92677`).

UpdateHub
=========

* Legacy Mbed TLS as an option for crypto support has been removed and PSA Crypto is now used in all
  cases. :kconfig:option:`CONFIG_UPDATEHUB` will automatically enable the Mbed TLS implementation of
  PSA Crypto if TF-M is not enabled in the build.

.. zephyr-keep-sorted-stop

Modules
*******

* The TinyCrypt library was removed as the upstream version is no longer maintained.
  PSA Crypto API is now the recommended cryptographic library for Zephyr.

MCUboot
=======

* The default operating mode for MCUboot has been changed to swap using offset, this provides
  faster swap updates needed less overhead and reduces the flash endurance cycles required to
  perform an update, the previous default was swap using move. If a board was optimised for swap
  using move by having a primary slot that was one sector larger than the secondary then this
  needs to change to have the secondary slot one sector larger than the primary (for optimised
  usage, it is still supported to have both slots the same number of sectors). Alternatively, the
  previous swap using move mode can be selected in sysbuild by using
  :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SWAP_USING_MOVE`.

Silabs
======

* Aligned the name of the Rail options with the other SiSDK related options:

   * :kconfig:option:`CONFIG_RAIL_PA_CURVE_HEADER` to
     :kconfig:option:`CONFIG_SILABS_SISDK_RAIL_PA_CURVE_HEADER`
   * :kconfig:option:`CONFIG_RAIL_PA_CURVE_TYPES_HEADER` to
     :kconfig:option:`CONFIG_SILABS_SISDK_RAIL_PA_CURVE_TYPES_HEADER`
   * :kconfig:option:`CONFIG_RAIL_PA_ENABLE_CALIBRATION` to
     :kconfig:option:`CONFIG_SILABS_SISDK_RAIL_PA_ENABLE_CALIBRATION`

* Fixed name of the :kconfig:option:`CONFIG_SOC_*`. These option contained PART_NUMBER in their
  while they shouldn't.

* The separate ``em3`` power state was removed from Series 2 SoCs. The system automatically
  transitions to EM2 or EM3 depending on hardware peripheral requests for the oscillators.

LVGL
====

* The PIXEL_FORMAT_MONO10 and PIXEL_FORMAT_MONO01 formats were swapped
  in :zephyr_file:`modules/lvgl/lvgl_display_mono.c`, which caused
  black and white to be inverted when using LVGL with monochrome displays.
  This issue has now been fixed. Any workarounds previously applied to achieve the expected
  behavior should be removed, otherwise black and white will be inverted again.

LED Strip
=========

* Renamed ``arduino,modulino-smartleds`` to :dtcompatible:`arduino,modulino-pixels`

Trusted Firmware-M
==================

* The signing process for BL2 (MCUboot) was updated. The boards that run using
  TF-M NS and require BL2 must have their flash layout with the flash controller
  information. This will ensure that when signing the hex/bin files all the
  details will be present in the S and NS images. The image now has the details
  to allow the FWU state machine be correct and allow FOTA.
  (:github:`94470`)

    * The ``--align`` parameter was fixed to 1. Now, it's set to the flash DT ``write_block_size``
      property, but still provides 1 as a fallback for specific vendors.
    * The ``--max-sectors`` value is now calculated based on the number of images, taking into
      consideration the largest image size.
    * The ``--confirm`` option now confirms both S and NS HEX images, ensuring that any image
      that runs is valid for production and development.
    * S and NS BIN images are now available. These are the correct images to be used in FOTA. Note
      that S and NS images are unconfirmed by default, and the application is responsible for
      confirming them with ``psa_fwu_accept()``. Otherwise, the images will roll back on the next
      reboot.

* A compatibility issue was identified in the TF-M attestation procedure introduced
  after the TF-M v2.1.0 release. As a result, systems using TF-M v2.1 cannot be
  upgraded to any later TF-M version without encountering failures.
  This limitation affects Zephyr versions using TF-M v2.1.0 through v2.1.2, specifically,
  Zephyr v3.7 through v4.2, preventing seamless upgrades between these releases.
  The issue was resolved in mainline TF-M as of October 25 and the fix is included
  in Zephyr v4.3.0. Users are advised to migrate directly from any earlier Zephyr
  release to Zephyr v4.3.0 or later to ensure full TF-M attestation functionality
  and upgrade compatibility.
  (:github:`94859`)

* Support for automatically downloading MCUboot and ethos by CMake in a build has been removed,
  the in-tree versions of these modules will be used instead. To use custom versions, create a
  :ref:`west manifest <west-manifest-files>` which pulls in the desired versions of these
  repositories instead.

Architectures
*************
