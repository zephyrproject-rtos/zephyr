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

Boards
******

* b_u585i_iot02a/ns: The flash layout was changed to be in sync with the upstream TF-M 2.2.1 board
  configurations. The new layout expands the flash partitions, moving the secondary ones to the
  external NOR flash. This change currently prevents upgrade from older Zephyr release images to
  Zephyr 4.3 release images. More details in the TF-M migration and release notes.

* mimxrt11x0: renamed lpadc1 to lpadc2 and renamed lpadc0 to lpadc1.

* NXP ``frdm_mcxa166`` is renamed to ``frdm_mcxa346``.
* NXP ``frdm_mcxa276`` is renamed to ``frdm_mcxa266``.

* Panasonic ``panb511evb`` is renamed to ``panb611evb``.

Device Drivers and Devicetree
*****************************

.. zephyr-keep-sorted-start re(^\w)

ADC
===

* ``iadc_gecko.c`` driver is replaced by ``adc_silabs_iadc.c``.
  :dtcompatible:`silabs,gecko-iadc` is replaced by :dtcompatible:`silabs,iadc`.

Clock Control
=============

* :kconfig:option:`CONFIG_CLOCK_STM32_HSE_CLOCK` is no longer user-configurable. Its value is now
  always taken from the ``clock-frequency`` property of ``&clk_hse`` DT node, but only if the node
  is enabled (otherwise, the symbol is not defined). This change should only affect STM32 MPU-based
  platforms and aligns them with existing practice from STM32 MCU platforms.

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

* The following Kconfig option have been renamed:

    * :kconfig:option:`CONFIG_BT_CTRL_ADV_ADI_IN_SCAN_RSP` to
      :kconfig:option:`CONFIG_BT_CTLR_ADV_ADI_IN_SCAN_RSP`

   * :c:func:`bt_ctlr_set_public_addr` is deprecated. To set the public Bluetooth device address,
     sending a vendor specific HCI command with :c:struct:`bt_hci_cp_vs_write_bd_addr` can be used.

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

.. zephyr-keep-sorted-stop

Modules
*******

* The TinyCrypt library was removed as the upstream version is no longer maintained.
  PSA Crypto API is now the recommended cryptographic library for Zephyr.

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

Architectures
*************
