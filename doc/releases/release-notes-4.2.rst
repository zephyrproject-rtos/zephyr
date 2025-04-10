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

New APIs and options
====================

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

* Stepper

  * :c:func:`stepper_stop()`

* Counter

  * :c:func:`counter_reset`

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

* Adafruit Industries, LLC

   * :zephyr:board:`adafruit_feather_esp32s3_tft` (``adafruit_feather_esp32s3_tft``)
   * :zephyr:board:`adafruit_feather_esp32s3` (``adafruit_feather_esp32s3``)

* FANKE Technology Co., Ltd.

   * :zephyr:board:`fk743m5_xih6` (``fk743m5_xih6``)

* Nuvoton Technology Corporation

   * :zephyr:board:`numaker_m55m1` (``numaker_m55m1``)

* Octavo Systems LLC

   * :zephyr:board:`osd32mp1_brk` (``osd32mp1_brk``)

* Pimoroni Ltd.

   * :zephyr:board:`pico_plus2` (``pico_plus2``)

* Renesas Electronics Corporation

   * :zephyr:board:`rza3ul_smarc` (``rza3ul_smarc``)
   * :zephyr:board:`rzn2l_rsk` (``rzn2l_rsk``)
   * :zephyr:board:`rzt2l_rsk` (``rzt2l_rsk``)

* STMicroelectronics

   * :zephyr:board:`stm32h757i_eval` (``stm32h757i_eval``)

* Silicon Laboratories

   * :zephyr:board:`slwrb4180b` (``slwrb4180b``)

* Texas Instruments

   * :zephyr:board:`sk_am64` (``sk_am64``)

* WIZnet Co., Ltd.

   * :zephyr:board:`w5500_evb_pico2` (``w5500_evb_pico2``)

* WinChipHead

   * :zephyr:board:`ch32v003f4p6_dev_board` (``ch32v003f4p6_dev_board``)
   * :zephyr:board:`linkw` (``linkw``)

New Drivers
***********

..
  Same as above for boards, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* :abbr:`ADC (Analog to Digital Converter)`

   * :dtcompatible:`adi,ad4050-adc`
   * :dtcompatible:`adi,ad4052-adc`
   * :dtcompatible:`renesas,rz-adc`

* Clock control

   * :dtcompatible:`wch,ch32v20x_30x-pll-clock`

* Comparator

   * :dtcompatible:`renesas,ra-acmphs`
   * :dtcompatible:`renesas,ra-acmphs-global`

* Counter

   * :dtcompatible:`zephyr,native-sim-counter`

* CPU

   * :dtcompatible:`wch,qingke-v4c`
   * :dtcompatible:`zephyr,native-sim-cpu`

* Cryptographic accelerator

   * :dtcompatible:`ti,cc23x0-aes`

* Display

   * :dtcompatible:`sitronix,st7567`

* :abbr:`DMA (Direct Memory Access)`

   * :dtcompatible:`renesas,rz-dma`
   * :dtcompatible:`wch,wch-dma`

* Ethernet

   * :dtcompatible:`st,stm32n6-ethernet`
   * :dtcompatible:`ti,dp83867`
   * :dtcompatible:`xlnx,axi-ethernet-1.00.a`

* Flash controller

   * :dtcompatible:`silabs,series2-flash-controller`

* File system

   * :dtcompatible:`zephyr,fstab,fatfs`

* :abbr:`GPIO (General Purpose Input/Output)` and Headers

   * :dtcompatible:`adi,max14915-gpio`
   * :dtcompatible:`nxp,lcd-pmod`
   * :dtcompatible:`raspberrypi,pico-gpio-port`
   * :dtcompatible:`renesas,ra-parallel-graphics-header`

* :abbr:`I2C (Inter-Integrated Circuit)`

   * :dtcompatible:`cdns,i2c`
   * :dtcompatible:`renesas,rz-riic`
   * :dtcompatible:`sensry,sy1xx-i2c`

* Input

   * :dtcompatible:`realtek,rts5912-kbd`
   * :dtcompatible:`st,stm32-tsc`
   * :dtcompatible:`tsc-keys`

* Mailbox

   * :dtcompatible:`renesas,rz-mhu-mbox`

* :abbr:`MDIO (Management Data Input/Output)`

   * :dtcompatible:`xlnx,axi-ethernet-1.00.a-mdio`

* Memory controller

   * :dtcompatible:`realtek,rts5912-bbram`

* :abbr:`MFD (Multi-Function Device)`

   * :dtcompatible:`adi,maxq10xx`
   * :dtcompatible:`x-powers,axp2101`

* Miscellaneous

   * :dtcompatible:`renesas,rz-sci`

* Multi-bit SPI

   * :dtcompatible:`snps,designware-ssi`

* :abbr:`MTD (Memory Technology Device)`

   * :dtcompatible:`jedec,mspi-nor`

* Networking

   * :dtcompatible:`nordic,nrf-nfct-v2`

* Pin control

   * :dtcompatible:`renesas,rza-pinctrl`
   * :dtcompatible:`renesas,rzn-pinctrl`
   * :dtcompatible:`renesas,rzt-pinctrl`
   * :dtcompatible:`wch,20x_30x-afio`

* :abbr:`PWM (Pulse Width Modulation)`

   * :dtcompatible:`silabs,siwx91x-pwm`

* Regulator

   * :dtcompatible:`x-powers,axp2101-regulator`

* :abbr:`RNG (Random Number Generator)`

   * :dtcompatible:`adi,maxq10xx-trng`
   * :dtcompatible:`zephyr,native-sim-rng`

* :abbr:`RTC (Real Time Clock)`

   * :dtcompatible:`realtek,rts5912-rtc`

* Sensors

   * :dtcompatible:`bosch,bmm350`
   * :dtcompatible:`invensense,icm45686`
   * :dtcompatible:`pixart,paa3905`
   * :dtcompatible:`vishay,veml6031`

* Serial controller

   * :dtcompatible:`renesas,rz-sci-uart`
   * :dtcompatible:`zephyr,native-pty-uart`

* Watchdog

   * :dtcompatible:`realtek,rts5912-watchdog`
   * :dtcompatible:`renesas,ra-wdt`
   * :dtcompatible:`silabs,siwx91x-wdt`

New Samples
***********

..
  Same as above for boards and drivers, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

* :zephyr:code-sample:`bmg160`
* :zephyr:code-sample:`debug-ulp`
* :zephyr:code-sample:`fatfs-fstab`
* :zephyr:code-sample:`renesas_comparator`
* :zephyr:code-sample:`rz-openamp-linux-zephyr`
* :zephyr:code-sample:`stepper`
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
