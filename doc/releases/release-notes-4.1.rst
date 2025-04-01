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

.. _zephyr_4.1:

Zephyr 4.1.0
############

We are pleased to announce the release of Zephyr version 4.1.0.
Major enhancements with this release include:

**Performance improvements**
  Multiple performance improvements of core Zephyr kernel functions have been implemented,
  benefiting all supported hardware architectures.

  An official port of the :zephyr_file:`thread_metric <tests/benchmarks/thread_metric>` RTOS
  benchmark has also been added to make it easier for developers to measure the performance of
  Zephyr on their hardware and compare it to other RTOSes.

**Experimental support for IAR compiler**
  :ref:`toolchain_iar_arm` can now be used to build Zephyr applications. This is an experimental
  feature that is expected to be improved in future releases.

**Initial support for Rust on Zephyr**
  It is now possible to write Zephyr applications in Rust. :ref:`language_rust` is available through
  an optional Zephyr module, and several code samples are available as a starting point.

**USB MIDI Class Driver**
  Introduction of a new :ref:`USB MIDI 2.0 <usbd_midi2>` device driver, allowing Zephyr devices to
  communicate with MIDI controllers and instruments over USB.

**Expanded Board Support**
  Support for 70 :ref:`new boards <boards_added_in_zephyr_4_1>` and 11
  :ref:`new shields <shields_added_in_zephyr_4_1>` has been added in this release.

  This includes highly popular boards such as :zephyr:board:`rpi_pico2` and
  :zephyr:board:`ch32v003evt`, several boards with CAN+USB capabilities making them good candidates
  for running the Zephyr-based open source `CANnectivity`_ firmware, and dozens of other boards
  across all supported architectures.

.. _CANnectivity: https://cannectivity.org/

An overview of the changes required or recommended when migrating your application from Zephyr
v4.0.0 to Zephyr v4.1.0 can be found in the separate :ref:`migration guide<migration_4.1>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* :cve:`2025-1673` `Zephyr project bug tracker GHSA-jjhx-rrh4-j8mx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jjhx-rrh4-j8mx>`_

* :cve:`2025-1674` `Zephyr project bug tracker GHSA-x975-8pgf-qh66
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-x975-8pgf-qh66>`_

* :cve:`2025-1675` `Zephyr project bug tracker GHSA-2m84-5hfw-m8v4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2m84-5hfw-m8v4>`_

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

Removed APIs and options
========================

* The legacy Bluetooth HCI driver API has been removed. It has been replaced
  by a :c:group:`new API<bt_hci_api>` that follows the normal Zephyr driver
  model.

* The ``CAN_MAX_STD_ID`` (replaced by :c:macro:`CAN_STD_ID_MASK`) and
  ``CAN_MAX_EXT_ID`` (replaced by :c:macro:`CAN_EXT_ID_MASK`) CAN API macros
  have been removed.

* The ``can_get_min_bitrate()`` (replaced by :c:func:`can_get_bitrate_min`)
  and ``can_get_max_bitrate()`` (replaced by :c:func:`can_get_bitrate_max`)
  CAN API functions have been removed.

* The ``can_calc_prescaler()`` CAN API function has been removed.

* The :kconfig:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` option has been
  removed.  It was a legacy option and was used to allow user to call BSD
  socket API while not enabling POSIX API.  This removal means that in order
  to use POSIX API socket calls, one needs to enable the
  :kconfig:option:`CONFIG_POSIX_API` option.  If the application does not want
  or is not able to enable that option, then the socket API calls need to be
  prefixed by a ``zsock_`` string.

* Removed ``video_pix_fmt_bpp()`` function that was returning a *byte* count
  and only supported 8-bit depth to :c:func:`video_bits_per_pixel()` returning
  the *bit* count and supporting any color depth.

* The ``video_stream_start()`` and ``video_stream_stop()`` driver APIs have been
  replaced by ``video_set_stream()``.

* :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO`

* The :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_EXCLUSIVE` option has been removed
  after being deprecated in favor of :kconfig:option:`CONFIG_PM_DEVICE_SYSTEM_MANAGED`.

* The ``z_pm_save_idle_exit()`` PM API function has been removed.

* Struct ``z_arch_esf_t`` has been removed. Use ``struct arch_esf`` instead.

* The following networking options have been removed:

    * ``CONFIG_NET_PKT_BUF_DATA_POOL_SIZE``
    * ``CONFIG_NET_TCP_ACK_TIMEOUT``


Deprecated APIs and options
===========================

* the :c:func:`bt_le_set_auto_conn` API function. Application developers can achieve
  the same functionality in their application code by reconnecting to the peer when the
  :c:member:`bt_conn_cb.disconnected` callback is invoked.

* :kconfig:option:`CONFIG_NATIVE_APPLICATION` has been deprecated.

* Deprecated the :c:func:`stream_flash_erase_page` from Stream Flash API. The same functionality
  can be achieved using :c:func:`flash_area_erase` or :c:func:`flash_erase`. Nevertheless
  erasing of a device, while stream flash is supposed to do so, as configured, will result in
  data lost from stream flash. There are only two situations where device should be erased
  directly:

  1. when Stream Flash is not configured to do erase on its own
  2. when erase is used for removal of a data prior or after Stream Flash uses the designated area.

* The pipe API has been reworked.
  The new API is enabled by default when ``CONFIG_MULTITHREADING`` is set.

  * Deprecates the ``CONFIG_PIPES`` Kconfig option.
  * Introduces the ``k_pipe_close(..)`` function.
  * ``k_pipe_put(..)`` translates to ``k_pipe_write(..)``.
  * ``k_pipe_get(..)`` translates to ``k_pipe_read(..)``.
  * ``k_pipe_flush(..)`` & ``k_pipe_buffer_flush()`` can be translated to ``k_pipe_reset(..)``.

  * Dynamic allocation of pipes is no longer supported.

    - ``k_pipe_alloc_init(..)`` API has been removed.
    - ``k_pipe_cleanup(..)`` API has been removed.

  * Querying the number of bytes in the pipe is no longer supported.

    - ``k_pipe_read_avail(..)`` API has been removed.
    - ``k_pipe_write_avail(..)`` API has been removed.


* For the native_sim target :kconfig:option:`CONFIG_NATIVE_SIM_NATIVE_POSIX_COMPAT` has been
  switched to ``n`` by default, and this option has been deprecated.

* :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT`

* All HWMv1 board name aliases which were added as deprecated in v3.7 are now removed
  (:github:`82247`).

* The TinyCrypt library has been deprecated as the upstream version is no longer maintained.
  PSA Crypto API is now the recommended cryptographic library for Zephyr.

* The :kconfig:option:`CONFIG_BT_DIS_MODEL` and :kconfig:option:`CONFIG_BT_DIS_MANUF` have been
  deprecated. Application developers can achieve the same configuration by using the new
  :kconfig:option:`CONFIG_BT_DIS_MODEL_NUMBER_STR` and
  :kconfig:option:`CONFIG_BT_DIS_MANUF_NAME_STR` Kconfig options.

New APIs and options
====================

..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

* Architectures

  * :kconfig:option:`CONFIG_ARCH_HAS_CUSTOM_CURRENT_IMPL`
  * :kconfig:option:`CONFIG_RISCV_CURRENT_VIA_GP`

* Bluetooth

  * Audio

    * :c:func:`bt_bap_broadcast_source_register_cb`
    * :c:func:`bt_bap_broadcast_source_unregister_cb`
    * :c:func:`bt_cap_commander_distribute_broadcast_code`
    * ``bt_ccp`` API (in progress)
    * :c:func:`bt_pacs_register`
    * :c:func:`bt_pacs_unregister`

  * Host

    * :c:func:`bt_conn_is_type`

  * Mesh

    * :c:member:`bt_mesh_health_cli::update` callback can be used to periodically update the message
      published by the Health Client.

  * Services

    * The :kconfig:option:`CONFIG_BT_DIS_MODEL_NUMBER` and
      :kconfig:option:`CONFIG_BT_DIS_MANUF_NAME` Kconfig options can be used to control the
      presence of the Model Number String and Manufacturer Name String characteristics inside
      the Device Information Service (DIS). The :kconfig:option:`CONFIG_BT_DIS_MODEL_NUMBER_STR`
      and :kconfig:option:`CONFIG_BT_DIS_MANUF_NAME_STR` Kconfig options are now used to set the
      string values in these characteristics. They replace the functionality of the deprecated
      :kconfig:option:`CONFIG_BT_DIS_MODEL` and :kconfig:option:`CONFIG_BT_DIS_MANUF` Kconfigs.

* Build system

  * Sysbuild

    * The newly introduced MCUboot swap using offset mode can be selected from sysbuild by using
      ``SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET``, this mode is experimental.

* Crypto

  * :kconfig:option:`CONFIG_MBEDTLS_PSA_STATIC_KEY_SLOTS`
  * :kconfig:option:`CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT`

* I3C

  * :kconfig:option:`CONFIG_I3C_TARGET_BUFFER_MODE`
  * :kconfig:option:`CONFIG_I3C_RTIO`
  * :c:func:`i3c_ibi_hj_response`
  * :c:func:`i3c_ccc_do_getacccr`
  * :c:func:`i3c_device_controller_handoff`

* Management

  * hawkBit

    * The hawkBit subsystem now uses the State Machine Framework internally.
    * :kconfig:option:`CONFIG_HAWKBIT_TENANT`
    * :kconfig:option:`CONFIG_HAWKBIT_EVENT_CALLBACKS`
    * :kconfig:option:`CONFIG_HAWKBIT_SAVE_PROGRESS`

  * MCUmgr

    * Image management :c:macro:`MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED` now has image data field
      :c:struct:`img_mgmt_image_confirmed`.

* MCUboot

  * Signed hex files where an encryption key Kconfig is set now have the encrypted flag set in
    the image header.

* Networking:

  * CoAP

    * :c:func:`coap_client_cancel_request`

  * DHCP

    * :kconfig:option:`CONFIG_NET_DHCPV4_SERVER_OPTION_ROUTER`
    * :kconfig:option:`CONFIG_NET_DHCPV4_OPTION_DNS_ADDRESS`
    * :kconfig:option:`CONFIG_NET_DHCPV6_OPTION_DNS_ADDRESS`

  * DNS

    * :kconfig:option:`CONFIG_MDNS_RESPONDER_PROBE`

  * Ethernet

    * Allow user to specify protocol extensions when receiving data from Ethernet network.
      This makes it possible to register a handler for Ethernet protocol type without changing
      core Zephyr network code. :c:macro:`NET_L3_REGISTER`
    * :kconfig:option:`CONFIG_NET_L2_ETHERNET_RESERVE_HEADER`

  * HTTP

    * Extended :c:macro:`HTTP_SERVICE_DEFINE` to allow to specify a default
      fallback resource handler.
    * :kconfig:option:`CONFIG_HTTP_SERVER_REPORT_FAILURE_REASON`
    * :kconfig:option:`CONFIG_HTTP_SERVER_TLS_USE_ALPN`

  * IPv4

    * :kconfig:option:`CONFIG_NET_IPV4_PMTU`

  * IPv6

    * :kconfig:option:`CONFIG_NET_IPV6_PMTU`

  * LwM2M

    * :c:func:`lwm2m_pull_context_set_sockopt_callback`

  * MQTT-SN

    * Added Gateway Advertisement and Discovery support:

      * :c:func:`mqtt_sn_add_gw`
      * :c:func:`mqtt_sn_search`

  * OpenThread

    * :kconfig:option:`CONFIG_OPENTHREAD_WAKEUP_COORDINATOR`
    * :kconfig:option:`CONFIG_OPENTHREAD_WAKEUP_END_DEVICE`
    * :kconfig:option:`CONFIG_OPENTHREAD_PLATFORM_MESSAGE_MANAGEMENT`
    * :kconfig:option:`CONFIG_OPENTHREAD_TCAT_MULTIRADIO_CAPABILITIES`

  * Sockets

    * Added support for new socket options:

      * :c:macro:`IP_LOCAL_PORT_RANGE`
      * :c:macro:`IP_MULTICAST_IF`
      * :c:macro:`IPV6_MULTICAST_IF`
      * :c:macro:`IP_MTU`
      * :c:macro:`IPV6_MTU`

  * Other

    * :kconfig:option:`CONFIG_NET_STATISTICS_VIA_PROMETHEUS`

* Video

  * :c:func:`video_set_stream()` driver API has replaced :c:func:`video_stream_start()` and
    :c:func:`video_stream_stop()` driver APIs.

* Other

  * :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA`
  * :c:macro:`DT_ANY_INST_HAS_BOOL_STATUS_OKAY`
  * :c:struct:`led_dt_spec`
  * :kconfig:option:`CONFIG_STEP_DIR_STEPPER`

.. _boards_added_in_zephyr_4_1:

New Boards
**********
..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

* Adafruit Industries, LLC

   * :zephyr:board:`adafruit_feather_m4_express` (``adafruit_feather_m4_express``)
   * :zephyr:board:`adafruit_macropad_rp2040` (``adafruit_macropad_rp2040``)
   * :zephyr:board:`adafruit_qt_py_esp32s3` (``adafruit_qt_py_esp32s3``)

* Advanced Micro Devices (AMD), Inc.

   * :zephyr:board:`acp_6_0_adsp` (``acp_6_0_adsp``)

* Analog Devices, Inc.

   * :zephyr:board:`ad_swiot1l_sl` (``ad_swiot1l_sl``)
   * :zephyr:board:`max32650evkit` (``max32650evkit``)
   * :zephyr:board:`max32650fthr` (``max32650fthr``)
   * :zephyr:board:`max32660evsys` (``max32660evsys``)
   * :zephyr:board:`max78000evkit` (``max78000evkit``)
   * :zephyr:board:`max78000fthr` (``max78000fthr``)
   * :zephyr:board:`max78002evkit` (``max78002evkit``)

* Antmicro

   * :zephyr:board:`myra_sip_baseboard` (``myra_sip_baseboard``)

* BeagleBoard.org Foundation

   * :zephyr:board:`beagley_ai` (``beagley_ai``)

* FANKE Technology Co., Ltd.

   * :zephyr:board:`fk750m1_vbt6` (``fk750m1_vbt6``)

* Google, Inc.

   * :zephyr:board:`google_icetower` (``google_icetower``)
   * :zephyr:board:`google_quincy` (``google_quincy``)

* Infineon Technologies

   * :zephyr:board:`cy8ckit_062s2_ai` (``cy8ckit_062s2_ai``)

* Khadas

   * :zephyr:board:`khadas_edge2` (``khadas_edge2``)

* Lilygo Shenzhen Xinyuan Electronic Technology Co., Ltd

   * :zephyr:board:`ttgo_t7v1_5` (``ttgo_t7v1_5``)
   * :zephyr:board:`ttgo_t8s3` (``ttgo_t8s3``)

* M5Stack

   * :zephyr:board:`m5stack_cores3` (``m5stack_cores3``)

* Makerbase Co., Ltd.

   * :zephyr:board:`mks_canable_v20` (``mks_canable_v20``)

* MediaTek Inc.

   * MT8186 (``mt8186``)
   * MT8188 (``mt8188``)
   * MT8196 (``mt8196``)

* NXP Semiconductors

   * :zephyr:board:`frdm_mcxw72` (``frdm_mcxw72``)
   * :zephyr:board:`imx91_evk` (``imx91_evk``)
   * :zephyr:board:`mcxw72_evk` (``mcxw72_evk``)
   * :zephyr:board:`mimxrt700_evk` (``mimxrt700_evk``)

* Nordic Semiconductor

   * :zephyr:board:`nrf54l09pdk` (``nrf54l09pdk``)

* Norik Systems

   * :zephyr:board:`octopus_io_board` (``octopus_io_board``)
   * :zephyr:board:`octopus_som` (``octopus_som``)

* Panasonic Corporation

   * :zephyr:board:`panb511evb` (``panb511evb``)

* Peregrine Consultoria e Servicos

   * :zephyr:board:`sam4l_wm400_cape` (``sam4l_wm400_cape``)

* Qorvo, Inc.

   * :zephyr:board:`decawave_dwm3001cdk` (``decawave_dwm3001cdk``)

* RAKwireless Technology Limited

   * :zephyr:board:`rak3172` (``rak3172``)

* Raspberry Pi Foundation

   * :zephyr:board:`rpi_pico2` (``rpi_pico2``)

* Realtek Semiconductor Corp.

   * :zephyr:board:`rts5912_evb` (``rts5912_evb``)

* Renesas Electronics Corporation

   * :zephyr:board:`ek_ra2l1` (``ek_ra2l1``)
   * :zephyr:board:`ek_ra4l1` (``ek_ra4l1``)
   * :zephyr:board:`ek_ra4m1` (``ek_ra4m1``)
   * :zephyr:board:`fpb_ra4e1` (``fpb_ra4e1``)
   * :zephyr:board:`rzg3s_smarc` (``rzg3s_smarc``)
   * :zephyr:board:`voice_ra4e1` (``voice_ra4e1``)

* STMicroelectronics

   * :zephyr:board:`nucleo_c071rb` (``nucleo_c071rb``)
   * :zephyr:board:`nucleo_f072rb` (``nucleo_f072rb``)
   * :zephyr:board:`nucleo_h7s3l8` (``nucleo_h7s3l8``)
   * :zephyr:board:`nucleo_n657x0_q` (``nucleo_n657x0_q``)
   * :zephyr:board:`nucleo_wb07cc` (``nucleo_wb07cc``)
   * :zephyr:board:`stm32f413h_disco` (``stm32f413h_disco``)
   * :zephyr:board:`stm32n6570_dk` (``stm32n6570_dk``)

* Seeed Technology Co., Ltd

   * :zephyr:board:`xiao_esp32c6` (``xiao_esp32c6``)

* Shenzhen Fuyuansheng Electronic Technology Co., Ltd.

   * :zephyr:board:`ucan` (``ucan``)

* Silicon Laboratories

   * :zephyr:board:`siwx917_rb4338a` (``siwx917_rb4338a``)
   * :zephyr:board:`xg23_rb4210a` (``xg23_rb4210a``)
   * :zephyr:board:`xg24_ek2703a` (``xg24_ek2703a``)
   * :zephyr:board:`xg29_rb4412a` (``xg29_rb4412a``)

* Texas Instruments

   * :zephyr:board:`lp_em_cc2340r5` (``lp_em_cc2340r5``)

* Toradex AG

   * :zephyr:board:`verdin_imx8mm` (``verdin_imx8mm``)

* Waveshare Electronics

   * :zephyr:board:`rp2040_zero` (``rp2040_zero``)

* WeAct Studio

   * :zephyr:board:`mini_stm32h7b0` (``mini_stm32h7b0``)
   * :zephyr:board:`weact_stm32h5_core` (``weact_stm32h5_core``)

* WinChipHead

   * :zephyr:board:`ch32v003evt` (``ch32v003evt``)

* Würth Elektronik GmbH.

   * :zephyr:board:`we_oceanus1ev` (``we_oceanus1ev``)
   * :zephyr:board:`we_orthosie1ev` (``we_orthosie1ev``)

* Others

   * :zephyr:board:`canbardo` (``canbardo``)
   * :zephyr:board:`candlelight` (``candlelight``)
   * :zephyr:board:`candlelightfd` (``candlelightfd``)
   * :zephyr:board:`esp32c3_supermini` (``esp32c3_supermini``)
   * :zephyr:board:`promicro_nrf52840` (``promicro_nrf52840``)

.. _shields_added_in_zephyr_4_1:

New shields
============

  * :ref:`Abrobot ESP32 C3 OLED Shield <abrobot_esp32c3oled_shield>`
  * :ref:`Adafruit Adalogger Featherwing Shield <adafruit_adalogger_featherwing_shield>`
  * :ref:`Adafruit AW9523 GPIO Expander and LED Driver <adafruit_aw9523>`
  * :ref:`MikroElektronika ETH 3 Click <mikroe_eth3_click>`
  * :ref:`P3T1755DP Arduino® Shield Evaluation Board <p3t1755dp_ard_i2c_shield>`
  * :ref:`P3T1755DP Arduino® Shield Evaluation Board <p3t1755dp_ard_i3c_shield>`
  * :ref:`Digilent Pmod SD <pmod_sd>`
  * :ref:`Renesas DA14531 Pmod Board <renesas_us159_da14531evz_shield>`
  * :ref:`RTKMIPILCDB00000BE MIPI Display <rtkmipilcdb00000be>`
  * :ref:`Seeed W5500 Ethernet Shield <seeed_w5500>`
  * :ref:`ST B-CAMS-OMV-MB1683 <st_b_cams_omv_mb1683>`

New Drivers
***********
..
  Same as above for boards, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* :abbr:`ADC (Analog to Digital Converter)`

   * :dtcompatible:`adi,ad4114-adc`
   * :dtcompatible:`adi,ad7124-adc`
   * :dtcompatible:`st,stm32n6-adc`
   * :dtcompatible:`ti,ads114s06`
   * :dtcompatible:`ti,ads124s06`
   * :dtcompatible:`ti,ads124s08`
   * :dtcompatible:`ti,ads131m02`
   * :dtcompatible:`ti,tla2022`
   * :dtcompatible:`ti,tla2024`

* ARM architecture

   * :dtcompatible:`nxp,nbu`

* Audio

   * :dtcompatible:`cirrus,cs43l22`
   * :dtcompatible:`intel,adsp-mic-privacy`

* Bluetooth

   * :dtcompatible:`renesas,bt-hci-da1453x`
   * :dtcompatible:`silabs,siwx91x-bt-hci`
   * :dtcompatible:`st,hci-stm32wb0`

* Charger

   * :dtcompatible:`nxp,pf1550-charger`

* Clock control

   * :dtcompatible:`atmel,sam0-gclk`
   * :dtcompatible:`atmel,sam0-mclk`
   * :dtcompatible:`atmel,sam0-osc32kctrl`
   * :dtcompatible:`nordic,nrf-hsfll-global`
   * :dtcompatible:`nuvoton,npcm-pcc`
   * :dtcompatible:`realtek,rts5912-sccon`
   * :dtcompatible:`renesas,rz-cpg`
   * :dtcompatible:`st,stm32n6-cpu-clock-mux`
   * :dtcompatible:`st,stm32n6-hse-clock`
   * :dtcompatible:`st,stm32n6-ic-clock-mux`
   * :dtcompatible:`st,stm32n6-pll-clock`
   * :dtcompatible:`st,stm32n6-rcc`
   * :dtcompatible:`wch,ch32v00x-hse-clock`
   * :dtcompatible:`wch,ch32v00x-hsi-clock`
   * :dtcompatible:`wch,ch32v00x-pll-clock`
   * :dtcompatible:`wch,rcc`

* Comparator

   * :dtcompatible:`silabs,acmp`

* Counter

   * :dtcompatible:`adi,max32-rtc-counter`
   * :dtcompatible:`renesas,rz-gtm-counter`

* CPU

   * :dtcompatible:`wch,qingke-v2`

* :abbr:`DAC (Digital to Analog Converter)`

   * :dtcompatible:`adi,max22017-dac`
   * :dtcompatible:`renesas,ra-dac`
   * :dtcompatible:`renesas,ra-dac-global`

* :abbr:`DAI (Digital Audio Interface)`

   * :dtcompatible:`mediatek,afe`
   * :dtcompatible:`nxp,dai-micfil`

* Display

   * :dtcompatible:`ilitek,ili9806e-dsi`
   * :dtcompatible:`renesas,ra-glcdc`
   * :dtcompatible:`solomon,ssd1309fb`

* :abbr:`DMA (Direct Memory Access)`

   * :dtcompatible:`infineon,cat1-dma`
   * :dtcompatible:`nxp,sdma`
   * :dtcompatible:`silabs,ldma`
   * :dtcompatible:`silabs,siwx91x-dma`
   * :dtcompatible:`xlnx,axi-dma-1.00.a`
   * :dtcompatible:`xlnx,eth-dma`

* :abbr:`DSA (Distributed Switch Architecture)`

   * :dtcompatible:`nxp,netc-switch`

* :abbr:`EEPROM (Electrically Erasable Programmable Read-Only Memory)`

  *  :dtcompatible:`fujitsu,mb85rsxx`

* Ethernet

   * :dtcompatible:`davicom,dm8806-phy`
   * :dtcompatible:`microchip,lan9250`
   * :dtcompatible:`microchip,t1s-phy`
   * :dtcompatible:`microchip,vsc8541`
   * :dtcompatible:`renesas,ra-ethernet`
   * :dtcompatible:`sensry,sy1xx-mac`

* Firmware

   * :dtcompatible:`arm,scmi-power`

* Flash controller

   * :dtcompatible:`silabs,siwx91x-flash-controller`
   * :dtcompatible:`ti,cc23x0-flash-controller`

* :abbr:`FPGA (Field Programmable Gate Array)`

   * :dtcompatible:`lattice,ice40-fpga-base`
   * :dtcompatible:`lattice,ice40-fpga-bitbang`

* :abbr:`GPIO (General Purpose Input/Output)`

   * :dtcompatible:`adi,max22017-gpio`
   * :dtcompatible:`adi,max22190-gpio`
   * :dtcompatible:`awinic,aw9523b-gpio`
   * :dtcompatible:`ite,it8801-gpio`
   * :dtcompatible:`microchip,mec5-gpio`
   * :dtcompatible:`nordic,npm2100-gpio`
   * :dtcompatible:`nxp,pca6416`
   * :dtcompatible:`raspberrypi,rp1-gpio`
   * :dtcompatible:`realtek,rts5912-gpio`
   * :dtcompatible:`renesas,ra-gpio-mipi-header`
   * :dtcompatible:`renesas,rz-gpio`
   * :dtcompatible:`renesas,rz-gpio-int`
   * :dtcompatible:`sensry,sy1xx-gpio`
   * :dtcompatible:`silabs,siwx91x-gpio`
   * :dtcompatible:`silabs,siwx91x-gpio-port`
   * :dtcompatible:`silabs,siwx91x-gpio-uulp`
   * :dtcompatible:`st,dcmi-camera-fpu-330zh`
   * :dtcompatible:`st,mfxstm32l152`
   * :dtcompatible:`stemma-qt-connector`
   * :dtcompatible:`ti,cc23x0-gpio`
   * :dtcompatible:`wch,gpio`

* IEEE 802.15.4 HDLC RCP interface

   * :dtcompatible:`nxp,hdlc-rcp-if`
   * :dtcompatible:`uart,hdlc-rcp-if`

* :abbr:`I2C (Inter-Integrated Circuit)`

   * :dtcompatible:`nordic,nrf-twis`
   * :dtcompatible:`nxp,ii2c`
   * :dtcompatible:`ti,omap-i2c`
   * :dtcompatible:`ti,tca9544a`

* :abbr:`I3C (Improved Inter-Integrated Circuit)`

   * :dtcompatible:`snps,designware-i3c`
   * :dtcompatible:`st,stm32-i3c`

* IEEE 802.15.4

   * :dtcompatible:`nxp,mcxw-ieee802154`

* Input

   * :dtcompatible:`cypress,cy8cmbr3xxx`
   * :dtcompatible:`ite,it8801-kbd`
   * :dtcompatible:`microchip,cap12xx`
   * :dtcompatible:`nintendo,nunchuk`

* Interrupt controller

   * :dtcompatible:`renesas,rz-ext-irq`
   * :dtcompatible:`wch,pfic`

* Mailbox

   * :dtcompatible:`linaro,ivshmem-mbox`
   * :dtcompatible:`ti,omap-mailbox`

* :abbr:`MDIO (Management Data Input/Output)`

   * :dtcompatible:`microchip,lan865x-mdio`
   * :dtcompatible:`renesas,ra-mdio`
   * :dtcompatible:`sensry,sy1xx-mdio`

* Memory controller

   * :dtcompatible:`renesas,ra-sdram`

* :abbr:`MFD (Multi-Function Device)`

   * :dtcompatible:`adi,max22017`
   * :dtcompatible:`awinic,aw9523b`
   * :dtcompatible:`ite,it8801-altctrl`
   * :dtcompatible:`ite,it8801-mfd`
   * :dtcompatible:`ite,it8801-mfd-map`
   * :dtcompatible:`maxim,ds3231-mfd`
   * :dtcompatible:`nordic,npm2100`
   * :dtcompatible:`nxp,pf1550`

* :abbr:`MIPI DSI (Mobile Industry Processor Interface Display Serial Interface)`

   * :dtcompatible:`renesas,ra-mipi-dsi`

* Miscellaneous

   * :dtcompatible:`nordic,nrf-bicr`
   * :dtcompatible:`nordic,nrf-ppib`
   * :dtcompatible:`renesas,ra-external-interrupt`

* :abbr:`MMU / MPU (Memory Management Unit / Memory Protection Unit)`

   * :dtcompatible:`nxp,sysmpu`

* :abbr:`MTD (Memory Technology Device)`

   * :dtcompatible:`nxp,s32-qspi-hyperflash`
   * :dtcompatible:`nxp,xspi-mx25um51345g`
   * :dtcompatible:`ti,cc23x0-ccfg-flash`

* Networking

   * :dtcompatible:`silabs,series2-radio`

* :abbr:`PCIe (Peripheral Component Interconnect Express)`

   * :dtcompatible:`brcm,brcmstb-pcie`

* PHY

   * :dtcompatible:`renesas,ra-usbphyc`
   * :dtcompatible:`st,stm32u5-otghs-phy`

* Pin control

   * :dtcompatible:`realtek,rts5912-pinctrl`
   * :dtcompatible:`renesas,rzg-pinctrl`
   * :dtcompatible:`sensry,sy1xx-pinctrl`
   * :dtcompatible:`silabs,dbus-pinctrl`
   * :dtcompatible:`silabs,siwx91x-pinctrl`
   * :dtcompatible:`ti,cc23x0-pinctrl`
   * :dtcompatible:`wch,afio`

* :abbr:`PWM (Pulse Width Modulation)`

   * :dtcompatible:`atmel,sam0-tc-pwm`
   * :dtcompatible:`ite,it8801-pwm`
   * :dtcompatible:`renesas,rz-gpt-pwm`
   * :dtcompatible:`zephyr,fake-pwm`

* Quad SPI

   * :dtcompatible:`nxp,s32-qspi-sfp-frad`
   * :dtcompatible:`nxp,s32-qspi-sfp-mdad`

* Regulator

   * :dtcompatible:`nordic,npm2100-regulator`
   * :dtcompatible:`nxp,pf1550-regulator`

* :abbr:`RNG (Random Number Generator)`

   * :dtcompatible:`nordic,nrf-cracen-ctrdrbg`
   * :dtcompatible:`nxp,ele-trng`
   * :dtcompatible:`renesas,ra-sce5-rng`
   * :dtcompatible:`renesas,ra-sce7-rng`
   * :dtcompatible:`renesas,ra-sce9-rng`
   * :dtcompatible:`renesas,ra-trng`
   * :dtcompatible:`sensry,sy1xx-trng`
   * :dtcompatible:`silabs,siwx91x-rng`
   * :dtcompatible:`st,stm32-rng-noirq`

* :abbr:`RTC (Real Time Clock)`

   * :dtcompatible:`epson,rx8130ce-rtc`
   * :dtcompatible:`maxim,ds1337`
   * :dtcompatible:`maxim,ds3231-rtc`
   * :dtcompatible:`microcrystal,rv8803`
   * :dtcompatible:`ti,bq32002`

* SDHC

   * :dtcompatible:`renesas,ra-sdhc`

* Sensors

   * :dtcompatible:`adi,adxl366`
   * :dtcompatible:`hc-sr04`
   * :dtcompatible:`invensense,icm42370p`
   * :dtcompatible:`invensense,icm42670s`
   * :dtcompatible:`invensense,icp101xx`
   * :dtcompatible:`maxim,ds3231-sensor`
   * :dtcompatible:`melexis,mlx90394`
   * :dtcompatible:`nordic,npm2100-vbat`
   * :dtcompatible:`phosense,xbr818`
   * :dtcompatible:`renesas,hs400x`
   * :dtcompatible:`sensirion,scd40`
   * :dtcompatible:`sensirion,scd41`
   * :dtcompatible:`sensirion,sts4x`
   * :dtcompatible:`st,lis2duxs12`
   * :dtcompatible:`st,lsm6dsv16x`
   * :dtcompatible:`ti,tmag3001`
   * :dtcompatible:`ti,tmp435`
   * :dtcompatible:`we,wsen-pads-2511020213301`
   * :dtcompatible:`we,wsen-pdus-25131308XXXXX`
   * :dtcompatible:`we,wsen-tids-2521020222501`

* Serial controller

   * :dtcompatible:`microchip,mec5-uart`
   * :dtcompatible:`realtek,rts5912-uart`
   * :dtcompatible:`renesas,rz-scif-uart`
   * :dtcompatible:`silabs,eusart-uart`
   * :dtcompatible:`silabs,usart-uart`
   * :dtcompatible:`ti,cc23x0-uart`
   * :dtcompatible:`wch,usart`

* :abbr:`SPI (Serial Peripheral Interface)`

   * :dtcompatible:`ite,it8xxx2-spi`
   * :dtcompatible:`nxp,lpspi`
   * :dtcompatible:`nxp,xspi`
   * :dtcompatible:`renesas,ra-spi`

* Stepper

   * :dtcompatible:`adi,tmc2209`
   * :dtcompatible:`ti,drv8424`

* :abbr:`TCPC (USB Type-C Port Controller)`

   * :dtcompatible:`richtek,rt1715`

* Timer

   * :dtcompatible:`mediatek,ostimer64`
   * :dtcompatible:`realtek,rts5912-rtmr`
   * :dtcompatible:`realtek,rts5912-slwtimer`
   * :dtcompatible:`renesas,rz-gpt`
   * :dtcompatible:`renesas,rz-gtm`
   * :dtcompatible:`riscv,machine-timer`
   * :dtcompatible:`ti,cc23x0-systim-timer`
   * :dtcompatible:`wch,systick`

* USB

   * :dtcompatible:`ambiq,usb`
   * :dtcompatible:`renesas,ra-udc`
   * :dtcompatible:`renesas,ra-usbfs`
   * :dtcompatible:`renesas,ra-usbhs`
   * :dtcompatible:`zephyr,midi2-device`

* Video

   * :dtcompatible:`zephyr,video-emul-imager`
   * :dtcompatible:`zephyr,video-emul-rx`

* Watchdog

   * :dtcompatible:`atmel,sam4l-watchdog`
   * :dtcompatible:`nordic,npm2100-wdt`
   * :dtcompatible:`nxp,rtwdog`

* Wi-Fi

   * :dtcompatible:`infineon,airoc-wifi`
   * :dtcompatible:`silabs,siwx91x-wifi`

New Samples
***********

..
  Same as above for boards and drivers, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

* :zephyr:code-sample:`6dof_motion_drdy`
* :zephyr:code-sample:`ble_cs`
* :zephyr:code-sample:`bluetooth_ccp_call_control_client`
* :zephyr:code-sample:`bluetooth_ccp_call_control_server`
* :zephyr:code-sample:`coresight_stm_sample`
* :zephyr:code-sample:`dfu-next`
* :zephyr:code-sample:`i2c-rtio-loopback`
* :zephyr:code-sample:`lvgl-screen-transparency`
* :zephyr:code-sample:`mctp_endpoint_sample`
* :zephyr:code-sample:`mctp_host_sample`
* :zephyr:code-sample:`openthread-shell`
* :zephyr:code-sample:`ot-coap`
* :zephyr:code-sample:`rtc`
* :zephyr:code-sample:`sensor_batch_processing`
* :zephyr:code-sample:`sensor_clock`
* :zephyr:code-sample:`stream_fifo`
* :zephyr:code-sample:`tdk_apex`
* :zephyr:code-sample:`tmc50xx`
* :zephyr:code-sample:`uart`
* :zephyr:code-sample:`usb-midi2-device`
* :zephyr:code-sample:`usbd-cdc-acm-console`
* :zephyr:code-sample:`webusb-next`

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?

* A header file has been introduced to allocate ID ranges for persistent keys in the PSA Crypto API.
  It defines the ID ranges allocated to different users of the API (application, subsystems...).
  Users of the API must now use this header file to construct persistent key IDs.
  See :zephyr_file:`include/zephyr/psa/key_ids.h` for more information. (:github:`85581`)

* Space-separated lists support has been removed from Twister configuration
  files. This feature was deprecated a long time ago. Projects that do still use
  them can use the :zephyr_file:`scripts/utils/twister_to_list.py` script to
  automatically migrate Twister configuration files.

* Test case names for Ztest now include the Ztest suite name, meaning the resulting identifier has
  three sections and looks like: ``<test_scenario_name>.<ztest_suite_name>.<ztest_name>``.
  These extended identifiers are used in log output, twister.json and testplan.json,
  as well as for ``--sub-test`` command line parameters.

* The ``--no-detailed-test-id`` command line option can be used to shorten the test case name
  by excluding the test scenario name prefix which is the same as the parent test suite id.

* Added support for HTTP PUT/PATCH/DELETE methods for HTTP server dynamic resources.

* Driver API structures are now available through iterable sections and a new
  :c:macro:`DEVICE_API_IS` macro has been introduced to allow to check if a device supports a
  given API. Many shell commands now use this to provide "smarter" auto-completion and only list
  compatible devices when they expect a device argument.

* Zephyr's :ref:`interactive board catalog <boards>` has been extended to allow searching for boards
  based on supported hardware features. A new :rst:dir:`zephyr:board-supported-hw` Sphinx directive
  can now be used in boards' documentation pages to automatically include a list of the hardware
  features supported by a board, and many boards have already adopted this new feature in their
  documentation.
