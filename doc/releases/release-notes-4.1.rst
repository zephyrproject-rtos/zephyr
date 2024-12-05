:orphan:

..
  What goes here: removed/deprecated apis, new boards, new drivers, notable
  features. If you feel like something new can be useful to a user, put it
  under "Other Enhancements" in the first paragraph, if you feel like something
  is worth mentioning in the project media (release blog post, release
  livestream) put it under "Major enhancement".

  No list of bugfixes, minor changes, those are already in the git log, this is
  not a changelog.

  Does the entry have a link that contains the details? Just add the link, if
  you think it needs more details, put them in the content that shows up on the
  link.

  Are you thinking about generating this? Don't put anything at all.

  Does the thing require the user to change their application? Put it on the
  migration guide instead. (TODO: move the removed APIs section in the
  migration guide)

  Could the detailed paragraph that you are about to write go in the actual
  project documentation instead and just be linked here? Do that instead.

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

Removed APIs
============

 * The Bluetooth HCI driver API has been removed. It has been replaced by a
   :c:group:`new API<bt_hci_api>` that follows the normal Zephyr driver model.

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

Deprecated APIs
===============

* the :c:func:`bt_le_set_auto_conn` API function. Application developers can achieve
  the same functionality in their application code by reconnecting to the peer when the
  :c:member:`bt_conn_cb.disconnected` callback is invoked.

* :kconfig:option:`CONFIG_NATIVE_APPLICATION` has been deprecated.

* For the native_sim target :kconfig:option:`CONFIG_NATIVE_SIM_NATIVE_POSIX_COMPAT` has been
  switched to ``n`` by default, and this option has been deprecated.

* :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT`

* All HWMv1 board name aliases which were added as deprecated in v3.7 are now removed
  (:github:`82247`).

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

* Crypto

  * :kconfig:option:`CONFIG_MBEDTLS_PSA_STATIC_KEY_SLOTS`
  * :kconfig:option:`CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT`

* Other

  * :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA`
  * :c:macro:`DT_ANY_INST_HAS_BOOL_STATUS_OKAY`
  * :c:struct:`led_dt_spec`

New Boards & SoC
****************
..
  Just link the board, details go in the board description

* :zephyr:board:`max78002evkit`
* :zephyr:board:`acp_6_0_adsp`
* :zephyr:board:`ucan`
* :zephyr:board:`ttgo_t7v1_5`
* :zephyr:board:`ttgo_t8s3`
* :zephyr:board:`m5stack_cores3`
* :zephyr:board:`mks_canable_v20`
* :zephyr:board:`octopus_io_board`
* :zephyr:board:`octopus_som`
* :zephyr:board:`candlelight`
* :zephyr:board:`candlelightfd`
* :zephyr:board:`xiao_esp32c6`
* :zephyr:board:`rp2040_zero`
* :zephyr:board:`ch32v003evt`
* :zephyr:board:`we_orthosie1ev`
* :zephyr:board:`mini_stm32h7b0`

New Drivers
***********
..
  Just link the driver, details go in the binding description

* :dtcompatible:`silabs,dbus-pinctrl`

* :dtcompatible:`renesas,bt-hci-da1453x`
* :dtcompatible:`st,hci-stm32wb0`
* :dtcompatible:`nuvoton,npcm-pcc`
* :dtcompatible:`wch,ch32v00x-hse-clock`
* :dtcompatible:`wch,ch32v00x-hsi-clock`
* :dtcompatible:`wch,ch32v00x-pll-clock`
* :dtcompatible:`wch,rcc`
* :dtcompatible:`adi,max32-rtc-counter`
* :dtcompatible:`wch,qingke-v2`
* :dtcompatible:`xlnx,axi-dma-1.00.a`
* :dtcompatible:`xlnx,eth-dma`
* :dtcompatible:`renesas,ra-ethernet`
* :dtcompatible:`lattice,ice40-fpga-base`
* :dtcompatible:`lattice,ice40-fpga-bitbang`
* :dtcompatible:`awinic,aw9523b-gpio`
* :dtcompatible:`ite,it8801-gpio`
* :dtcompatible:`nordic,npm2100-gpio`
* :dtcompatible:`stemma-qt-connector`
* :dtcompatible:`wch,gpio`
* :dtcompatible:`nxp,hdlc-rcp-if`
* :dtcompatible:`ti,tca9544a`
* :dtcompatible:`ti,tca9544a-channel`
* :dtcompatible:`ite,it8801-kbd`
* :dtcompatible:`wch,pfic`
* :dtcompatible:`linaro,ivshmem-mbox`
* :dtcompatible:`renesas,ra-mdio`
* :dtcompatible:`awinic,aw9523b`
* :dtcompatible:`ite,it8801-altctrl`
* :dtcompatible:`ite,it8801-mfd-map`
* :dtcompatible:`ite,it8801-mfd`
* :dtcompatible:`nordic,npm2100`
* :dtcompatible:`nordic,nrf-ppib`
* :dtcompatible:`fujitsu,mb85rsxx`
* :dtcompatible:`silabs,dbus-pinctrl`
* :dtcompatible:`wch,afio`
* :dtcompatible:`ite,it8801-pwm`
* :dtcompatible:`nordic,npm2100-regulator`
* :dtcompatible:`adi,adxl366`
* :dtcompatible:`adi,adxl366`
* :dtcompatible:`hc-sr04`
* :dtcompatible:`nordic,npm2100-vbat`
* :dtcompatible:`sensirion,scd40`
* :dtcompatible:`sensirion,scd41`
* :dtcompatible:`sensirion,sts4x`
* :dtcompatible:`wch,usart`
* :dtcompatible:`ite,it8xxx2-spi`
* :dtcompatible:`renesas,ra-spi`
* :dtcompatible:`vnd,i2s`
* :dtcompatible:`mediatek,ostimer64`
* :dtcompatible:`wch,systick`
* :dtcompatible:`ambiq,usb`
* :dtcompatible:`zephyr,video-emul-imager`
* :dtcompatible:`zephyr,video-emul-rx`
* :dtcompatible:`nordic,npm2100-wdt`

New Samples
***********

..
  Just link the samples, details go in the sample documentation itself

* :zephyr:code-sample:`ble_cs`
* :zephyr:code-sample:`coresight_stm_sample`
* :zephyr:code-sample:`lvgl-screen-transparency`
* :zephyr:code-sample:`mqtt-sn-publisher`
* :zephyr:code-sample:`rtc`
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
