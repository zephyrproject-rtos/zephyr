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

Phy
===

* Nodes with compatible property :dtcompatible:`st,stm32u5-otghs-phy` now need to select the
  CLKSEL (phy reference clock) in the SYSCFG_OTGHSPHYCR register using the new property
  clock-reference. The selection directly depends on the value on OTGHSSEL (OTG_HS PHY kernel
  clock source selection) located in the RCC_CCIPR2 register.

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

.. zephyr-keep-sorted-start re(^\w)

Bluetooth Audio
===============

* :c:struct:`bt_audio_codec_cfg` now requires setting the target latency and target PHY explicitly,
  rather than always setting the target latency to "Balanced" and the target PHY to LE 2M.
  To keep current functionality, set the ``target_latency`` to
  :c:enumerator:`BT_AUDIO_CODEC_CFG_TARGET_LATENCY_BALANCED` and ``target_phy`` to
  :c:enumerator:`BT_AUDIO_CODEC_CFG_TARGET_PHY_2M`.
  The :c:macro:`BT_AUDIO_CODEC_CFG` macro defaults to these values.
  (:github:`93825``)
* Setting the BGS role for GMAP now requires also supporting and implementing the
  :kconfig:option:`CONFIG_BT_BAP_BROADCAST_ASSISTANT`.
  See the :zephyr:code-sample:`bluetooth_bap_broadcast_assistant` sample as a reference.

.. zephyr-keep-sorted-stop

Bluetooth HCI
=============

* The deprecated ``ipm`` value was removed from ``bt-hci-bus`` devicetree property.
  ``ipc`` should be used instead.

Ethernet
========

* The :dtcompatible:`microchip,vsc8541` PHY driver now expects the reset-gpios entry to specify
  the GPIO_ACTIVE_LOW flag when the reset is being used as active low. Previously the active-low
  nature was hard-coded into the driver. (:github:`91726`).

Networking
**********

* The HTTP server now respects the configured ``_config`` value. Check that
  you provide applicable value to :c:macro:`HTTP_SERVICE_DEFINE_EMPTY`,
  :c:macro:`HTTPS_SERVICE_DEFINE_EMPTY`, :c:macro:`HTTP_SERVICE_DEFINE` and
  :c:macro:`HTTPS_SERVICE_DEFINE`.

.. zephyr-keep-sorted-start re(^\w)

.. zephyr-keep-sorted-stop

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

Other subsystems
****************

.. zephyr-keep-sorted-start re(^\w)

Logging
=======

* The UART dictionary log parsing script
  :zephyr_file:`scripts/logging/dictionary/log_parser_uart.py` has been deprecated. Instead, the
  more generic script of :zephyr_file:`scripts/logging/dictionary/live_log_parser.py` should be
  used. The new script supports the same functionality (and more), but requires different command
  line arguments when invoked.

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

Architectures
*************
