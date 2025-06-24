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

Zephyr 4.2.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.2.0.

Major enhancements with this release include:

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

* :cve:`2025-2962` Under embargo until 2025-06-07

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

Removed APIs and options
========================

* Removed the deprecated the ``net_buf_put()`` and ``net_buf_get()`` API functions.

* Removed the deprecated ``include/zephyr/net/buf.h`` header file.

* Removed the ``--disable-unrecognized-section-test`` Twister option. Test has been removed and the
  option became the default behavior.

* Removed the deprecated ``kscan`` subsystem.

* Removed :dtcompatible:`meas,ms5837` and replaced with :dtcompatible:`meas,ms5837-30ba`
  and :dtcompatible:`meas,ms5837-02ba`.

* Removed the ``get_ctrl`` video driver API

Deprecated APIs and options
===========================

* The scheduler Kconfig options CONFIG_SCHED_DUMB and CONFIG_WAITQ_DUMB were
  renamed and deprecated. Use :kconfig:option:`CONFIG_SCHED_SIMPLE` and
  :kconfig:option:`CONFIG_WAITQ_SIMPLE` instead.

* The :kconfig:option:`CONFIG_LWM2M_ENGINE_MESSAGE_HEADER_SIZE` Kconfig option has been removed.
  The required header size should be included in the message size, configured using
  :kconfig:option:`CONFIG_LWM2M_COAP_MAX_MSG_SIZE`. Special care should be taken to ensure that
  used CoAP block size :kconfig:option:`CONFIG_LWM2M_COAP_BLOCK_SIZE` can fit given message size
  with headers. Previous headroom was 48 bytes.

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

New APIs and options
====================

* Architectures

  * NIOS2 Architecture was removed from Zephyr.
  * :kconfig:option:`ARCH_HAS_VECTOR_TABLE_RELOCATION`
  * :kconfig:option:`CONFIG_SRAM_VECTOR_TABLE` moved from ``zephyr/Kconfig.zephyr`` to
    ``zephyr/arch/Kconfig`` and added dependencies to it.

* Kernel

 * :c:macro:`K_TIMEOUT_ABS_SEC`

* I2C

  * :c:func:`i2c_configure_dt`.

..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

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

* Display

  * :c:func:`display_clear`

* Networking:

  * IPv4

    * :kconfig:option:`CONFIG_NET_IPV4_MTU`

  * MQTT

    * :kconfig:option:`CONFIG_MQTT_VERSION_5_0`

  * Sockets

    * :kconfig:option:`CONFIG_NET_SOCKETS_INET_RAW`

  * OpenThread

    * Moved OpenThread-related Kconfig options from :zephyr_file:`subsys/net/l2/openthread/Kconfig`
      to :zephyr_file:`modules/openthread/Kconfig`.
    * Refactored OpenThread networking API, see the OpenThread section of the
      :ref:`migration guide <migration_4.2>`.
    * :kconfig:option:`CONFIG_OPENTHREAD_SYS_INIT`
    * :kconfig:option:`CONFIG_OPENTHREAD_SYS_INIT_PRIORITY`

  * zperf

    * :kconfig:option:`CONFIG_ZPERF_SESSION_PER_THREAD`
    * :c:member:`zperf_upload_params.data_loader`

* Sensor

  * :c:func:`sensor_value_to_deci`
  * :c:func:`sensor_value_to_centi`

* Stepper

  * :c:func:`stepper_stop()`

* Storage

  * :c:func:`flash_area_copy()`

* Counter

  * :c:func:`counter_reset`

* Sys

  * :c:func:`util_eq`
  * :c:func:`util_memeq`

* LoRaWAN
   * :c:func:`lorawan_request_link_check`

* Video

  * :c:func:`video_api_ctrl_t`
  * :c:func:`video_query_ctrl`
  * :c:func:`video_print_ctrl`

* PCIe

   * :kconfig:option:`CONFIG_NVME_PRP_PAGE_SIZE`

* Debug

  * Core Dump

    * :kconfig:option:`CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP`, enabled by default for ARM Cortex M when :kconfig:option:`CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_MIN` is selected.
    * :kconfig:option:`CONFIG_DEBUG_COREDUMP_BACKEND_IN_MEMORY`
    * :kconfig:option:`CONFIG_DEBUG_COREDUMP_BACKEND_IN_MEMORY_SIZE`

* Other

  * :kconfig:option:`CONFIG_LV_Z_COLOR_MONO_HW_INVERSION`

* ZBus

  * Runtime observers can work without heap. Now it is possible to choose between static, dynamic,
    and none allocation for the runtime observers nodes.
  * Runtime observers using :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_NONE` must use
    the new function :c:func:`zbus_chan_add_obs_with_node`.

  * :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_DYNAMIC`
  * :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_STATIC`
  * :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_NONE`
  * :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_POOL_SIZE`


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

* Advanced Micro Devices (AMD), Inc.

   * :zephyr:board:`versalnet_rpu` (``versalnet_rpu``)

* Aesc Silicon

   * :zephyr:board:`elemrv` (``elemrv``)

* aithinker

   * :zephyr:board:`ai_wb2_12f` (``ai_wb2_12f``)

* Ambiq Micro, Inc.

   * :zephyr:board:`apollo510_evb` (``apollo510_evb``)

* Analog Devices, Inc.

   * :zephyr:board:`max32657evkit` (``max32657evkit``)

* BeagleBoard.org Foundation

   * :zephyr:board:`pocketbeagle_2` (``pocketbeagle_2``)

* Blues Wireless

   * :zephyr:board:`cygnet` (``cygnet``)

* Bouffalo Lab (Nanjing) Co., Ltd.

   * :zephyr:board:`bl604e_iot_dvk` (``bl604e_iot_dvk``)

* Espressif Systems

   * :zephyr:board:`esp32_devkitc` (``esp32_devkitc``)

* Ezurio

   * :zephyr:board:`bl54l15_dvk` (``bl54l15_dvk``)
   * :zephyr:board:`bl54l15u_dvk` (``bl54l15u_dvk``)

* FANKE Technology Co., Ltd.

   * :zephyr:board:`fk743m5_xih6` (``fk743m5_xih6``)

* IAR Systems AB

   * :zephyr:board:`stm32f429ii_aca` (``stm32f429ii_aca``)

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

* Microchip Technology Inc.

   * :zephyr:board:`mec_assy6941` (``mec_assy6941``)

* Nuvoton Technology Corporation

   * :zephyr:board:`numaker_m55m1` (``numaker_m55m1``)

* NXP Semiconductors

   * :zephyr:board:`frdm_mcxa153` (``frdm_mcxa153``)
   * :zephyr:board:`frdm_mcxa166` (``frdm_mcxa166``)
   * :zephyr:board:`frdm_mcxa276` (``frdm_mcxa276``)
   * :zephyr:board:`mcx_n9xx_evk` (``mcx_n9xx_evk``)

* Octavo Systems LLC

   * :zephyr:board:`osd32mp1_brk` (``osd32mp1_brk``)

* OpenHW Group

   * :zephyr:board:`cv32a6_genesys_2` (``cv32a6_genesys_2``)
   * :zephyr:board:`cv64a6_genesys_2` (``cv64a6_genesys_2``)

* Pimoroni Ltd.

   * :zephyr:board:`pico_plus2` (``pico_plus2``)

* QEMU

   * :zephyr:board:`qemu_rx` (``qemu_rx``)

* Renesas Electronics Corporation

   * :zephyr:board:`rsk_rx130` (``rsk_rx130``)
   * :zephyr:board:`rza2m_evk` (``rza2m_evk``)
   * :zephyr:board:`rza3ul_smarc` (``rza3ul_smarc``)
   * :zephyr:board:`rzg2l_smarc` (``rzg2l_smarc``)
   * :zephyr:board:`rzg2lc_smarc` (``rzg2lc_smarc``)
   * :zephyr:board:`rzn2l_rsk` (``rzn2l_rsk``)
   * :zephyr:board:`rzt2l_rsk` (``rzt2l_rsk``)
   * :zephyr:board:`rzt2m_rsk` (``rzt2m_rsk``)
   * :zephyr:board:`rzv2l_smarc` (``rzv2l_smarc``)

* Seeed Technology Co., Ltd

   * :zephyr:board:`xiao_mg24` (``xiao_mg24``)
   * :zephyr:board:`xiao_ra4m1` (``xiao_ra4m1``)

* sensry.io

   * :zephyr:board:`ganymed_sk` (``ganymed_sk``)

* Shanghai Ruiside Electronic Technology Co., Ltd.

   * :zephyr:board:`art_pi2` (``art_pi2``)

* Silicon Laboratories

   * :zephyr:board:`slwrb4180b` (``slwrb4180b``)

* STMicroelectronics

   * :zephyr:board:`nucleo_f439zi` (``nucleo_f439zi``)
   * :zephyr:board:`nucleo_wba65ri` (``nucleo_wba65ri``)
   * :zephyr:board:`stm32h757i_eval` (``stm32h757i_eval``)
   * :zephyr:board:`stm32mp135f_dk` (``stm32mp135f_dk``)
   * :zephyr:board:`stm32u5g9j_dk2` (``stm32u5g9j_dk2``)

* Texas Instruments

   * :zephyr:board:`sk_am64` (``sk_am64``)

* Variscite Ltd.

   * :zephyr:board:`imx8mp_var_som` (``imx8mp_var_som``)

* WinChipHead

   * :zephyr:board:`ch32v003f4p6_dev_board` (``ch32v003f4p6_dev_board``)
   * :zephyr:board:`linkw` (``linkw``)

* WIZnet Co., Ltd.

   * :zephyr:board:`w5500_evb_pico2` (``w5500_evb_pico2``)

* Würth Elektronik GmbH.

   * :zephyr:board:`ophelia4ev` (``ophelia4ev``)

New Drivers
***********

..
  Same as above for boards, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* :abbr:`ADC (Analog to Digital Converter)`

   * :dtcompatible:`adi,ad4050-adc`
   * :dtcompatible:`adi,ad4052-adc`
   * :dtcompatible:`adi,ad4130-adc`
   * :dtcompatible:`ite,it51xxx-adc`
   * :dtcompatible:`realtek,rts5912-adc`
   * :dtcompatible:`renesas,rz-adc`

* Audio

   * :dtcompatible:`ti,tlv320aic3110`

* Charger

   * :dtcompatible:`ti,bq25713`
   * :dtcompatible:`x-powers,axp2101-charger`

* Clock control

   * :dtcompatible:`ite,it51xxx-ecpm`
   * :dtcompatible:`nordic,nrfs-audiopll`
   * :dtcompatible:`renesas,rx-cgc-pclk`
   * :dtcompatible:`renesas,rx-cgc-pclk-block`
   * :dtcompatible:`renesas,rx-cgc-pll`
   * :dtcompatible:`renesas,rx-cgc-root-clock`
   * :dtcompatible:`renesas,rza2m-cpg`
   * :dtcompatible:`st,stm32mp13-cpu-clock-mux`
   * :dtcompatible:`st,stm32mp13-pll-clock`
   * :dtcompatible:`wch,ch32v20x_30x-pll-clock`

* Comparator

   * :dtcompatible:`renesas,ra-acmphs`
   * :dtcompatible:`renesas,ra-acmphs-global`

* Counter

   * :dtcompatible:`ite,it8xxx2-counter`
   * :dtcompatible:`neorv32,gptmr`
   * :dtcompatible:`realtek,rts5912-timer`
   * :dtcompatible:`wch,gptm`
   * :dtcompatible:`zephyr,native-sim-counter`

* CPU

   * :dtcompatible:`intel,bartlett-lake`
   * :dtcompatible:`openhwgroup,cva6`
   * :dtcompatible:`renesas,rx`
   * :dtcompatible:`wch,qingke-v4c`
   * :dtcompatible:`zephyr,native-sim-cpu`

* Cryptographic accelerator

   * :dtcompatible:`ti,cc23x0-aes`

* :abbr:`DAC (Digital to Analog Converter)`

   * :dtcompatible:`ti,dac161s997`

* Display

   * :dtcompatible:`sinowealth,sh1122`
   * :dtcompatible:`sitronix,st75256`
   * :dtcompatible:`sitronix,st7567`
   * :dtcompatible:`sitronix,st7701`
   * :dtcompatible:`solomon,ssd1320`

* :abbr:`DMA (Direct Memory Access)`

   * :dtcompatible:`renesas,rz-dma`
   * :dtcompatible:`ti,cc23x0-dma`
   * :dtcompatible:`wch,wch-dma`

* Ethernet

   * :dtcompatible:`st,stm32-ethernet-controller`
   * :dtcompatible:`st,stm32n6-ethernet`
   * :dtcompatible:`ti,dp83867`
   * :dtcompatible:`xlnx,axi-ethernet-1.00.a`

* Firmware

   * :dtcompatible:`nordic,ironside-call`
   * :dtcompatible:`nxp,scmi-cpu`

* Flash controller

   * :dtcompatible:`renesas,rx-flash`
   * :dtcompatible:`silabs,series2-flash-controller`

* File system

   * :dtcompatible:`zephyr,fstab,fatfs`

* :abbr:`GPIO (General Purpose Input/Output)` and Headers

   * :dtcompatible:`adi,max14915-gpio`
   * :dtcompatible:`adi,max14917-gpio`
   * :dtcompatible:`adi,max22199-gpio`
   * :dtcompatible:`bflb,gpio`
   * :dtcompatible:`espressif,esp32-lpgpio`
   * :dtcompatible:`ite,it51xxx-gpio`
   * :dtcompatible:`nxp,lcd-pmod`
   * :dtcompatible:`raspberrypi,pico-gpio-port`
   * :dtcompatible:`renesas,ra-parallel-graphics-header`
   * :dtcompatible:`renesas,rx-gpio`
   * :dtcompatible:`renesas,rza2m-gpio`
   * :dtcompatible:`renesas,rza2m-gpio-int`

* :abbr:`I2C (Inter-Integrated Circuit)`

   * :dtcompatible:`cdns,i2c`
   * :dtcompatible:`ite,it51xxx-i2c`
   * :dtcompatible:`litex,litei2c`
   * :dtcompatible:`renesas,ra-i2c-sci-b`
   * :dtcompatible:`renesas,rz-riic`
   * :dtcompatible:`sensry,sy1xx-i2c`
   * :dtcompatible:`wch,i2c`

* :abbr:`I3C (Improved Inter-Integrated Circuit)`

   * :dtcompatible:`ite,it51xxx-i3cm`
   * :dtcompatible:`ite,it51xxx-i3cs`

* Input

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

* :abbr:`LED (Light Emitting Diode)`

   * :dtcompatible:`dac-leds`
   * :dtcompatible:`x-powers,axp192-led`
   * :dtcompatible:`x-powers,axp2101-led`

* Mailbox

   * :dtcompatible:`arm,mhuv3`
   * :dtcompatible:`renesas,rz-mhu-mbox`

* :abbr:`MDIO (Management Data Input/Output)`

   * :dtcompatible:`xlnx,axi-ethernet-1.00.a-mdio`

* Memory controller

   * :dtcompatible:`adi,max32-hpb`
   * :dtcompatible:`realtek,rts5912-bbram`
   * :dtcompatible:`st,stm32-xspi-psram`

* :abbr:`MFD (Multi-Function Device)`

   * :dtcompatible:`adi,maxq10xx`
   * :dtcompatible:`ambiq,iom`
   * :dtcompatible:`x-powers,axp2101`

* :abbr:`MIPI DBI (Mobile Industry Processor Interface Display Bus Interface)`

   * :dtcompatible:`nxp,mipi-dbi-dcnano-lcdif`

* Miscellaneous

   * :dtcompatible:`nordic,nrf-mpc`
   * :dtcompatible:`renesas,ra-ulpt`
   * :dtcompatible:`renesas,rx-sci`
   * :dtcompatible:`renesas,rz-sci`

* Multi-bit SPI

   * :dtcompatible:`snps,designware-ssi`

* :abbr:`MTD (Memory Technology Device)`

   * :dtcompatible:`fixed-subpartitions`
   * :dtcompatible:`jedec,mspi-nor`
   * :dtcompatible:`renesas,ra-nv-code-flash`
   * :dtcompatible:`renesas,ra-nv-data-flash`
   * :dtcompatible:`renesas,rx-nv-flash`

* Networking

   * :dtcompatible:`nordic,nrf-nfct-v2`

* Pin control

   * :dtcompatible:`ambiq,apollo5-pinctrl`
   * :dtcompatible:`arm,mps2-pinctrl`
   * :dtcompatible:`arm,mps3-pinctrl`
   * :dtcompatible:`arm,v2m_beetle-pinctrl`
   * :dtcompatible:`bflb,pinctrl`
   * :dtcompatible:`renesas,rx-pinctrl`
   * :dtcompatible:`renesas,rx-pinmux`
   * :dtcompatible:`renesas,rza-pinctrl`
   * :dtcompatible:`renesas,rza2m-pinctrl`
   * :dtcompatible:`renesas,rzn-pinctrl`
   * :dtcompatible:`renesas,rzt-pinctrl`
   * :dtcompatible:`renesas,rzv-pinctrl`
   * :dtcompatible:`wch,20x_30x-afio`

* Power management

   * :dtcompatible:`realtek,rts5912-ulpm`

* :abbr:`PWM (Pulse Width Modulation)`

   * :dtcompatible:`arduino-header-pwm`
   * :dtcompatible:`neorv32,pwm`
   * :dtcompatible:`realtek,rts5912-pwm`
   * :dtcompatible:`silabs,siwx91x-pwm`
   * :dtcompatible:`wch,gptm-pwm`

* Regulator

   * :dtcompatible:`x-powers,axp2101-regulator`

* Reset controller

   * :dtcompatible:`reset-mmio`

* :abbr:`RNG (Random Number Generator)`

   * :dtcompatible:`adi,maxq10xx-trng`
   * :dtcompatible:`zephyr,native-sim-rng`

* :abbr:`RTC (Real Time Clock)`

   * :dtcompatible:`nxp,pcf2123`
   * :dtcompatible:`realtek,rts5912-rtc`

* :abbr:`SDHC (Secure Digital Host Controller)`

   * :dtcompatible:`ambiq,sdio`
   * :dtcompatible:`xlnx,versal-8.9a`

* Sensors

   * :dtcompatible:`bosch,bmm350`
   * :dtcompatible:`everlight,als-pt19`
   * :dtcompatible:`invensense,icm45686`
   * :dtcompatible:`invensense,icp201xx`
   * :dtcompatible:`liteon,ltr329`
   * :dtcompatible:`meas,ms5837-02ba`
   * :dtcompatible:`meas,ms5837-30ba`
   * :dtcompatible:`pixart,paa3905`
   * :dtcompatible:`pixart,paj7620`
   * :dtcompatible:`pixart,pat9136`
   * :dtcompatible:`st,lsm6dsv32x`
   * :dtcompatible:`vishay,veml6031`
   * :dtcompatible:`we,wsen-itds-2533020201601`

* Serial controller

   * :dtcompatible:`bflb,uart`
   * :dtcompatible:`espressif,esp32-lpuart`
   * :dtcompatible:`ite,it51xxx-uart`
   * :dtcompatible:`renesas,rx-uart-sci`
   * :dtcompatible:`renesas,rx-uart-sci-qemu`
   * :dtcompatible:`renesas,rz-sci-uart`
   * :dtcompatible:`renesas,rza2m-scif-uart`
   * :dtcompatible:`zephyr,native-pty-uart`

* :abbr:`SPI (Serial Peripheral Interface)`

   * :dtcompatible:`cdns,spi`
   * :dtcompatible:`silabs,gspi`
   * :dtcompatible:`ti,cc23x0-spi`

* Stepper

   * :dtcompatible:`adi,tmc51xx`

* Tachometer

   * :dtcompatible:`ite,it51xxx-tach`
   * :dtcompatible:`realtek,rts5912-tach`

* Timer

   * :dtcompatible:`ite,it51xxx-timer`
   * :dtcompatible:`renesas,ra-ulpt-timer`
   * :dtcompatible:`renesas,rx-timer-cmt`
   * :dtcompatible:`renesas,rx-timer-cmt-start-control`
   * :dtcompatible:`renesas,rza2m-ostm`

* USB

   * :dtcompatible:`adi,max32-usbhs`
   * :dtcompatible:`st,stm32n6-otghs`

* Watchdog

   * :dtcompatible:`ite,it51xxx-watchdog`
   * :dtcompatible:`realtek,rts5912-watchdog`
   * :dtcompatible:`renesas,ra-wdt`
   * :dtcompatible:`silabs,siwx91x-wdt`
   * :dtcompatible:`wch,iwdg`

New Samples
***********

..
  Same as above for boards and drivers, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

* :zephyr:code-sample:`bmg160`
* :zephyr:code-sample:`debug-ulp`
* :zephyr:code-sample:`echo-ulp`
* :zephyr:code-sample:`fatfs-fstab`
* :zephyr:code-sample:`net-pkt-filter`
* :zephyr:code-sample:`paj7620_gesture`
* :zephyr:code-sample:`pressure_interrupt`
* :zephyr:code-sample:`pressure_polling`
* :zephyr:code-sample:`renesas_comparator`
* :zephyr:code-sample:`rz-openamp-linux-zephyr`
* :zephyr:code-sample:`spis-wakeup`
* :zephyr:code-sample:`stepper`
* :zephyr:code-sample:`uart_async`
* :zephyr:code-sample:`uuid`
* :zephyr:code-sample:`veml6031`

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?

* Added support for Armv8.1-M MPU's PXN (Privileged Execute Never) attribute.
  With this, the MPU attributes for ``__ramfunc`` and ``__ram_text_reloc`` were modified such that,
  PXN attribute is set for these regions if compiled with ``CONFIG_ARM_MPU_PXN`` and ``CONFIG_USERSPACE``.
  This results in a change in behaviour for code being executed from these regions because,
  if these regions have pxn attribute set in them, they cannot be executed in privileged mode.

* Removed support for Nucleo WBA52CG board (``nucleo_wba52cg``) since it is NRND (Not Recommended
  for New Design) and it is not supported anymore in the STM32CubeWBA from version 1.1.0 (July 2023).
  The migration to :zephyr:board:`nucleo_wba55cg` (``nucleo_wba55cg``) is recommended instead.

* Updated Mbed TLS to version 3.6.3 (from 3.6.2). The release notes can be found at:
  https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.3

* Updated TF-M to version 2.1.2 (from 2.1.1). The release notes can be found at:
  https://trustedfirmware-m.readthedocs.io/en/tf-mv2.1.2/releases/2.1.2.html
