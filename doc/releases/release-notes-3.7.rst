:orphan:

.. _zephyr_3.7:

Zephyr 3.7.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.7.0.

This release is the last non-maintenance 3.x release and, as such, will be the next
:ref:`Long Term Support (LTS) release <release_process_lts>`.

Major enhancements with this release include:

* A new, completely overhauled hardware model has been introduced. This change the way both SoCs and
  boards are named, defined and constructed in Zephyr.
  Additional information can be found in the :ref:`board_porting_guide`.
* A long-awaited HTTP server library, and associated service API, allow to easily implement HTTP/1.1
  and HTTP/2 servers in Zephyr. Resources can be registered statically or at runtime, and WebSocket
  support is included.
* POSIX support has been extended, with all the Option Requirements of POSIX Subprofiling Option
  Groups now being supported for PSE51, PSE52, and PSE53 profiles.
* Bluetooth Host has been extended with support for the Nordic UART Service (NUS), Hands-free Audio
  Gateway (AG), Advanced Audio Distribution Profile (A2DP), and Audio/Video Distribution Transport
  Protocol (AVDTP).
* Sensor abstraction model has been overhauled to adopt a read-then-decode approach that enables
  more types of sensors and data flows than the previous fetch/get APIs.
* A new LLEXT Extension Development Kit (EDK) makes it easier to develop and integrate custom
  extensions into Zephyr, including outside of the Zephyr tree.
* Native simulator now supports leveraging native host networking stack without having to rely on
  complex setup of the host environment.
* Trusted Firmware-M (TF-M) 2.1.0 and Mbed TLS 3.6.0 have been integrated into Zephyr.
  Both of these versions are LTS releases.
* New documentation pages have been introduced to help developers setup their local development
  environment with Visual Studio Code and CLion.

An overview of the changes required or recommended when migrating your application from Zephyr
v3.6.0 to Zephyr v3.7.0 can be found in the separate :ref:`migration guide<migration_3.7>`.

While you may refer to release notes from previous 3.x releases for a full description, other major
enhancements and changes since previous LTS release, Zephyr 2.7.0, include:

* Added support for Picolibc as the new default C library.
* Added support for the following types of hardware peripherals:

  * 1-Wire
  * Battery Charger
  * Cellular Modem
  * Fuel Gauge
  * GNSS
  * Hardware Spinlock
  * I3C
  * RTC (Real Time Clock)
  * SMBus

* Added support for snippets. Snippets are common configuration settings that can be used across
  platforms.
* Added support for Linkable Loadable Extensions (LLEXT).
* Summary of breaking changes (refer to release notes and migration guides from previous release
  notes for more details):

  * All Zephyr public headers have been moved to :file:`include/zephyr`, meaning they need to be
    prefixed with ``<zephyr/...>`` when included.
  * Pinmux API has been removed. Pin control needs to be used as its replacement, refer to
    :ref:`pinctrl-guide` for more details.

  * The following deprecated or experimental features have been removed:

    * 6LoCAN
    * civetweb module. See Zephyr 3.7's new HTTP server as a replacement.
    * tinycbor module. You may use zcbor as a replacement.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2024-3077 `Zephyr project bug tracker GHSA-gmfv-4vfh-2mh8
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gmfv-4vfh-2mh8>`_

* CVE-2024-3332  `Zephyr project bug tracker GHSA-jmr9-xw2v-5vf4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jmr9-xw2v-5vf4>`_

* CVE-2024-4785: Under embargo until 2024-08-07

* CVE-2024-5754: Under embargo until 2024-09-04

* CVE-2024-5931: Under embargo until 2024-09-10

* CVE-2024-6135: Under embargo until 2024-09-11

* CVE-2024-6258: Under embargo until 2024-09-05

* CVE-2024-6259: Under embargo until 2024-09-12

* CVE-2024-6442: Under embargo until 2024-09-22

* CVE-2024-6443: Under embargo until 2024-09-22

* CVE-2024-6444: Under embargo until 2024-09-22

API Changes
***********

Removed APIs in this release
============================

 * The Bluetooth subsystem specific debug symbols are removed. They have been replaced with the
   Zephyr logging ones.

 * Removed deprecated ``pcie_probe`` and ``pcie_bdf_lookup`` functions from the PCIe APIs.

 * Removed deprecated ``CONFIG_EMUL_EEPROM_AT2X`` Kconfig option.

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

* CAN

  * Deprecated the :c:func:`can_calc_prescaler` API function, as it allows for bitrate
    errors. Bitrate errors between nodes on the same network leads to them drifting apart after the
    start-of-frame (SOF) synchronization has taken place, leading to bus errors.
  * Deprecated the :c:func:`can_get_min_bitrate` and :c:func:`can_get_max_bitrate` API functions in
    favor of :c:func:`can_get_bitrate_min` and :c:func:`can_get_bitrate_max`.
  * Deprecated the :c:macro:`CAN_MAX_STD_ID` and :c:macro:`CAN_MAX_EXT_ID` macros in favor of
    :c:macro:`CAN_STD_ID_MASK` and :c:macro:`CAN_EXT_ID_MASK`.

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

  * Added support to save/restore HiFi AudioEngine registers.

  * Added support to utilize MPU.

  * Added support to automatically generate interrupt handlers.

  * Added support to generate vector table at build time to be included in the linker script.

  * Added kconfig :kconfig:option:`CONFIG_XTENSA_BREAK_ON_UNRECOVERABLE_EXCEPTIONS` to guard
    using break instruction for unrecoverable exceptions. Enabling the break instruction via
    this kconfig may result in an infinite interrupt storm which may hinder debugging efforts.

  * Fixed an issue where passing the 7th argument via syscall was handled incorrectly.

  * Fixed an issue where :c:func:`arch_user_string_nlen` accessing unmapped memory resulted
    in an unrecoverable exception.

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

  * :kconfig:option:`CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER` and
    :kconfig:option:`CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER` now depend on
    :kconfig:option:`CONFIG_BT_CONN` as they do not work without connections.

* HCI Driver

  * Added support for Ambiq Apollo3 Blue series.

Boards & SoC Support
********************

* Added support for these SoC series:

  * Added support for Ambiq Apollo3 Blue and Apollo3 Blue Plus SoC series.
  * Added support for STM32H7R/S SoC series.

* Made these changes in other SoC series:

  * ITE: Rename the Kconfig symbol for all ITE SoC variants.
  * STM32: Enabled ART Accelerator, I-cache, D-cache and prefetch on compatible series.
  * STM32H5: Added support for Stop mode and :kconfig:option:`CONFIG_PM`.
  * STM32WL: Decreased Sub-GHz SPI frequency from 12 to 8MHz.
  * STM32C0: Added support for :kconfig:option:`CONFIG_POWEROFF`.
  * STM32U5: Added support for Stop3 mode.

* Added support for these ARM boards:

  * Added support for :ref:`Ambiq Apollo3 Blue board <apollo3_evb>`: ``apollo3_evb``.
  * Added support for :ref:`Ambiq Apollo3 Blue Plus board <apollo3p_evb>`: ``apollo3p_evb``.
  * Added support for :ref:`Raspberry Pi 5 board <rpi_5>`: ``rpi_5``.
  * Added support for :ref:`Seeed Studio XIAO RP2040 board <xiao_rp2040>`: ``xiao_rp2040``.
  * Added support for :ref:`Mikroe RA4M1 Clicker board <mikroe_clicker_ra4m1>`: ``mikroe_clicker_ra4m1``.
  * Added support for :ref:`Arduino UNO R4 WiFi board <arduino_uno_r4>`: ``arduino_uno_r4_wifi``.
  * Added support for :ref:`Renesas EK-RA8M1 board <ek_ra8m1>`: ``ek_ra8m1``.
  * Added support for :ref:`ST Nucleo H533RE <nucleo_h533re_board>`: ``nucleo_h533re``.
  * Added support for :ref:`ST STM32C0116-DK Discovery Kit <stm32c0116_dk_board>`: ``stm32c0116_dk``.
  * Added support for :ref:`ST STM32H745I Discovery <stm32h745i_disco_board>`: ``stm32h745i_disco``.
  * Added support for :ref:`ST STM32H7S78-DK Discovery <stm32h7s78_dk_board>`: ``stm32h7s78_dk``.
  * Added support for :ref:`ST STM32L152CDISCOVERY board <stm32l1_disco_board>`: ``stm32l152c_disco``.
  * Added support for :ref:`ST STEVAL STWINBX1 Development kit <steval_stwinbx1_board>`: ``steval_stwinbx1``.

* Added support for these Xtensa boards:

* Made these changes for ARM boards:

  * On :ref:`ST STM32H7B3I Discovery Kit <stm32h7b3i_dk_board>`: ``stm32h7b3i_dk_board``,
    enabled full cache management, Chrom-ART, double frame buffer and full refresh for
    optimal LVGL performance.
  * On ST STM32 boards, stm32cubeprogrammer runner can now be used to program external
    flash using ``--extload`` option.

* Made these changes for RISC-V boards:

* Made these changes for native/POSIX boards:

  * Introduced the simulated :ref:`nrf54l15bsim<nrf54l15bsim>` target.

  * The nrf5x bsim targets now support BT LE Coded PHY.

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

  * Fixed issue whereby files used in a project (e.g. devicetree overlays or Kconfig fragments)
    were not correctly watched and CMake would not reconfigure if they were changed.

  * Added flash support for Intel Hex files for the LinkServer runner.

Drivers and Sensors
*******************

* ADC

  * Added support for STM32H7R/S series.

  * Changed phandle type DT property ``nxp,reference-supply`` to phandle-array type DT property
    ``nxp,references`` in ``nxp,lpc-lpadc`` binding. The NXP LPADC driver now supports passing
    the reference voltage value by using ``nxp,references``.

* Auxiliary Display

* Audio

* Battery

  * Added ``re-charge-voltage-microvolt`` property to the ``battery`` binding. This allows to set
    limit to automatically start charging again.

* Battery backed up RAM

  * Added support for STM32G0 and STM32H5 series.

* CAN

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
  * Added a new CAN controller API function :c:func:`can_get_bitrate_min` for getting the minimum
    supported bitrate of a CAN controller/transceiver combination.
  * Updated the CAN timing functions to take the minimum supported bitrate into consideration when
    validating the bitrate.
  * Made the ``sample-point`` and ``sample-point-data`` devicetree properties optional.
  * Renamed the ``bus_speed`` and ``bus_speed_data`` fields of :c:struct:`can_driver_config` to
    ``bitrate`` and ``bitrate_data``.
  * Added driver for :dtcompatible:`nordic,nrf-can`.
  * Added driver support for Numaker M2l31x to the :dtcompatible:`nuvoton,numaker-canfd` driver.
  * Added host communication test suite.

* Charger

  * Added ``chgin-to-sys-current-limit-microamp`` property to ``maxim,max20335-charger``.
  * Added ``system-voltage-min-threshold-microvolt`` property to ``maxim,max20335-charger``.
  * Added ``re-charge-threshold-microvolt`` property to ``maxim,max20335-charger``.
  * Added ``thermistor-monitoring-mode`` property to ``maxim,max20335-charger``.

* Clock control

  * Added support for Microcontroller Clock Output (MCO) on STM32H5 series.
  * Added support for MSI clock on STM32WL series.

* Counter

  * Added support for Ambiq Apollo3 series.
  * Added support for STM32H7R/S series.

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
  * Added SDMMC support for STM32H5 series.

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

  * Added support for STM32H7R/S series.

* EEPROM

  * Added property for specifying ``address-width`` to :dtcompatible:`zephyr,i2c-target-eeprom`.

* eSPI

  * Renamed eSPI virtual wire direction macros, enum values and KConfig to match the new
    terminology in eSPI 1.5 specification.

* Ethernet

  * Deprecated eth_mcux driver in favor of the reworked nxp_enet driver.
  * Driver nxp_enet is no longer experimental.
  * All boards and SOCs with :dtcompatible:`nxp,kinetis-ethernet` compatible nodes
    reworked to use the new :dtcompatible:`nxp,enet` binding.
  * Added support for PTP on compatible STM32 series (STM32F7, STM32H5 and STM32H7).

* Flash

  * Added support for Ambiq Apollo3 series.
  * Added support for multiple instances of the SPI NOR driver (spi_nor.c).
  * Added preliminary support for non-erase devices with introduction of
    device capabilities to c:struct:`flash_parameters` and the utility function
    c:func:`flash_params_get_erase_cap` that allows to obtain the erase type
    provided by a device; added c:macro:`FLASH_ERASE_C_EXPLICIT`, which is
    currently the only supported erase type and is set by all flash devices.
  * Added the c:func:`flash_flatten` function that can be used on devices,
    with or without erase requirement, when erase has been used not for preparing
    a device for a random data write, but rather to remove/scramble data from
    that device.
  * Added the c:func:`flash_fill` utility function which allows to write
    a single value across a provided range in a selected device.
  * Added support for RRAM on nrf54l15 devices.
  * Added support of non busy wait polling in STM32 OSPI driver.
  * Added support for STM32 XSPI external NOR flash driver (:dtcompatible:`st,stm32-xspi-nor`).
  * Added support for XIP on external NOR flash in STM32 OSPI, QSPI and XSPI driver.
  * STM32 OSPI driver: clk, dqs, ncs ports can now be configured by device tree
    configurable (see :dtcompatible:`st,stm32-ospi`).

* GNSS

* GPIO

  * Added support for Ambiq Apollo3 series.
  * Added Broadcom Set-top box(brcmstb) SoC GPIO driver.
  * Added c:macro:`STM32_GPIO_WKUP` flag which allows to configure specific pins as wakeup source
    from Power Off state on STM32 L4, U5, WB, & WL SoC series.

* Hardware info

  * Added device EUI64 ID support and implementation for STM32WB, STM32WBA and STM32WL series.

* I2C

  * Added support for Ambiq Apollo3 series.
  * In STM32 V2 driver, added support for a new :kconfig:option:`CONFIG_I2C_STM32_V2_TIMING`
    which automatically computes bus timings which should be used to configure the hardware
    block depending on the clock configuration in use. To avoid embedding this heavy algorithm
    in a production application, a dedicated sample :zephyr:code-sample:`stm32_i2c_v2_timings` is provided
    to get the output of the algorithm. Once bus timings configuration is available,
    :kconfig:option:`CONFIG_I2C_STM32_V2_TIMING` could be disabled, bus timings configured
    using device tree.
  * Added support for STM32H5 series.

* I2S

  * Added support for STM32H5 series.

* I3C

  * Added shell support for querying bus and CCC commands.

  * Added driver to support the I3C controller on NPCX.

  * Improvements and bug fixes on :dtcompatible:`nxp,mcux-i3c`, including handling the bus
    being busy more gracefully instead of simply returning errors.

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

* Mailbox

  * Added support for HSEM based STM32 driver.

* MDIO

  * Added support for STM32 MDIO controller driver.

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

  * ``pcie_bdf_lookup`` and ``pcie_probe`` have been removed since they have been
    deprecated since v3.3.0.

* MEMC

* MIPI-DBI

* Pin control

* PWM

  * Added support for STM32H7R/S series.

* Regulators

* Reset

  * Added driver for reset controller on Nuvoton NPCX chips.
  * Added reset controller driver for NXP SYSCON.
  * Added reset controller driver for NXP RSTCTL.

* Retained memory

* RTC

  * Added Raspberry Pi Pico RTC driver.
  * Added support for :kconfig:option:`CONFIG_RTC_ALARM` on all STM32 MCU series (except STM32F1).

* SMBUS

* SDHC

  * Added ESP32 SDHC driver (:dtcompatible:`espressif,esp32-sdhc`).
  * Added SDHC driver for Renesas MMC controller (:dtcompatible:`renesas,rcar-mmc`).

* Sensors

  * Added TMP114 driver
  * Added DS18S20 1-wire temperature sensor driver.
  * STM32 QDEC driver now supports encoder mode configuration (see :dtcompatible:`st,stm32-qdec`).
  * Added support for STM32 Digital Temperature Sensor (:dtcompatible:`st,stm32-digi-temp`).

* Serial

  * Added driver to support UART over Bluetooth LE using NUS (Nordic UART Service). This driver
    enables using Bluetooth as a transport to all the subsystems that are currently supported by
    UART (e.g: Console, Shell, Logging).
  * Added :kconfig:option:`CONFIG_NOCACHE_MEMORY` support in async DMA mode in STM32 driver.
    It is now possible to use UART in DMA mode with :kconfig:option:`CONFIG_DCACHE` enabled
    on STM32 F7 & H7 SoC series, as long as DMA buffers are placed in an uncached memory section.
  * Added support for STM32H7R/S series.

  * Added support for HSCIF (High Speed Serial Communication Interface with FIFO) in the UART
    driver for Renesas RCar platforms.

  * Added driver for ENE KB1200 UART.

  * Added driver for UART on Analog Devices MAX32 series microcontrollers.

  * Added driver for UART on Renesas RA8 devices.

  * ``uart_emul`` (:dtcompatible:`zephyr,uart-emul`):

    * Added support for asynchronous API for the emulated UART driver.

  * ``uart_esp32`` (:dtcompatible:`espressif,esp32-uart`):

    * Added support to invert TX and RX pin signals.

    * Added support for ESP32C6 SoC.

  * ``uart_native_tty`` (:dtcompatible:`zephyr,native-tty-uart`):

    * Added support to emulate interrupt driven UART.

  * ``uart_mcux_lpuart`` (:dtcompatible:`nxp,kinetis-lpuart`):

    * Added support for single wire half-duplex communication.

    * Added support to invert TX and RX pin signals.

  * ``uart_npcx`` (:dtcompatible:`nuvoton,npcx-uart`):

    * Added support for asynchronous API.

    * Added support for baud rate of 3MHz.

  * ``uart_nrfx_uarte`` (:dtcompatible:`nordic,nrf-uarte`):

    * Added support to put TX and RX pins into low power mode when UART is not active.

  * ``uart_nrfx_uarte2`` (:dtcompatible:`nordic,nrf-uarte`):

    * Prevents UART from transmitting when device is suspended.

    * Fixed some events not being triggered.

  * ``uart_pl011`` (:dtcompatible:`arm,pl011`):

    * Added support for runtime configuration.

    * Added support for reset device.

    * Added support to use clock control to determine frequency.

    * Added support for hardware flow control.

    * Added support for UART on Ambiq Apollo3 SoC.

  * ``uart_smartbond`` (:dtcompatible:`renesas,smartbond-uart`):

    * Added support for power management.

    * Added support to wake up via DTR and RX lines.

  * ``uart_stm32`` (:dtcompatible:`st,stm32-uart`):

    * Added support to identify if DMA buffers are in data cache or non-cacheable memory.

* SPI

  * Added support for Ambiq Apollo3 series general IOM based SPI.
  * Added support for Ambiq Apollo3 BLEIF based SPI, which is specific for internal HCI.
  * Added support for :kconfig:option:`CONFIG_PM` and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` on STM32 SPI driver.
  * Added support for :kconfig:option:`CONFIG_NOCACHE_MEMORY` in DMA SPI mode for STM32F7x SoC series.
  * Added support for STM32H7R/S series.

* USB

* Video

  * Added support for STM32 Digital camera interface (DCMI) driver (:dtcompatible:`st,stm32-dcmi`).

* W1

* Watchdog

  * Added :kconfig:option:`CONFIG_WDT_NPCX_WARNING_LEADING_TIME_MS` to set the leading warning time
    in milliseconds. Removed no longer used :kconfig:option:`CONFIG_WDT_NPCX_DELAY_CYCLES`.
  * Added support for Ambiq Apollo3 series.
  * Added support for STM32H7R/S series.

* Wi-Fi

  * Fixed message parsing for esp-at.
  * Fixed esp-at connect failures.
  * Implement :c:func:`bind` and :c:func:`recvfrom` for UDP sockets for esp-at.
  * Added option for setting maximum data size for eswifi.

Networking
**********

* ARP:

  * Added support for gratuitous ARP transmission.
  * Fixed a possible deadlock between TX and RX threads within ARP module.
  * Fixed a possible ARP entry leak.
  * Improved ARP debug logs.

* CoAP:

  * Fixed CoAP observe age overflows.
  * Increased upper limit for CoAP retransmissions (:kconfig:option:`CONFIG_COAP_MAX_RETRANSMIT`).
  * Fixed CoAP observations in CoAP client library.
  * Added new CoAP client :c:func:`coap_client_cancel_requests` API which allows
    to cancel active observations.
  * Fixed CoAP ID generation for responses in CoAP Server sample.

* Connection manager:

  * Added support for new net_mgmt events, which allow to track IPv4 and IPv6
    connectivity independently:

    * :c:macro:`NET_EVENT_L4_IPV4_CONNECTED`
    * :c:macro:`NET_EVENT_L4_IPV4_DISCONNECTED`
    * :c:macro:`NET_EVENT_L4_IPV6_CONNECTED`
    * :c:macro:`NET_EVENT_L4_IPV6_DISCONNECTED`

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
  * The syslog server address can now be set with DHCPv4 option. This is done by
    default, if :kconfig:option:`CONFIG_LOG_BACKEND_NET_USE_DHCPV4_OPTION` is enabled.
  * Fixed a bug, where options with registered callbacks were not requested from
    the server.
  * Fixed a bug, where netmask received from the server was not applied correctly.
  * Reimplemented DHCPv4 client RENEW/REBIND logic to be compliant with RFC2131.
  * Improved declined addresses management in DHCPv4 server, which now can be
    reused after configured time.
  * Fixed including the client ID option in the DHCPv4 server response, according to RFC6842.
  * Added :kconfig:option:`CONFIG_NET_DHCPV4_SERVER_NAK_UNRECOGNIZED_REQUESTS` which
    allows to override RFC-defined behavior, and NAK requests from unrecognized
    clients.
  * Other minor fixes in DHCPv4 client and server implementations.

* DHCPv6:

  * Fixed incorrect DHCPv6 events code base for net_mgmt events.
  * Added :kconfig:option:`CONFIG_NET_DHCPV6_DUID_MAX_LEN` which allows to configure
    maximum supported DUID length.
  * Added documentation page for DHCPv6.

* DNS/mDNS/LLMNR:

  * Fixed an issue where the mDNS Responder did not work when the mDNS Resolver was also enabled.
    The mDNS Resolver and mDNS Responder can now be used simultaneously.
  * Reworked LLMNR and mDNS responders, and DNS resolver to use sockets and socket services API.
  * Added ANY query resource type.
  * Added support for mDNS to provide records in runtime.
  * Added support for caching DNS records.
  * Fixed error codes returned when socket creation fails, and when all results have been returned.
  * Fixed DNS retransmission timeout calculation.

* gPTP/PTP:

  * Added support for IEEE 1588-2019 PTP.
  * Added support for SO_TIMESTAMPING socket option to get timestamping information in socket
    ancillary data.
  * Fixed race condition on timestamp callback.
  * Fixed clock master sync send SM if we are not the GM clock.

* HTTP:

  * Added HTTP/2 server library and sample application with support for static,
    dynamic and Websocket resource types.
  * Added HTTP shell component.
  * Improved HTTP client error reporting.
  * Moved HTTP client library out of experimental.

* IPSP:

  * Removed IPSP support. ``CONFIG_NET_L2_BT`` does not exist anymore.

* IPv4:

  * Implemented IPv4 Address Conflict Detection, according to RFC 5227.
  * Added :c:func:`net_ipv4_is_private_addr` API function.
  * IPv4 netmask is now set individually for each address instead of being set
    for the whole interface.
  * Other minor fixes and improvements.

* IPv6:

  * Implemented IPv6 Privacy Extensions according to RFC 8981.
  * Added :c:func:`net_ipv6_is_private_addr` API function.
  * Implemented reachability hint for IPv6. Upper layers can use
    c:func:`net_if_nbr_reachability_hint` to report Neighbor reachability and
    avoid unnecessary Neighbor Discovery solicitations.
  * Added :kconfig:option:`CONFIG_NET_IPV6_MTU` allowing to set custom IPv6 MTU.
  * Added :kconfig:option:`CONFIG_NET_MCAST_ROUTE_MAX_IFACES` which allows to set
    multiple interfaces for multicast forwarding entries.
  * Added :kconfig:option:`CONFIG_NET_MCAST_ROUTE_MLD_REPORTS` which allows to
    report multicast routes in MLDv2 reports.
  * Fixed IPv6 hop limit handling for multicast packets.
  * Improved IPv6 Neighbor Discovery test coverage.
  * Fixed a bug, where Neighbor Advertisement packets reporting Duplicate address
    detection conflict were dropped.
  * Other minor fixes and improvements.

* LwM2M:

  * Added new API functions:

    * :c:func:`lwm2m_set_bulk`
    * :c:func:`lwm2m_rd_client_set_ctx`

  * Added new ``offset`` parameter to :c:type:`lwm2m_engine_set_data_cb_t` callback type.
    This affects post write and validate callbacks as well as some firmware callbacks.
  * Fixed block context not being reset upon receiving block number 0 in block transfer.
  * Fixed block size negotiation with the server in block transfer.
  * Added :kconfig:option:`CONFIG_LWM2M_ENGINE_ALWAYS_REPORT_OBJ_VERSION` which
    allows to force the client to always report object version.
  * Block transfer is now possible with resource w/o registered callback.
  * Fixed a bug, where an empty ACK sent from the registered callback would not
    be sent immediately.
  * Removed deprecated API functions and definitions.
  * Other minor fixes and improvements.

* Misc:

  * Improved overall networking API doxygen documentation.
  * Converted TFTP library to use ``zsock_*`` API.
  * Added SNTP :c:func:`sntp_simple_addr` API function to perform SNTP query
    when the server IP address is already known.
  * Added :kconfig:option:`CONFIG_NET_TC_THREAD_PRIO_CUSTOM` allowing to override
    default traffic class threads priority.
  * Fixed the IPv6 event handler initialization order in net config library.
  * Reworked telnet shell backend to use sockets and socket services API.
  * Fixed double dereference of IGMP packets.
  * Moved from ``native_posix`` to ``native_sim`` support in various tests and
    samples.
  * Added support for copying user data in network buffers.
  * Fixed cloning of zero sized network buffers.
  * Added net_buf APIs to handle 40 bit data format.
  * Added receive callback for dummy L2, applicable in certain use cases
    (for example packet capture).
  * Implemented pseudo interface, a.k.a "any" interface for packet capture use
    case.
  * Added cooded mode capture support. This allows non-IP based network data capture.
  * Generate network events when starting or stopping packet capture.
  * Removed obsolete and unused ``tcp_first_msg`` :c:struct:`net_pkt` flag.
  * Added new :zephyr:code-sample:`secure-mqtt-sensor-actuator` sample.
  * Added support for partial L3 and L4 checksum offloading.
  * Updated :zephyr:code-sample:`mqtt-azure` with new CA certificates, the current
    on expires soon.
  * Added new driver for Native Simulator offloaded sockets.
  * Overhauled VLAN support to use Virtual network interfaces.
  * Added statistics collection for Virtual network interfaces.

* MQTT:

  * Added ALPN support for MQTT TLS backend.
  * Added user data field in :c:struct:`mqtt_client` context structure.

* Network Interface:

  * Added new API functions:

    * :c:func:`net_if_ipv4_maddr_foreach`
    * :c:func:`net_if_ipv6_maddr_foreach`

  * Improved debug logging in the network interface code.
  * Added reference counter to the :c:struct:`net_if_addr` structure.
  * Fixed IPv6 DAD and MLDv2 operation when interface goes up.
  * Other minor fixes.

* OpenThread

 * Removed deprecated ``openthread_set_state_changed_cb()`` function.
 * Added implementation of BLE TCAT advertisement API.

* PPP

  * Removed deprecated ``gsm_modem`` driver and sample.
  * Optimized memory allocation in PPP driver.
  * Misc improvements in the :zephyr:code-sample:`cellular-modem` sample
  * Added PPP low level packet capture support.

* Shell:

  * Added ``net ipv4 gateway`` command to set IPv4 gateway address.
  * Added argument validation in network shell macros.
  * Fixed net_mgmt sockets information printout.
  * Reworked VLAN information printout.
  * Added option to set random MAC address with ``net iface set_mac`` command.
  * Added multicast join status when printing multicast address information.

* Sockets:

  * Implemented new networking POSIX APIs:

    * :c:func:`if_nameindex`
    * :c:func:`inet_ntoa`
    * :c:func:`inet_addr`

  * Added support for tracing socket API calls.
  * TLS sockets are no longer experimental API.
  * Fixed the protocol field endianness for ``AF_PACKET`` type sockets.
  * Fixed :c:func:`getsockname` for TCP.
  * Improve :c:func:`sendmsg` support when using DTLS sockets.

* Syslog:

  * Added new API functions:

    * :c:func:`log_backend_net_set_ip` to initialize syslog net backend with IP
      address directly.
    * :c:func:`log_backend_net_start` to facilitate syslog net backend activation.

  * Added structured logging support to syslog net backend.
  * Added TCP support to syslog net backend.

* TCP:

  * Fixed possible deadlock when accepting new TCP connection.
  * Fixed ACK number verification during connection teardown.
  * Fixed a bug, where data bytes included in FIN packet were ignored.
  * Fixed a possible TCP context leak in case initial SYN packet transmission failed.
  * Deprecated :kconfig:option:`CONFIG_NET_TCP_ACK_TIMEOUT` as it was redundant with other configs.
  * Improved debug logs, so that they're easier to follow under heavy load.
  * ISN generation now uses SHA-256 instead of MD5. Moreover it now relies on PSA APIs
    instead of legacy Mbed TLS functions for hash computation.

* Websocket:

  * Added new Websocket APIs:

    * :c:func:`websocket_register`
    * :c:func:`websocket_unregister`

  * Converted Websocket library to use ``zsock_*`` API.
  * Added Object Core support to Websocket sockets.

* Wi-Fi:

  * Reduce memory usage of 5 GHz channel list.
  * Added channel validity check in AP mode.
  * Added support for BSSID configuration in connect call.
  * Wifi shell help text fixes. Option parsing fixes.
  * Support WPA auto personal security mode.
  * Collect unicast received/sent network packet statistics.
  * Added support for configuring RTS threshold. With this, users can set the RTS threshold
    value or disable the RTS mechanism.
  * Added support for configuring AP parameters. With this, users can set AP parameters at
    build and run time.
  * Added support to configure ``max_inactivity`` BSS parameter. Users can set this both
    build and runtime duration to control the maximum time duration after which AP may
    disconnect a STA due to inactivity from STA.
  * Added support to configure ``inactivity_poll`` BSS parameter. Users can set build
    only AP parameter to control whether AP may poll the STA before throwing away STA
    due to inactivity.
  * Added support to configure ``max_num_sta`` BSS parameter. Users can set this both
    build and run time parameter to control the maximum number of STA entries.

* zperf:

  * Fixed ``IP_TOS`` and ``IPV6_TCLASS`` options handling in zperf.
  * Fixed throughput calculation during long zperf sessions.
  * Fixed error on TCP upload session end in case multicast IP address was used.
  * Fixed a bug, where IPv6 socket was bound with IPv4 address, giving error.
  * Added an option to specify the network interface to use during zperf sessions.
  * Added a new ``ZPERF_SESSION_PERIODIC_RESULT`` event for periodic updates
    during TCP upload sessions.
  * Fixed possible socket leak in case of errors during zperf session.

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

* CMSIS-NN

  * CMSIS-NN was updated to v6.0.0 from v4.1.0:
    https://arm-software.github.io/CMSIS-NN/latest/rev_hist.html

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

  * Flash Map: TinyCrypt has been replaced with PSA Crypto in Flash Area integrity check.

  * Flash Map: :c:func:`flash_area_flatten` has been added to be used where an erase
    operation has been previously used for removing/scrambling data rather than
    to prepare a device for a random data write.

  * Flash Map: :c:macro:`FIXED_PARTITION_NODE_OFFSET`, :c:macro:`FIXED_PARTITION_NODE_SIZE`
    and :c:macro:`FIXED_PARTITION_NODE_DEVICE` have been added to allow obtaining
    fixed partition information from a devicetree node rather than a label.

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

  * Updated STM32F0 to cube version V1.11.5.
  * Updated STM32F3 to cube version V1.11.5.
  * Updated STM32F4 to cube version V1.28.0.
  * Updated STM32F7 to cube version V1.17.2.
  * Updated STM32G0 to cube version V1.6.2.
  * Updated STM32G4 to cube version V1.5.2.
  * Updated STM32H5 to cube version V1.2.0.
  * Updated STM32H7 to cube version V1.11.2.
  * Updated STM32L5 to cube version V1.5.1.
  * Updated STM32U5 to cube version V1.5.0.
  * Updated STM32WB to cube version V1.19.1.
  * Updated STM32WBA to cube version V1.3.1.
  * Added STM32H7R/S with cube version V1.0.0.

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

LVGL was updated to 8.4.0. Release notes can be found at:
https://docs.lvgl.io/8.4/CHANGELOG.html#v8-4-0-19-march-2024

Additionally, the following changes in Zephyr were done:

  * Added support to place memory pool buffers in ``.lvgl_heap`` section by enabling
    :kconfig:option:`CONFIG_LV_Z_MEMORY_POOL_CUSTOM_SECTION`

  * Removed kscan-based pointer input wrapper code.

  * Corrected encoder button behavior to emit ``LV_KEY_ENTER`` events correctly.

  * Improved handling for :samp:`invert-{x,y}` and ``swap-xy`` configurations.

  * Added ``LV_MEM_CUSTOM_FREE`` call on file closure.

  * Added missing Kconfig stubs for DMA2D symbols.

  * Integrated support for LVGL rounder callback function.

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

  * BT LE Coded PHY is now runtime tested in CI with the nrf5x bsim targets.

Issue Related Items
*******************

Known Issues
============

- :github:`74345` - Bluetooth: Non functional on nRF51 with fault
