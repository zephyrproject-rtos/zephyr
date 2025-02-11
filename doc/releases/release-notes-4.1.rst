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

Zephyr 4.1.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.1.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.0.0 to Zephyr v4.1.0 can be found in the separate :ref:`migration guide<migration_4.1>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

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

* For the native_sim target :kconfig:option:`CONFIG_NATIVE_SIM_NATIVE_POSIX_COMPAT` has been
  switched to ``n`` by default, and this option has been deprecated.

* :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT`

* All HWMv1 board name aliases which were added as deprecated in v3.7 are now removed
  (:github:`82247`).

* The TinyCrypt library has been deprecated as the upstream version is no longer maintained.
  PSA Crypto API is now the recommended cryptographic library for Zephyr.

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

* Build system

  * Sysbuild

    * The newly introduced MCUboot swap using offset mode can be selected from sysbuild by using
      ``SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET``, this mode is experimental.

* Crypto

  * :kconfig:option:`CONFIG_MBEDTLS_PSA_STATIC_KEY_SLOTS`
  * :kconfig:option:`CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT`

* Management

  * hawkBit

    * The hawkBit subsystem now uses the State Machine Framework internally.
    * :kconfig:option:`CONFIG_HAWKBIT_TENANT`
    * :kconfig:option:`CONFIG_HAWKBIT_EVENT_CALLBACKS`
    * :kconfig:option:`CONFIG_HAWKBIT_SAVE_PROGRESS`

  * MCUmgr

    * Image management :c:macro:`MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED` now has image data field
      :c:struct:`img_mgmt_image_confirmed`.

* Video

  * :c:func:`video_set_stream()` driver API has replaced :c:func:`video_stream_start()` and
    :c:func:`video_stream_stop()` driver APIs.

* Other

  * :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA`
  * :c:macro:`DT_ANY_INST_HAS_BOOL_STATUS_OKAY`
  * :c:struct:`led_dt_spec`
  * :kconfig:option:`CONFIG_STEP_DIR_STEPPER`

New Boards
**********
..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

* Adafruit Industries, LLC

   * :zephyr:board:`adafruit_feather_m4_express` (``adafruit_feather_m4_express``)
   * :zephyr:board:`adafruit_qt_py_esp32s3` (``adafruit_qt_py_esp32s3``)

* Advanced Micro Devices (AMD), Inc.

   * :zephyr:board:`acp_6_0_adsp` (``acp_6_0_adsp``)

* Analog Devices, Inc.

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

   * :zephyr:board:`mimxrt700_evk` (``mimxrt700_evk``)

* Nordic Semiconductor

   * :zephyr:board:`nrf54l09pdk` (``nrf54l09pdk``)

* Norik Systems

   * :zephyr:board:`octopus_io_board` (``octopus_io_board``)
   * :zephyr:board:`octopus_som` (``octopus_som``)

* Qorvo, Inc.

   * :zephyr:board:`decawave_dwm3001cdk` (``decawave_dwm3001cdk``)

* Raspberry Pi Foundation

   * :zephyr:board:`rpi_pico2` (``rpi_pico2``)

* Realtek Semiconductor Corp.

   * :zephyr:board:`rts5912_evb` (``rts5912_evb``)

* Renesas Electronics Corporation

   * :zephyr:board:`fpb_ra4e1` (``fpb_ra4e1``)
   * :zephyr:board:`rzg3s_smarc` (``rzg3s_smarc``)
   * :zephyr:board:`voice_ra4e1` (``voice_ra4e1``)

* STMicroelectronics

   * :zephyr:board:`nucleo_c071rb` (``nucleo_c071rb``)
   * :zephyr:board:`nucleo_f072rb` (``nucleo_f072rb``)
   * :zephyr:board:`nucleo_h7s3l8` (``nucleo_h7s3l8``)
   * :zephyr:board:`nucleo_wb07cc` (``nucleo_wb07cc``)
   * :zephyr:board:`stm32f413h_disco` (``stm32f413h_disco``)

* Seeed Technology Co., Ltd

   * :zephyr:board:`xiao_esp32c6` (``xiao_esp32c6``)

* Shenzhen Fuyuansheng Electronic Technology Co., Ltd.

   * :zephyr:board:`ucan` (``ucan``)

* Silicon Laboratories

   * :zephyr:board:`xg23_rb4210a` (``xg23_rb4210a``)
   * :zephyr:board:`xg24_ek2703a` (``xg24_ek2703a``)
   * :zephyr:board:`xg29_rb4412a` (``xg29_rb4412a``)

* Toradex AG

   * :zephyr:board:`verdin_imx8mm` (``verdin_imx8mm``)

* Waveshare Electronics

   * :zephyr:board:`rp2040_zero` (``rp2040_zero``)

* WeAct Studio

   * :zephyr:board:`mini_stm32h7b0` (``mini_stm32h7b0``)

* WinChipHead

   * :zephyr:board:`ch32v003evt` (``ch32v003evt``)

* Würth Elektronik GmbH.

   * :zephyr:board:`we_oceanus1ev` (``we_oceanus1ev``)
   * :zephyr:board:`we_orthosie1ev` (``we_orthosie1ev``)

* others

   * :zephyr:board:`canbardo` (``canbardo``)
   * :zephyr:board:`candlelight` (``candlelight``)
   * :zephyr:board:`candlelightfd` (``candlelightfd``)
   * :zephyr:board:`esp32c3_supermini` (``esp32c3_supermini``)
   * :zephyr:board:`promicro_nrf52840` (``promicro_nrf52840``)

New Drivers
***********
..
  Same as above for boards, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* :abbr:`ADC (Analog to Digital Converter)`

   * :dtcompatible:`adi,ad4114-adc`
   * :dtcompatible:`ti,ads131m02`
   * :dtcompatible:`ti,tla2022`
   * :dtcompatible:`ti,tla2024`
   * :dtcompatible:`ti,ads114s06`
   * :dtcompatible:`ti,ads124s06`
   * :dtcompatible:`ti,ads124s08`

* ARM architecture

   * :dtcompatible:`nxp,nbu`

* Bluetooth

   * :dtcompatible:`renesas,bt-hci-da1453x`
   * :dtcompatible:`st,hci-stm32wb0`

* Clock control

   * :dtcompatible:`atmel,sam0-gclk`
   * :dtcompatible:`atmel,sam0-mclk`
   * :dtcompatible:`atmel,sam0-osc32kctrl`
   * :dtcompatible:`nordic,nrf-hsfll-global`
   * :dtcompatible:`nuvoton,npcm-pcc`
   * :dtcompatible:`realtek,rts5912-sccon`
   * :dtcompatible:`wch,ch32v00x-hse-clock`
   * :dtcompatible:`wch,ch32v00x-hsi-clock`
   * :dtcompatible:`wch,ch32v00x-pll-clock`
   * :dtcompatible:`wch,rcc`

* Counter

   * :dtcompatible:`adi,max32-rtc-counter`

* CPU

   * :dtcompatible:`wch,qingke-v2`

* :abbr:`DAC (Digital to Analog Converter)`

   * :dtcompatible:`adi,max22017-dac`

* :abbr:`DAI (Digital Audio Interface)`

   * :dtcompatible:`mediatek,afe`

* Display

   * :dtcompatible:`ilitek,ili9806e-dsi`
   * :dtcompatible:`renesas,ra-glcdc`
   * :dtcompatible:`solomon,ssd1309fb`

* :abbr:`DMA (Direct Memory Access)`

   * :dtcompatible:`infineon,cat1-dma`
   * :dtcompatible:`nxp,sdma`
   * :dtcompatible:`silabs,ldma`
   * :dtcompatible:`xlnx,axi-dma-1.00.a`
   * :dtcompatible:`xlnx,eth-dma`

* Ethernet

   * :dtcompatible:`davicom,dm8806-phy`
   * :dtcompatible:`microchip,lan9250`
   * :dtcompatible:`microchip,t1s-phy`
   * :dtcompatible:`microchip,vsc8541`
   * :dtcompatible:`renesas,ra-ethernet`

* Firmware

   * :dtcompatible:`arm,scmi-power`

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
   * :dtcompatible:`raspberrypi,rp1-gpio`
   * :dtcompatible:`realtek,rts5912-gpio`
   * :dtcompatible:`renesas,ra-gpio-mipi-header`
   * :dtcompatible:`renesas,rz-gpio`
   * :dtcompatible:`renesas,rz-gpio-int`
   * :dtcompatible:`sensry,sy1xx-gpio`
   * :dtcompatible:`st,mfxstm32l152`
   * :dtcompatible:`stemma-qt-connector`
   * :dtcompatible:`wch,gpio`

* IEEE 802.15.4 HDLC RCP interface

   * :dtcompatible:`nxp,hdlc-rcp-if`
   * :dtcompatible:`uart,hdlc-rcp-if`

* :abbr:`I2C (Inter-Integrated Circuit)`

   * :dtcompatible:`nxp,ii2c`
   * :dtcompatible:`ti,omap-i2c`
   * :dtcompatible:`ti,tca9544a`

* :abbr:`I3C (Improved Inter-Integrated Circuit)`

   * :dtcompatible:`st,stm32-i3c`

* Input

   * :dtcompatible:`ite,it8801-kbd`
   * :dtcompatible:`microchip,cap12xx`
   * :dtcompatible:`nintendo,nunchuk`

* Interrupt controller

   * :dtcompatible:`wch,pfic`

* Mailbox

   * :dtcompatible:`linaro,ivshmem-mbox`
   * :dtcompatible:`ti,omap-mailbox`

* :abbr:`MDIO (Management Data Input/Output)`

   * :dtcompatible:`microchip,lan865x-mdio`
   * :dtcompatible:`renesas,ra-mdio`

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

* :abbr:`MIPI DSI (Mobile Industry Processor Interface Display Serial Interface)`

   * :dtcompatible:`renesas,ra-mipi-dsi`

* Miscellaneous

   * :dtcompatible:`nordic,nrf-bicr`
   * :dtcompatible:`nordic,nrf-ppib`
   * :dtcompatible:`renesas,ra-external-interrupt`

* :abbr:`MMU / MPU (Memory Management Unit / Memory Protection Unit)`

   * :dtcompatible:`nxp,sysmpu`

* :abbr:`MTD (Memory Technology Device)`

   * :dtcompatible:`fujitsu,mb85rsxx`
   * :dtcompatible:`nxp,s32-qspi-hyperflash`
   * :dtcompatible:`nxp,xspi-mx25um51345g`

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
   * :dtcompatible:`wch,afio`

* :abbr:`PWM (Pulse Width Modulation)`

   * :dtcompatible:`atmel,sam0-tc-pwm`
   * :dtcompatible:`ite,it8801-pwm`
   * :dtcompatible:`zephyr,fake-pwm`

* Quad SPI

   * :dtcompatible:`nxp,s32-qspi-sfp-frad`
   * :dtcompatible:`nxp,s32-qspi-sfp-mdad`

* Regulator

   * :dtcompatible:`nordic,npm2100-regulator`

* :abbr:`RNG (Random Number Generator)`

   * :dtcompatible:`nordic,nrf-cracen-ctrdrbg`
   * :dtcompatible:`renesas,ra-sce5-rng`
   * :dtcompatible:`renesas,ra-sce7-rng`
   * :dtcompatible:`renesas,ra-sce9-rng`
   * :dtcompatible:`renesas,ra-trng`
   * :dtcompatible:`st,stm32-rng-noirq`

* :abbr:`RTC (Real Time Clock)`

   * :dtcompatible:`maxim,ds3231-rtc`
   * :dtcompatible:`microcrystal,rv8803`

* SDHC

   * :dtcompatible:`renesas,ra-sdhc`

* Sensors

   * :dtcompatible:`adi,adxl366`
   * :dtcompatible:`hc-sr04`
   * :dtcompatible:`invensense,icm42670s`
   * :dtcompatible:`invensense,icm42370`
   * :dtcompatible:`maxim,ds3231-sensor`
   * :dtcompatible:`melexis,mlx90394`
   * :dtcompatible:`nordic,npm2100-vbat`
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
   * :dtcompatible:`wch,usart`

* :abbr:`SPI (Serial Peripheral Interface)`

   * :dtcompatible:`ite,it8xxx2-spi`
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
   * :dtcompatible:`riscv,machine-timer`
   * :dtcompatible:`wch,systick`

* USB

   * :dtcompatible:`ambiq,usb`
   * :dtcompatible:`renesas,ra-udc`
   * :dtcompatible:`renesas,ra-usb`

* Video

   * :dtcompatible:`zephyr,video-emul-imager`
   * :dtcompatible:`zephyr,video-emul-rx`

* Watchdog

   * :dtcompatible:`atmel,sam4l-watchdog`
   * :dtcompatible:`nordic,npm2100-wdt`
   * :dtcompatible:`nxp,rtwdog`

* Wi-Fi

   * :dtcompatible:`infineon,airoc-wifi`

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
* :zephyr:code-sample:`uart`
* :zephyr:code-sample:`webusb-next`

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?

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
