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

* CVE-2024-8798: Under embargo until 2024-11-22

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

* HCI Drivers

Boards & SoC Support
********************

* Added support for these SoC series:

* Made these changes in other SoC series:

  * NXP S32Z270: Added support for the new silicon cut version 2.0. Note that the previous
    versions (1.0 and 1.1) are no longer supported.

* Added support for these boards:

* Made these board changes:

  * :ref:`native_posix<native_posix>` has been deprecated in favour of
    :ref:`native_sim<native_sim>`.
  * Support for Google Kukui EC board (``google_kukui``) has been dropped.
  * STM32: Deprecated MCO configuration via Kconfig in favour of setting it through devicetree.
    See ``samples/boards/stm32/mco`` sample.
  * Removed the ``nrf54l15pdk`` board, use :ref:`nrf54l15dk_nrf54l15` instead.
  * PHYTEC: ``mimx8mp_phyboard_pollux`` has been renamed to :ref:`phyboard_pollux<phyboard_pollux>`,
    with the old name marked as deprecated.
  * PHYTEC: ``mimx8mm_phyboard_polis`` has been renamed to :ref:`phyboard_polis<phyboard_polis>`,
    with the old name marked as deprecated.

* Added support for the following shields:

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
      can be enabled witbh :kconfig:option:`CONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO_HOOK`, the data
      structure is :c:struct:`os_mgmt_bootloader_info_data`.
    * Added support for img mgmt slot info command, which allows for listing information on
      images and slots on the device.

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

  * The following new Kconfigs were added to specify which library to use for the
    signature:

    * :kconfig:option:`CONFIG_JWT_USE_PSA` (default) use the PSA Crypto API;
    * :kconfig:option:`CONFIG_JWT_USE_LEGACY` use legacy libraries, i.e. TinyCrypt
      for ECDSA and Mbed TLS for RSA.

HALs
****

* Nordic

* STM32

* ADI

* Espressif

MCUboot
*******

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

Issue Related Items
*******************

Known Issues
============
