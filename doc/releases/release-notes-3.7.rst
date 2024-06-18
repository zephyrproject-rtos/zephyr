:orphan:

.. _zephyr_3.7:

Zephyr 3.7.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.7.0.

Major enhancements with this release include:

* A new, completely overhauled hardware model has been introduced. This changes
  the way both SoCs and boards are named, defined and constructed in Zephyr.
  Additional information can be found in the :ref:`board_porting_guide`.
* Zephyr now requires Python 3.10 or higher
* Trusted Firmware-M (TF-M) 2.1.0 and Mbed TLS 3.6.0 have been integrated into Zephyr.
  Both of these versions are LTS releases.

An overview of the changes required or recommended when migrating your application from Zephyr
v3.6.0 to Zephyr v3.7.0 can be found in the separate :ref:`migration guide<migration_3.7>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2024-3077 `Zephyr project bug tracker GHSA-gmfv-4vfh-2mh8
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gmfv-4vfh-2mh8>`_

* CVE-2024-3332  Under embargo until 2024-07-01

* CVE-2024-4785: Under embargo until 2024-08-07

* CVE-2024-5754: Under embargo until 2024-09-04

* CVE-2024-5931: Under embargo until 2024-09-10

* CVE-2024-6135: Under embargo until 2024-09-11

API Changes
***********

Removed APIs in this release
============================

 * The Bluetooth subsystem specific debug symbols are removed. They have been replaced with the
   Zephyr logging ones.

 * Removed deprecated ``pcie_probe`` and ``pcie_bdf_lookup`` functions from the PCIe APIs.

Deprecated in this release
==========================

 * Bluetooth advertiser options :code:`BT_LE_ADV_OPT_USE_NAME` and
   :code:`BT_LE_ADV_OPT_FORCE_NAME_IN_AD` are now deprecated. That means the following macro are
   deprecated:

    * :c:macro:`BT_LE_ADV_CONN_NAME`
    * :c:macro:`BT_LE_ADV_CONN_NAME_AD`
    * :c:macro:`BT_LE_ADV_NCONN_NAME`
    * :c:macro:`BT_LE_EXT_ADV_CONN_NAME`
    * :c:macro:`BT_LE_EXT_ADV_SCAN_NAME`
    * :c:macro:`BT_LE_EXT_ADV_NCONN_NAME`
    * :c:macro:`BT_LE_EXT_ADV_CODED_NCONN_NAME`

   Application developer will now need to set the advertised name themselves by updating the advertising data
   or the scan response data.

.. _zephyr_3.7_posix_api_deprecations:

 * POSIX API

  * Deprecated :c:macro:`PTHREAD_BARRIER_DEFINE` has been removed.
  * Deprecated :c:macro:`EFD_IN_USE` and :c:macro:`EFD_FLAGS_SET` have been removed.

  * In efforts to use Kconfig options that map directly to the Options and Option Groups in
    IEEE 1003.1-2017, the following Kconfig options have been deprecated (replaced by):

    * :kconfig:option:`CONFIG_EVENTFD_MAX` (:kconfig:option:`CONFIG_ZVFS_EVENTFD_MAX`)
    * :kconfig:option:`CONFIG_FNMATCH` (:kconfig:option:`CONFIG_POSIX_C_LIB_EXT`)
    * :kconfig:option:`CONFIG_GETENTROPY` (:kconfig:option:`CONFIG_POSIX_C_LIB_EXT`)
    * :kconfig:option:`CONFIG_GETOPT` (:kconfig:option:`CONFIG_POSIX_C_LIB_EXT`)
    * :kconfig:option:`CONFIG_MAX_PTHREAD_COUNT` (:kconfig:option:`CONFIG_POSIX_THREAD_THREADS_MAX`)
    * :kconfig:option:`CONFIG_MAX_PTHREAD_KEY_COUNT` (:kconfig:option:`CONFIG_POSIX_THREAD_KEYS_MAX`)
    * :kconfig:option:`CONFIG_MAX_TIMER_COUNT` (:kconfig:option:`CONFIG_POSIX_TIMER_MAX`)
    * :kconfig:option:`CONFIG_POSIX_LIMITS_RTSIG_MAX` (:kconfig:option:`CONFIG_POSIX_RTSIG_MAX`)
    * :kconfig:option:`CONFIG_POSIX_CLOCK` (:kconfig:option:`CONFIG_POSIX_CLOCK_SELECTION`,
      :kconfig:option:`CONFIG_POSIX_CPUTIME`, :kconfig:option:`CONFIG_POSIX_MONOTONIC_CLOCK`,
      :kconfig:option:`CONFIG_POSIX_TIMERS`, and :kconfig:option:`CONFIG_POSIX_TIMEOUTS`)
    * :kconfig:option:`CONFIG_POSIX_CONFSTR` (:kconfig:option:`CONFIG_POSIX_SINGLE_PROCESS`)
    * :kconfig:option:`CONFIG_POSIX_ENV` (:kconfig:option:`CONFIG_POSIX_SINGLE_PROCESS`)
    * :kconfig:option:`CONFIG_POSIX_FS` (:kconfig:option:`CONFIG_POSIX_FILE_SYSTEM`)
    * :kconfig:option:`CONFIG_POSIX_MAX_FDS` (:kconfig:option:`CONFIG_POSIX_OPEN_MAX` and
      :kconfig:option:`CONFIG_ZVFS_OPEN_MAX`)
    * :kconfig:option:`CONFIG_POSIX_MAX_OPEN_FILES` (:kconfig:option:`CONFIG_POSIX_OPEN_MAX` and
      :kconfig:option:`CONFIG_ZVFS_OPEN_MAX`)
    * :kconfig:option:`CONFIG_POSIX_MQUEUE` (:kconfig:option:`CONFIG_POSIX_MESSAGE_PASSING`)
    * :kconfig:option:`CONFIG_POSIX_PUTMSG` (:kconfig:option:`CONFIG_XOPEN_STREAMS`)
    * :kconfig:option:`CONFIG_POSIX_SIGNAL` (:kconfig:option:`CONFIG_POSIX_SIGNALS`)
    * :kconfig:option:`CONFIG_POSIX_SYSCONF` (:kconfig:option:`CONFIG_POSIX_SINGLE_PROCESS`)
    * :kconfig:option:`CONFIG_POSIX_SYSLOG` (:kconfig:option:`CONFIG_XSI_SYSTEM_LOGGING`)
    * :kconfig:option:`CONFIG_POSIX_UNAME` (:kconfig:option:`CONFIG_POSIX_SINGLE_PROCESS`)
    * :kconfig:option:`CONFIG_PTHREAD` (:kconfig:option:`CONFIG_POSIX_THREADS`)
    * :kconfig:option:`CONFIG_PTHREAD_BARRIER` (:kconfig:option:`CONFIG_POSIX_BARRIERS`)
    * :kconfig:option:`CONFIG_PTHREAD_COND` (:kconfig:option:`CONFIG_POSIX_THREADS`)
    * :kconfig:option:`CONFIG_PTHREAD_IPC` (:kconfig:option:`CONFIG_POSIX_THREADS`)
    * :kconfig:option:`CONFIG_PTHREAD_KEY` (:kconfig:option:`CONFIG_POSIX_THREADS`)
    * :kconfig:option:`CONFIG_PTHREAD_MUTEX` (:kconfig:option:`CONFIG_POSIX_THREADS`)
    * :kconfig:option:`CONFIG_PTHREAD_RWLOCK` (:kconfig:option:`CONFIG_POSIX_READER_WRITER_LOCKS`)
    * :kconfig:option:`CONFIG_PTHREAD_SPINLOCK` (:kconfig:option:`CONFIG_POSIX_SPIN_LOCKS`)
    * :kconfig:option:`CONFIG_SEM_NAMELEN_MAX` (:kconfig:option:`CONFIG_POSIX_SEM_NAMELEN_MAX`)
    * :kconfig:option:`CONFIG_SEM_VALUE_MAX` (:kconfig:option:`CONFIG_POSIX_SEM_VALUE_MAX`)
    * :kconfig:option:`CONFIG_TIMER` (:kconfig:option:`CONFIG_POSIX_TIMERS`)
    * :kconfig:option:`CONFIG_TIMER_DELAYTIMER_MAX` (:kconfig:option:`CONFIG_POSIX_DELAYTIMER_MAX`)

    Please see the :ref:`POSIX API migration guide <zephyr_3.7_posix_api_migration>`.

 * SPI

  * Deprecated :c:func:`spi_is_ready` API function has been removed.
  * Deprecated :c:func:`spi_transceive_async` API function has been removed.
  * Deprecated :c:func:`spi_read_async` API function has been removed.
  * Deprecated :c:func:`spi_write_async` API function has been removed.

Architectures
*************

* ARC

* ARM

* ARM64

  * Implemented symbol names in the backtraces, enable by selecting :kconfig:option:`CONFIG_SYMTAB`

* RISC-V

  * The fatal error message triggered from a fault now contains the callee-saved-registers states.

  * Implemented stack unwinding

    * Frame-pointer can be selected to enable precise stack traces at the expense of slightly
      increased size and decreased speed.

    * Symbol names can be enabled by selecting :kconfig:option:`CONFIG_EXCEPTION_STACK_TRACE_SYMTAB`

* Xtensa

Kernel
******

  * Added :c:func:`k_uptime_seconds` function to simplify `k_uptime_get() / 1000` usage.

  * Added :c:func:`k_realloc`, that uses kernel heap to implement traditional :c:func:`realloc`
    semantics.

Bluetooth
*********
* Audio

  * Removed ``err`` from :c:struct:`bt_bap_broadcast_assistant_cb.recv_state_removed` as it was
    redundant.

  * The broadcast_audio_assistant sample has been renamed to bap_broadcast_assistant.
    The broadcast_audio_sink sample has been renamed to bap_broadcast_sink.
    The broadcast_audio_source sample has been renamed to bap_broadcast_source.
    The unicast_audio_client sample has been renamed to bap_unicast_client.
    The unicast_audio_server sample has been renamed to bap_unicast_server.
    The public_broadcast_sink sample has been renamed to pbp_public_broadcast_sink.
    The public_broadcast_source sample has been renamed to pbp_public_broadcast_source.

  * The CAP Commander and CAP Initiator now no longer require CAS to be discovered for
    :code:`BT_CAP_SET_TYPE_AD_HOC` sets. This allows applications to use these APIs on e.g.
    BAP Unicast Servers that do not implement the CAP Acceptor role.

* Host

  * Added Nordic UART Service (NUS), enabled by the :kconfig:option:`CONFIG_BT_ZEPHYR_NUS`.
    This Service exposes the ability to declare multiple instances of the GATT service,
    allowing multiple serial endpoints to be used for different purposes.

  * Implemented Hands-free Audio Gateway (AG), enabled by the :kconfig:option:`CONFIG_BT_HFP_AG`.
    It works as a device that is the gateway of the audio. Typical device acting as Audio
    Gateway is cellular phone. It controls the device (Hands-free Unit), that is the remote
    audio input and output mechanism.

  * Implemented Advanced Audio Distribution Profile (A2DP) and Audio/Video Distribution Transport
    Protocol (AVDTP), A2DP is enabled by :kconfig:option:`CONFIG_BT_A2DP`, AVDTP is enabled
    by :kconfig:option:`CONFIG_BT_AVDTP`. They implement the protocols and procedures that
    realize distribution of audio content of high quality in mono, stereo, or multi-channel modes.
    A typical use case is the streaming of music content from a stereo music player to headphones
    or speakers. The audio data is compressed in a proper format for efficient use of the limited
    bandwidth.

  * Reworked the transmission path for data and commands. The "BT TX" thread has been removed, along
    with the buffer pools for HCI fragments and L2CAP segments. All communication with the
    Controller is now exclusively done in the system workqueue context.

* HCI Driver

  * Added support for Ambiq Apollo3 Blue series.

Boards & SoC Support
********************

* Added support for these SoC series:

  * Added support for Ambiq Apollo3 Blue and Apollo3 Blue Plus SoC series.

* Made these changes in other SoC series:

  * ITE: Rename the Kconfig symbol for all ITE SoC variants.

* Added support for these ARM boards:

  * Added support for :ref:`Ambiq Apollo3 Blue board <apollo3_evb>`: ``apollo3_evb``.
  * Added support for :ref:`Ambiq Apollo3 Blue Plus board <apollo3p_evb>`: ``apollo3p_evb``.
  * Added support for :ref:`Raspberry Pi 5 board <rpi_5>`: ``rpi_5``.
  * Added support for :ref:`Seeed Studio XIAO RP2040 board <xiao_rp2040>`: ``xiao_rp2040``.
  * Added support for :ref:`Mikroe RA4M1 Clicker board <mikroe_clicker_ra4m1>`: ``mikroe_clicker_ra4m1``.
  * Added support for :ref:`Arduino UNO R4 WiFi board <arduino_uno_r4>`: : ``arduino_uno_r4_wifi``.

* Added support for these Xtensa boards:

* Made these changes for ARM boards:

* Made these changes for RISC-V boards:

* Made these changes for native/POSIX boards:

  * Introduced the simulated :ref:`nrf54l15bsim<nrf54l15bsim>` target.

  * LLVM fuzzing support has been refactored while adding support for it in native_sim.

* Added support for these following shields:

Build system and Infrastructure
*******************************

  * CI-enabled blackbox tests were added in order to verify correctness of the vast majority of Twister flags.

  * A ``socs`` folder for applications has been introduced that allows for Kconfig fragments and
    devicetree overlays that should apply to any board target using a particular SoC and board
    qualifier.

  * :ref:`Board/SoC flashing configuration<flashing-soc-board-config>` settings have been added.

  * Deprecated the global CSTD cmake property in favor of the :kconfig:option:`CONFIG_STD_C`
    choice to select the C Standard version. Additionally subsystems can select a minimum
    required C Standard version, with for example :kconfig:option:`CONFIG_REQUIRES_STD_C11`.

  * Fixed issue with passing UTF-8 configs to applications using sysbuild.

  * Fixed issue whereby domain file in sysbuild projects would be loaded and used with outdated
    information if sysbuild configuration was changed, and ``west flash`` was ran directly after.

  * Fixed issue with Zephyr modules not being listed in sysbuild if they did not have a Kconfig
    file set.

  * Add sysbuild ``SB_CONFIG_COMPILER_WARNINGS_AS_ERRORS`` Kconfig option to turn on
    "warning as error" toolchain flags for all images, if set.

Drivers and Sensors
*******************

* ADC

* Auxiliary Display

* Audio

* Battery

  * Added ``re-charge-voltage-microvolt`` property to the ``battery`` binding. This allows to set
    limit to automatically start charging again.

* Battery backed up RAM

* CAN

  * Deprecated the :c:func:`can_calc_prescaler` API function, as it allows for bitrate
    errors. Bitrate errors between nodes on the same network leads to them drifting apart after the
    start-of-frame (SOF) synchronization has taken place, leading to bus errors.
  * Added :c:func:`can_get_bitrate_min` and :c:func:`can_get_bitrate_max` for retrieving the minimum
    and maximum supported bitrate for a given CAN controller/CAN transceiver combination, reflecting
    that retrieving the bitrate limits can no longer fail. Deprecated the existing
    :c:func:`can_get_min_bitrate` and :c:func:`can_get_max_bitrate` API functions.
  * Extended support for automatic sample point location to also cover :c:func:`can_calc_timing` and
    :c:func:`can_calc_timing_data`.
  * Added optional ``min-bitrate`` devicetree property for CAN transceivers.
  * Added devicetree macros :c:macro:`DT_CAN_TRANSCEIVER_MIN_BITRATE` and
    :c:macro:`DT_INST_CAN_TRANSCEIVER_MIN_BITRATE` for getting the minimum supported bitrate of a CAN
    transceiver.
  * Added support for specifying the minimum bitrate supported by a CAN controller in the internal
    ``CAN_DT_DRIVER_CONFIG_GET`` and ``CAN_DT_DRIVER_CONFIG_INST_GET`` macros.
  * Added a new CAN controller API function :c:func:`can_get_min_bitrate` for getting the minimum
    supported bitrate of a CAN controller/transceiver combination.
  * Updated the CAN timing functions to take the minimum supported bitrate into consideration when
    validating the bitrate.
  * Made the ``sample-point`` and ``sample-point-data`` devicetree properties optional.
  * Renamed the ``bus_speed`` and ``bus_speed_data`` fields of :c:struct:`can_driver_config` to
    ``bitrate`` and ``bitrate_data``.

* Charger

  * Added ``chgin-to-sys-current-limit-microamp`` property to ``maxim,max20335-charger``.
  * Added ``system-voltage-min-threshold-microvolt`` property to ``maxim,max20335-charger``.
  * Added ``re-charge-threshold-microvolt`` property to ``maxim,max20335-charger``.
  * Added ``thermistor-monitoring-mode`` property to ``maxim,max20335-charger``.

* Clock control

* Counter

  * Added support for Ambiq Apollo3 series.

* Crypto

* Disk

  * Support for eMMC devices was added to the STM32 SD driver. This can
    be enabled with :kconfig:option:`CONFIG_SDMMC_STM32_EMMC`.
  * Added a loopback disk driver, to expose a disk device backed by a file.
    A file can be registered with the loopback disk driver using
    :c:func:`loopback_disk_access_register`
  * Added support for :c:macro:`DISK_IOCTL_CTRL_INIT` and
    :c:macro:`DISK_IOCTL_CTRL_DEINIT` macros, which allow for initializing
    and de-initializing a disk at runtime. This allows hotpluggable
    disk devices (like SD cards) to be removed and reinserted at runtime.

* Display

  * All in tree displays capable of supporting the :ref:`mipi_dbi_api` have
    been converted to use it. GC9X01X, UC81XX, SSD16XX, ST7789V, ST7735R based
    displays have been converted to this API. Boards using these displays will
    need their devicetree updated, see the display section of
    :ref:`migration_3.7` for examples of this process.
  * Added driver for ST7796S display controller (:dtcompatible:`sitronix,st7796s`)
  * Added support for :c:func:`display_read` API to ILI9XXX display driver,
    which can be enabled with :kconfig:option:`CONFIG_ILI9XXX_READ`
  * Added support for :c:func:`display_set_orientation` API to SSD16XXX
    display driver
  * Added driver for NT35510 MIPI-DSI display controller
    (:dtcompatible:`frida,nt35510`)
  * Added driver to abstract LED strip devices as displays
    (:dtcompatible:`led-strip-matrix`)
  * Added support for :c:func:`display_set_pixel_format` API to NXP eLCDIF
    driver. ARGB8888, RGB888, and BGR565 formats are supported.
  * Added support for inverting color at runtime to the SSD1306 driver, via
    the :c:func:`display_set_pixel_format` API.
  * Inversion mode can now be disabled in the ST7789V driver
    (:dtcompatible:`sitronix,st7789v`) using the ``inversion-off`` property.

* DMA

* Entropy

* eSPI

  * Renamed eSPI virtual wire direction macros, enum values and KConfig to match the new
    terminology in eSPI 1.5 specification.

* Ethernet

  * Deperecated eth_mcux driver in favor of the reworked nxp_enet driver.
  * Driver nxp_enet is no longer experimental.
  * All boards and SOCs with :dtcompatible:`nxp,kinetis-ethernet` compatible nodes
    reworked to use the new :dtcompatible:`nxp,enet` binding.

* Flash

  * Added support for Ambiq Apollo3 series.

* GNSS

* GPIO

  * Added support for Ambiq Apollo3 series.
  * Added Broadcom Set-top box(brcmstb) SoC GPIO driver.

* I2C

  * Added support for Ambiq Apollo3 series.

* I2S

* I3C

* IEEE 802.15.4

* Input

  * New drivers: :dtcompatible:`adc-keys`, :dtcompatible:`chipsemi,chsc6x`,
    :dtcompatible:`cirque,pinnacle`, :dtcompatible:`futaba,sbus`,
    :dtcompatible:`pixart,pat912x`, :dtcompatible:`pixart,paw32xx`,
    :dtcompatible:`pixart,pmw3610` and :dtcompatible:`sitronix,cf1133`.
  * Migrated :dtcompatible:`holtek,ht16k33` and
    :dtcompatible:`microchip,xec-kbd` from kscan to input subsystem.

* LED Strip

  * The ``chain-length`` and ``color-mapping`` properties have been added to all LED strip
    bindings.


* LoRa

  * Added driver for Reyax LoRa module

* MDIO

* MFD

* Modem

  * Removed deprecated ``GSM_PPP`` driver along with its dts compatible ``zephyr,gsm-ppp``.

  * Removed deprecated ``UART_MUX`` and ``GSM_MUX`` previously used by ``GSM_PPP``.

  * Removed support for dts compatible ``zephyr,gsm-ppp`` from ``MODEM_CELLULAR`` driver.

  * Removed integration with ``UART_MUX`` from ``MODEM_IFACE_UART_INTERRUPT`` module.

  * Removed integration with ``UART_MUX`` from ``MODEM_SHELL`` module.

  * Implemented modem pipelinks in ``MODEM_CELLULAR`` driver for additional DLCI channels
    available by the different modems. This includes generic AT mode DLCI channels, named
    ``user_pipe_<index>`` and DLCI channels reserved for GNSS tunneling named
    ``gnss_pipe``.

  * Added new set of shell commands for sending AT commands directly to a modem using the
    newly implemented modem pipelinks. The implementation of the new shell commands is
    both functional and together with the ``MODEM_CELLULAR`` driver will provide an
    example of how implement and use the modem pipelink module.

* PCIE

* MEMC

* MIPI-DBI

* Pin control

* PWM

* Regulators

* Retained memory

* RTC

  * Added Raspberry Pi Pico RTC driver.

* SMBUS:

* SDHC

  * Added ESP32 SDHC driver (:dtcompatible:`espressif,esp32-sdhc`).
  * Added SDHC driver for Renesas MMC controller (:dtcompatible:`renesas,rcar-mmc`).

* Sensor

  * Added TMP114 driver
  * Added DS18S20 1-wire temperature sensor driver.

* Serial

  * Added driver to support UART over Bluetooth LE using NUS (Nordic UART Service). This driver
    enables using Bluetooth as a transport to all the subsystems that are currently supported by
    UART (e.g: Console, Shell, Logging).

* SPI

  * Added support for Ambiq Apollo3 series general IOM based SPI.
  * Added support for Ambiq Apollo3 BLEIF based SPI, which is specific for internal HCI.

* USB

* W1

* Watchdog

  * Added :kconfig:option:`CONFIG_WDT_NPCX_WARNING_LEADING_TIME_MS` to set the leading warning time
    in milliseconds. Removed no longer used :kconfig:option:`CONFIG_WDT_NPCX_DELAY_CYCLES`.
  * Added support for Ambiq Apollo3 series.

* Wi-Fi

  * Added support for configuring RTS threshold. With this, users can set the RTS threshold value or
    disable the RTS mechanism.

  * Added support for configuring AP parameters. With this, users can set AP parameters at
    build and run time.

  * Added support to configure "max_inactivity" BSS parameter. Users can set this both build and runtime
    duration to control the maximum time duration after which AP may disconnect a STA due to inactivity
    from STA.

  * Added support to configure "inactivity_poll" BSS parameter. Users can set build only AP parameter
    to control whether AP may poll the STA before throwing away STA due to inactivity.

  * Added support to configure "max_num_sta" BSS parameter. Users can set this both build and run time
    parameter to control the maximum numuber of STA entries.

Networking
**********

* DHCPv4:

  * Added support for encapsulated vendor specific options. By enabling
    :kconfig:option:`CONFIG_NET_DHCPV4_OPTION_CALLBACKS_VENDOR_SPECIFIC` callbacks can be
    registered with :c:func:`net_dhcpv4_add_option_vendor_callback` to handle these options after
    being initialised with :c:func:`net_dhcpv4_init_option_vendor_callback`.

  * Added support for the "Vendor class identifier" option. Use the
    :kconfig:option:`CONFIG_NET_DHCPV4_VENDOR_CLASS_IDENTIFIER` to enable it and
    :kconfig:option:`CONFIG_NET_DHCPV4_VENDOR_CLASS_IDENTIFIER_STRING` to set it.

  * The NTP server from the DHCPv4 option can now be used to set the system time. This is done by
    default, if :kconfig:option:`CONFIG_NET_CONFIG_CLOCK_SNTP_INIT` is enabled.

* LwM2M:

  * Added new API function:

    * :c:func:`lwm2m_set_bulk`

  * Added new ``offset`` parameter to :c:type:`lwm2m_engine_set_data_cb_t` callback type.
    This affects post write and validate callbacks as well as some firmware callbacks.

* IPSP:

  * Removed IPSP support. ``CONFIG_NET_L2_BT`` does not exist anymore.

* TCP:

  * ISN generation now uses SHA-256 instead of MD5. Moreover it now relies on PSA APIs
    instead of legacy Mbed TLS functions for hash computation.

* mDNS:

  * Fixed an issue where the mDNS Responder did not work when the mDNS Resolver was also enabled.
    The mDNS Resolver and mDNS Responder can now be used simultaneously.

USB
***

Devicetree
**********

Libraries / Subsystems
**********************

* Debug

  * symtab

   * By enabling :kconfig:option:`CONFIG_SYMTAB`, the symbol table will be
     generated with Zephyr link stage executable on supported architectures.

* Management

  * hawkBit

    * The hawkBit subsystem has been reworked to use the settings subsystem to store the hawkBit
      configuration.

    * By enabling :kconfig:option:`CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME`, the hawkBit settings can
      be configured at runtime. Use the :c:func:`hawkbit_set_config` function to set the hawkBit
      configuration. It can also be set via the hawkBit shell, by using the ``hawkbit set``
      command.

    * When using the hawkBit autohandler and an update is installed, the device will now
      automatically reboot after the installation is complete.

    * By enabling :kconfig:option:`CONFIG_HAWKBIT_CUSTOM_DEVICE_ID`, a callback function can be
      registered to set the device ID. Use the :c:func:`hawkbit_set_device_identity_cb` function to
      register the callback.

    * By enabling :kconfig:option:`CONFIG_HAWKBIT_CUSTOM_ATTRIBUTES`, a callback function can be
      registered to set the device attributes that are sent to the hawkBit server. Use the
      :c:func:`hawkbit_set_custom_data_cb` function to register the callback.

  * MCUmgr

    * Instructions for the deprecated mcumgr go tool have been removed, a list of alternative,
      supported clients can be found on :ref:`mcumgr_tools_libraries`.

* Logging

  * By enabling :kconfig:option:`CONFIG_LOG_BACKEND_NET_USE_DHCPV4_OPTION`, the IP address of the
    syslog server for the networking backend is set by the DHCPv4 Log Server Option (7).

* Modem modules

  * Added modem pipelink module which shares modem pipes globally, allowing device drivers to
    create and set up pipes for the application to use.

  * Simplified the modem pipe module's synchronization mechanism to only protect the callback
    and user data. This matches the actual in-tree usage of the modem pipes.

  * Added ``modem_stats`` module which tracks the usage of buffers throughout the modem
    subsystem.

* Picolibc

* Power management

* Crypto

  * Mbed TLS was updated to 3.6.0. Release notes can be found at:
    https://github.com/Mbed-TLS/mbedtls/releases/tag/v3.6.0
  * When any PSA crypto provider is available in the system
    (:kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT` is enabled), desired PSA features
    must now be explicitly selected through ``CONFIG_PSA_WANT_xxx`` symbols.
  * Choice symbols :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_LEGACY_RNG` and
    :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG` were added in order
    to allow the user to specify how Mbed TLS PSA crypto core should generate random numbers.
    The former option, which is the default, relies on legacy entropy and CTR_DRBG/HMAC_DRBG
    modules, while the latter relies on CSPRNG drivers.
  * :kconfig:option:`CONFIG_MBEDTLS_PSA_P256M_DRIVER_ENABLED` enables support
    for the Mbed TLS's p256-m driver PSA crypto library. This is a Cortex-M SW
    optimized implementation of secp256r1 curve.

* Random

  * Besides the existing :c:func:`sys_rand32_get` function, :c:func:`sys_rand8_get`,
    :c:func:`sys_rand16_get` and :c:func:`sys_rand64_get` are now also available.
    These functions are all implemented on top of :c:func:`sys_rand_get`.

* Retention

* SD

  * SDMMC and SDIO frequency and timing selection logic have been reworked,
    to resolve an issue where a timing mode would not be selected if the
    SDHC device in use did not report support for the maximum frequency
    possible in that mode. Now, if the host controller and card both report
    support for a given timing mode but not the highest frequency that
    mode supports, the timing mode will be selected and configured at
    the reduced frequency (:github:`72705`).

* State Machine Framework

  * The :c:macro:`SMF_CREATE_STATE` macro now always takes 5 arguments.
  * Transition sources that are parents of the state that was run now choose the correct Least
    Common Ancestor for executing Exit and Entry Actions.
  * Passing ``NULL`` to :c:func:`smf_set_state` is now not allowed.

* Storage

  * FAT FS: It is now possible to expose file system formatting functionality for FAT without also
    enabling automatic formatting on mount failure by setting the
    :kconfig:option:`CONFIG_FS_FATFS_MKFS` Kconfig option. This option is enabled by default if
    :kconfig:option:`CONFIG_FILE_SYSTEM_MKFS` is set.

  * FS: It is now possible to truncate a file while opening using :c:func:`fs_open`
    and by passing ``FS_O_TRUNC`` flag.

* POSIX API

* LoRa/LoRaWAN

  * Added the Fragmented Data Block Transport service, which can be enabled via
    :kconfig:option:`CONFIG_LORAWAN_FRAG_TRANSPORT`. In addition to the default fragment decoder
    implementation from Semtech, an in-tree implementation with reduced memory footprint is
    available.

  * Added a sample to demonstrate LoRaWAN firmware-upgrade over the air (FUOTA).

* ZBus

HALs
****

* STM32

MCUboot
*******

  * Fixed memory leak in bootutil HKDF implementation

  * Fixed enforcing TLV entries to be protected

  * Fixed disabling instruction/data caches

  * Fixed estimated image overhead size calculation

  * Fixed issue with swap-move algorithm failing to validate multiple-images

  * Fixed align script error in imgtool

  * Fixed img verify for hex file format in imgtool

  * Fixed issue with reading the flash image reset vector

  * Fixed too-early ``check_config.h`` include in mbedtls

  * Refactored image dependency functions to reduce code size

  * Added MCUboot support for ``ESP32-C6``

  * Added optional MCUboot boot banner

  * Added TLV querying for protected region

  * Added using builtin keys for verification in bootutil

  * Added builtin ECDSA key support for PSA Crypto backend

  * Added ``OVERWRITE_ONLY_KEEP_BACKUP`` option for secondary images

  * Added defines for ``SOC_FLASH_0_ID`` and ``SPI_FLASH_0_ID``

  * The MCUboot version in this release is version ``2.1.0+0-dev``.

Trusted Firmware-M
******************

* TF-M was updated to 2.1.0. Release notes can be found at:
  https://tf-m-user-guide.trustedfirmware.org/releases/2.1.0.html

* Support for MCUboot signature types other than RSA-3072 has been added.
  The type can be chosen with the :kconfig:option:`CONFIG_TFM_MCUBOOT_SIGNATURE_TYPE` Kconfig option.
  Using EC-P256, the new default, reduces flash usage by several KBs compared to RSA.

zcbor
*****

LVGL
****

Tests and Samples
*****************

  * Added snippet for easily enabling UART over Bluetooth LE by passing ``-S nus-console`` during
    ``west build``. This snippet sets the :kconfig:option:`CONFIG_BT_ZEPHYR_NUS_AUTO_START_BLUETOOTH`
    which allows non-Bluetooth samples that use the UART APIs to run without modifications
    (e.g: Console and Logging examples).

  * Removed ``GSM_PPP`` specific configuration overlays from samples ``net/cloud/tagoio`` and
    ``net/mgmt/updatehub``. The ``GSM_PPP`` device driver has been deprecated and removed. The new
    ``MODEM_CELLULAR`` device driver which replaces it uses the native networking stack and ``PM``
    subsystem, which like ethernet, requires no application specific actions to set up networking.

  * Removed ``net/gsm_modem`` sample as the ``GSM_PPP`` device driver it depended on has been
    deprecated and removed. The sample has been replaced by the sample ``net/cellular_modem``
    based on the ``MODEM_CELLULAR`` device driver.
