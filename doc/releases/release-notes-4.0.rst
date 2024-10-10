:orphan:

.. _zephyr_4.0:

Zephyr 4.0.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.0.0.

Major enhancements with this release include:

* The introduction of the :ref:`secure storage<secure_storage>` subsystem. It allows the use of the
  PSA Secure Storage API and of persistent keys in the PSA Crypto API on all board targets. It
  is now the standard way to provide device-specific protection to data at rest. (:github:`76222`)

An overview of the changes required or recommended when migrating your application from Zephyr
v3.7.0 to Zephyr v4.0.0 can be found in the separate :ref:`migration guide<migration_4.0>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* :cve:`2024-8798`: Under embargo until 2024-11-22

API Changes
***********

* Removed deprecated arch-level CMSIS header files
  ``include/zephyr/arch/arm/cortex_a_r/cmsis.h`` and
  ``include/zephyr/arch/arm/cortex_m/cmsis.h``. ``cmsis_core.h`` needs to be
  included now.

* Removed deprecated ``ceiling_fraction`` macro. :c:macro:`DIV_ROUND_UP` needs
  to be used now.

* Deprecated ``EARLY``, ``APPLICATION`` and ``SMP`` init levels can no longer be
  used for devices.

Removed APIs in this release
============================

* Macro ``K_THREAD_STACK_MEMBER``, deprecated since v3.5.0, has been removed.
  Use :c:macro:`K_KERNEL_STACK_MEMBER` instead.
* ``CBPRINTF_PACKAGE_COPY_*`` macros, deprecated since Zephyr 3.5.0, have been removed.
* ``_ENUM_TOKEN`` and ``_ENUM_UPPER_TOKEN`` macros, deprecated since Zephyr 2.7.0,
  are no longer generated.

Deprecated in this release
==========================

* Deprecated the :c:func:`net_buf_put` and :c:func:`net_buf_get` API functions in favor of
  :c:func:`k_fifo_put` and :c:func:`k_fifo_get`.

* The :ref:`kscan_api` subsystem has been marked as deprecated.

* The TinyCrypt library was marked as deprecated (:github:`79566`). The reasons
  for this are (:github:`43712``):

  * the upstream version of this library is unmaintained.

  * to reduce the number of crypto libraries available in Zephyr (currently there are
    3 different implementations: TinyCrypt, MbedTLS and PSA Crypto APIs).

  The PSA Crypto API is now the de-facto standard to perform crypto operations.

Architectures
*************

* ARC

* ARM

* ARM64

  * Added initial support for :c:func:`arch_stack_walk` that supports unwinding via esf only

* RISC-V

  * The stack traces upon fatal exception now prints the address of stack pointer (sp) or frame
    pointer (fp) depending on the build configuration.

  * When :kconfig:option:`CONFIG_EXTRA_EXCEPTION_INFO` is enabled, the exception stack frame (arch_esf)
    has an additional field ``csf`` that points to the callee-saved-registers upon an fatal error,
    which can be accessed in :c:func:`k_sys_fatal_error_handler` by ``esf->csf``.

    * For SoCs that select ``RISCV_SOC_HAS_ISR_STACKING``, the ``SOC_ISR_STACKING_ESF_DECLARE`` has to
      include the ``csf`` member, otherwise the build would fail.

* Xtensa

* x86

  * Added initial support for :c:func:`arch_stack_walk` that supports unwinding via esf only

Kernel
******

Bluetooth
*********

* Audio

  * :c:func:`bt_tbs_client_register_cb` now supports multiple listeners and may now return an error.

  * Added APIs for getting and setting the assisted listening stream values in codec capabilities
    and codec configuration:

    * :c:func:`bt_audio_codec_cfg_meta_get_assisted_listening_stream`
    * :c:func:`bt_audio_codec_cfg_meta_set_assisted_listening_stream`
    * :c:func:`bt_audio_codec_cap_meta_get_assisted_listening_stream`
    * :c:func:`bt_audio_codec_cap_meta_set_assisted_listening_stream`

  * Added APIs for getting and setting the broadcast name in codec capabilities
    and codec configuration:

    * :c:func:`bt_audio_codec_cfg_meta_get_broadcast_name`
    * :c:func:`bt_audio_codec_cfg_meta_set_broadcast_name`
    * :c:func:`bt_audio_codec_cap_meta_get_broadcast_name`
    * :c:func:`bt_audio_codec_cap_meta_set_broadcast_name`

* Host

  * Added API :c:func:`bt_gatt_get_uatt_mtu` to get current Unenhanced ATT MTU of a given
    connection (experimental).
  * Added :kconfig:option:`CONFIG_BT_CONN_TX_NOTIFY_WQ`.
    The option allows using a separate workqueue for connection TX notify processing
    (:c:func:`bt_conn_tx_notify`) to make Bluetooth stack more independent from the system workqueue.

  * The host now disconnects from the peer upon ATT timeout.

  * Added a warning to :c:func:`bt_conn_le_create` and :c:func:`bt_conn_le_create_synced` if
    the connection pointer passed as an argument is not NULL.

  * Added Kconfig option :kconfig:option:`CONFIG_BT_CONN_CHECK_NULL_BEFORE_CREATE` to enforce
    :c:func:`bt_conn_le_create` and :c:func:`bt_conn_le_create_synced` return an error if the
    connection pointer passed as an argument is not NULL.

* Controller

  * Added Periodic Advertising Sync Transfer (PAST) support with support for both sending and receiving roles.
    The option can be enabled by :kconfig:option:`CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER` and
    :kconfig:option:`CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER`.

* HCI Drivers

Boards & SoC Support
********************

* Added support for these SoC series:

* Made these changes in other SoC series:

  * NXP S32Z270: Added support for the new silicon cut version 2.0. Note that the previous
    versions (1.0 and 1.1) are no longer supported.

* Added support for these boards:

   * :zephyr:board:`01space ESP32C3 0.42 OLED <esp32c3_042_oled>` (``esp32c3_042_oled``)
   * :zephyr:board:`ADI MAX32662EVKIT <max32662evkit>` (``max32662evkit``)
   * :zephyr:board:`ADI MAX32666EVKIT <max32666evkit>` (``max32666evkit``)
   * :zephyr:board:`ADI MAX32666FTHR <max32666fthr>` (``max32666fthr``)
   * :zephyr:board:`ADI MAX32675EVKIT <max32675evkit>` (``max32675evkit``)
   * :zephyr:board:`ADI MAX32690FTHR <max32690fthr>` (``max32690fthr``)
   * :ref:`Arduino Nicla Vision <arduino_nicla_vision_board>` (``arduino_nicla_vision``)
   * :zephyr:board:`BeagleBone AI-64 <beaglebone_ai64>` (``beaglebone_ai64``)
   * :zephyr:board:`BeaglePlay (CC1352) <beagleplay>` (``beagleplay``)
   * :zephyr:board:`DPTechnics Walter <walter>` (``walter``)
   * :zephyr:board:`Espressif ESP32-C3-DevKitC <esp32c3_devkitc>` (``esp32c3_devkitc``)
   * :zephyr:board:`Espressif ESP32-C3-DevKit-RUST <esp32c3_rust>` (``esp32c3_rust``)
   * :zephyr:board:`Espressif ESP32-S3-EYE <esp32s3_eye>` (``esp32s3_eye``)
   * :zephyr:board:`Espressif ESP8684-DevKitM <esp8684_devkitm>` (``esp8684_devkitm``)
   * :zephyr:board:`Gardena Smart Garden Radio Module <sgrm>` (``sgrm``)
   * :zephyr:board:`mikroe STM32 M4 Clicker <mikroe_stm32_m4_clicker>` (``mikroe_stm32_m4_clicker``)
   * :ref:`Nordic Semiconductor nRF54L15 DK <nrf54l15dk_nrf54l15>` (``nrf54l15dk``)
   * :ref:`Nordic Semiconductor nRF54L20 PDK <nrf54l20pdk_nrf54l20>` (``nrf54l20pdk``)
   * :ref:`Nordic Semiconductor nRF7002 DK <nrf7002dk_nrf5340>` (``nrf7002dk``)
   * :zephyr:board:`Nuvoton NPCM400_EVB <npcm400_evb>` (``npcm400_evb``)
   * :zephyr:board:`NXP FRDM-MCXA156 <frdm_mcxa156>` (``frdm_mcxa156``)
   * :zephyr:board:`NXP FRDM-MCXC242 <frdm_mcxc242>` (``frdm_mcxc242``)
   * :zephyr:board:`NXP FRDM-MCXC444 <frdm_mcxc444>` (``frdm_mcxc444``)
   * :zephyr:board:`NXP FRDM-MCXN236 <frdm_mcxn236>` (``frdm_mcxn236``)
   * :zephyr:board:`NXP FRDM-MCXW71 <frdm_mcxw71>` (``frdm_mcxw71``)
   * :zephyr:board:`NXP i.MX95 EVK <imx95_evk>` (``imx95_evk``)
   * :zephyr:board:`NXP MIMXRT1180-EVK <mimxrt1180_evk>` (``mimxrt1180_evk``)
   * :ref:`PHYTEC phyBOARD-Nash i.MX93 <phyboard_nash>` (``phyboard_nash``)
   * :ref:`Renesas RA2A1 Evaluation Kit <ek_ra2a1>` (``ek_ra2a1``)
   * :ref:`Renesas RA4E2 Evaluation Kit <ek_ra4e2>` (``ek_ra4e2``)
   * :ref:`Renesas RA4M2 Evaluation Kit <ek_ra4m2>` (``ek_ra4m2``)
   * :ref:`Renesas RA4M3 Evaluation Kit <ek_ra4m3>` (``ek_ra4m3``)
   * :ref:`Renesas RA4W1 Evaluation Kit <ek_ra4w1>` (``ek_ra4w1``)
   * :ref:`Renesas RA6E2 Evaluation Kit <ek_ra6e2>` (``ek_ra6e2``)
   * :ref:`Renesas RA6M1 Evaluation Kit <ek_ra6m1>` (``ek_ra6m1``)
   * :ref:`Renesas RA6M2 Evaluation Kit <ek_ra6m2>` (``ek_ra6m2``)
   * :ref:`Renesas RA6M3 Evaluation Kit <ek_ra6m3>` (``ek_ra6m3``)
   * :ref:`Renesas RA6M4 Evaluation Kit <ek_ra6m4>` (``ek_ra6m4``)
   * :ref:`Renesas RA6M5 Evaluation Kit <ek_ra6m5>` (``ek_ra6m5``)
   * :ref:`Renesas RA8D1 Evaluation Kit <ek_ra8d1>` (``ek_ra8d1``)
   * :ref:`Renesas RA6E1 Fast Prototyping Board <fpb_ra6e1>` (``fpb_ra6e1``)
   * :ref:`Renesas RA6E2 Fast Prototyping Board <fpb_ra6e2>` (``fpb_ra6e2``)
   * :ref:`Renesas RA8T1 Evaluation Kit <mcb_ra8t1>` (``mck_ra8t1``)
   * :zephyr:board:`Renode Cortex-R8 Virtual <cortex_r8_virtual>` (``cortex_r8_virtual``)
   * :ref:`sensry.io Ganymed Break-Out-Board (BOB) <ganymed_bob>` (``ganymed_bob``)
   * :zephyr:board:`SiLabs SiM3U1xx 32-bit MCU USB Development Kit <sim3u1xx_dk>` (``sim3u1xx_dk``)
   * :ref:`SparkFun Thing Plus Matter <sparkfun_thing_plus_mgm240p>` (``sparkfun_thing_plus_matter_mgm240p``)
   * :zephyr:board:`ST Nucleo G431KB <nucleo_g431kb>` (``nucleo_g431kb``)
   * :zephyr:board:`ST Nucleo H503RB <nucleo_h503rb>` (``nucleo_h503rb``)
   * :zephyr:board:`ST Nucleo H755ZI-Q <nucleo_h755zi_q>` (``nucleo_h755zi_q``)
   * :zephyr:board:`ST Nucleo U031R8 <nucleo_u031r8>` (``nucleo_u031r8``)
   * :zephyr:board:`ST Nucleo U083RC <nucleo_u083rc>` (``nucleo_u083rc``)
   * :zephyr:board:`ST Nucleo WB05KZ <nucleo_wb05kz>` (``nucleo_wb05kz``)
   * :zephyr:board:`ST Nucleo WB09KE <nucleo_wb09ke>` (``nucleo_wb09ke``)
   * :zephyr:board:`ST STM32U083C-DK <stm32u083c_dk>` (``stm32u083c_dk``)
   * :zephyr:board:`TI CC1352P7 LaunchPad <cc1352p7_lp>` (``cc1352p7_lp``)
   * :zephyr:board:`vcc-gnd YD-STM32H750VB <yd_stm32h750vb>` (``yd_stm32h750vb``)
   * :zephyr:board:`WeAct Studio STM32F405 Core Board V1.0 <weact_stm32f405_core>` (``weact_stm32f405_core``)
   * :zephyr:board:`WeAct Studio USB2CANFDV1 <usb2canfdv1>` (``usb2canfdv1``)
   * :zephyr:board:`Witte Technology Linum Board <linum>` (``linum``)

* Made these board changes:

  * :ref:`native_posix<native_posix>` has been deprecated in favour of
    :ref:`native_sim<native_sim>`.
  * The nrf54l15bsim target now includes models of the AAR, CCM and ECB peripherals, and many
    other improvements.
  * Support for Google Kukui EC board (``google_kukui``) has been dropped.
  * STM32: Deprecated MCO configuration via Kconfig in favour of setting it through devicetree.
    See ``samples/boards/stm32/mco`` sample.
  * Removed the ``nrf54l15pdk`` board, use :ref:`nrf54l15dk_nrf54l15` instead.
  * PHYTEC: ``mimx8mp_phyboard_pollux`` has been renamed to :ref:`phyboard_pollux<phyboard_pollux>`,
    with the old name marked as deprecated.
  * PHYTEC: ``mimx8mm_phyboard_polis`` has been renamed to :ref:`phyboard_polis<phyboard_polis>`,
    with the old name marked as deprecated.
  * The board qualifier for MPS3/AN547 is changed from:

    * ``mps3/an547`` to ``mps3/corstone300/an547`` for secure and
    * ``mps3/an547/ns`` to ``mps3/corstone300/an547/ns`` for non-secure.

  * Added Thingy53 forwarding of network core pins to network core for SPI peripheral (disabled
    by default) including pin mappings.

* Added support for the following shields:

  * :ref:`ADI EVAL-ADXL362-ARDZ <eval_adxl362_ardz>`
  * :ref:`ADI EVAL-ADXL372-ARDZ <eval_adxl372_ardz>`
  * :ref:`Digilent Pmod ACL <pmod_acl>`
  * :ref:`MikroElektronika BLE TINY Click <mikroe_ble_tiny_click_shield>`
  * :ref:`Nordic SemiConductor nRF7002 EB <nrf7002eb>`
  * :ref:`Nordic SemiConductor nRF7002 EK <nrf7002ek>`
  * :ref:`ST X-NUCLEO-WB05KN1: BLE expansion board <x-nucleo-wb05kn1>`
  * :ref:`WeAct Studio MiniSTM32H7xx OV2640 Camera Sensor <weact_ov2640_cam_module>`

Build system and Infrastructure
*******************************

* Added support for .elf files to the west flash command for jlink, pyocd and linkserver runners.

* Extracted pickled EDT generation from gen_defines.py into gen_edt.py. This moved the following
  parameters from the cmake variable ``EXTRA_GEN_DEFINES_ARGS`` to ``EXTRA_GEN_EDT_ARGS``:

   * ``--dts``
   * ``--dtc-flags``
   * ``--bindings-dirs``
   * ``--dts-out``
   * ``--edt-pickle-out``
   * ``--vendor-prefixes``
   * ``--edtlib-Werror``

* Switched to using imgtool directly from the build system when signing images instead of calling
  ``west sign``.

* Added support for selecting MCUboot operating mode in sysbuild using ``SB_CONFIG_MCUBOOT_MODE``.

* Added support for RAM-load MCUboot operating mode in build system, including sysbuild support.

Documentation
*************

 * Added two new build commands, ``make html-live`` and ``make html-live-fast``, that automatically locally
   host the generated documentation. They also automatically rebuild and rehost the documentation when changes
   to the input ``.rst`` files are detected on the filesystem.

Drivers and Sensors
*******************

* ADC

* Battery

* CAN

* Charger

* Clock control

* Comparator

  * Introduced comparator device driver subsystem selected with :kconfig:option:`CONFIG_COMPARATOR`
  * Introduced comparator shell commands selected with :kconfig:option:`CONFIG_COMPARATOR_SHELL`
  * Added support for Nordic nRF COMP (:dtcompatible:`nordic,nrf-comp`)
  * Added support for Nordic nRF LPCOMP (:dtcompatible:`nordic,nrf-lpcomp`)
  * Added support for NXP Kinetis ACMP (:dtcompatible:`nxp,kinetis-acmp`)

* Counter

* DAC

* Disk

* Display

* Ethernet

  * LiteX: Renamed the ``compatible`` from ``litex,eth0`` to :dtcompatible:`litex,liteeth`.

* Flash

  * Fixed SPI NOR driver issue where wp, hold and reset pins were incorrectly initialized from
    device tee when SFDP at run-time has been enabled (:github:`80383`)

* GNSS

* GPIO

  * tle9104: Add support for the parallel output mode via setting the properties ``parallel-out12`` and
    ``parallel-out34``.

* Hardware info

* I2C

* I2S

* I3C

* Input

* LED

  * lp5562: added ``enable-gpios`` property to describe the EN/VCC GPIO of the lp5562.

  * lp5569: added ``charge-pump-mode`` property to configure the charge pump of the lp5569.

  * lp5569: added ``enable-gpios`` property to describe the EN/PWM GPIO of the lp5569.

  * LED code samples have been consolidated under the :zephyr_file:`samples/drivers/led` directory.

* LED Strip

  * Updated ws2812 GPIO driver to support dynamic bus timings

* LoRa

* Mailbox

* MDIO

* MFD

* Modem

  * Added support for the U-Blox LARA-R6 modem.
  * Added support for setting the modem's UART baudrate during init.

* MIPI-DBI

* MSPI

* Pin control

* PWM

  * rpi_pico: The driver now configures the divide ratio adaptively.

* Regulators

* Reset

* RTC

* RTIO

* SDHC

* Sensors

  * The existing driver for the Microchip MCP9808 temperature sensor transformed and renamed
    to support all JEDEC JC 42.4 compatible temperature sensors. It now uses the
    :dtcompatible:`jedec,jc-42.4-temp` compatible string instead to the ``microchip,mcp9808``
    string.

  * WE

    * Added Würth Elektronik HIDS-2525020210002
      :dtcompatible:`we,wsen-hids-2525020210002` humidity sensor driver.

* Serial

  * LiteX: Renamed the ``compatible`` from ``litex,uart0`` to :dtcompatible:`litex,uart`.
  * Nordic: Removed ``CONFIG_UART_n_GPIO_MANAGEMENT`` Kconfig options (where n is an instance
    index) which had no use after pinctrl driver was introduced.

* SPI

* Steppers

  * Introduced stepper controller device driver subsystem selected with
    :kconfig:option:`CONFIG_STEPPER`
  * Introduced stepper shell commands for controlling and configuring
    stepper motors with :kconfig:option:`CONFIG_STEPPER_SHELL`
  * Added support for ADI TMC5041 (:dtcompatible:`adi,tmc5041`)
  * Added support for gpio-stepper-controller (:dtcompatible:`gpio-stepper-controller`)
  * Added stepper api test-suite
  * Added stepper shell test-suite

* USB

* Video

* Watchdog

* Wi-Fi

Networking
**********

* ARP:

* CoAP:

* Connection manager:

* DHCPv4:

* DHCPv6:

* DNS/mDNS/LLMNR:

* gPTP/PTP:

* HTTP:

* IPSP:

* IPv4:

* IPv6:

* LwM2M:
  * Location object: optional resources altitude, radius, and speed can now be
  used optionally as per the location object's specification. Users of these
  resources will now need to provide a read buffer.

  * lwm2m_senml_cbor: Regenerated generated code files using zcbor 0.9.0

* Misc:

* MQTT:

* Network Interface:

* OpenThread

* PPP

* Shell:

* Sockets:

* Syslog:

* TCP:

* Websocket:

* Wi-Fi:

* zperf:

USB
***

Devicetree
**********

Kconfig
*******

Libraries / Subsystems
**********************

* Debug

* Demand Paging

* Formatted output

* Management

  * MCUmgr

    * Added support for :ref:`mcumgr_smp_group_10`, which allows for listing information on
      supported groups.
    * Fixed formatting of milliseconds in :c:enum:`OS_MGMT_ID_DATETIME_STR` by adding
      leading zeros.
    * Added support for custom os mgmt bootloader info responses using notification hooks, this
      can be enabled with :kconfig:option:`CONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO_HOOK`, the data
      structure is :c:struct:`os_mgmt_bootloader_info_data`.
    * Added support for img mgmt slot info command, which allows for listing information on
      images and slots on the device.
    * Added support for LoRaWAN MCUmgr transport, which can be enabled with
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_LORAWAN`.

  * hawkBit

    * :c:func:`hawkbit_autohandler` now takes one argument. If the argument is set to true, the
      autohandler will reshedule itself after running. If the argument is set to false, the
      autohandler will not reshedule itself. Both variants are sheduled independent of each other.
      The autohandler always runs in the system workqueue.

    * Use the :c:func:`hawkbit_autohandler_wait` function to wait for the autohandler to finish.

    * Running hawkBit from the shell is now executed in the system workqueue.

    * Use the :c:func:`hawkbit_autohandler_cancel` function to cancel the autohandler.

    * Use the :c:func:`hawkbit_autohandler_set_delay` function to delay the next run of the
      autohandler.

    * The hawkBit header file was separated into multiple header files. The main header file is now
      ``<zephyr/mgmt/hawkbit/hawkbit.h>``, the autohandler header file is now
      ``<zephyr/mgmt/hawkbit/autohandler.h>`` and the configuration header file is now
      ``<zephyr/mgmt/hawkbit/config.h>``.

* Logging

* Modem modules

* Power management

* Crypto

  * Mbed TLS was updated to version 3.6.2 (from 3.6.0). The release notes can be found at:

    * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.1
    * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.2

  * The Kconfig symbol :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG_ALLOW_NON_CSPRNG`
    was added to allow ``psa_get_random()`` to make use of non-cryptographically
    secure random sources when :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG`
    is also enabled. This is only meant to be used for test purposes, not in production.
    (:github:`76408`)
  * The Kconfig symbol :kconfig:option:`CONFIG_MBEDTLS_TLS_VERSION_1_3` was added to
    enable TLS 1.3 support from Mbed TLS. When this is enabled the following
    new Kconfig symbols can also be enabled:

    * :kconfig:option:`CONFIG_MBEDTLS_TLS_SESSION_TICKETS` to enable session tickets
      (RFC 5077);
    * :kconfig:option:`CONFIG_MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED`
      for TLS 1.3 PSK key exchange mode;
    * :kconfig:option:`CONFIG_MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED`
      for TLS 1.3 ephemeral key exchange mode;
    * :kconfig:option:`CONFIG_MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED`
      for TLS 1.3 PSK ephemeral key exchange mode.

* CMSIS-NN

* FPGA

* Random

* SD

* Settings

  * Settings has been extended to allow prioritizing the commit handlers using
    ``SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(...)`` for static_handlers and
    ``settings_register_with_cprio(...)`` for dynamic_handlers.

* Shell:

  * Reorganized the ``kernel threads`` and ``kernel stacks`` shell command under the
    L1 ``kernel thread`` shell command as ``kernel thread list`` & ``kernel thread stacks``
  * Added multiple shell command to configure the CPU mask affinity / pinning a thread in
    runtime, do ``kernel thread -h`` for more info.
  * ``kernel reboot`` shell command without any additional arguments will now do a cold reboot
    instead of requiring you to type ``kernel reboot cold``.

* State Machine Framework

* Storage

  * LittleFS: The module has been updated with changes committed upstream
    from version 2.8.1, the last module update, up to and including
    the released version 2.9.3.

* Task Watchdog

* POSIX API

* LoRa/LoRaWAN

* ZBus

* JWT (JSON Web Token)

  * The following new symbols were added to allow specifying both the signature
    algorithm and crypto library:

    * :kconfig:option:`CONFIG_JWT_SIGN_RSA_PSA` (default) RSA signature using the PSA Crypto API;
    * :kconfig:option:`CONFIG_JWT_SIGN_RSA_LEGACY` RSA signature using Mbed TLS;
    * :kconfig:option:`CONFIG_JWT_SIGN_ECDSA_PSA` ECDSA signature using the PSA Crypto API.

    (:github:`79653`)

HALs
****

* Nordic

* STM32

* ADI

* Espressif

MCUboot
*******

  * Removed broken target config header feature.
  * Removed ``image_index`` from ``boot_encrypt``.
  * Renamed boot_enc_decrypt to boot_decrypt_key.
  * Updated to use ``EXTRA_CONF_FILE`` instead of the deprecated ``OVERLAY_CONFIG`` argument.
  * Updated ``boot_encrypt()`` to instead be ``boot_enc_encrypt()`` and ``boot_enc_decrypt()``.
  * Updated ``boot_enc_valid`` to take slot instead of image index.
  * Updated ``boot_enc_load()`` to take slot number instead of image.
  * Updated logging to debug level in boot_serial.
  * Updated Kconfig to allow disabling NRFX_WDT on nRF devices.
  * Updated CMake ERROR statements into FATAL_ERROR.
  * Added application version that is being booted output prior to booting it.
  * Added sysbuild support to the hello-world sample.
  * Added SIG_PURE TLV to bootutil.
  * Added write block size checking to bootutil.
  * Added check for unexpected flash sector size.
  * Added SHA512 support to MCUboot code and support for calculating SHA512 hash in imgtool.
  * Added fallback to USB DFU option.
  * Added better mode selection checks to bootutil.
  * Added bootuil protected TLV size to image size check.
  * Added functionaliy to remove images with conflicting flags or where features are required
    that are not supported.
  * Added compressed image flags and TLVs to MCUboot, Kconfig options and support for generating
    compressed LZMA2 images with ARM thumb filter to imgtool.
  * Added image header verification before checking image.
  * Added state to ``boot_is_header_valid()`` function.
  * Added ``CONFIG_MCUBOOT_ENC_BUILTIN_KEY`` Kconfig option.
  * Added non-bootable flag to imgtool.
  * Added zephyr prefix to generated header path.
  * Added optional img mgmt slot info feature.
  * Added bootutil support for maximum image size details for additional images.
  * Added support for automatically calculcating max sectors.
  * Added missing ``boot_enc_init()`` function.
  * Added support for keeping image encrypted in scratch area in bootutil.
  * Fixed serial recovery for NXP IMX.RT, LPC55x and MCXNx platforms
  * Fixed issue with public RSA signing in imgtool.
  * Fixed issue with ``boot_serial_enter()`` being defined but not used warning.
  * Fixed issue with ``main()`` in sample returning wrong type warning.
  * Fixed issue with using pointers in bootutil.
  * Fixed wrong usage of slot numbers in boot_serial.
  * Fixed slot info for directXIP/RAM load in bootutil.
  * Fixed bootutil issue with not zeroing AES and SHA-256 contexts with mbedTLS.
  * Fixed boot_serial ``format`` and ``incompatible-pointer-types`` warnings.
  * Fixed booltuil wrong definition of ``find_swap_count``.
  * Fixed bootutil swap move max app size calculation.
  * Fixed imgtool issue where getpub failed for ed25519 key.
  * Fixed issue with sysbuild if something else is named mcuboot.
  * Fixed RAM load chain load address.
  * Fixed issue with properly retrieving image headers after interrupted swap-scratch in bootutil.
  * The MCUboot version in this release is version ``2.1.0+0-dev``.

OSDP
****

Trusted Firmware-M
******************

LVGL
****

zcbor
*****

* Updated the zcbor library to version 0.9.0.
  Full release notes at https://github.com/NordicSemiconductor/zcbor/blob/0.9.0/RELEASE_NOTES.md
  Migration guide at https://github.com/NordicSemiconductor/zcbor/blob/0.9.0/MIGRATION_GUIDE.md
  Highlights:

    * Many code generation bugfixes

    * You can now decide at run-time whether the decoder should enforce canonical encoding.

    * Allow --file-header to accept a path to a file with header contents

Tests and Samples
*****************

* Together with the deprecation of :ref:`native_posix<native_posix>`, many tests which were
  explicitly run in native_posix now run in :ref:`native_sim<native_sim>` instead.
  native_posix as a platform remains tested though.

Issue Related Items
*******************

Known Issues
============
