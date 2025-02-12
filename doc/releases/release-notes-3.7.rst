:orphan:

.. _zephyr_3.7:

.. _zephyr_3.7.1:

Zephyr 3.7.1
############

This is an LTS maintenance release with fixes.

Issues fixed
************

These GitHub issues were addressed since the previous 3.7.0 tagged release:

* :github:`77088` - net: buf: Pre-initialize user_data on net_buf_alloc()
* :github:`81380` - test: twister: hwmap: missing serial_pty definition
* :github:`81570` - ESP32 fails on assert when WiFi enabled
* :github:`81083` - DA1469x dma driver initialization fails
* :github:`80542` - Hard fault on boot w/ GCC 14 on Cortex-M
* :github:`71896` - xtensa: remove unnecessary padding in arch_mem_domain
* :github:`81372` - arch: riscv: userspace: potential security risk when CONFIG_RISCV_GP=y
* :github:`42227` - Teensy41 support SDHC - Storage init Error
* :github:`66800` - [Coverity CID: 338107] Division or modulo by zero in subsys/shell/backends/shell_uart.c
* :github:`70498` - cellular_modem: Pinnacle 100 does not work with CONFIG_UART_INTERRUPT_DRIVEN
* :github:`71397` - Espressif HAL5.1 - Core panic'ed on EPS32S3 with hello_world sample
* :github:`71916` - Bump minimum Python version (3.10?) ahead of LTSv3 release
* :github:`74291` - Multiple USB contexts for same device lead to bus faults
* :github:`74750` - [Coverity CID: 353644] Unintended sign extension in drivers/sensor/ams/tsl2591/tsl2591.c
* :github:`74752` - [Coverity CID: 353654] Unintended sign extension in drivers/sensor/ams/tsl2591/tsl2591.c
* :github:`75256` - [Coverity CID: 394233] Division or modulo by zero in drivers/sensor/tdk/icm42688/icm42688.h
* :github:`75257` - [Coverity CID: 394232] Division or modulo by zero in drivers/sensor/tdk/icm42688/icm42688.h
* :github:`75265` - display: stm32-ltdc: incorrect return value for blanking on/off calls when panel controller is not passed in DT
* :github:`75318` - Zephyr 3.7 RC2 BLE on STM32 Nucleo WB55 does not work
* :github:`75319` - `fatal error: sys/_timespec.h: No such file or directory` with v3.7.0-rc2
* :github:`75427` - `CONFIG_MBEDTLS_PEM_CERTIFICATE_FORMAT` is misleading as it only enables PEM parsing
* :github:`75676` - tests: dma: chan_blen_transfer: test breakage on Nucleo-H743ZI
* :github:`75686` - ST LIS2DUX12 driver: temperature is not converted to Celsius and raw values are returned
* :github:`75736` - Logging stops on all backend when overloaded for a time
* :github:`75758` - ST LIS2DUX12 driver: will not correctly read values on big-endian HW
* :github:`75834` - xtensa: mpu: adding or removing partitions from memory domain of current running thread has no effect
* :github:`75853` - C++: Boot fails with global STL objects
* :github:`75909` - CONFIG_NET_NATIVE_OFFLOADED_SOCKETS fails after DTLS handshake
* :github:`75912` - RAM retention not working on NXP S32K3
* :github:`75967` - bl5340_dvk LCD does not work
* :github:`76000` - coap: block_number in coap_get_block1_option() is too small
* :github:`76090` - TF-M: builds download source from external git repository
* :github:`76185` - scripts: 'west blobs fetch' does not verify digests after downloading
* :github:`76254` - linker: section_tags: missing include
* :github:`76268` - Bluetooth: PAwR Sync does not compile without PAST receiver compiled in
* :github:`76325` - ESP32S3: Zephyr freezes during OTA Update over UDP and BLE
* :github:`76360` - Crash if enabling v4-mapping-to-v6 and DHCPv4
* :github:`76397` - gdbstub: compilation error on ARM
* :github:`76398` - arch: riscv: stacktrace: user thread stack bound check is wrong
* :github:`76403` - net: wifi: NM type check is incorrect
* :github:`76427` - CONFIG_BT_CTLR_OPTIMIZE_FOR_SPEED not selected by default in zephyr 3.7
* :github:`76446` - drivers: mdio: mdio_nxp_enet: Link goes up and down sporadically
* :github:`76447` - Odroid Go Boot Failure
* :github:`76471` - TCP Delay ACK in case no PSH flag is present causing extremly low download throughput for some clients
* :github:`76519` - Get I3C basic device fails for devices that do not support GETCAP
* :github:`76549` - Bug in STM32 Disco L475 Arduino I2C header
* :github:`76551` - Bluetooth: USB Audio Starvation in BAP Broadcast Sink Sample
* :github:`76603` - sensortile_box_pro board does not support hts221 sensor
* :github:`76607` - Fixes an uninitialized value in lis2dux12
* :github:`76625` - drivers: usb: stm32 udc driver get the global otg interrupt
* :github:`76655` - RT11xx device tree build warnings
* :github:`76717` - Check L2 send properly for offloaded ifaces
* :github:`76718` - PTP - incorrect clock offset calculation
* :github:`76737` - Bluetooth L2CAP TX fragmentation state not cleared upon disconnect
* :github:`76738` - Sending Bluetooth L2CAP messages after reconnecting sometimes leads to crashes due to uninitalized net_buf callback
* :github:`76747` - CONFIG_DEVICE_DT_METADATA=y not tested with usespace enabled
* :github:`76762` - arch: riscv: stacktrace: trace is incomplete when unwinding from function without callee
* :github:`76776` - http1 dynamic post request response doesn't send header a second time
* :github:`76912` - drivers/bluetooth/hci/userchan.c build waring w gcc13
* :github:`76932` - Compilation error on Ubuntu 24.04 when CONFIG_FUSE_FS_ACCESS=y
* :github:`77006` - Bluetooth: Host: ISO buffer not unreffed
* :github:`77007` - Bluetooth: Host: ISO not clearing TX context on disconnect
* :github:`77078` - manifest: update percepio
* :github:`77089` - nxp: lpc55sxx: sram node address error
* :github:`77096` - nxp lpc55xx will crash when trying to use a nxp,ctimer-pwm DTS node
* :github:`77126` - RTC RV3028 clock output disabled by enabling alarm/update callback
* :github:`77129` - NXP's IMX RT1010 triggers assert on SPI Loopback test when EDMA enabled
* :github:`77175` - SCSI Read Capacity reports wrong capacity when used with USB Mass storage device
* :github:`77188` - Unaligned 32-bit reads following casts on dns_socket_dispatcher::local_addr (Cortex-M0)
* :github:`77221` - Websocket compile issue if Posix API is not set
* :github:`77241` - bluetooth: Behavior change in host causing MESH to fail on sending messages
* :github:`77269` - da1469x: entropy and gpio drivers throw assertions
* :github:`77303` - net: socket: recvmsg() doesn't update msg_controllen accordingly
* :github:`77371` - RT11xx bus clocks set incorrectly
* :github:`77396` - doc: Documentation scrolling does not work with arrow keys
* :github:`77424` - IMXRT11xx secondary core improperly deinit the ENET PLL
* :github:`77545` - modules: percepio: Fix python executable name in CMakeLists.txt
* :github:`77557` - drivers: ethernet: nxp_enet: Fused MAC address handling is wrong
* :github:`77594` - Bluetooth: Failures in unit tests
* :github:`77633` - drivers: dai: nxp: sai: RTOS crashes on TX/RX underrun/overrun
* :github:`77658` - backported PRs maintain fail status after issues are added to the PR description
* :github:`77669` - BLE EDTT GATT tests fail with python >= 3.11
* :github:`77784` - STM32 I2S Driver Clock Configuration
* :github:`77824` - llext samples fail with BUS ERROR (zephyr-3.7 stable)
* :github:`77952` - ESP32: Interrupts with ISRs in flash are not disabled during flash operations
* :github:`78138` - riscv: plic: IRQs may not be enabled in non-zero HART
* :github:`78146` - mDNS reponder might fail to serve if mDNS resolver is enabled
* :github:`78244` - arch: riscv: IRQ stacks of secondary cores are not initialized to `0xAA` when `CONFIG_INIT_STACKS=y`
* :github:`78263` - building samples/cpp/hello_world reports "undefined reference to `_impure_ptr` error
* :github:`78268` - LPC55xx USB not working with SPEED_OPTIMIZATIONS=y
* :github:`78490` - net: l2: ieee802154: IEEE-802.15.4 packets dropped when CONFIG_NET_L2_IEEE802154_SECURITY is enabled
* :github:`78494` - subsys: net: socket_tls: Connection vanishes -> Caller blocked forever
* :github:`78495` - net: l2: ieee802154: ctx_lock taken but not given
* :github:`78608` - Bluetooth: classic: avdtp: missing buffer length check
* :github:`78622` - Wrong package for bz2
* :github:`78651` - drivers: PHY Microchip KSZ8081 will block system work queue for 500/1000ms
* :github:`78724` - net: checksum offloading not working for virtual network interfaces used for Virtual LAN (VLAN)
* :github:`78769` - backport: v3.7: dts: arm: st: stm32h5: fix spi 1-3 clocks
* :github:`78927` - doc: mcumgr: Wrong license for mcumgr-client
* :github:`79208` - sysbuild: ExternalZephyrProject_Add() cannot cope with hwmv2 board and revision
* :github:`79288` - driver: clock-control: imx-ccm-rev2: get_rate(FLEXSPI2) returns FLEXSPI1 clock rate
* :github:`79298` - arch: riscv: the `s0` dumped during a fatal error is not correct
* :github:`79301` - arch: irq: multilevel: the value return from `irq_parent_level_3()` is wrong
* :github:`79518` - Temporary Modbus Client "Soft Lock"
* :github:`79576` - Dependencies for `SOC_FLASH_MCUX` are incorrect on LPC55xxx platforms
* :github:`79788` - net: ip: igmp: IGMPv2 does not work when IGMPv3 is enabled
* :github:`79800` - stream_flash: stream_flash_erase_page does not check whether requested offset is in range of stream flash owned area
* :github:`79947` - ESP32 touch_sensor not working
* :github:`80005` - doc: Documentation issue in 'contribute/documentation/guidelines'
* :github:`80134` - up_squared: tests/arch/x86/info/arch.x86.info.userspace [timeout]
* :github:`80200` - CMake: Find package with version ranges are ignored for Zephyr SDK
* :github:`80383` - drivers: flash: spi_nor: values vom DT not correctly used, when `CONFIG_SPI_NOR_SFDP_RUNTIME=y`
* :github:`80429` - Bug in ACD implementation RFC5227
* :github:`80476` - mgmt: mcumgr: stat_mgmt: stat_mgmt_list always send legacy behavior
* :github:`80541` - Incorrect part number LPC55S69 in Konfig.soc
* :github:`80593` - Not possible to write beyond 16MB using flash sample on FRDM_RW612
* :github:`80701` - Changes to cortex_m swap code have major impact on kernel context switching times.
* :github:`80723` - testsuite: coverage: application unable to include `coverage.h`
* :github:`80750` - drivers: can: sam: wrong MRBA configuration
* :github:`80810` - samples: subsys: mgmt: mcumgr: smp_svr: UDP overlay missing random selection
* :github:`80818` - shell: shell_help: terminal offset of subcommands' help text isn't aligned

Mbed TLS
********

Mbed TLS was updated to version 3.6.2 (from 3.6.0). The release notes can be found at:

  * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.1
  * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.2

Mbed TLS 3.6 is an LTS release that will be supported
with security and bug fixes until at least March 2027.

MCUboot
*******

MCUboot commit has been updated to ``ea2410697dd0262edec041a0ccb07fdbde7c1aff``.

Trusted Firmware-M
******************

TF-M was updated to version 2.1.1 (from 2.1.0). The release notes can be found at:

  * https://trustedfirmware-m.readthedocs.io/en/tf-mv2.1.1/releases/2.1.1.html

TF-M 2.1 is an LTS release synchronized with Mbed TLS 3.6 that
will be supported with security and bug fixes until March 2027.

.. _zephyr_3.7.0:

Zephyr 3.7.0
############

We are pleased to announce the release of Zephyr version 3.7.0.

This release is the last non-maintenance 3.x release and, as such, will be the next
:ref:`Long Term Support (LTS) release <release_process_lts>`.

Major enhancements with this release include:

* A new, completely :ref:`overhauled hardware model <hw_model_v2>` has been introduced.
  It changes the way both SoCs and boards are named, defined and constructed in Zephyr.
  Additional information can be found in the :ref:`board_porting_guide`.
* A long-awaited :ref:`HTTP Server <http_server_interface>` library, and associated service API,
  allow to easily implement HTTP/1.1 and HTTP/2 servers in Zephyr. Resources can be registered
  statically or dynamically, and WebSocket support is included.
* :ref:`POSIX support <posix_support>` has been extended, with most Options of the IEEE 1003-2017
  :ref:`System Interfaces <posix_system_interfaces_required>` receiving support, as well as most
  Options and Option groups required for :ref:`PSE51 <posix_aep_pse51>`,
  :ref:`PSE52 <posix_aep_pse52>`, and :ref:`PSE53 <posix_aep_pse53>`.
* Bluetooth Host has been extended with support for the Nordic UART Service (NUS), Hands-free Audio
  Gateway (AG), Advanced Audio Distribution Profile (A2DP), and Audio/Video Distribution Transport
  Protocol (AVDTP).
* Sensor abstraction model has been overhauled to adopt a
  :ref:`read-then-decode approach <sensor-read-and-decode>` that enables more types of sensors and
  data flows than the previous fetch/get APIs.
* A new :ref:`LLEXT Extension Developer Kit (EDK) <llext_build_edk>` makes it easier to develop and
  integrate custom extensions into Zephyr, including outside of the Zephyr tree.
* :ref:`Native simulator <native_sim>` now supports leveraging the native host networking stack
  without having to rely on a complex setup of the host environment.
* Trusted Firmware-M (TF-M) 2.1.0 and Mbed TLS 3.6.0 have been integrated into Zephyr.
  Both of these versions are LTS releases. What's more, :ref:`psa_crypto` has been adopted as a replacement
  for TinyCrypt and provides enhanced security and performance.
* A new experimental implementation of the :ref:`Precision Time Protocol <ptp_interface>` (PTP, IEEE
  1588) allows to synchronize time across devices with sub-microsecond accuracy.
* New documentation pages have been introduced to help developers setup their local development
  environment for :ref:`vscode_ide` and :ref:`clion_ide`.

An overview of the changes required or recommended when migrating your application from Zephyr
v3.6.0 to Zephyr v3.7.0 can be found in the separate :ref:`migration guide<migration_3.7>`.

While you may refer to release notes from previous 3.x releases for a full change log, other major
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
    * civetweb module. See Zephyr 3.7's new :ref:`http_server_interface` as a replacement.
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

* CVE-2024-6137: Under embargo until 2024-09-11

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

 * Removed ``pm_device_state_lock``, ``pm_device_state_is_locked`` and ``pm_device_state_unlock``
   functions from the Device PM APIs.

 * Removed deprecated MCUmgr transport API functions: ``zephyr_smp_rx_req``,
   ``zephyr_smp_alloc_rsp`` and ``zephyr_smp_free_buf``.

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

   Application developers will now need to set the advertised name themselves by updating the
   advertising data or the scan response data.

* CAN

  * Deprecated the :c:func:`can_calc_prescaler` API function, as it allows for bitrate
    errors. Bitrate errors between nodes on the same network leads to them drifting apart after the
    start-of-frame (SOF) synchronization has taken place, leading to bus errors.
  * Deprecated the :c:func:`can_get_min_bitrate` and :c:func:`can_get_max_bitrate` API functions in
    favor of :c:func:`can_get_bitrate_min` and :c:func:`can_get_bitrate_max`.
  * Deprecated the :c:macro:`CAN_MAX_STD_ID` and :c:macro:`CAN_MAX_EXT_ID` macros in favor of
    :c:macro:`CAN_STD_ID_MASK` and :c:macro:`CAN_EXT_ID_MASK`.

* PM

  * Deprecated :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_EXCLUSIVE`. Similar behavior can be achieved
    using :kconfig:option:`CONFIG_PM_DEVICE_SYSTEM_MANAGED`.

.. _zephyr_3.7_posix_api_deprecations:

* POSIX API

  * Deprecated :c:macro:`PTHREAD_BARRIER_DEFINE` has been removed.
  * Deprecated :c:macro:`EFD_IN_USE` and :c:macro:`EFD_FLAGS_SET` have been removed.

  * In efforts to use Kconfig options that map directly to the Options and Option Groups in
    IEEE 1003.1-2017, the following Kconfig options have been deprecated (replaced by):

    * :kconfig:option:`CONFIG_EVENTFD_MAX` (:kconfig:option:`CONFIG_ZVFS_EVENTFD_MAX`)
    * :kconfig:option:`CONFIG_FNMATCH` (:kconfig:option:`CONFIG_POSIX_C_LIB_EXT`)
    * :kconfig:option:`CONFIG_GETOPT` (:kconfig:option:`CONFIG_POSIX_C_LIB_EXT`)
    * :kconfig:option:`CONFIG_MAX_PTHREAD_COUNT` (:kconfig:option:`CONFIG_POSIX_THREAD_THREADS_MAX`)
    * :kconfig:option:`CONFIG_MAX_PTHREAD_KEY_COUNT` (:kconfig:option:`CONFIG_POSIX_THREAD_KEYS_MAX`)
    * :kconfig:option:`CONFIG_MAX_TIMER_COUNT` (:kconfig:option:`CONFIG_POSIX_TIMER_MAX`)
    * :kconfig:option:`CONFIG_POSIX_LIMITS_RTSIG_MAX` (:kconfig:option:`CONFIG_POSIX_RTSIG_MAX`)
    * :kconfig:option:`CONFIG_POSIX_CLOCK` (:kconfig:option:`CONFIG_POSIX_CLOCK_SELECTION`,
      :kconfig:option:`CONFIG_POSIX_CPUTIME`, :kconfig:option:`CONFIG_POSIX_MONOTONIC_CLOCK`,
      :kconfig:option:`CONFIG_POSIX_TIMERS`, and :kconfig:option:`CONFIG_POSIX_TIMEOUTS`)
    * :kconfig:option:`CONFIG_POSIX_FS` (:kconfig:option:`CONFIG_POSIX_FILE_SYSTEM`)
    * :kconfig:option:`CONFIG_POSIX_MAX_FDS` (:kconfig:option:`CONFIG_POSIX_OPEN_MAX` and
      :kconfig:option:`CONFIG_ZVFS_OPEN_MAX`)
    * :kconfig:option:`CONFIG_POSIX_MAX_OPEN_FILES` (:kconfig:option:`CONFIG_POSIX_OPEN_MAX` and
      :kconfig:option:`CONFIG_ZVFS_OPEN_MAX`)
    * :kconfig:option:`CONFIG_POSIX_MQUEUE` (:kconfig:option:`CONFIG_POSIX_MESSAGE_PASSING`)
    * :kconfig:option:`CONFIG_POSIX_PUTMSG` (:kconfig:option:`CONFIG_XOPEN_STREAMS`)
    * :kconfig:option:`CONFIG_POSIX_SIGNAL` (:kconfig:option:`CONFIG_POSIX_SIGNALS`)
    * :kconfig:option:`CONFIG_POSIX_SYSCONF` (:kconfig:option:`CONFIG_POSIX_SINGLE_PROCESS`)
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

  * Added ARC MWDT toolchain support for ARC-V targets
  * Added HW memory barrier API support for multicore targets
  * Enabled TLS by default if C++ is used in case of ARC MWDT toolchain
  * Fixed the issue when mbedtls failed to build with ARC MWDT toolchain & minimal LibC due to
    incorrect define which mark support of bounds-checking interfaces C library extension
  * Fixed device deferred initialization in case of ARC MWDT toolchain

* ARM

  * Added initial support for Cortex-M85 Core

* ARM64

  * Implemented symbol names in the backtraces, enable by selecting :kconfig:option:`CONFIG_SYMTAB`

  * Add compiler tuning for Cortex-R82

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

  * Added :c:func:`k_uptime_seconds` function to simplify ``k_uptime_get() / 1000`` usage.

  * Added :c:func:`k_realloc`, that uses kernel heap to implement traditional :c:func:`realloc`
    semantics.

  * Devices can now store devicetree metadata such as nodelabels by turning on
    :kconfig:option:`CONFIG_DEVICE_DT_METADATA`. This option may be useful in
    e.g. shells as devices can be obtained using human-friendly names thanks to
    APIs like :c:func:`device_get_by_dt_nodelabel`.

  * Any device initialization can be deferred if its associated devicetree node
    has the special ``zephyr,deferred-init`` property set. The device can be
    initialized later in time by using :c:func:`device_init`.

  * The declaration of statically allocated thread stacks has been updated to utilize
    :c:macro:`K_THREAD_STACK_LEN` for both single thread stack declaration and array thread
    stack declarations. This ensures correct alignment for all thread stacks. For user
    threads, this may increase the size of the statically allocated stack objects depending
    on architecture alignment requirements.

  * Fix an edge case deadlock in :c:func:`k_thread_abort` (and join)
    where racing ISRs on SMP systems could become stuck spinning to
    signal each other's interrupted threads.

  * Fix a bug where :kconfig:option:`CONFIG_SCHED_SCALABLE` and
    :kconfig:option:`CONFIG_SCHED_DEADLINE` would corrupt the
    scheduling queue when used together.

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
    It works as a device that is the gateway of the audio. The typical device acting as Audio
    Gateway is a cellular phone. It controls the device (Hands-free Unit), that is the remote
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

  * Improve :c:func:`bt_foreach_bond` to support Bluetooth Classic key traversal.

* HCI Drivers

  * Completely redesigned HCI driver interface. See the Bluetooth HCI section in
    :ref:`migration_3.7` for more information.
  * Added support for Ambiq Apollo3 Blue series.
  * Added support for NXP RW61x.
  * Added support for Infineon CYW208XX.
  * Added support for Renesas SmartBond DA1469x.
  * Removed unmaintained B91 driver.
  * Added support for NXP IW612 on boards mimxrt1170_evkb and mimxrt1040_evk.
    It can be enabled by :kconfig:option:`CONFIG_BT_NXP_NW612`.

Boards & SoC Support
********************

* Added support for these SoC series:

  * Added support for Ambiq Apollo3 Blue and Apollo3 Blue Plus SoC series.
  * Added support for Synopsys ARC-V RMX1xx simulation platform.
  * Added support for STM32H7R/S SoC series.
  * Added support for NXP mke15z7, mke17z7, mke17z9, MCXNx4x, RW61x
  * Added support for Analog Devices MAX32 SoC series.
  * Added support for Infineon Technologies AIROC |trade| CYW20829 Bluetooth LE SoC series.
  * Added support for MediaTek MT8195 Audio DSPs
  * Added support for Nuvoton Numaker M2L31X SoC series.
  * Added support for the Microchip PolarFire ICICLE Kit SMP variant.
  * Added support for Renesas RA8 series SoC.

* Made these changes in other SoC series:

  * Intel ACE Audio DSP: Use dedicated registers to report boot status instead of arbitrary memory.
  * ITE: Rename the Kconfig symbol for all ITE SoC variants.
  * STM32: Enabled ART Accelerator, I-cache, D-cache and prefetch on compatible series.
  * STM32H5: Added support for Stop mode and :kconfig:option:`CONFIG_PM`.
  * STM32WL: Decreased Sub-GHz SPI frequency from 12 to 8MHz.
  * STM32C0: Added support for :kconfig:option:`CONFIG_POWEROFF`.
  * STM32U5: Added support for Stop3 mode.
  * Synopsys:

    * nsim: split nsim platform for arc_classic (ARCv2 and ARCv3 ISA based) and arc_v (RISC-V ISA based)
    * nsim/nsim_hs5x/smp: align sys clock frequency with other SMP nSIM configs

  * NXP IMX8M: added resource domain controller support
  * NXP s32k146: set RTC clock source to internal oscillator
  * GD32F4XX: Fixed an incorrect uart4 irq number.
  * Nordic nRF54L: Added support for the FLPR (fast lightweight processor) RISC-V CPU.
  * Espressif: Removed idf-bootloader dependency from all ESP32 SoC variants.
  * Espressif: Added Simple boot support for ESP32 SoC variants, which allows loading application
    using a single binary image without a 2nd stage bootloader.
  * Espressif: Re-worked and optimized all SoCs memory map.
  * LiteX:

    * Added support for :c:func:`sys_arch_reboot()`.
    * :kconfig:option:`CONFIG_RISCV_ISA_EXT_A` is no longer erroneously y-selected.
  * rp2040: The proprietary UART driver has been discontinued and replaced with PL011.

  * Renesas RZ/T2M: Added default values for System Clock Control register.

* Added support for these boards:

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
  * Added support for NXP boards: ``frdm_mcxn947``, ``ke17z512``, ``rd_rw612_bga``, ``frdm_rw612``, ``frdm_ke15z``, ``frdm_ke17z``
  * Added support for :ref:`Synopsys ARC-V RMX1xx nSIM-based simulation platform <nsim_arc_v>`: ``nsim_arc_v/rmx100``.
  * Added support for :ref:`Analog Devices MAX32690EVKIT <max32690_evkit>`: ``max32690evkit``.
  * Added support for :ref:`Analog Devices MAX32680EVKIT <max32680_evkit>`: ``max32680evkit``.
  * Added support for :ref:`Analog Devices MAX32672EVKIT <max32672_evkit>`: ``max32672evkit``.
  * Added support for :ref:`Analog Devices MAX32672FTHR <max32672_fthr>`: ``max32672fthr``.
  * Added support for :ref:`Analog Devices MAX32670EVKIT <max32670_evkit>`: ``max32670evkit``.
  * Added support for :ref:`Analog Devices MAX32655EVKIT <max32655_evkit>`: ``max32655evkit``.
  * Added support for :ref:`Analog Devices MAX32655FTHR <max32655_fthr>`: ``max32655fthr``.
  * Added support for :ref:`Analog Devices AD-APARD32690-SL <ad_apard32690_sl>`: ``ad_apard32690_sl``.
  * Added support for :ref:`Infineon Technologies CYW920829M2EVK-02 <cyw920829m2evk_02>`: ``cyw920829m2evk_02``.
  * Added support for :ref:`Nuvoton Numaker M2L31KI board <nuvoton_m2l31ki>`: ``numaker_m2l31ki``.
  * Added support for :ref:`Espressif ESP32-S2 DevKit-C <esp32s2_devkitc>`: ``esp32s2_devkitc``.
  * Added support for :ref:`Espressif ESP32-S3 DevKit-C <esp32s3_devkitc>`: ``esp32s3_devkitc``.
  * Added support for :ref:`Espressif ESP32-C6 DevKit-C <esp32c6_devkitc>`: ``esp32c6_devkitc``.
  * Added support for :ref:`Waveshare ESP32-S3-Touch-LCD-1.28 <esp32s3_touch_lcd_1_28>`: ``esp32s3_touch_lcd_1_28``.
  * Added support for :ref:`M5Stack ATOM Lite <m5stack_atom_lite>`: ``m5stack_atom_lite``.
  * Added support for :ref:`CTHINGS.CO Connectivity Card nRF52840 <ctcc_nrf52840>`: ``ctcc_nrf52840``.

* Made these board changes:

  * On :ref:`ST STM32H7B3I Discovery Kit <stm32h7b3i_dk_board>`: ``stm32h7b3i_dk_board``,
    enabled full cache management, Chrom-ART, double frame buffer and full refresh for
    optimal LVGL performance.
  * On ST STM32 boards, stm32cubeprogrammer runner can now be used to program external
    flash using ``--extload`` option.
  * Add HEX file support for Linkserver to all NXP boards
  * Updated the Linkserver west runner to reflect changes to the CLI of LinkServer v1.5.xx
  * Add LinkServer support to NXP ``mimxrt1010_evk``, ``mimxrt1160_evk``, ``frdm_rw612``, ``rd_rw612_bga``, ``frdm_mcxn947``
  * Introduced the simulated :ref:`nrf54l15bsim<nrf54l15bsim>` target.
  * The nrf5x bsim targets now support BT LE Coded PHY.
  * LLVM fuzzing support has been refactored while adding support for it in native_sim.
  * nRF54H20 PDK (pre-release) converted to :ref:`nrf54h20dk_nrf54h20`
  * PPR core target in :ref:`nrf54h20dk_nrf54h20` runs from RAM by default. A
    new ``xip`` variant has been introduced which runs from MRAM (XIP).
  * Refactored :ref:`beagleconnect_freedom` external antenna switch handling.
  * Added Arduino dts node labels for the nRF5340 Audio DK.
  * Changed the default revision of the nRF54L15 PDK from 0.2.1 to 0.3.0.
  * In boards based on the nRF5340 SoC, replaced direct accesses to the register
    that controls the network core Force-OFF signal with a module that uses an
    on-off manager to keep track of the network core use and exposes its API
    in ``<nrf53_cpunet_mgmt.h>``.
  * Laird Connectivity boards are rebranded to Ezurio.

* Added support for the following shields:

  * :ref:`adafruit_2_8_tft_touch_v2` (``adafruit_2_8_tft_touch_v2``)
  * :ref:`adafruit_neopixel_grid_bff` (``adafruit_neopixel_grid_bff``)
  * :ref:`arduino_uno_click` (``arduino_uno_click``)
  * :ref:`dvp_fpc24_mt9m114` (``dvp_fpc24_mt9m114``)
  * :ref:`lcd_par_s035` (``lcd_par_s035``)
  * :ref:`mikroe_weather_click` (``mikroe_weather_click``)
  * :ref:`nxp_btb44_ov5640` (``nxp_btb44_ov5640``)
  * :ref:`reyax_lora` (``reyax_lora``)
  * :ref:`rk043fn02h_ct` (``rk043fn02h_ct``)
  * :ref:`rk043fn66hs_ctg` (``rk043fn66hs_ctg``)
  * :ref:`rpi_pico_uno_flexypin` (``rpi_pico_uno_flexypin``)
  * :ref:`seeed_xiao_expansion_board` (``seeed_xiao_expansion_board``)
  * :ref:`seeed_xiao_round_display` (``seeed_xiao_round_display``)
  * :ref:`sparkfun_carrier_asset_tracker` (``sparkfun_carrier_asset_tracker``)
  * :ref:`st_b_lcd40_dsi1_mb1166` (``st_b_lcd40_dsi1_mb1166``)
  * :ref:`waveshare_epaper` (``waveshare_epaper``)
  * :ref:`x_nucleo_bnrg2a1` (``x_nucleo_bnrg2a1``)

Build system and Infrastructure
*******************************

  * CI-enabled blackbox tests were added to verify the correctness of most Twister flags.

  * A ``socs`` folder for applications has been introduced that allows for Kconfig fragments and
    devicetree overlays that should apply to any board target using a particular SoC and board
    qualifier (:github:`70418`). Support has also been added to sysbuild (:github:`71320`).

  * :ref:`Board/SoC flashing configuration<flashing-soc-board-config>` settings have been added
    (:github:`69748`).

  * Deprecated the global CSTD cmake property in favor of the :kconfig:option:`CONFIG_STD_C`
    choice to select the C Standard version. Additionally, subsystems can select a minimum
    required C Standard version, with, for example, :kconfig:option:`CONFIG_REQUIRES_STD_C11`.

  * Fixed issue with passing UTF-8 configs to applications using sysbuild (:github:`74152`).

  * Fixed issue whereby domain file in sysbuild projects would be loaded and used with outdated
    information if sysbuild configuration was changed, and ``west flash`` was ran directly after
    (:github:`73864`).

  * Fixed issue with Zephyr modules not being listed in sysbuild if they did not have a Kconfig
    file set (:github:`72070`).

  * Added sysbuild ``SB_CONFIG_COMPILER_WARNINGS_AS_ERRORS`` Kconfig option to turn on
    "warning as error" toolchain flags for all images, if set (:github:`70217`).

  * Fixed issue whereby files used in a project (e.g. devicetree overlays or Kconfig fragments)
    were not correctly watched and CMake would not reconfigure if they were changed
    (:github:`74655`).

  * Added flash support for Intel Hex files for the LinkServer runner.

  * Added sysbuild ``sysbuild/CMakeLists.txt`` entry point and added support for
    ``APPLICATION_CONFIG_DIR`` which allows for adjusting how sysbuild functions (:github:`72923`).

  * Fixed issue with armfvp find path if it contained a colon-separated list (:github:`74868`).

  * Fixed issue with version.cmake field sizes not being enforced (:github:`74357`).

  * Fixed issue with sysbuild not clearing ``EXTRA_CONF_FILE`` before processing images which
    prevented this option being passed on to the image (:github:`74082`).

  * Added sysbuild root support which works similarly to the existing root module, adjusting paths
    relative to ``APP_DIR`` (:github:`73390`).

  * Added warning/error message for blobs that are missing (:github:`73051`).

  * Fixed issue with correct python executable detection on some systems (:github:`72232`).

  * Added support for enabling LTO for whole application (:github:`69519`).

  * Fixed ``FILE_SUFFIX`` issues relating to double application of suffixes, non-application in
    sysbuild and variable name clashes in CMake functions (:github:`70124`, :github:`71280`).

  * Added support for new aggressive size optimisation flag (for GCC and Clang) using
    :kconfig:option:`CONFIG_SIZE_OPTIMIZATIONS_AGGRESSIVE` (:github:`70511`).

  * Fixed issue with printing out ``BUILD_VERSION`` if it was empty (:github:`70970`).

  * Fixed sysbuild issue of ``sysbuild_cache_set()`` cmake function wrongly detecting partial
    matches for de-duplication (:github:`71381`).

  * Fixed issue with detecting wrong ``VERSION`` file (:github:`71385`).

  * Added support for disabling output disassembly having the source code in using
    :kconfig:option:`CONFIG_OUTPUT_DISASSEMBLY_WITH_SOURCE` (:github:`71535`).

  * Twister now supports ``--flash-before`` parameter that allows flashing DUT before
    opening serial port (:github:`47037`).

Drivers and Sensors
*******************

* ADC

  * Added ``ADC_DT_SPEC_*BY_NAME()`` macros to get ADC IO-channel information from DT by name.
  * Added support for voltage biasing:

    * Added a :kconfig:option:`CONFIG_ADC_CONFIGURABLE_VBIAS_PIN` selected by drivers that support
      voltage biasing.
    * Added a ``zephyr,vbias-pins`` property to the adc-controller base binding to describe voltage
      bias pins.
    * Implemented for the TI ADC114s08 ADC driver.
  * Sample changes

    * Renamed existing ADC sample to adc_dt.
    * Added a new sample called adc_sequence that shows more of the runtime
      :c:struct:`adc_sequence` features.
  * New ADC Drivers

    * Added driver for the ENE KB1200.
    * Added driver for the NXP GAU ADC.
  * ADI AD559x changes

    * Added support for ADI's ad5593.
    * Added I2C bus support for ADI ad559x.
    * Added configuration of internal reference voltage value to ad559x to support
      calls of :c:func:`adc_raw_to_millivolts()`.
    * Fixed issue with driver initialization causing improper operation in the ad559x driver
      regarding the availability of :kconfig:option:`CONFIG_THREAD_NAME`.
    * Improved the ADC read efficiency and validation in ad559x driver.
  * ESP32 changes

    * Updated ESP32 ADC driver to work with version 5.1 of hal_espressif.
    * Added support for DMA mode operation for ESP32S3 and ESP32C3.
  * nRF changes

    * Added support for nRF54L15 and nRF54H20 in the nrfx_saadc driver.
    * Improved the nRF SAADC driver by disabling burst mode on unused channels, avoiding freezes.
    * Fixed issue which allowed negative ADC readings in single-ended mode using the
      ``adc_nrfx_saadc.c`` device driver.
      Note that this fix prevents the nRF54H and nRF54L series from performing
      8-bit resolution single-ended readings due to hardware limitations.
  * NXP LPADC changes

    * Enabled acquisition time feature in the NXP LPADC driver.
    * Added support for regulator output as reference to NXP LPADC.
    * Changed phandle type DT property ``nxp,reference-supply`` to phandle-array type DT property
      ``nxp,references`` in ``nxp,lpc-lpadc`` binding. The NXP LPADC driver now supports passing
      the reference voltage value by using ``nxp,references``.
  * Smartbond changes

    * Added support for power management to the Smartbond SDADC and GPADC drivers.
    * Fixed support for :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` in the Smartbond ADC driver.
  * STM32 changes

    * Fixed various issues with DMA support in the STM32 ADC driver.
    * Added support for STM32H7R/S series.
  * Other driver changes

    * Added support for Nuvoton m2l31x in the numaker ADC driver.
    * Fixed issue with configuration register access in the ads1119 driver.
    * Fixed uninitialized value in kb1200 driver found in static analysis.
    * Fixed issue with :c:func:`adc_raw_to_millivolts` returning half the actual voltage with
      the tla2021 driver by correcting the reference voltage value.


  * Added support for Nuvoton Numaker M2L31X series.

* Battery

  * Added ``re-charge-voltage-microvolt`` property to the ``battery`` binding. This allows to set
    limit to automatically start charging again.

* Battery backed up RAM

  * Added support for STM32G0 and STM32H5 series.

* CAN

  * Extended support for automatic sample point location to also cover :c:func:`can_calc_timing` and
    :c:func:`can_calc_timing_data`.
  * Added optional ``min-bitrate`` devicetree property for CAN transceivers.
  * Added devicetree macros :c:macro:`DT_CAN_TRANSCEIVER_MIN_BITRATE` and
    :c:macro:`DT_INST_CAN_TRANSCEIVER_MIN_BITRATE` for getting the minimum supported bitrate of a CAN
    transceiver.
  * Added support for specifying the minimum bitrate supported by a CAN controller in the internal
    ``CAN_DT_DRIVER_CONFIG_GET`` and ``CAN_DT_DRIVER_CONFIG_INST_GET`` macros.
  * Added :c:func:`can_get_bitrate_min` and :c:func:`can_get_bitrate_max` for retrieving the minimum
    and maximum supported bitrate for a given CAN controller/CAN transceiver combination, reflecting
    that retrieving the bitrate limits can no longer fail. Deprecated the existing
    :c:func:`can_get_max_bitrate` API function.
  * Updated the CAN timing functions to take the minimum supported bitrate into consideration when
    validating the bitrate.
  * Made the ``sample-point`` and ``sample-point-data`` devicetree properties optional.
  * Renamed the ``bus_speed`` and ``bus_speed_data`` fields of :c:struct:`can_driver_config` to
    ``bitrate`` and ``bitrate_data``.
  * Added driver for :dtcompatible:`nordic,nrf-can`.
  * Added driver support for Numaker M2L31X to the :dtcompatible:`nuvoton,numaker-canfd` driver.
  * Added host communication test suite.

* Charger

  * Added ``chgin-to-sys-current-limit-microamp`` property to ``maxim,max20335-charger``.
  * Added ``system-voltage-min-threshold-microvolt`` property to ``maxim,max20335-charger``.
  * Added ``re-charge-threshold-microvolt`` property to ``maxim,max20335-charger``.
  * Added ``thermistor-monitoring-mode`` property to ``maxim,max20335-charger``.

* Clock control

  * Added support for Microcontroller Clock Output (MCO) on STM32H5 series.
  * Added support for MSI clock on STM32WL series.
  * Added driver for Analog Devices MAX32 SoC series.
  * Added support for Nuvoton Numaker M2L31X series.
  * Refactored ESP32 clock control driver to support ESP32-C6.
  * In LiteX (:file:`drivers/clock_control/clock_control_litex.c`) added return code checking for
    :c:func:`litex_clk_get_duty_cycle()` and :c:func:`litex_clk_get_clkout_divider`.

* Counter

  * Added support for Ambiq Apollo3 series.
  * Added support for STM32H7R/S series.
  * Added driver for LPTMR to NXP MCXN947
  * Added the ``resolution`` property in ``nxp,lptmr`` binding to represent the maximum width
    in bits the LPTMR peripheral uses for its counter.

* DAC

  * Added support for NXP RW SOC series DAC (:dtcompatible:`nxp,gau-dac`).
  * Added support for Analog Devices AD5691 / AD5692 / AD5693 DACs
    (:dtcompatible:`adi,ad5691`, :dtcompatible:`adi,ad5692` and :dtcompatible:`adi,ad5693`).
  * Added support for Texas Instruments DACx0501 series DACs (:dtcompatible:`ti,dacx0501`).

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
  * Added support for NXP MCXNx4x

* DMA

  * Error callback configuration renamed to better signal enable/disable status
  * Add support to NXP MCXN947

* DMIC

  * Added support for NXP ``rd_rw612_bga``

* Entropy

  * Added support for STM32H7R/S series.

* EEPROM

  * Added property for specifying ``address-width`` to :dtcompatible:`zephyr,i2c-target-eeprom`.

* eSPI

  * Renamed eSPI virtual wire direction macros, enum values and Kconfig to match the new
    terminology in eSPI 1.5 specification.

* Ethernet

  * Introduced :kconfig:option:`CONFIG_ETH_DRIVER_RAW_MODE`. This option allows building
    ethernet drivers without the zephyr L2 ethernet layer.
  * Removed the ethernet-fixed-link DT binding.
  * Removed VLAN handling from ethernet drivers since it is now handled by the
    generic ethernet L2 code.
  * Implemented/reworked HW MAC Address filtering in the eth_mcux, eth_nxp_enet,
    and eth_nxp_s32_gmac, eth_stm32, and eth_nxp_s32_netc drivers.
  * New Drivers

    * Added new eth_nxp_enet_qos driver for the ethernet controller present on NXP MCXN SOCs.
    * Added support for adin1100 phy.
    * Added support for the Realtek RTL8211F phy.
  * NXP ENET driver changes

    * eth_nxp_enet driver is no longer experimental.
    * Deprecated eth_mcux driver.
    * All boards and SOCs with :dtcompatible:`nxp,kinetis-ethernet` compatible nodes
      reworked to use the new :dtcompatible:`nxp,enet` binding.
    * Added support for network device power management with nxp_enet driver on Kinetis platforms.
    * Converted eth_nxp_enet driver to use a dedicated workqueue for RX
      managed by the kernel rather than a manual infinite loop.
    * Disabled hardware checksum acceleration when IPV6 is enabled with eth_nxp_enet, since
      the hardware does not support accelerating ICMPv6 checksums.
    * Added support for :dtcompatible:`nxp,enet1g`.
    * Added support to use a fused MAC address for nxp_enet MAC on some platforms.
    * Fixed issue with LAA bit not being set and a confusing description of the nxp,unique-mac
      property used with the nxp_enet driver.
    * Fixed cache maintain being enabled when using a noncache DMA buffer in nxp_enet driver.
    * Added MMIO mappings to nxp_enet driver.
    * Clarified DSA supported with eth_nxp_enet.
  * NXP S32 ethernet changes

    * The eth_nxp_s32_gmac driver now implies :kconfig:option:`CONFIG_MDIO`.
    * eth_nxp_s32_netc driver updated to use new MBOX API.
  * Adin2111 driver changes

    * Corrected the bitfield position of IAMSK1 TX_READY_MASK in adin2111 driver.
    * Changed adin2111 driver to always append crc32 to the end of the frame.
    * Adjusted eth_adin2111 driver to have the appropriate multicaster filter mask.
    * Fixed the "generic SPI without crc8" mode of adin2111 driver.
    * Added Open Alliance SPI protocol support to the adin2111 driver.
    * Added custom driver extension APIs for adin2111 driver.
    * Enabled support for promiscuous mode in the adin2111 driver.
    * Moved OA buffers out of device data of the adin2111 driver to save ~32KB of space
      when using the generic SPI protocol.
    * Fixed a build warning in eth_adin2111 driver on 64-bit platforms.
    * Various small changes to adin2111 driver.
  * STM32 ethernet driver changes

    * Added support for PTP on compatible STM32 series (STM32F7, STM32H5 and STM32H7).
    * Changed eth_stm32 to use phy APIs to access the phy to avoid collisions when multitasking.
    * Removed legacy STM32Cube HAL API support for STM32 F4, F7, and H7 series.
    * Added support for RX/TX timestamping to eth_stm32_hal driver.
  * ESP32 ethernet driver changes

    * Added support to esp32 ethernet driver to set the MAC address during runtime.
    * Updated esp32 ethernet driver to work with version 5.1 of hal_espressif.
    * Fixed build of esp32 ethernet driver when :kconfig:option:`CONFIG_NET_STATISTICS` is enabled.
    * Fixed ESP32 ethernet driver not clocking external PHY correctly over GPIO.
  * Other ethernet driver changes

    * Added link status detection to the w5500 ethernet driver, configurable via Kconfig.
    * Added ability to set MAC address at runtime with eth_liteeth driver.
    * Fixed issue in the eth_stellaris driver where it was previously not taken into account
      that the number of interrupts received by the driver may be less than the number of
      data packets received by the ethernet controller.
    * Added a devicetree property for the enc28j60 to set the RX filter.
    * Fixed ESTAT TXABRT bit not being cleared on error in the enc28j60 driver.
    * Added conditions to enable ptp_clock driver implementation for the native_posix
      ethernet driver when PTP subsystem is enabled.
    * Fixed DSA driver for KSZ8xxx to correctly initialize LAN devices.
    * Fixed the wrong register address being used for tail tag enable in ksz8863.
  * Phy driver changes

    * Fixed various control issues with the KSZ8081 phy driver regarding
      resets, autonegotiation, link detection, and missing/spamming logging messages.
    * Changed property names of the reset and interrupt gpios in the KSZ8081 DT binding.
    * Fixed bus fault in phy_mii driver when using fixed-link mode.

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
  * Added FlexSPI support to NXP MCXN947
  * Added support for Nuvoton Numaker M2L31X series.

* Fuel Gauge

  * max17048: Corrected voltage units from mV to uV.

* GNSS

  * Added GNSS device driver API test suite.
  * Added support for the u-blox UBX protocol.
  * Added device driver for the u-blox M10 GNSS modem (:dtcompatible:`u-blox,m10`).
  * Added device driver for the Luatos Air530z GNSS modem (:dtcompatible:`luatos,air530z`).

* GPIO

  * Added support for Ambiq Apollo3 series.
  * Added Broadcom Set-top box(brcmstb) SoC GPIO driver.
  * Added c:macro:`STM32_GPIO_WKUP` flag which allows to configure specific pins as wakeup source
    from Power Off state on STM32 L4, U5, WB, & WL SoC series.
  * Added driver for Analog Devices MAX32 SoC series.
  * Added support for Nuvoton Numaker M2L31X series.
  * Added interrupt support to the Renesas RZ/T2M GPIO driver (:dtcompatible:`renesas,rzt2m-gpio`).

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
  * Added support to NXP MCXN947
  * Added driver for Analog Devices MAX32 SoC series.
  * Added support for Nuvoton Numaker M2L31X series.
  * LiteX I2C driver (:file:`drivers/i2c/i2c_litex.c`):

    * Added support for bitrate setting from the devicetree.
    * Added :c:func:`i2c_litex_recover_bus()` and :c:func:`i2c_litex_get_config()` API
      implementations.

* I2S

  * Added support for STM32H5 series.
  * Extended the MCUX Flexcomm driver to support additional channels and formats.
  * Added support for Nordic nRF54L Series.
  * Fixed divider calculations in the nRF I2S driver.

* I3C

  * Added shell support for querying bus and CCC commands.

  * Added driver to support the I3C controller on NPCX.

  * Improvements and bug fixes on :dtcompatible:`nxp,mcux-i3c`, including handling the bus
    being busy more gracefully instead of simply returning errors.

* Input

  * New drivers: :dtcompatible:`adc-keys`, :dtcompatible:`chipsemi,chsc6x`,
    :dtcompatible:`cirque,pinnacle`, :dtcompatible:`futaba,sbus`,
    :dtcompatible:`pixart,pat912x`, :dtcompatible:`pixart,paw32xx`,
    :dtcompatible:`pixart,pmw3610` and :dtcompatible:`sitronix,cf1133`.
  * Migrated :dtcompatible:`holtek,ht16k33` and
    :dtcompatible:`microchip,xec-kbd` from kscan to input subsystem.

* LED

  * Added device completion to LED shell commands and made the ``get_info`` command display
    colors as strings.

  * Added driver for Lumissil Microsystems (a division of ISSI) IS31FL3194 controller
    (:dtcompatible:`issi,is31fl3194`).

* LED Strip

  * The ``chain-length`` and ``color-mapping`` properties have been added to all LED strip
    bindings.

  * The length of a strip is now checked before updating it, an error is returned if the provided
    data is too long.

  * A length function has been added which returns the length of the LED strip
    (:c:func:`led_strip_length`).

  * The update channels function is now optional and can be left unimplemented.

  * The ``in-gpios`` and ``output-pin`` properties of the respective
    :dtcompatible:`worldsemi,ws2812-gpio` and :dtcompatible:`worldsemi,ws2812-rpi_pico-pio`
    devicetree bindings have been renamed to ``gpios``.

  * Removed ``CONFIG_WS2812_STRIP`` and ``CONFIG_WS2812_STRIP_DRIVER`` Kconfig options. They became
    useless after refactoring.

  * Added driver for Texas Instruments TLC59731 RGB controller.

* LoRa

  * Added driver for Reyax LoRa module

* Mailbox

  * Added support for HSEM based STM32 driver.

* MDIO

  * Made the ``bus_enable`` and ``bus_disable`` functions optional for drivers to
    implement, and removed empty implementation from many drivers.
  * Added NXP ENET QOS MDIO controller driver.
  * Fixed but with NXP ENET MDIO driver blocking the system workqueue.
  * :kconfig:option:`CONFIG_MDIO_NXP_ENET_TIMEOUT` units change to microseconds.
  * Added support for STM32 MDIO controller driver.

* MFD

  * New driver :dtcompatible:`nxp,lp-flexcomm`.
  * New driver :dtcompatible:`rohm,bd8lb600fs`.
  * New driver :dtcompatible:`maxim,max31790`.
  * New driver :dtcompatible:`infineon,tle9104`
  * New driver :dtcompatible:`adi,ad559x`
  * Added option to disable N_VBUSEN for :dtcompatible:`x-powers,axp192`.
  * Added GPIO input edge events for :dtcompatible:`nordic,npm1300`.
  * Added long press reset configuration for :dtcompatible:`nordic,npm1300`.
  * Fixed initialisation of hysteretic mode for :dtcompatible:`nordic,npm6001`.

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
    example of how to implement and use the modem pipelink module.

* PCIE

  * ``pcie_bdf_lookup`` and ``pcie_probe`` have been removed since they have been
    deprecated since v3.3.0.

* MIPI-DBI

  * Added release API
  * Added support for mode selection via the device tree

* MSPI

  * Add the new experimental :ref:`MSPI(Multi-bit SPI) <mspi_api>` API, enabling support for
    advanced SPI controllers and peripherals that typically require command, address and data
    phases as well as variable latency for a transfer. The API now supports from single wire
    SDR up to hex wires DDR communication in sync/async ways.
  * Added MSPI bus emulator under bus emulators to showcase the implementation of the MSPI API.
  * Added MSPI flash device emulator to showcase the use of the MSPI API and interfacing with
    MSPI bus controllers.
  * Added APS6404L QPI pSRAM device driver.
  * Added ATXP032 OPI NOR flash device driver.
  * Added Ambiq Apollo3p MSPI controller driver.
  * Added :zephyr:code-sample:`mspi-async` and :zephyr:code-sample:`mspi-flash` samples to
    showcase the use of MSPI device drivers.
  * Added mspi/api and mspi/flash testcase for developers to check their implementations.

* Pin control

  * Added driver for Renesas RA8 series
  * Added driver for Infineon PSoC6 (legacy)
  * Added driver for Analog Devices MAX32 SoC series.
  * Added driver for Ambiq Apollo3
  * Added driver for ENE KB1200
  * Added driver for NXP RW
  * Espressif driver now supports ESP32C6
  * STM32 driver now supports remap functionality for STM32C0
  * Added support for Nuvoton Numaker M2L31X series.

* PWM

  * Added support for STM32H7R/S series.
  * Added a Add QTMR PWM driver for NXP imxrt11xx
  * Made the NXP MCUX PWM driver thread safe
  * Fix zephyr:code-sample:`pwm-blinky` code sample to demonstrate PWM support for
    :ref:`beagleconnect_freedom`.
  * Added driver for ENE KB1200.
  * Added support for Nordic nRF54H and nRF54L Series SoCs.
  * Added support for Nuvoton Numaker M2L31X series.

* Regulators

  * New driver :dtcompatible:`cirrus,cp9314`.
  * Added ``regulator-boot-off`` property to common regulator driver.
    Updated :dtcompatible:`adi,adp5360-regulator`, :dtcompatible:`nordic,npm1300-regulator`,
    :dtcompatible:`nordic,npm6001-regulator` and :dtcompatible:`x-powers,axp192-regulator`
    to use this new property.
  * Added power management for :dtcompatible:`renesas,smartbond-regulator`.
  * Added ``is_enabled`` shell command.
  * Removed use of busy wait for single threaded systems.
  * Fixed control of DCDC2 output for :dtcompatible:`x-powers,axp192-regulator`.
  * Fixed current and voltage get functions for :dtcompatible:`renesas,smartbond-regulator`.
  * Fixed NXP VREF Kconfig leakage.
  * Fixed display of micro values in shell.
  * Fixed strcmp usage bug in ``adset`` shell command.

* Reset

  * Added driver for reset controller on Nuvoton NPCX chips.
  * Added reset controller driver for NXP SYSCON.
  * Added reset controller driver for NXP RSTCTL.
  * Added support for Nuvoton Numaker M2L31X series.

* RTC

  * Added Raspberry Pi Pico RTC driver.
  * Added support for :kconfig:option:`CONFIG_RTC_ALARM` on all STM32 MCU series (except STM32F1).
  * Added support for Nuvoton Numaker M2L31X series.

* RTIO

  * Move lock-free queues out of RTIO into lib, dropping the ``rtio_`` prefix to SPSC and MPSC queues.
  * Added tests and fixed bugs related to chained callback requests.
  * Wrapper around p4wq (rtio workq) created to go from blocking to non-blocking behavior in cases
    where native asynchronous RTIO functionality is unavailable.

* SDHC

  * Added ESP32 SDHC driver (:dtcompatible:`espressif,esp32-sdhc`).
  * Added SDHC driver for Renesas MMC controller (:dtcompatible:`renesas,rcar-mmc`).

* Sensors

  * General

    * Added a channel specifier to the new read/decoder API.
    * Added a blocking sensor read call :c:func:`sensor_read`.
    * Decoupled RTIO requests using RTIO workqueues service to turn
      :c:func:`sensor_submit_callback` into an asynchronous request.
    * Moved most drivers to vendor subdirectories.

  * AMS

    * Added TSL2591 light sensor driver (:dtcompatible:`ams,tsl2591`).

  * Aosong

    * Added DHT20 digital-output humidity and temperature sensor driver
      (:dtcompatible:`aosong,dht20`).

    * Added :kconfig:option:`CONFIG_DHT_LOCK_IRQS` for the dht11 driver which allows for locking
      interrupts during sensor reading to prevent issues with reading the sensor.

  * Bosch

    * Updated BME280 to the new async API.

  * Infineon

    * Added TLE9104 power train switch diagnostics sensor driver
      (:dtcompatible:`infineon,tle9104-diagnostics`).

  * Maxim

    * Added DS18S20 1-Wire temperature sensor driver (:dtcompatible:`maxim,ds18s20`).
    * Added MAX31790 fan speed and fan fault sensor
      (:dtcompatible:`maxim,max31790-fan-fault` and :dtcompatible:`maxim,max31790-fan-speed`).

  * NXP

    * Added low power comparator driver (:dtcompatible:`nxp,lpcmp`).

  * Rohm

    * Added BD8LB600FS diagnostics sensor driver (:dtcompatible:`rohm,bd8lb600fs-diagnostics`).

  * Silabs

    * Made various fixes and enhancements to the SI7006 humidity/temperature sensor driver.

  * ST

    * QDEC driver now supports encoder mode configuration (see :dtcompatible:`st,stm32-qdec`).
    * Added support for STM32 Digital Temperature Sensor (:dtcompatible:`st,stm32-digi-temp`).
    * Added IIS328DQ I2C/SPI accelerometer sensor driver (:dtcompatible:`st,iis328dq`).

  * TDK

    * Added support for the MPU6500 3-axis accelerometer and 3-axis gyroscope sensor to the
      MPU6050 driver.

  * TI

    * Added TMP114 driver (:dtcompatible:`ti,tmp114`).
    * Added INA226 bidirectional current and power monitor driver (:dtcompatible:`ti,ina226`).
    * Added LM95234 quad remote diode and local temperature sensor driver
      (:dtcompatible:`national,lm95234`).

  * Other vendors

    * Added Angst+Pfister FCX-MLDX5 O2 sensor driver (:dtcompatible:`ap,fcx-mldx5`).
    * Added ENE KB1200 tachometer sensor driver (:dtcompatible:`ene,kb1200-tach`).
    * Added Festo VEAA-X-3 series proportional pressure regulator driver
      (:dtcompatible:`festo,veaa-x-3`).
    * Added Innovative Sensor Technology TSic xx6 temperature sensor driver
      (:dtcompatible:`ist,tsic-xx6`).
    * Added ON Semiconductor NCT75 temperature sensor driver (:dtcompatible:`onnn,nct75`).
    * Added ScioSense ENS160 digital metal oxide multi-gas sensor driver
      (:dtcompatible:`sciosense,ens160`).
    * Made various fixes and enhancements to the GROW_R502A fingerprint sensor driver.

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

  * Added support for Nuvoton Numaker M2L31X series.

* SPI

  * Added support to NXP MCXN947
  * Added support for Ambiq Apollo3 series general IOM based SPI.
  * Added support for Ambiq Apollo3 BLEIF based SPI, which is specific for internal HCI.
  * Added support for :kconfig:option:`CONFIG_PM` and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` on STM32 SPI driver.
  * Added support for :kconfig:option:`CONFIG_NOCACHE_MEMORY` in DMA SPI mode for STM32F7x SoC series.
  * Added support for STM32H7R/S series.
  * Added driver for Analog Devices MAX32 SoC series.
  * Fixed an incorrect register assignment in gd32 spi.

* USB

  * Added UDC shim driver for NXP EHCI and IP3511 USB controller.
  * Various fixes and improvements in IT82xx2, DWC2, STM32, RP2040, Smartbond
    USB controller drivers.

* Video

  * Added support for STM32 Digital camera interface (DCMI) driver (:dtcompatible:`st,stm32-dcmi`).
  * Enabled NXP USB Device controllers
  * Added support for the ov7670 camera
  * Added support for the ov5640 camera
  * Added CSI-2 MIPI driver for NXP MCUX
  * Added support for DVP FPC 24-pins mt9m114 camera module shield

* Watchdog

  * Added :kconfig:option:`CONFIG_WDT_NPCX_WARNING_LEADING_TIME_MS` to set the leading warning time
    in milliseconds. Removed no longer used :kconfig:option:`CONFIG_WDT_NPCX_DELAY_CYCLES`.
  * Added support for Ambiq Apollo3 series.
  * Added support for STM32H7R/S series.
  * Added support for Nuvoton Numaker M2L31X series.
  * Added watchdog for external 32kHz crystal in ESP32 SoC variants.

* Wi-Fi

  * Fixed message parsing for esp-at.
  * Fixed esp-at connect failures.
  * Implement :c:func:`bind` and :c:func:`recvfrom` for UDP sockets for esp-at.
  * Added option for setting maximum data size for eswifi.
  * Fixed ESP32 Wi-Fi driver memory leak.

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
  * Fixed client ID generation in DHCPv4 server.
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
  * Added POLLOUT monitoring when sending response in HTTP client.

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
    detection conflicts were dropped.
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

  * Improved overall networking API Doxygen documentation.
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
    (for example, packet capture).
  * Implemented pseudo interface, a.k.a "any" interface for packet capture use
    case.
  * Added cooked mode capture support. This allows non-IP based network data capture.
  * Generate network events when starting or stopping packet capture.
  * Removed obsolete and unused ``tcp_first_msg`` :c:struct:`net_pkt` flag.
  * Added new :zephyr:code-sample:`secure-mqtt-sensor-actuator` sample.
  * Added support for partial L3 and L4 checksum offloading.
  * Updated :zephyr:code-sample:`mqtt-azure` with new CA certificates, the current
    on expires soon.
  * Added new driver for Native Simulator offloaded sockets.
  * Overhauled VLAN support to use Virtual network interfaces.
  * Added statistics collection for Virtual network interfaces.
  * Fixed system workqueue block in :c:func:`mgmt_event_work_handler`
    when :kconfig:option:`CONFIG_NET_MGMT_EVENT_SYSTEM_WORKQUEUE` is enabled.

* MQTT:

  * Added ALPN support for MQTT TLS backend.
  * Added user data field in :c:struct:`mqtt_client` context structure.
  * Fixed a potential socket leak in MQTT Websockets transport.

* Network Interface:

  * Added new API functions:

    * :c:func:`net_if_ipv4_maddr_foreach`
    * :c:func:`net_if_ipv6_maddr_foreach`

  * Improved debug logging in the network interface code.
  * Added reference counter to the :c:struct:`net_if_addr` structure.
  * Fixed IPv6 DAD and MLDv2 operation when interface goes up.
  * Added unique default name for OpenThread interfaces.
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
  * Fixed :c:func:`net_socket_service_register` function stall in case socket
    services thread stopped.
  * Fixed potential socket services thread stoppage when deregistering service.
  * Removed support for asynchronous timeouts in socket services library.
  * Fixed potential busy looping when using :c:func:`zsock_accept` in case of
    file descriptors shortage.

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
  * ISN generation now uses SHA-256 instead of MD5. Moreover, it now relies on PSA APIs
    instead of legacy Mbed TLS functions for hash computation.
  * Improved ACK reply logic in case no PSH flag is present to reduce redundant ACKs.

* Websocket:

  * Added new Websocket APIs:

    * :c:func:`websocket_register`
    * :c:func:`websocket_unregister`

  * Converted Websocket library to use ``zsock_*`` API.
  * Added Object Core support to Websocket sockets.
  * Added POLLOUT monitoring when sending.

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
  * Improved performance in the default configuration for the zperf sample.

USB
***

* New USB device stack:

  * Added support for HID devices
  * Introduced speed-specific configurations and made high-speed support
    compliant with the USB2.0 specification
  * Added notification support and initial BOS support

Devicetree
**********

* Added :c:macro:`DT_INST_NODE_HAS_COMPAT` to check if a node has a compatible.
  This is useful for nodes that have multiple compatibles.
* Added :c:macro:`DT_CHILD_NUM` and variants to count the number of children of a node.
* Added :c:macro:`DT_FOREACH_NODELABEL` and variants, which can be used to iterate over the
  node labels of a devicetree node.
* Added :c:macro:`DT_NODELABEL_STRING_ARRAY` and :c:macro:`DT_NUM_NODELABELS` and their variants.
* Added :c:macro:`DT_REG_HAS_NAME` and variants.
* Reworked :c:macro:`DT_ANY_INST_HAS_PROP_STATUS_OKAY` so that the result can
  be used with macros like :c:macro:`IS_ENABLED`, IF_ENABLED, or COND_CODE_x.
* Reworked :c:macro:`DT_NODE_HAS_COMPAT_STATUS` so that it can be evaluated at preprocessor time.
* Updated PyYaml version used in dts scripts to 6.0 to remove supply chain vulnerabilities.

Kconfig
*******

* Added a ``substring`` Kconfig preprocessor function.
* Added a ``dt_node_ph_prop_path`` Kconfig preprocessor function.
* Added a ``dt_compat_any_has_prop`` Kconfig preprocessor function.

Libraries / Subsystems
**********************

* Debug

  * symtab

   * By enabling :kconfig:option:`CONFIG_SYMTAB`, the symbol table will be
     generated with Zephyr link stage executable on supported architectures.

* Demand Paging

  * NRU (Not Recently Used) eviction algorithm has updated its selection logic to avoid
    picking the same page to evict constantly. The updated login now searches for a new
    candidate linearly after the last evicted page.

  * Added LRU (Least Recently Used) eviction algorithm.

* Formatted output

  * Fix warning when compiling cbprintf with ARCMWDT.

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

    * Fixed an issue with the SMP structure not being packed which would cause a fault on devices
      that do not support unaligned memory accesses.

    * Added :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_DYNAMIC_SVC_REGISTRATION` that allows users
      to select whether MCUmgr BT service is statically registered at compile time or
      dynamically at run time.

    * In FS group, TinyCrypt has been replaced with PSA calls for SHA calculation.

* Logging

  * By enabling :kconfig:option:`CONFIG_LOG_BACKEND_NET_USE_DHCPV4_OPTION`, the IP address of the
    syslog server for the networking backend is set by the DHCPv4 Log Server Option (7).

  * Use real time clock as timestamp on POSIX.

  * Add support for syslog (POSIX).

  * Add :c:macro:`LOG_WRN_ONCE` for logging warning message where only the first occurrence is
    logged.

  * Add :c:func:`log_thread_trigger` for triggering processing of the log messages.

  * Fix case when deferred logging not compiling when :kconfig:option:`CONFIG_MULTITHREADING` was
    disabled.

  * Fix case when logging strings could be stripped from the binary when dictionary based logging
    was mixed with non-dictionary.

  * Fix dictionary database not being generated in certain situations.

  * Fix dictionary logging parser not handling long long arguments correctly.

  * Fix support for :kconfig:option:`CONFIG_LOG_MSG_APPEND_RO_STRING_LOC`.

* Modem modules

  * Added modem pipelink module which shares modem pipes globally, allowing device drivers to
    create and set up pipes for the application to use.

  * Simplified the modem pipe module's synchronization mechanism to only protect the callback
    and user data. This matches the actual in-tree usage of the modem pipes.

  * Added ``modem_stats`` module which tracks the usage of buffers throughout the modem
    subsystem.

* Power management

  * Devices can now declare which system power states cause power loss.
    This information can be used to set and release power state
    constraints when it is needed by the device. This feature is enabled with
    :kconfig:option:`CONFIG_PM_POLICY_DEVICE_CONSTRAINTS`. Use functions
    :c:func:`pm_policy_device_power_lock_get` and :c:func:`pm_policy_device_power_lock_put`
    to lock and unlock all power states that cause power loss in a device.

  * Added shell support for device power management.

  * Device power management was de-coupled from system power management. The new
    :kconfig:option:`CONFIG_PM_DEVICE_SYSTEM_MANAGED` option is used to enable
    whether devices must be suspended when the system sleeps.

  * Make it possible to disable system device power management individually per
    power state using ``zephyr,pm-device-disabled``. This allows targets tuning which
    states should (and which should not) trigger device power management.

* Crypto

  * TinyCrypt remains available but is now being phased out in favor
    of PSA Crypto for enhanced security and performance.
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

* FPGA

  * Improve handling of drivers missing ``reset``, ``load``, ``get_status``, and ``get_info``
    methods.
  * Add support for Agilex and Agilex 5.

* Random

  * Besides the existing :c:func:`sys_rand32_get` function, :c:func:`sys_rand8_get`,
    :c:func:`sys_rand16_get` and :c:func:`sys_rand64_get` are now also available.
    These functions are all implemented on top of :c:func:`sys_rand_get`.

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

  * Added :kconfig:option:`CONFIG_NVS_DATA_CRC`, to add CRC protection for data.
    Note that enabling this option makes NVS incompatible with existing storage
    that have not been previously using CRC on data.

  * Fixed NVS issue where :c:func:`nvs_calc_free_space` would return larger
    size than available, because space for reserved ate was not subtracted.

  * Fixed ext2 incorrectly calculating free space when attempting to format
    partition.

  * Fixed FAT driver leaving disk in initialized state after unmount.

* Task Watchdog

  * Added shell (mainly for testing purposes during development).

* POSIX API

  * Improved Kconfig options to reflect standard POSIX Options and Option Groups.

  * Added support for the following Option Groups

    * :ref:`POSIX_MAPPED_FILES <posix_option_group_mapped_files>`
    * :ref:`POSIX_MEMORY_PROTECTION <posix_option_group_memory_protection>`
    * :ref:`POSIX_NETWORKING <posix_option_group_networking>`
    * :ref:`POSIX_SINGLE_PROCESS <posix_option_group_single_process>`
    * :ref:`POSIX_TIMERS <posix_option_group_timers>`
    * :ref:`XSI_SYSTEM_LOGGING <posix_option_group_xsi_system_logging>`

  * Added support for the following Options

    * :ref:`_POSIX_ASYNCHRONOUS_IO <posix_option_asynchronous_io>`
    * :ref:`_POSIX_CPUTIME <posix_option_cputime>`
    * :ref:`_POSIX_FSYNC <posix_option_fsync>`
    * :ref:`_POSIX_MEMLOCK <posix_option_memlock>`
    * :ref:`_POSIX_MEMLOCK_RANGE <posix_option_memlock_range>`
    * :ref:`_POSIX_READER_WRITER_LOCKS <posix_option_reader_writer_locks>`
    * :ref:`_POSIX_SHARED_MEMORY_OBJECTS <posix_shared_memory_objects>`
    * :ref:`_POSIX_THREAD_CPUTIME <posix_option_thread_cputime>`
    * :ref:`_POSIX_THREAD_PRIO_PROTECT <posix_option_thread_prio_protect>`
    * :ref:`_POSIX_THREAD_PRIORITY_SCHEDULING <posix_option_thread_priority_scheduling>`
    * :ref:`_XOPEN_STREAMS <posix_option_xopen_streams>`

  * Fixed eventfd ``F_SETFL`` handling to avoid overwriting internal flags.
  * Fixed thread stack address printed in debug message.
  * Fixed macro parameter usage in signal code.

* LoRa/LoRaWAN

  * Added the Fragmented Data Block Transport service, which can be enabled via
    :kconfig:option:`CONFIG_LORAWAN_FRAG_TRANSPORT`. In addition to the default fragment decoder
    implementation from Semtech, an in-tree implementation with reduced memory footprint is
    available.

  * Added a sample to demonstrate LoRaWAN firmware-upgrade over the air (FUOTA).

* ZBus

  * Improved the VDED process by optimizing the channel reference copying for the clones delivered
    during the message subscriber delivery notification.

  * Improved the initialization phase by statically initiating the semaphores and runtime observer
    list. That decreased the duration of the zbus initialization.

  * Added a way of isolating a channel message subscribers pool. Some channels can now share an
    isolated pool to avoid delivery failures and shorten communication latency. It is only necessary
    to enable the :kconfig:option:`CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_ISOLATION` and use the
    function :c:func:`zbus_chan_set_msg_sub_pool` to change the msg pool used by the channel.
    Channels can share the same message pool.

HALs
****

* Nordic

  * Updated nrfx to version 3.5.0.
  * Added nRF Services (nrfs) library.

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

* ADI

  * Introduced the ``hal_adi`` module, which is a subset of the Maxim Software
    Development Kit (MSDK) that contains device header files and bare metal
    peripheral drivers (:github:`72391`).

* Espressif

  * Updated HAL to version v5.1, which has new SoCs low-level files.

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

  * Fixed ASN.1 support for mbedtls version >= 3.1

  * Fixed bootutil signed/unsigned comparison in ``boot_read_enc_key``

  * Updated imgtool version.py to take command line arguments

  * Added imgtool improvements to dumpinfo

  * Fixed various imgtool dumpinfo issues

  * Fixed imgtool verify command for edcsa-p384 signed images

  * Added support for NXP MCXN947

  * The MCUboot version in this release is version ``2.1.0+0-dev``.

OSDP
****

* Fixed issue in CP secure channel handshake where R-MAC can be reverted to an
  old one by a rogue PD sending an out-of-order secure channel response resulting
  in a replay attack.

Trusted Firmware-M
******************

* TF-M was updated to 2.1.0. Release notes can be found at:
  https://tf-m-user-guide.trustedfirmware.org/releases/2.1.0.html

* Support for MCUboot signature types other than RSA-3072 has been added.
  The type can be chosen with the :kconfig:option:`CONFIG_TFM_MCUBOOT_SIGNATURE_TYPE` Kconfig option.
  Using EC-P256, the new default, reduces flash usage by several KBs compared to RSA.

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
    (e.g.: Console and Logging examples).

  * Removed ``GSM_PPP`` specific configuration overlays from samples ``net/cloud/tagoio`` and
    ``net/mgmt/updatehub``. The ``GSM_PPP`` device driver has been deprecated and removed. The new
    ``MODEM_CELLULAR`` device driver which replaces it uses the native networking stack and ``PM``
    subsystem, which like ethernet, requires no application specific actions to set up networking.

  * Removed ``net/gsm_modem`` sample as the ``GSM_PPP`` device driver it depended on has been
    deprecated and removed. The sample has been replaced by the sample ``net/cellular_modem``
    based on the ``MODEM_CELLULAR`` device driver.

  * BT LE Coded PHY is now runtime tested in CI with the nrf5x bsim targets.

  * External ethernet network interfaces have been disabled in the ``tests/net`` tests, since these
    tests are meant to use simulated network interfaces.

Issue Related Items
*******************

Known Issues
============

- :github:`74345` - Bluetooth: Non functional on nRF51 with fault
