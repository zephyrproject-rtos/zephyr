:orphan:

.. _zephyr_3.4:

Zephyr 3.4.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.4.0.

Major enhancements with this release include:

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

API Changes
***********

Changes in this release
=======================

* Any applications using the mcuboot image manager
  (:kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER`) will now need to also select
  :kconfig:option:`CONFIG_FLASH_MAP` and :kconfig:option:`CONFIG_STREAM_FLASH`,
  this prevents a cmake dependency loop if the image manager Kconfig is enabled
  manually without also manually enabling the other options.

* Including hawkbit in an application now requires additional Kconfig options
  to be selected, previously these options would have been selected
  automatically but have changed from ``select`` options in Kconfig files to
  ``depends on``:

   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NVS`                     |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_FLASH`                   |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_FLASH_MAP`               |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_STREAM_FLASH`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_REBOOT`                  |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_HWINFO`                  |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_TCP`                 |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_SOCKETS`             |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_IMG_MANAGER`             |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NETWORKING`              |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_HTTP_CLIENT`             |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_DNS_RESOLVER`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_JSON_LIBRARY`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`      |
   +--------------------------------------------------+

* Including updatehub in an application now requires additional Kconfig options
  to be selected, previously these options would have been selected
  automatically but have changed from ``select`` options in Kconfig files to
  ``depends on``:

   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_FLASH`                   |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_STREAM_FLASH`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_FLASH_MAP`               |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_REBOOT`                  |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER`     |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_IMG_MANAGER`             |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_IMG_ENABLE_IMAGE_CHECK`  |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`      |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_MPU_ALLOW_FLASH_WRITE`   |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NETWORKING`              |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_UDP`                 |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_SOCKETS`             |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_COAP`                    |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_DNS_RESOLVER`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_JSON_LIBRARY`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_HWINFO`                  |
   +--------------------------------------------------+

* The sensor driver API clarified :c:func:`sensor_trigger_set` to state that
  the user-allocated sensor trigger will be stored by the driver as a pointer,
  rather than a copy, and passed back to the handler. This enables the handler
  to use :c:macro:`CONTAINER_OF` to retrieve a context pointer when the trigger
  is embedded in a larger struct and requires that the trigger is not allocated
  on the stack. Applications that allocate a sensor trigger on the stack need
  to be updated.

* Converted few drivers to the :ref:`input` subsystem.

  * ``gpio_keys``: moved out of ``gpio``, replaced the custom API to use input
    events instead, the :dtcompatible:`zephyr,gpio-keys` binding is unchanged
    but now requires ``zephyr,code`` to be set.
  * ``ft5336``: moved from :ref:`kscan_api` to :ref:`input`, renamed the Kconfig
    options from ``CONFIG_KSCAN_FT5336``, ``CONFIG_KSCAN_FT5336_PERIOD`` and
    ``KSCAN_FT5336_INTERRUPT`` to :kconfig:option:`CONFIG_INPUT_FT5336`,
    :kconfig:option:`CONFIG_INPUT_FT5336_PERIOD` and
    :kconfig:option:`CONFIG_INPUT_FT5336_INTERRUPT`.
  * ``kscan_sdl``: moved from :ref:`kscan_api` to :ref:`input`, renamed the Kconfig
    option from ``KSCAN_SDL`` to :kconfig:option:`CONFIG_INPUT_SDL_TOUCH` and the
    compatible from ``zephyr,sdl-kscan`` to
    :dtcompatible:`zephyr,input-sdl-touch`.
  * ``nuvoton,npcx-kscan`` moved to :ref:`input`, renamed the Kconfig option
    names from ``KSCAN_NPCX_...`` to ``INPUT_NPCX_KBD...`` and the compatible
    from ``nuvoton,npcx-kscan`` to :dtcompatible:`nuvoton,npcx-kbd`.
  * Touchscreen drivers converted to use the input APIs can use the
    :dtcompatible:`zephyr,kscan-input` driver to maintain Kscan compatilibity.

* The declaration of :c:func:`main` has been changed from ``void
  main(void)`` to ``int main(void)``. The main function is required to
  return the value zero. All other return values are reserved. This aligns
  Zephyr with the C and C++ language specification requirements for
  "hosted" environments, avoiding compiler warnings and errors. These
  compiler messages are generated when applications are built in "hosted"
  mode (which means without the ``-ffreestanding`` compiler flag). As the
  ``-ffreestanding`` flag is currently enabled unless the application is
  using picolibc, only applications using picolibc will be affected by this
  change at this time.

* The following network interface APIs now take additional,
  ``struct net_if * iface`` parameter:

  * :c:func:`net_if_ipv4_maddr_join`
  * :c:func:`net_if_ipv4_maddr_leave`
  * :c:func:`net_if_ipv6_maddr_join`
  * :c:func:`net_if_ipv6_maddr_leave`

* MCUmgr transports now need to set up the struct before registering it by
  setting the function pointers to the function handlers, these have been
  moved to a ``functions`` struct object of type
  :c:struct:`smp_transport_api_t`. Because of these changes, the legacy
  transport registration function and object are no longer available. The
  registration function now returns a value which is 0 for success or a
  negative error code if an error occurred.

* Added a new flag :c:struct:`dac_channel_cfg` ``buffered`` for DAC channels in
  :c:struct:`dac_channel_cfg` to allow the configuration of the output buffer.
  The actual interpretation of this depends on the hardware and is so far only
  implemented for the STM32 DAC driver. Implicitly for this driver this changes
  the default from being buffered to unbuffered.

* MCUmgr fs_mgmt group's file access hook is now called for all fs_mgmt group
  functions (adding support for file status and file hash/checksum). In
  addition, if the file access state is not lost, it will now only be called
  once for the file access instead of each time a command is received.
  Note that the structure for the notification has changed, the ``upload`` bool
  has been replaced with an enum to indicate what function is used, see
  :c:struct:`fs_mgmt_file_access` for the new structure definition.

* Iterable sections API is now available at
  :zephyr_file:`include/zephyr/sys/iterable_sections.h`. LD linker snippets are
  available at :zephyr_file:`include/zephyr/linker/iterable_sections.h`.

* Cache API functions are now fully inlined by compilers.

* The Bluetooth HCI headers have been reworked, with ``hci.h`` now containing
  only the function prototypes and the new ``hci_types.h`` defining all
  HCI-related macros and structs. The previous ``hci_err.h`` has been merged
  into ``hci_types.h``.

Removed APIs in this release
============================

* Pinmux API has been removed. Pin control needs to be used as its replacement,
  refer to :ref:`pinctrl-guide` for more details.

Deprecated in this release
==========================

* Configuring applications with ``prj_<board>.conf`` files has been deprecated,
  this should be replaced by using a prj.conf with the common configuration and
  board-specific configuration in board Kconfig fragments in the ``boards``
  folder of the application.

* On nRF51 and nRF52-based boards, the behaviour of the reset reason being
  provided to :c:func:`sys_reboot` and being set in the GPREGRET register has
  been dropped. This function will now just reboot the device without changing
  the register contents. The new method for setting this register uses the boot
  mode feature of the retention subsystem, see the
  :ref:`boot mode API <boot_mode_api>` for details. To restore the deprecated
  functionality, enable
  :kconfig:option:`CONFIG_NRF_STORE_REBOOT_TYPE_GPREGRET`.

* Deprecated :c:macro:`PTHREAD_BARRIER_DEFINE` in favor of the standardized
  :c:func:`pthread_barrier_init`

* On all STM32 targets except STM32F2 series, ethernet drivers implementation
  based on STM32Cube Ethernet API V1 (:kconfig:option:`CONFIG_ETH_STM32_HAL_API_V1`)
  is now deprecated in favor of implementation based on more reliable and performant
  STM32Cube Ethernet API V2.

Stable API changes in this release
==================================

* Removed `bt_set_oob_data_flag` and replaced it with two new API calls:
  * :c:func:`bt_le_oob_set_sc_flag` for setting/clearing OOB flag in SC pairing
  * :c:func:`bt_le_oob_set_legacy_flag` for setting/clearing OOB flag in legacy paring

* :c:macro:`SYS_INIT` callback no longer requires a :c:struct:`device` argument.
  The new callback signature is ``int f(void)``. A utility script to
  automatically migrate existing projects can be found in
  :zephyr_file:`scripts/utils/migrate_sys_init.py`.

* Changed :c:struct:`spi_config` ``cs`` (:c:struct:`spi_cs_control`) from
  pointer to struct member. This allows using the existing SPI dt-spec macros in
  C++. SPI controller drivers doing ``NULL`` checks on the ``cs`` field to check
  if CS is GPIO-based or not, must now use :c:func:`spi_cs_is_gpio` or
  :c:func:`spi_cs_is_gpio_dt` calls.

New APIs in this release
========================

* Introduced :c:func:`flash_ex_op` function. This allows to perform extra
  operations on flash devices, defined by Zephyr Flash API or by vendor specific
  header files. Support for extra operations is enabled by
  :kconfig:option:`CONFIG_FLASH_EX_OP_ENABLED` which depends on
  :kconfig:option:`CONFIG_FLASH_HAS_EX_OP` selected by driver.

* Introduced :ref:`rtc_api` API which adds experimental support for real-time clock
  devices. These devices previously used the :ref:`counter_api` API combined with
  conversion between unix-time and broken-down time. The new API adds the mandatory
  functions :c:func:`rtc_set_time` and :c:func:`rtc_get_time`, the optional functions
  :c:func:`rtc_alarm_get_supported_fields`, :c:func:`rtc_alarm_set_time`,
  :c:func:`rtc_alarm_get_time`, :c:func:`rtc_alarm_is_pending` and
  :c:func:`rtc_alarm_set_callback` are enabled with
  :kconfig:option:`CONFIG_RTC_ALARM`, the optional function
  :c:func:`rtc_update_set_callback` is enabled with
  :kconfig:option:`CONFIG_RTC_UPDATE`, and lastly, the optional functions
  :c:func:`rtc_set_calibration` and :c:func:`rtc_get_calibration` are enabled with
  :kconfig:option:`CONFIG_RTC_CALIBRATION`.

* Introduced :ref:`auxdisplay_api` for auxiliary (alphanumeric-based) displays.

* Introduced :ref:`barriers_api` for barrier operations.

* Added :c:macro:`CAN_FRAME_ESI` CAN-FD Error State Indicator flag.

Kernel
******

* Removed absolute symbols :c:macro:`___cpu_t_SIZEOF`,
  :c:macro:`_STRUCT_KERNEL_SIZE`, :c:macro:`K_THREAD_SIZEOF` and
  :c:macro:`_DEVICE_STRUCT_SIZEOF`

Architectures
*************

* ARC

  * Removed absolute symbols :c:macro:`___callee_saved_t_SIZEOF` and
    :c:macro:`_K_THREAD_NO_FLOAT_SIZEOF`

* ARM

  * Removed absolute symbols :c:macro:`___basic_sf_t_SIZEOF`,
    :c:macro:`_K_THREAD_NO_FLOAT_SIZEOF`, :c:macro:`___cpu_context_t_SIZEOF`
    and :c:macro:`___thread_stack_info_t_SIZEOF`

* ARM64

  * Removed absolute symbol :c:macro:`___callee_saved_t_SIZEOF`
  * Enabled FPU and FPU_SHARING for v8r aarch64
  * Fixed the STACK_INIT logic during the reset
  * Introduced and enabled safe exception stack
  * Fixed possible deadlock on SMP with FPU sharing
  * Added ISBs after SCTLR Modifications

* NIOS2

  * Removed absolute symbol :c:macro:`_K_THREAD_NO_FLOAT_SIZEOF`

* POSIX:

  * Added :c:macro:`Z_SPIN_DELAY` to allow to conditionally compile a k_busy_wait() for this arch
    in tests and samples.

* RISC-V

  * Added :kconfig:option:`CONFIG_PMP_NO_TOR`, :kconfig:option:`CONFIG_PMP_NO_NA4`, and
    :kconfig:option:`CONFIG_PMP_NO_NAPOT` to allow disabling unsupported PMP range modes.
  * Removed unused symbols: :c:macro:`_thread_offset_to_tp`,
    :c:macro:`_thread_offset_to_priv_stack_start`, :c:macro:`_thread_offset_to_user_sp`.
  * Added support for setting PMP granularity with :kconfig:option:`CONFIG_PMP_GRANULARITY`.
  * Switched from accessing CSRs from inline assembly to using the :c:func:`csr_read` helper
    function.
  * Enabled single-threading support.

* SPARC

  * Removed absolute symbol :c:macro:`_K_THREAD_NO_FLOAT_SIZEOF`

* X86

* Xtensa

  * Fixed the cross stack call mechanism during nested interrupts where stack would be
    corrupted under certain conditions.
  * Added initial support for MMU on Xtensa.
  * Now supports building with :kconfig:option:`CONFIG_MULTITHREADING` disabled so
    target can run in single thread only operations.
  * Added C structs to represent interrupt frames to help with debugging.

Bluetooth
*********

* General

  * Moved all logging symbols together in a new ``Kconfig.logging`` file.
  * Deprecated the ``BT_DEBUG_LOG`` option. Instead ``BT_LOG`` should be used.
  * Made the ``BT_LOG`` and ``BT_LOG_LEGACY`` options hidden.
  * Removed ``BT_DEBUG`` entirely.


* Audio

  * Implemented the CAP initiator broadcast audio start, stop and metadata
    update procedures.
  * Implemented the CAP unicast audio start, stop and metadata update procedures.
  * Implemented the Telephony and Media Audio Service (TMAS).
  * Added additional validation for MCC and MCS, including opcodes, values, etc.
  * Refactored and extended the scan delegator implementation, including
    integration with broadcast sink.
  * Added support for creating a broadcast sink from a PA sink.
  * Added support for optional characteristics in CSIP.
  * Implemented discovery by UUID instead of reading by UUID for multiple
    characteristics.
  * Added support for long reads and writes for multiple profiles.
  * Added support for long BAP ASE notifications and optimized long notify
    reads.
  * Offloaded MCS notifications to the system workqueue.
  * Added the CAP initiator cancel procedure.

* Direction Finding

* Host

  * Updated the Host to the v5.4 specification.
  * The GATT DB Hash is now recalculated upon loading settings.
  * Added experimental support for SMP keypress notifications.
  * Downgraded the severity of select log messages to avoid log flooding.
  * Separated the handling of LE SC OOB pairing from the legacy OOB logic.
  * Implemented the Encrypted Advertising Data feature.
  * Added support for the new Periodic Advertising with Responses (PAwR), both
    as an advertiser and as a scanner.
  * Added support for initiating connections from PAwR, as well as receiving
    connections while synced.
  * Clarified the behavior that is enabled by the ``BT_PRIVACY`` Kconfig option.
  * Introduced a new ``seg_recv`` L2CAP API for an application to receive
    segments directly and manage credits explicitly.

* Mesh

  * Added experimental support for Mesh Protocol d1.1r18 specification, gated
    by a new configuration option. This includes:

    * Enhanced Provisioning Authentication support.
    * Mesh Remote Provisioning support including:
      * Remote Provisioning Server and Client models.
      * Composition Data Page 128 and Models Metadata Page 128 support.
    * Large Composition Data support including:
      * Large Composition Data Server and Client models.
      * Models Metadata Page 0 support.
    * New Transport Segmentation and Reassembly (SAR) implementation including:
      * SAR Configuration Server and Client models.
    * Mesh Private Beacons support including:
      * Mesh Private Beacon Server and Client models.
    * Opcodes Aggregator support including:
      * Opcodes Aggregator Server and Client models.
    * Proxy Solicitation support including:
      * Solicitation PDU RPL Configuration Server and Client models.
      * On-Demand Private Proxy Server and Client models.
    * Composition Data Page 1 support.
    * Other Mesh Profile Enhancements.
  * Added experimental support for Mesh Binary Large Object Transfer Model d1.0r04_PRr00 specification.
  * Added experimental support for Mesh Device Firmware Update Model d1.0r04_PRr00 specification.
  * Fixed multiple profile errata.
  * Added experimental support for the PSA crypto APIs.
  * Added a new work queue to store mesh settings, including a new API for
    storing user data.
  * Disabled the models initialization macros for C++ as they use the compound
    literal feature from C99.
  * Deprecated Health Client and Configuration Client API have been removed.

* Controller

  * Implemented support for the central with multiple CIS usecase.
  * Implemented support for multiple peripheral CIS establishment.
  * Updated the Controller to the v5.4 specification.
  * Added support for coexistence with other transceivers.
  * Added support for multiple CIS/CIG setup/connect and teardown procedures in
    sequence.
  * Extended the ticker API to return expiration info.
  * Re-implemented Extended and Periodic Advertising, as well as and Broadcast
    ISO, using the new ticket expiration info feature.
  * Modified the ticker implementation to reschedule unreserved tickers that use
    ``ticks_slot_window``. Implement continuous scanning with it.
  * Added support for considering the SDU interval, along with the packet
    sequence number and time stamps, in SDU fragmentation.
  * Added a new ``BT_CTRL_TX_PWR_DBM`` option to set the TX power directly in
    dBm.
  * Optimized the RX path with support for piggy-backing notifications on
    already-allocated RX nodes.

* HCI Driver

Boards & SoC Support
********************

* Added support for these SoC series:

  * STM32C0 series are now supported (with introduction of STM32C031 SoC).
  * STM32H5 series are now supported (with introduction of STM32H503 and STM32H573 SoCs).
  * Added support for STM32U599 SoC variants

* Removed support for these SoC series:

* Made these changes in other SoC series:

* Added support for these ARC boards:

* Added support for these ARM boards:

  * Alientek STM32L475 Pandora
  * MXChip AZ3166 IoT DevKit
  * Seeed Studio Wio Terminal
  * ST Nucleo C031C6
  * ST Nucleo H563ZI
  * ST STM32H573I-DK Discovery
  * Raspberry Pi Pico W

* Added support for these ARM64 boards:

  * PHYTEC phyCORE-AM62x A53
  * MIMX93 EVK A53 (SOF)

* Added support for these RISC-V boards:

* Added support for these X86 boards:

* Added support for these Xtensa boards:

* Made these changes for ARC boards:

* Made these changes for ARM boards:

  * ``atsamc21n_xpro``: Enable support to CAN.
  * ``atsame54_xpro``: Read Ethernet MAC from I2C.
  * Changed the default board revision to 0.14.0 for the Nordic boards
    ``nrf9160dk_nrf9160`` and ``nrf9160dk_nrf52840``. To build for an
    older revision of the nRF9160 DK without external flash, specify that
    older board revision when building.
  * ``nrf9160dk_nrf52840``: Enabled external_flash_pins_routing switch by default.
  * ``nrf9160dk_nrf9160``: Changed the order of buttons and switches on the GPIO
    expander to match the order when using GPIO directly on the nRF9160 SoC.
  * ``STM32H747i_disco``: Enabled support for ST B-LCD40-DSI1 display extension

* Made these changes for ARM64 boards:

  * FVP revc_2xaemv8a / aemv8r: Added ethernet, PHY and MDIO nodes

* Made these changes to POSIX boards:

   * nrf52_bsim now includes support and models for:

     * 802.15.4 in the RADIO.
     * EGU.
     * FLASH (NVMC & UICR).
     * TEMP.
     * UART connected to a host ptty.
     * Many more minor CMSIS API and nRF APIs and drivers.

* Made these changes for RISC-V boards:

  * ``gd32vf103``: No longer requires special OpenOCD version.

* Made these changes for X86 boards:

* Made these changes for Xtensa boards:

* Removed support for these ARC boards:

* Removed support for these ARM boards:

* Removed support for these RISC-V boards:

  * BeagleV Starlight JH7100

* Removed support for these X86 boards:

* Removed support for these Xtensa boards:

* Made these changes in other boards:

* Added support for these following shields:

  * Adafruit Data Logger Shield
  * nPM1300 EK (Power Management Integrated Circuit (PMIC))
  * Panasonic Grid-EYE Shields
  * ST B_LCD40_DSI1_MB1166

Build system and infrastructure
*******************************

* Fixed an issue whereby older versions of the Zephyr SDK toolchain were used
  instead of the latest compatible version.

* Fixed an issue whereby building an application with sysbuild and specifying
  mcuboot's verification to be checksum only did not build a bootable image.

* Fixed an issue whereby if no prj.conf file was present then board
  configuration files would not be included by emitting a fatal error. As a
  result, prj.conf files are now mandatory in projects.

* Introduced support for extending/replacing the signing mechanism in zephyr,
  see :ref:`West extending signing <west-extending-signing>` for further
  details.

* Fixed an issue whereby when using ``*_ROOT`` variables with Sysbuild, these
  were lost for images.

* Enhanced ``zephyr_get`` CMake helper function to optionally support merging
  of scoped variables into a list.

* Added a new CMake helper function for setting/updating sysbuild CMake cache
  variables: ``sysbuild_cache_set``.

* Enhanced ``zephyr_get`` CMake helper function to lookup multiple variables
  and return the result in a variable of different name.

* Introduced ``EXTRA_CONF_FILE``, ``EXTRA_DTC_OVERLAY_FILE``, and
  ``EXTRA_ZEPHYR_MODULES`` for better naming consistency and uniform behavior
  for applying extra build settings in addition to Zephyr automatic build
  setting lookup.
  ``EXTRA_CONF_FILE`` replaces ``OVERLAY_CONFIG``.
  ``EXTRA_ZEPHYR_MODULES`` replaces ``ZEPHYR_EXTRA_MODULES``.
  ``EXTRA_DTC_OVERLAY_FILE`` is new, see
  :ref:`Set devicetree overlays <set-devicetree-overlays>` for further details.

* Twister now supports ``gtest`` harness for running tests written in gTest.

* Added an option to validate device initialization priorities at build time.
  To use it, enable :kconfig:option:`CONFIG_CHECK_INIT_PRIORITIES`, see
  :ref:`check_init_priorities.py` for more details.

* Added a new option to disable tracking of macro expansion when compiling,
  :kconfig:option:`CONFIG_COMPILER_TRACK_MACRO_EXPANSION`. This option may be
  disabled to reduce compiler verbosity when errors occur during macro
  expansions, e.g. in device definition macros.

* Twister now supports loading test configurations from alternative root
  folder/s by using ``--alt-config-root``. When a test is found, Twister will
  check if a test configuration file exist in any of the alternative test
  configuration root folders. For example, given
  ``$test_root/tests/foo/testcase.yaml``, Twister will use
  ``$alt_config_root/tests/foo/testcase.yaml`` if it exists.

* Twister now uses native YAML lists for fields that were previously defined
  using space-separated strings. For example:

  .. code-block:: yaml

     platform_allow: foo bar

  can now be written as:

  .. code-block:: yaml

     platform_allow:
       - foo
       - bar

  This applies to the following properties:

    - ``arch_exclude``
    - ``arch_allow``
    - ``depends_on``
    - ``extra_args``
    - ``extra_sections``
    - ``platform_exclude``
    - ``platform_allow``
    - ``tags``
    - ``toolchain_exclude``
    - ``toolchain_allow``

  Note that the old behavior is kept as deprecated. The
  :zephyr_file:`scripts/utils/twister_to_list.py` script can be used to
  automatically migrate Twister configuration files.

* When MCUboot image signing is enabled, a warning will now be emitted by cmake
  if no signing key is set in the project, this warning can be safely ignored
  if signing is performed manually or outside of zephyr. This warning informs
  the user that the generated image will not be bootable by MCUboot as-is.

* Babblesim is now included in the west manifest. Users can fetch it by enabling
  the ``babblesim`` group with west config.

* `west sign` now uses DT labels, of "fixed-partition" compatible nodes, to identify
  application image slots, instead of previously used DT node label properties.
  If you have been using custom partition layout for MCUboot, you will have to label
  your MCUboot slot partitions with proper DT node labels; for example partition
  with "image-0" label property will have to be given slot0_partition DT node label.
  Label property does not have to be removed from partition node, but will not be used.

  DT node labels used are listed below

  .. table::
     :align: center

     +---------------------------------+---------------------------+
     | Partition with label property   | Required DT node label    |
     +=================================+===========================+
     | "image-0"                       | slot0_partition           |
     +---------------------------------+---------------------------+
     | "image-1"                       | slot1_partition           |
     +---------------------------------+---------------------------+

Drivers and Sensors
*******************

* Device model

  * Devices that do not require an initialization routine can now pass ``NULL``
    to the ``DEVICE_*_DEFINE()`` macros.

* Auxiliary display

  * New auxiliary display (auxdisplay) peripheral has been added, this allows
    for interfacing with simple alphanumeric displays that do not feature
    graphic capabilities. This peripheral is marked as unstable.

  * HD44780 driver added.

  * Noritake Itron driver added.

  * Grove LCD driver added (ported from existing sample).

* ADC

 * MCUX LPADC driver now uses the channel parameter to select a software channel
   configuration buffer. Use ``zephyr,input-positive`` and
   ``zephyr,input-negative`` devicetree properties to select the hardware
   channel(s) to link a software channel configuration to.

 * MCUX LPADC driver ``voltage-ref`` and ``power-level`` devicetree properties
   were shifted to match the hardware as described in reference manual instead
   of matching the NXP SDK enum identifers.

 * Added support for STM32C0 and STM32H5.

 * Added DMA support for STM32H7.

 * STM32: Resolutions are now listed in the device tree for each ADC instance

 * STM32: Sampling times are now listed in the device tree for each ADC instance

* Battery-backed RAM

  * Added MCP7940N battery-backed RTC SRAM driver.

* CAN

  * The CAN statistics are now reset when calling :c:func:`can_start`.

  * Renamed the NXP FlexCAN devicetree binding compatible from ``nxp,kinetis-flexcan`` to
    :dtcompatible:`nxp,flexcan`.

  * Added support for the CAN-FD variant of the NXP FlexCAN controller using devicetree binding
    :dtcompatible:`nxp,flexcan-fd`.

  * Added support for the NXP NXP S32 CANEXCEL controller using devicetree binding
    :dtcompatible:`nxp,s32-canxl`.

  * Added support for the Atmel SAM0 CAN controller using devicetree binding
    :dtcompatible:`atmel,sam0-can`.

  * Refactored the Bosch M_CAN controller driver backend to allow for per-instance configuration via
    devicetree.

  * Now supports STM32H5 series.

* Clock control

  * Atmel SAM/SAM0: Introduce peripheral clock control.
  * Atmel SAM0: Improved ``samd20``/``samd21``/``samr21`` clocking mechanism.
  * STM32F4: Added support for PLL I2S

* Console:

  * The native_posix and bsim console drivers have been merged into one generic
    driver usable by all POSIX arch based boards.

* Counter

  * Added support on timer based counter on STM32H7 and STM32H5
  * Added support on RTC based counter on STM32C0 and STM32H5

* Crypto

  * Added support for STM32H5 AES

* DAC

  * Added support on STM32H5 series.

* DFU

* Disk

  * SDMMC STM32L4+: Now compatible with internal DMA

* Display

* DMA

  * STM32C0: Added support for DMA
  * STM32H5: Added support for GPDMA
  * STM32H7: Added support for BDMA
  * Added DMA support for the RP2040 SoC

* EEPROM

  * Switched from :dtcompatible:`atmel,at24` to dedicated :dtcompatible:`zephyr,i2c-target-eeprom` for I2C EEPROM target driver.

* Entropy

  * Added support for STM32H5 series.

* ESPI

* Ethernet

* Flash

  * Introduced new flash API call :c:func:`flash_ex_op` which calls
    :c:func:`ec_op` callback provided by a flash driver. This allows to perform
    extra operations on flash devices, defined by Zephyr Flash API or by vendor
    specific header files. :kconfig:option:`CONFIG_FLASH_HAS_EX_OP` should be
    selected by the driver to indicate that extra operations are supported.
    To enable extra operations user should select
    :kconfig:option:`CONFIG_FLASH_EX_OP_ENABLED`.
  * STM32F4: Now supports write protection and readout protection through
    new flash API call :c:func:`flash_ex_op`.
  * nrf_qspi_nor: Replaced custom API function ``nrf_qspi_nor_base_clock_div_force``
    with ``nrf_qspi_nor_xip_enable`` which apart from forcing the clock divider
    prevents the driver from deactivating the QSPI peripheral so that the XIP
    operation is actually possible.
  * flash_simulator:

    * A memory region can now be used as the storage area for the
      flash simulator. Using the memory region allows the flash simulator to keep
      its contents over a device reboot.
    * When building in native_posix, command line options have been added to select
      if the flash should be cleared at boot, the flash content kept in RAM,
      or the flash content file be deleted on exit.

  * spi_flash_at45: Fixed erase procedure to properly handle chips that have
    their initial sector split into two parts (usually marked as 0a and 0b).
  * STM32H5 now supports OSPI

* FPGA

* Fuel Gauge

* GPIO

  * Converted the ``gpio_keys`` driver to the input subsystem.
  * Added single-ended IO support for the RP2040 SoC

  * STM32: Supports newly introduced experimental API to enable/disable interrupts
    without re-config

* hwinfo

* I2C

  * Added support for STM32C0 and STM32H5 series

* I2S

  * STM32: Domain clock should now be configured by device tree.

* I3C

* IEEE 802.15.4

* Input

  * Introduced the :ref:`input` subsystem.

* Interrupt Controller

* IPM

* KSCAN

  * Added a :dtcompatible:`zephyr,kscan-input` input to kscan compatibility driver.
  * Converted the ``ft5336`` and ``kscan_sdl`` drivers to the input subsystem.

* LED

* MBOX

* MEMC

* MIPI-DSI

  * Added support on STM32H7

* Misc

   * Added PIO support for the RP2040 SoC

* PCIE

  * Enable filtering PCIe devices by class/revision.

* PECI

* Retained memory

  * Retained memory (retained_mem) driver has been added with backends for
    Nordic nRF GPREGRET, and uninitialised RAM.

* Pin control

  * Added support for Infineon CAT1
  * Added support for TI K3
  * Added support for ARC emdsp

* PWM

  * Added support for STM32C0.
  * STM32: Now supports 6-PWM channels

* Power domain

* Regulators

  * The regulator API can now be built without thread-safe reference counting
    by using :kconfig:option:`CONFIG_REGULATOR_THREAD_SAFE_REFCNT`. This
    feature can be useful in applications that do not enable
    :kconfig:option:`CONFIG_MULTITHREADING`.
  * Added support for ADP5360 PMIC
  * Added support for nPM1300 PMIC
  * Added support for Raspberry Pi Pico core supply regulator

* Reset

* SDHC

* Sensor

  * Added generic voltage measurement sample
  * Removed STM32 Vbat measurement sample (replaced by a generic one)
  * Added STM32 Vref sensor driver
  * Added STM32 Vref/Vbat measurement through the new generic voltage measurement sample
  * Added temperature measurement driver for STM32C0 and STM32F0x0
  * Removed STM32 temperature measurement sample (replaced by a generic one)
  * Added STM32 temperature measurement through the generic temperature measurement sample

* Serial

  * Add UART3 and UART4 configuration for ``gd32vf103`` SoCs.
  * uart_altera: added new driver for Altera Avalon UART.
  * uart_emul: added new driver for emulated UART.
  * uart_esp32:
    * Added support for ESP32S3 SoC.
    * Added support for RS-485 half duplex mode.
  * uart_hostlink: added new driver for virtual UART via Synopsys ARC hostlink channels.
  * uart_ifx_cat1: added new driver for Infineon CAT1 UART.
  * uart_mcux: added power management support.
  * uart_mcux_flexcomm: added support for asynchronous operations.
  * uart_mcux_lpuart: added support for parity.
  * uart_ns16550: now supports per instance hardware access mode instead of
    one access mode for all instances.
  * uart_pl011: fixed interrupt support.
  * uart_rpi_pico_pio: added new driver to support UART via
    Programmable Input/Output (PIO) on Raspberry Pi Pico.
  * uart_xmc4xxx: added support for asynchronous operations.
  * uart_stm32: Now support driver enable mode
  * Added hardware flow control support for the RP2040 SoC

* SPI

  * Added support on STM32H5 series.

* Timer

  * Support added for stopping Nordic nRF RTC system timer, which fixes an
    issue when booting applications built in prior version of Zephyr.

  * STM32: Now supports a prescaler at the input of clock (default not divided).
    Prescaler allows to achieve higher LPTIM timeout (up to 256s when lptim clocked by LSE)
    and consequently higher core sleep durations but impacts the tick precision.
    To be used with caution.

* USB

   * Added remote wakeup support for the RP2040 SoC

* W1

  * Added DS2482-800 1-Wire master driver. See the :dtcompatible:`maxim,ds2482-800`
    devicetree binding for more information.
  * Added :kconfig:option:`CONFIG_W1_NET_FORCE_MULTIDROP_ADDRESSING` which can be
    enabled force the 1-Wire network layer to use multidrop addressing.

* Watchdog

  * Added support for STM32C0 and STM32H5 series

* WiFi

Networking
**********
* Wi-Fi

  * TWT intervals are changed from milli-seconds to micro-seconds, interval variables are also renamed.

USB
***

Devicetree
**********

Bindings
========

* Generic or vendor-independent:

  * New bindings:

    * :dtcompatible:`gpio-radio-coex`
    * :dtcompatible:`grove-header`
    * :dtcompatible:`niosv-machine-timer`
    * :dtcompatible:`nordic-thingy53-edge-connector`
    * :dtcompatible:`ntc-thermistor-generic`
    * :dtcompatible:`nvme-controller`
    * :dtcompatible:`raspberrypi-40pins-header`
    * :dtcompatible:`st-morpho-header`
    * :dtcompatible:`test-gpio-enable-disable-interrupt`

  * Modified bindings:

    * :dtcompatible:`neorv32-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`ns16550`:

          * new property: ``io-mapped``
          * new property: ``stop-bits``
          * new property: ``data-bits``
          * new property: ``class-rev``
          * new property: ``class-rev-mask``
          * new property: ``pinctrl-0``
          * new property: ``pinctrl-1``
          * new property: ``pinctrl-2``
          * new property: ``pinctrl-3``
          * new property: ``pinctrl-4``
          * new property: ``pinctrl-names``

    * Generic pm base properties:

          * new property:: ``zephyr,pm-device-runtime-auto``

* Analog Devices, Inc. (adi):

  * New bindings:

    * :dtcompatible:`adi,adin2111`
    * :dtcompatible:`adi,adin2111-mdio`
    * :dtcompatible:`adi,adin2111-phy`
    * :dtcompatible:`adi,adp5360`
    * :dtcompatible:`adi,adp5360-regulator`
    * :dtcompatible:`adi,adt7310`

* Altera Corp. (altr):

  * New bindings:

    * :dtcompatible:`altr,uart`

  * Modified bindings:

    * :dtcompatible:`altr,jtag-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* AMS AG (ams):

  * New bindings:

    * :dtcompatible:`ams,tcs3400`
    * :dtcompatible:`ams,tmd2620`

* ARM Ltd. (arm):

  * New bindings:

    * :dtcompatible:`arm,sbsa-uart`

  * Modified bindings:

    * :dtcompatible:`arm,pl011`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`arm,cmsdk-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Asahi Kasei Corp. (asahi-kasei):

  * New bindings:

    * :dtcompatible:`asahi-kasei,akm09918c`

* Atmel Corporation (atmel):

  * New bindings:

    * :dtcompatible:`atmel,sam-adc`
    * :dtcompatible:`atmel,sam-pmc`
    * :dtcompatible:`atmel,sam0-can`

  * Modified bindings:

    * :dtcompatible:`atmel,sam-i2c-twi`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-i2c-twim`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-i2c-twihs`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-flash-controller`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-rstc`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-watchdog`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-afec`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-spi`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam4l-gpio`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-gpio`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-usbhs`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-usbc`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-pwm`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-dac`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam0-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`atmel,sam-usart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``
          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``
          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-can`:

          * new property: ``bosch,mram-cfg``
          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-smc`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,at45` (on spi bus):

          * new property: ``sector-0a-pages``
          * new property: ``no-chip-erase``
          * new property: ``no-sector-erase``

    * :dtcompatible:`atmel,sam-ssc`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-xdmac`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-trng`:

          * removed property: ``peripheral-id``

    * :dtcompatible:`atmel,sam-tc-qdec`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-gmac`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

    * :dtcompatible:`atmel,sam-tc`:

          * removed property: ``peripheral-id``
          * property ``clocks`` is now required

* Bosch Sensortec GmbH (bosch):

  * New bindings:

    * :dtcompatible:`bosch,bmi323`
    * :dtcompatible:`bosch,bmm150`

  * Removed bindings:

    * ``bosch,m_can-base``

  * Modified bindings:

    * :dtcompatible:`bosch,bmi270` (on i2c bus):

          * new property: ``irq-gpios``

* Cadence Design Systems Inc. (cdns):

  * Modified bindings:

    * :dtcompatible:`cdns,uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Cirrus Logic, Inc. (cirrus):

  * New bindings:

    * :dtcompatible:`cirrus,cs47l63`

* Cypress Semiconductor Corporation (cypress):

  * Modified bindings:

    * :dtcompatible:`cypress,psoc6-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Diglent, Inc. (digilent):

  * New bindings:

    * :dtcompatible:`digilent,pmod`

* Diodes Incorporated (diodes):

  * New bindings:

    * :dtcompatible:`diodes,pi3usb9201`

* EPCOS AG (epcos):

  * New bindings:

    * :dtcompatible:`epcos,b57861s0103a039`

* Espressif Systems (espressif):

  * Modified bindings:

    * :dtcompatible:`espressif,esp32-spi`:

          * new property: ``cs-setup-time``
          * new property: ``cs-hold-time``

    * :dtcompatible:`espressif,esp32-uart`:

          * new property: ``hw-rs485-hd-mode``
          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`espressif,esp32-usb-serial`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* FocalTech Systems Co.,Ltd (focaltech):

  * Modified bindings:

    * :dtcompatible:`focaltech,ft5336` (on i2c bus):

          * bus list changed from ['kscan'] to []

* Gaisler (gaisler):

  * Modified bindings:

    * :dtcompatible:`gaisler,apbuart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* GigaDevice Semiconductor (gd):

  * Modified bindings:

    * :dtcompatible:`gd,gd32-usart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Hamamatsu Photonics K.K. (hamamatsu):

  * New bindings:

    * :dtcompatible:`hamamatsu,s11059`

* Hitachi Ltd. (hit):

  * New bindings:

    * :dtcompatible:`hit,hd44780`

* ILI Technology Corporation (ILITEK) (ilitek):

  * New bindings:

    * :dtcompatible:`ilitek,ili9342c`

* Infineon Technologies (infineon):

  * New bindings:

    * :dtcompatible:`infineon,cat1-adc`
    * :dtcompatible:`infineon,cat1-bless-hci`
    * :dtcompatible:`infineon,cat1-flash-controller`
    * :dtcompatible:`infineon,cat1-gpio`
    * :dtcompatible:`infineon,cat1-i2c`
    * :dtcompatible:`infineon,cat1-pinctrl`
    * :dtcompatible:`infineon,cat1-scb`
    * :dtcompatible:`infineon,cat1-uart`
    * :dtcompatible:`infineon,cat1-watchdog`
    * :dtcompatible:`infineon,cyw43xxx-bt-hci`
    * :dtcompatible:`infineon,xmc4xxx-dma`
    * :dtcompatible:`infineon,xmc4xxx-spi`
    * :dtcompatible:`infineon,xmc4xxx-temp`

  * Modified bindings:

    * :dtcompatible:`infineon,xmc4xxx-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Intel Corporation (intel):

  * New bindings:

    * :dtcompatible:`intel,adsp-dmic-vss`
    * :dtcompatible:`intel,adsp-hda-dmic-cap`
    * :dtcompatible:`intel,adsp-hda-ssp-cap`
    * :dtcompatible:`intel,adsp-watchdog`
    * :dtcompatible:`intel,agilex-socfpga-sip-smc`
    * :dtcompatible:`intel,lpss`
    * :dtcompatible:`intel,niosv`
    * :dtcompatible:`intel,pch-smbus`
    * :dtcompatible:`intel,penwell-spi`
    * :dtcompatible:`intel,tco-wdt`

  * Modified bindings:

    * :dtcompatible:`intel,adsp-imr`:

          * property ``zephyr,memory-region-mpu`` enum value changed from ['RAM', 'RAM_NOCACHE', 'FLASH', 'PPB', 'IO'] to ['RAM', 'RAM_NOCACHE', 'FLASH', 'PPB', 'IO', 'EXTMEM']

    * :dtcompatible:`intel,adsp-hda-link-out`:

          * specifier cells for space "dma" are now named: ['channel'] (old value: None)

    * :dtcompatible:`intel,adsp-hda-host-in`:

          * specifier cells for space "dma" are now named: ['channel'] (old value: None)

    * :dtcompatible:`intel,adsp-hda-link-in`:

          * specifier cells for space "dma" are now named: ['channel'] (old value: None)

    * :dtcompatible:`intel,adsp-hda-host-out`:

          * specifier cells for space "dma" are now named: ['channel'] (old value: None)

    * :dtcompatible:`intel,ssp-dai`:

          * new property: ``i2svss``

    * :dtcompatible:`intel,e1000`:

          * new property: ``class-rev``
          * new property: ``class-rev-mask``

* ITE Tech. Inc. (ite):

  * New bindings:

    * :dtcompatible:`ite,it82xx2-usb`
    * :dtcompatible:`ite,it8xxx2-gpio-v2`
    * :dtcompatible:`ite,it8xxx2-intc-v2`

  * Modified bindings:

    * :dtcompatible:`ite,it8xxx2-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Shenzhen Jinghua Displays Electronics Co., Ltd. (jhd):

  * New bindings:

    * :dtcompatible:`jhd,jhd1313`

* Kvaser (kvaser):

  * Modified bindings:

    * :dtcompatible:`kvaser,pcican`:

          * new property: ``class-rev``
          * new property: ``class-rev-mask``

* Lattice Semiconductor (lattice):

  * Modified bindings:

    * :dtcompatible:`lattice,ice40-fpga` (on spi bus):

          * new property: ``creset-delay-us``
          * removed property: ``creset-delay-ns``

* LiteX SoC builder (litex):

  * Modified bindings:

    * :dtcompatible:`litex,uart0`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Linear Technology Corporation (lltc):

  * New bindings:

    * :dtcompatible:`lltc,ltc1660`
    * :dtcompatible:`lltc,ltc1665`

* lowRISC Community Interest Company (lowrisc):

  * New bindings:

    * :dtcompatible:`lowrisc,opentitan-aontimer`
    * :dtcompatible:`lowrisc,opentitan-pwrmgr`
    * :dtcompatible:`lowrisc,opentitan-spi`

  * Modified bindings:

    * :dtcompatible:`lowrisc,opentitan-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Maxim Integrated Products (maxim):

  * New bindings:

    * :dtcompatible:`maxim,ds2482-800`
    * :dtcompatible:`maxim,ds2482-800-channel`
    * :dtcompatible:`maxim,max17048`
    * :dtcompatible:`maxim,max31855`
    * :dtcompatible:`maxim,max31865`

* Microchip Technology Inc. (microchip):

  * New bindings:

    * :dtcompatible:`microchip,coreuart`
    * :dtcompatible:`microchip,mcp9600`
    * :dtcompatible:`microchip,mcp970x`
    * :dtcompatible:`microchip,xec-dmac`
    * :dtcompatible:`microchip,xec-pwmbbled`
    * :dtcompatible:`microchip,xec-symcr`

  * Removed bindings:

    * ``microchip,xec-adc-v2``
    * ``microchip,xec-pinmux``
    * ``microchip,xec-qmspi-full-duplex``

  * Modified bindings:

    * :dtcompatible:`microchip,xec-i2c`:

          * new property: ``pcrs``

    * :dtcompatible:`microchip,xec-adc`:

          * new property: ``girqs``
          * new property: ``pcrs``

    * :dtcompatible:`microchip,xec-qmspi-ldma`:

          * removed property: ``pcrs``
          * removed property: ``port-sel``
          * property ``clocks`` is now required
          * property ``interrupts`` is now required

    * :dtcompatible:`microchip,mcp23sxx` (on spi bus):

          * new property: ``int-gpios``
          * new property: ``reset-gpios``

    * :dtcompatible:`microchip,mcp230xx` (on i2c bus):

          * new property: ``int-gpios``
          * new property: ``reset-gpios``
          * property ``#gpio-cells`` const value changed from None to 2

    * :dtcompatible:`microchip,xec-pcr`:

          * specifier cells for space "clock" are now named: ['regidx', 'bitpos', 'domain'] (old value: ['regidx', 'bitpos'])
          * property ``#clock-cells`` const value changed from 2 to 3

    * :dtcompatible:`microchip,xec-pwm`:

          * specifier cells for space "pwm" are now named: ['channel', 'period', 'flags'] (old value: ['channel', 'period'])
          * property ``#pwm-cells`` const value changed from 2 to 3

    * :dtcompatible:`microchip,xec-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`microchip,xec-espi-vw-routing`:

          * new property: ``status``
          * new property: ``compatible``
          * new property: ``reg``
          * new property: ``reg-names``
          * new property: ``interrupts``
          * new property: ``interrupts-extended``
          * new property: ``interrupt-names``
          * new property: ``interrupt-parent``
          * new property: ``label``
          * new property: ``clocks``
          * new property: ``clock-names``
          * new property: ``#address-cells``
          * new property: ``#size-cells``
          * new property: ``dmas``
          * new property: ``dma-names``
          * new property: ``io-channels``
          * new property: ``io-channel-names``
          * new property: ``mboxes``
          * new property: ``mbox-names``
          * new property: ``wakeup-source``
          * new property: ``power-domain``

    * :dtcompatible:`microchip,enc28j60` (on spi bus):

          * new property: ``full-duplex``

* Microchip Technology Inc. (microsemi):

  * Removed bindings:

    * ``microsemi,coreuart``

* Motorola, Inc. (motorola):

  * Modified bindings:

    * :dtcompatible:`motorola,mc146818`:

          * new property: ``alarms-count``

* Nordic Semiconductor (nordic):

  * New bindings:

    * :dtcompatible:`nordic,npm1300`
    * :dtcompatible:`nordic,npm1300-charger`
    * :dtcompatible:`nordic,npm1300-gpio`
    * :dtcompatible:`nordic,npm1300-regulator`
    * :dtcompatible:`nordic,nrf-gpregret`

  * Modified bindings:

    * :dtcompatible:`nordic,nrf-twis`:

          * removed property: ``sda-pin``
          * removed property: ``scl-pin``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-twi`:

          * removed property: ``sda-pin``
          * removed property: ``scl-pin``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-twim`:

          * removed property: ``sda-pin``
          * removed property: ``scl-pin``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-qspi`:

          * removed property: ``sck-pin``
          * removed property: ``io-pins``
          * removed property: ``csn-pins``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,npm6001-regulator`:

          * removed property: ``nordic,ready-high-drive``
          * removed property: ``nordic,nint-high-drive``
          * removed property: ``nordic,sda-high-drive``
          * removed property: ``nordic,buck-mode0-input-type``
          * removed property: ``nordic,buck-mode0-pull-down``
          * removed property: ``nordic,buck-mode1-input-type``
          * removed property: ``nordic,buck-mode1-pull-down``
          * removed property: ``nordic,buck-mode2-input-type``
          * removed property: ``nordic,buck-mode2-pull-down``

    * :dtcompatible:`nordic,nrf-spi`:

          * new property: ``easydma-maxcnt-bits``
          * removed property: ``miso-pull-up``
          * removed property: ``miso-pull-down``
          * removed property: ``sck-pin``
          * removed property: ``mosi-pin``
          * removed property: ``miso-pin``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-spis`:

          * new property: ``easydma-maxcnt-bits``
          * removed property: ``csn-pin``
          * removed property: ``sck-pin``
          * removed property: ``mosi-pin``
          * removed property: ``miso-pin``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-spim`:

          * new property: ``easydma-maxcnt-bits``
          * removed property: ``miso-pull-up``
          * removed property: ``miso-pull-down``
          * removed property: ``sck-pin``
          * removed property: ``mosi-pin``
          * removed property: ``miso-pin``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-pdm`:

          * removed property: ``clk-pin``
          * removed property: ``din-pin``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-pwm`:

          * removed property: ``ch0-pin``
          * removed property: ``ch0-inverted``
          * removed property: ``ch1-pin``
          * removed property: ``ch1-inverted``
          * removed property: ``ch2-pin``
          * removed property: ``ch2-inverted``
          * removed property: ``ch3-pin``
          * removed property: ``ch3-inverted``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``
          * removed property: ``tx-pin``
          * removed property: ``rx-pin``
          * removed property: ``rts-pin``
          * removed property: ``cts-pin``
          * removed property: ``rx-pull-up``
          * removed property: ``cts-pull-up``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-uarte`:

          * new property: ``stop-bits``
          * new property: ``data-bits``
          * removed property: ``tx-pin``
          * removed property: ``rx-pin``
          * removed property: ``rts-pin``
          * removed property: ``cts-pin``
          * removed property: ``rx-pull-up``
          * removed property: ``cts-pull-up``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-rtc`:

          * new property: ``zli``

    * :dtcompatible:`nordic,npm6001` (on i2c bus):

          * new property: ``nordic,ready-high-drive``
          * new property: ``nordic,nint-high-drive``
          * new property: ``nordic,sda-high-drive``
          * new property: ``nordic,buck-mode0-input-type``
          * new property: ``nordic,buck-mode0-pull-down``
          * new property: ``nordic,buck-mode1-input-type``
          * new property: ``nordic,buck-mode1-pull-down``
          * new property: ``nordic,buck-mode2-input-type``
          * new property: ``nordic,buck-mode2-pull-down``

    * :dtcompatible:`nordic,nrf-qdec`:

          * removed property: ``a-pin``
          * removed property: ``b-pin``
          * removed property: ``led-pin``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-i2s`:

          * removed property: ``sck-pin``
          * removed property: ``lrck-pin``
          * removed property: ``sdout-pin``
          * removed property: ``sdin-pin``
          * removed property: ``mck-pin``
          * property ``pinctrl-0`` is now required

    * :dtcompatible:`nordic,nrf-timer`:

          * new property: ``max-bit-width``
          * new property: ``zli``

* Noritake Co., Inc. Electronics Division (noritake):

  * New bindings:

    * :dtcompatible:`noritake,itron`

* Nuvoton Technology Corporation (nuvoton):

  * New bindings:

    * :dtcompatible:`nuvoton,npcx-kbd`

  * Removed bindings:

    * ``nuvoton,npcx-kscan``

  * Modified bindings:

    * :dtcompatible:`nuvoton,numicro-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`nuvoton,npcx-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* NXP Semiconductors (nxp):

  * New bindings:

    * :dtcompatible:`nxp,dcnano-lcdif`
    * :dtcompatible:`nxp,flexcan`
    * :dtcompatible:`nxp,flexcan-fd`
    * :dtcompatible:`nxp,mipi-dsi-2l`
    * :dtcompatible:`nxp,pcal6416a`
    * :dtcompatible:`nxp,pcf8523`
    * :dtcompatible:`nxp,pint`
    * :dtcompatible:`nxp,s32-canxl`
    * :dtcompatible:`nxp,sc18im704`
    * :dtcompatible:`nxp,sc18im704-gpio`
    * :dtcompatible:`nxp,sc18im704-i2c`
    * :dtcompatible:`nxp,vf610-adc`

  * Removed bindings:

    * ``nxp,kinetis-flexcan``
    * ``nxp,lpc11u6x-pinmux``

  * Modified bindings:

    * :dtcompatible:`nxp,lpc-lpadc`:

          * property ``power-level`` enum value changed from None to [0, 1, 2, 3]
          * property ``voltage-ref`` enum value changed from None to [0, 1, 2, 3]
          * property ``calibration-average`` enum value changed from None to [1, 2, 4, 8, 16, 32, 64, 128]
          * property ``calibration-average`` is no longer required

    * :dtcompatible:`nxp,imx-flexspi`:

          * new property: ``rx-buffer-config``

    * :dtcompatible:`nxp,lpc-gpio`:

          * new property: ``int-source``

    * :dtcompatible:`nxp,imx-elcdif`:

          * new property: ``data-bus-width``
          * removed property: ``hsync``
          * removed property: ``hfp``
          * removed property: ``hbp``
          * removed property: ``vsync``
          * removed property: ``vfp``
          * removed property: ``vbp``
          * removed property: ``polarity``
          * removed property: ``data-buswidth``
          * property ``pixel-format`` type changed from string to int
          * property ``pixel-format`` enum value changed from ['rgb-888', 'bgr-565'] to None

    * :dtcompatible:`nxp,mcux-usbd`:

          * new property: ``pinctrl-0``
          * new property: ``pinctrl-1``
          * new property: ``pinctrl-2``
          * new property: ``pinctrl-3``
          * new property: ``pinctrl-4``
          * new property: ``pinctrl-names``

    * :dtcompatible:`nxp,imx-mipi-dsi`:

          * new property: ``phy-clock``

    * :dtcompatible:`nxp,imx-iuart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`nxp,kinetis-lpsci`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`nxp,imx-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`nxp,kinetis-lpuart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`nxp,lpc-usart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`nxp,kinetis-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`nxp,s32-linflexd`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`nxp,lpc11u6x-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`nxp,lpc-mcan`:

          * new property: ``bosch,mram-cfg``
          * removed property: ``std-filter-elements``
          * removed property: ``ext-filter-elements``
          * removed property: ``rx-fifo0-elements``
          * removed property: ``rx-fifo1-elements``
          * removed property: ``rx-buffer-elements``
          * removed property: ``tx-buffer-elements``

    * :dtcompatible:`nxp,lpc-dma`:

          * new property: ``nxp,dma-num-of-otrigs``
          * new property: ``nxp,dma-otrig-base-address``
          * new property: ``nxp,dma-itrig-base-address``

    * :dtcompatible:`nxp,mcux-qdec`:

          * new property: ``single-phase-mode``
          * new property: ``filter-count``
          * new property: ``filter-sample-period``

* open-isa.org (openisa):

  * Modified bindings:

    * :dtcompatible:`openisa,rv32m1-lpuart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Orise Technology (orisetech):

  * New bindings:

    * :dtcompatible:`orisetech,otm8009a`

* QEMU, a generic and open source machine emulator and virtualizer (qemu):

  * Modified bindings:

    * :dtcompatible:`qemu,ivshmem`:

          * new property: ``ivshmem-v2``
          * new property: ``class-rev``
          * new property: ``class-rev-mask``

* Quectel Wireless Solutions Co., Ltd. (quectel):

  * Modified bindings:

    * :dtcompatible:`quectel,bg9x` (on uart bus):

          * property ``mdm-reset-gpios`` is no longer required

* QuickLogic Corp. (quicklogic):

  * Modified bindings:

    * :dtcompatible:`quicklogic,usbserialport-s3b`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Raspberry Pi Foundation (raspberrypi):

  * New bindings:

    * :dtcompatible:`raspberrypi,core-supply-regulator`
    * :dtcompatible:`raspberrypi,pico-dma`
    * :dtcompatible:`raspberrypi,pico-pio`
    * :dtcompatible:`raspberrypi,pico-pio-device`
    * :dtcompatible:`raspberrypi,pico-uart-pio`

  * Modified bindings:

    * :dtcompatible:`raspberrypi,pico-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Renesas Electronics Corporation (renesas):

  * New bindings:

    * :dtcompatible:`renesas,smartbond-adc`
    * :dtcompatible:`renesas,smartbond-i2c`
    * :dtcompatible:`renesas,smartbond-lp-clk`
    * :dtcompatible:`renesas,smartbond-sdadc`
    * :dtcompatible:`renesas,smartbond-spi`
    * :dtcompatible:`renesas,smartbond-sys-clk`
    * :dtcompatible:`renesas,smartbond-trng`
    * :dtcompatible:`renesas,smartbond-usbd`
    * :dtcompatible:`renesas,smartbond-watchdog`

  * Modified bindings:

    * :dtcompatible:`renesas,rcar-scif`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`renesas,smartbond-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* ROHM Semiconductor Co., Ltd (rohm):

  * New bindings:

    * :dtcompatible:`rohm,bd8lb600fs`
    * :dtcompatible:`rohm,bh1750`

* SEGGER Microcontroller GmbH (segger):

  * Modified bindings:

    * :dtcompatible:`segger,rtt-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Siemens AG (siemens):

  * New bindings:

    * :dtcompatible:`siemens,ivshmem-eth`

* SiFive, Inc. (sifive):

  * New bindings:

    * :dtcompatible:`sifive,fu740-c000-ddr`

  * Modified bindings:

    * :dtcompatible:`sifive,uart0`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`sifive,e24`:

          * property ``riscv,isa`` enum value changed from ['rv32imac', 'rv32imafc', 'rv32imafcb', 'rv64imac', 'rv64imafdc'] to ['rv32emc', 'rv32imac', 'rv32imafc', 'rv32imafcb', 'rv64imac', 'rv64imafdc']

    * :dtcompatible:`sifive,e51`:

          * property ``riscv,isa`` enum value changed from ['rv32imac', 'rv32imafc', 'rv32imafcb', 'rv64imac', 'rv64imafdc'] to ['rv32emc', 'rv32imac', 'rv32imafc', 'rv32imafcb', 'rv64imac', 'rv64imafdc']

    * :dtcompatible:`sifive,e31`:

          * property ``riscv,isa`` enum value changed from ['rv32imac', 'rv32imafc', 'rv32imafcb', 'rv64imac', 'rv64imafdc'] to ['rv32emc', 'rv32imac', 'rv32imafc', 'rv32imafcb', 'rv64imac', 'rv64imafdc']

    * :dtcompatible:`sifive,s7`:

          * property ``riscv,isa`` enum value changed from ['rv32imac', 'rv32imafc', 'rv32imafcb', 'rv64imac', 'rv64imafdc'] to ['rv32emc', 'rv32imac', 'rv32imafc', 'rv32imafcb', 'rv64imac', 'rv64imafdc']

* Silicon Laboratories (silabs):

  * New bindings:

    * :dtcompatible:`silabs,gecko-burtc`
    * :dtcompatible:`silabs,gecko-iadc`

  * Modified bindings:

    * :dtcompatible:`silabs,gecko-usart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`silabs,gecko-leuart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`silabs,gecko-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Standard Microsystems Corporation (smsc):

  * New bindings:

    * :dtcompatible:`smsc,lan91c111`
    * :dtcompatible:`smsc,lan91c111-mdio`

* Synopsys, Inc. (snps):

  * New bindings:

    * :dtcompatible:`snps,designware-watchdog`
    * :dtcompatible:`snps,dwc2`
    * :dtcompatible:`snps,emsdp-pinctrl`
    * :dtcompatible:`snps,hostlink-uart`

  * Modified bindings:

    * :dtcompatible:`snps,designware-i2c`:

          * new property: ``class-rev``
          * new property: ``class-rev-mask``

    * :dtcompatible:`snps,designware-spi`:

          * new property: ``aux_reg``
          * new property: ``fifo-depth``
          * new property: ``pinctrl-0``
          * new property: ``pinctrl-1``
          * new property: ``pinctrl-2``
          * new property: ``pinctrl-3``
          * new property: ``pinctrl-4``
          * new property: ``pinctrl-names``

    * :dtcompatible:`snps,nsim-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Solomon Systech Limited (solomon):

  * New bindings:

    * :dtcompatible:`solomon,ssd1608`
    * :dtcompatible:`solomon,ssd1673`
    * :dtcompatible:`solomon,ssd1675a`
    * :dtcompatible:`solomon,ssd1680`
    * :dtcompatible:`solomon,ssd1681`

  * Removed bindings:

    * ``solomon,ssd16xxfb``

* STMicroelectronics (st):

  * New bindings:

    * :dtcompatible:`st,dsi-lcd-qsh-030`
    * :dtcompatible:`st,lsm6dso16is`
    * :dtcompatible:`st,lsm6dso16is`
    * :dtcompatible:`st,lsm6dsv16x`
    * :dtcompatible:`st,lsm6dsv16x`
    * :dtcompatible:`st,stm32-bdma`
    * :dtcompatible:`st,stm32-mipi-dsi`
    * :dtcompatible:`st,stm32-vref`
    * :dtcompatible:`st,stm32c0-hsi-clock`
    * :dtcompatible:`st,stm32c0-temp-cal`
    * :dtcompatible:`st,stm32f1-adc`
    * :dtcompatible:`st,stm32f4-adc`
    * :dtcompatible:`st,stm32f4-fsotg`
    * :dtcompatible:`st,stm32f4-plli2s-clock`
    * :dtcompatible:`st,stm32f412-plli2s-clock`
    * :dtcompatible:`st,stm32g0-exti`
    * :dtcompatible:`st,vl53l1x`

  * Modified bindings:

    * :dtcompatible:`st,stm32-i2c-v2`:

          * new property: ``scl-gpios``
          * new property: ``sda-gpios``

    * :dtcompatible:`st,stm32-i2c-v1`:

          * new property: ``scl-gpios``
          * new property: ``sda-gpios``

    * :dtcompatible:`st,stm32-flash-controller`:

          * new property: ``st,rdp1-enable-byte``

    * :dtcompatible:`st,stm32-adc`:

          * new property: ``resolutions``
          * new property: ``sampling-times``
          * new property: ``num-sampling-time-common-channels``

    * :dtcompatible:`st,stm32-ltdc`:

          * new property: ``window0-x0``
          * new property: ``window0-x1``
          * new property: ``window0-y0``
          * new property: ``window0-y1``
          * new property: ``pixel-format``
          * removed property: ``hsync-pol``
          * removed property: ``vsync-pol``
          * removed property: ``de-pol``
          * removed property: ``pclk-pol``
          * removed property: ``hsync-duration``
          * removed property: ``vsync-duration``
          * removed property: ``hbp-duration``
          * removed property: ``vbp-duration``
          * removed property: ``hfp-duration``
          * removed property: ``vfp-duration``
          * property ``pinctrl-0`` is no longer required

    * :dtcompatible:`st,stm32-ospi`:

          * new property: ``dlyb-bypass``
          * new property: ``ssht-enable``

    * :dtcompatible:`st,stm32-uart`:

          * new property: ``de-enable``
          * new property: ``de-assert-time``
          * new property: ``de-deassert-time``
          * new property: ``de-invert``
          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`st,stm32-lpuart`:

          * new property: ``de-enable``
          * new property: ``de-assert-time``
          * new property: ``de-deassert-time``
          * new property: ``de-invert``
          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`st,stm32-usart`:

          * new property: ``de-enable``
          * new property: ``de-assert-time``
          * new property: ``de-deassert-time``
          * new property: ``de-invert``
          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`st,stm32wl-subghz-radio` (on spi bus):

          * new property: ``power-amplifier-output``
          * new property: ``rfo-lp-max-power``
          * new property: ``rfo-hp-max-power``

    * :dtcompatible:`st,stm32h7-fdcan`:

          * new property: ``bosch,mram-cfg``

    * :dtcompatible:`st,stm32-fdcan`:

          * new property: ``bosch,mram-cfg``

    * :dtcompatible:`st,stm32-i2s`:

          * new property: ``mck-enabled``

    * :dtcompatible:`st,stm32-sdmmc`:

          * new property: ``clk-div``
          * new property: ``idma``

    * :dtcompatible:`st,stm32-lptim`:

          * new property: ``st,prescaler``

* Telink Semiconductor (telink):

  * Modified bindings:

    * :dtcompatible:`telink,b91-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Texas Instruments (ti):

  * New bindings:

    * :dtcompatible:`ti,ads114s08`
    * :dtcompatible:`ti,ads114s0x-gpio`
    * :dtcompatible:`ti,ads7052`
    * :dtcompatible:`ti,ina3221`
    * :dtcompatible:`ti,k3-pinctrl`
    * :dtcompatible:`ti,tps382x`

  * Modified bindings:

    * :dtcompatible:`ti,cc13xx-cc26xx-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`ti,cc32xx-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`ti,stellaris-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`ti,msp432p4xx-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Vishay Intertechnology, Inc (vishay):

  * New bindings:

    * :dtcompatible:`vishay,veml7700`

* A stand-in for a real vendor which can be used in examples and tests (vnd):

  * New bindings:

    * :dtcompatible:`vnd,device-with-props`
    * :dtcompatible:`vnd,input-device`
    * :dtcompatible:`vnd,string-array-unquoted`
    * :dtcompatible:`vnd,string-unquoted`

  * Modified bindings:

    * :dtcompatible:`vnd,serial`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Würth Elektronik GmbH. (we):

  * New bindings:

    * :dtcompatible:`we,wsen-pads`
    * :dtcompatible:`we,wsen-pads`
    * :dtcompatible:`we,wsen-pdus`
    * :dtcompatible:`we,wsen-tids`

* Worldsemi Co., Limited (worldsemi):

  * New bindings:

    * :dtcompatible:`worldsemi,ws2812-i2s`

* Xen Hypervisor (xen):

  * Modified bindings:

    * :dtcompatible:`xen,hvc-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Xilinx (xlnx):

  * New bindings:

    * :dtcompatible:`xlnx,xps-iic-2.00.a`
    * :dtcompatible:`xlnx,xps-iic-2.1`
    * :dtcompatible:`xlnx,xps-timebase-wdt-1.00.a`

  * Modified bindings:

    * :dtcompatible:`xlnx,xps-uartlite-1.00.a`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`xlnx,xuartps`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

* Shenzhen Xptek Technology Co., Ltd (xptek):

  * New bindings:

    * :dtcompatible:`xptek,xpt2046`

* Zephyr-specific binding (zephyr):

  * New bindings:

    * :dtcompatible:`zephyr,cdc-ecm-ethernet`
    * :dtcompatible:`zephyr,i2c-target-eeprom`
    * :dtcompatible:`zephyr,input-longpress`
    * :dtcompatible:`zephyr,input-sdl-touch`
    * :dtcompatible:`zephyr,kscan-input`
    * :dtcompatible:`zephyr,panel-timing`
    * :dtcompatible:`zephyr,retained-ram`
    * :dtcompatible:`zephyr,retention`
    * :dtcompatible:`zephyr,rtc-emul`
    * :dtcompatible:`zephyr,uart-emul`
    * :dtcompatible:`zephyr,udc-skeleton`

  * Removed bindings:

    * ``zephyr,ec-host-cmd-periph-espi``
    * ``zephyr,sdl-kscan``
    * ``zephyr,sim-ec-host-cmd-periph``

  * Modified bindings:

    * :dtcompatible:`zephyr,sim-flash`:

          * new property: ``memory-region``

    * :dtcompatible:`zephyr,cdc-acm-uart` (on usb bus):

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`zephyr,native-posix-uart`:

          * new property: ``stop-bits``
          * new property: ``data-bits``

    * :dtcompatible:`zephyr,memory-region`:

          * property ``zephyr,memory-region-mpu`` enum value changed from ['RAM', 'RAM_NOCACHE', 'FLASH', 'PPB', 'IO'] to ['RAM', 'RAM_NOCACHE', 'FLASH', 'PPB', 'IO', 'EXTMEM']

    * :dtcompatible:`zephyr,sdhc-spi-slot` (on spi bus):

          * new property: ``power-delay-ms``
          * new property: ``spi-clock-mode-cpol``
          * new property: ``spi-clock-mode-cpha``

Libraries / Subsystems
**********************

* File systems

  * Added :kconfig:option:`CONFIG_FS_FATFS_REENTRANT` to enable the FAT FS reentrant option.
  * With LittleFS as backend, :c:func:`fs_mount` return code was corrected to ``EFAULT`` when
    called with ``FS_MOUNT_FLAG_NO_FORMAT`` and the designated LittleFS area could not be
    mounted because it has not yet been mounted or it required reformatting.
  * The FAT FS initialization order has been updated to match LittleFS, fixing an issue where
    attempting to mount the disk in a global function caused FAT FS to fail due to not being registered beforehand.
    FAT FS is now initialized in POST_KERNEL.
  * Added :kconfig:option:`CONFIG_FS_LITTLEFS_FMP_DEV` to enable possibility of using LittleFS
    for block devices only, e.g. without Flash support. The option is set to 'y' by default in
    order to keep previous behaviour.

* IPC

  * :c:func:`ipc_service_close_instance` now only acts on bounded endpoints.
  * ICMSG: removed race condition during bonding.
  * ICMSG: removed internal API for clearing shared memory.
  * ICMSG: added mutual exclusion access to SHMEM.
  * Fixed CONFIG_OPENAMP_WITH_DCACHE.

* Management

  * Added optional input expiration to shell MCUmgr transport, this allows
    returning the shell to normal operation if a complete MCUmgr packet is not
    received in a specific duration. Can be enabled with
    :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT` and timeout
    set with
    :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT_TIME`.

  * MCUmgr fs_mgmt upload and download now caches the file handle to improve
    throughput when transferring data, the file is no longer opened and closed
    for each part of a transfer. In addition, new functionality has been added
    that will allow closing file handles of uploaded/downloaded files if they
    are idle for a period of time, the timeout is set with
    :kconfig:option:`MCUMGR_GRP_FS_FILE_AUTOMATIC_IDLE_CLOSE_TIME`. There is a
    new command that can be used to close open file handles which can be used
    after a file upload is complete to ensure that the file handle is closed
    correctly, allowing other transports or other parts of the application
    code to use it.

  * A new version of the SMP protocol used by MCUmgr has been introduced in the
    header, which is used to indicate the version of the protocol being used.
    This updated protocol allows returning much more detailed error responses
    per group, see the
    :ref:`MCUmgr SMP protocol specification <mcumgr_smp_protocol_specification>`
    for details.

  * MCUmgr has now been marked as a stable Zephyr API.

  * The MCUmgr UDP transport has been refactored to resolve some concurrency
    issues and fixes a potential issue whereby an application might call the
    open transport function whilst it is already open, causing an endless log
    output loop.

  * The MCUmgr fs_mgmt group Kconfig ``Insecure`` text has been replaced with
    a CMake warning which triggers if fs_mgmt hooks are not enabled, as these
    hooks can be used to ensure security of file access allowed by MCUmgr
    clients.

  * Fixed an issue with MCUmgr fs_mgmt file download not checking if the
    offset parameter was provided.

  * Fixed an issue with MCUmgr fs_mgmt file upload notification hook not
    setting upload to true.

  * Fixed an issue with MCUmgr img_mgmt image upload ``upgrade`` field wrongly
    checking if the new image was the same version of the application and
    allowing it to be uploaded if it was.

  * MCUmgr img_mgmt group will only verify the SHA256 hash provided by the
    client against the uploaded image (if support is enabled) if a full SHA256
    hash was provided.

  * MCUmgr Kconfig options have changed from ``select`` to ``depends on`` which
    means that some additional Kconfig options may now need to be selected by
    applications. :kconfig:option:`CONFIG_NET_BUF`,
    :kconfig:option:`CONFIG_ZCBOR` and :kconfig:option:`CONFIG_CRC` are needed
    to enable MCUmgr support, :kconfig:option:`CONFIG_BASE64` is needed to
    enable shell/UART/dummy MCUmgr transports,
    :kconfig:option:`CONFIG_NET_SOCKETS` is needed to enable the UDP MCUmgr
    transport, :kconfig:option:`CONFIG_FLASH` is needed to enable MCUmgr
    fs_mgmt, :kconfig:option:`CONFIG_FLASH` and
    :kconfig:option:`CONFIG_IMG_MANAGER` are needed to enable MCUmgr img_mgmt.

* POSIX API

  * Improved the locking strategy for :c:func:`eventfd_read()` and
    :c:func:`eventfd_write()`. This eliminated a deadlock scenario that was
    present since the initial contribution and increased performance by a
    factor of 10x.

  * Reimplemented :ref:`POSIX <posix_support>` threads, mutexes, condition
    variables, and barriers using native Zephyr counterparts. POSIX
    synchronization primitives in Zephyr were originally implemented
    separately and received less maintenance as a result. Unfortunately, this
    opened POSIX up to unique bugs and race conditions. Going forward, POSIX
    will benefit from all improvements to Zephyr's synchronization and
    threading API and race conditions have been mitigated.

* Retention

  * Retention subsystem has been added which adds enhanced features over
    retained memory drivers allowing for partitioning, magic headers and
    checksum validation. See :ref:`retention API <retention_api>` for details.
    Support for the retention subsystem is experimental.

  * Boot mode retention module has been added which allows for setting/checking
    the boot mode of an application, initial support has also been added to
    MCUboot to allow for applications to use this as an entrance method for
    MCUboot serial recovery mode. See :ref:`boot mode API <boot_mode_api>` for
    details.

* RTIO

  * Added policy that every ``sqe`` will generate a ``cqe`` (previously an RTIO_SQE_TRANSACTION
    entry would only trigger a ``cqe`` on the last ``sqe`` in the transaction.

* Power management

  * Added a new policy event API that can be used to register expected events
    that will wake the system up in the future. This can be used to influence
    the system on which low power states can be used.

HALs
****

* STM32

  * stm32cube: updated STM32F0 to cube version V1.11.4.
  * stm32cube: updated STM32F3 to cube version V1.11.4
  * stm32cube: updated STM32L0 to cube version V1.12.2
  * stm32cube: updated STM32U5 to cube version V1.2.0
  * stm32cube: updated STM32WB to cube version V1.16.0

* Raspberry Pi Pico

  * Updated hal_rpi_pico to version 1.5.0

MCUboot
*******

* Relocated the MCUboot Kconfig options from the main ``Kconfig.zephyr`` file to
  a new ``modules/Kconfig.mcuboot`` module-specific file. This means that, for
  interactive Kconfig interfaces, the MCUboot options will now be located under
  ``Modules`` instead of under ``Boot Options``.

* Added :kconfig:option:`CONFIG_MCUBOOT_CMAKE_WEST_SIGN_PARAMS` that allows to pass arguments to
  west sign when invoked from cmake.

Storage
*******

* Added :kconfig:option:`CONFIG_FLASH_MAP_LABELS`, which will enable runtime access to the labels
  property of fixed partitions. This option is implied if kconfig:option:`CONFIG_FLASH_MAP_SHELL`
  is enabled. These labels will be displayed in a separate column when using the ``flash_map list``
  shell command.

Trusted Firmware-M
******************

zcbor
*****

Updated from 0.6.0 to 0.7.0.
Among other things, this update brings:

* C++ improvements
* float16 support
* Improved docs
* -Wall and -Wconversion compliance

Documentation
*************

Tests and Samples
*****************

* Two Babblesim based networking (802.15.4) tests have been added, which are run in Zephyr's CI
  system. One of them including the OpenThread stack.
* For native_posix and the nrf52_bsim: Many tests have been fixed and enabled.
* LittleFS sample has been given SPI example configuration for nrf52840dk_nrf52840.

Issue Related Items
*******************

Known Issues
============

Addressed issues
================
