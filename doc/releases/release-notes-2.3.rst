:orphan:

.. _zephyr_2.3:

Zephyr 2.3.0
############

We are pleased to announce the release of Zephyr RTOS version 2.3.0.

Major enhancements with this release include:

* A new Zephyr CMake package has been introduced, reducing the need for
  environment variables
* A new Devicetree API, based on hierarchical macros, has been introduced. This
  new API allows the C code to access virtually all nodes and properties in a
  clean, organized fashion
* The kernel timeout API has been overhauled to be flexible and configurable,
  with future support for features like 64-bit and absolute timeouts in mind
* A new k_heap/sys_heap heap allocator has been introduced, with much better
  performance than the existing k_mem_pool/sys_mem_pool
* Zephyr now integrates with the TF-M (Trusted Firmware M) PSA-compliant
  framework
* The Bluetooth Low Energy Host now supports LE Advertising Extensions
* The CMSIS-DSP library is now included and integrated

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* CVE-2020-10022: UpdateHub Module Copies a Variable-Sized Hash String
  into a fixed-size array.
* CVE-2020-10059: UpdateHub Module Explicitly Disables TLS
  Verification
* CVE-2020-10061: Improper handling of the full-buffer case in the
  Zephyr Bluetooth implementation can result in memory corruption.
* CVE-2020-10062: Packet length decoding error in MQTT
* CVE-2020-10063: Remote Denial of Service in CoAP Option Parsing Due
  To Integer Overflow
* CVE-2020-10068: In the Zephyr project Bluetooth subsystem, certain
  duplicate and back-to-back packets can cause incorrect behavior,
  resulting in a denial of service.
* CVE-2020-10069: An unchecked parameter in bluetooth data can result
  in an assertion failure, or division by zero, resulting in a denial
  of service attack.
* CVE-2020-10070: MQTT buffer overflow on receive buffer
* CVE-2020-10071: Insufficient publish message length validation in MQTT

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Known issues
************

You can check all currently known issues by listing them using the GitHub
interface and listing all issues with the `bug label
<https://github.com/zephyrproject-rtos/zephyr/issues?q=is%3Aissue+is%3Aopen+label%3Abug>`_.

A single high-priority bug is currently open:

* :github:`23364` - Bluetooth: bt_recv deadlock on supervision timeout with
  pending GATT Write Commands

API Changes
***********

* HWINFO

  * The identifier data structure for hwinfo drivers is clarified.  Drivers are
    responsible for ensuring that the identifier data structure is a sequence
    of bytes. The returned ID value is not supposed to be interpreted based on
    vendor-specific assumptions of byte order and should express the identifier
    as a raw byte sequence.
    The changes have an impact on users that use the hwinfo API to identify
    their devices.
    The sam0 driver byte swaps each 32 bit word of the 128 bit identifier to
    big endian.
    The nordic driver byte swaps the entire 64 bit word to big endian.

* I2C

  * Added a new API for recovering an I2C bus from situations where the I2C
    master and one or more I2C slaves are out of synchronization (e.g. if the
    I2C master was reset in the middle of an I2C transaction or if a noise
    pulse was induced on the SCL line).

Deprecated in this release
==========================

* Kernel

  * k_uptime_delta_32(), use k_uptime_delta()
  * Timeout values

    * All timeout values are now encapsulated k_timeout_t opaque structure when
      passing them to the kernel. If you want to revert to the previous s32_t
      type for the timeout parameter, please enable
      :option:`CONFIG_LEGACY_TIMEOUT_API`

* Bluetooth

  * BT_LE_SCAN_FILTER_DUPLICATE, use BT_LE_SCAN_OPT_FILTER_DUPLICATE instead
  * BT_LE_SCAN_FILTER_WHITELIST, use BT_LE_SCAN_OPT_FILTER_WHITELIST instead
  * bt_le_scan_param::filter_dup, use bt_le_scan_param::options instead
  * bt_conn_create_le(), use bt_conn_le_create() instead
  * bt_conn_create_auto_le(), use bt_conn_le_create_auto() instead
  * bt_conn_create_slave_le(), use bt_le_adv_start() instead with
    bt_le_adv_param::peer set to the remote peers address.
  * BT_LE_ADV_* macros, use BT_GAP_ADV_* enums instead

* Boards

  * nrf51_pca10028 has been renamed to nrf51dk_nrf51422
  * nrf51_pca10031 has been renamed to nrf51dongle_nrf51422
  * nrf52810_pca10040 has been renamed to nrf52dk_nrf52810
  * nrf52_pca10040 has been renamed to nrf52dk_nrf52832
  * nrf52833_pca10100 has been renamed to nrf52833dk_nrf52833
  * nrf52811_pca10056 has been renamed to nrf52840dk_nrf52811
  * nrf52840_pca10056 has been renamed to nrf52840dk_nrf52840
  * nrf52840_pca10059 has been renamed to nrf52840dongle_nrf52840
  * nrf9160_pca10090 has been renamed to nrf9160dk_nrf9160
  * nrf52840_pca10090 has been renamed to nrf9160dk_nrf52840
  * nrf52_pca20020 has been renamed to thingy52_nrf52832
  * nrf5340_dk_nrf5340 has been renamed to nrf5340pdk_nrf5340
  * efr32_slwstk6061a has been renamed to efr32_radio_brd4250b

* Devicetree

  * The C macros generated from the devicetree in previous releases are now
    deprecated in favor of a new ``<devicetree.h>`` API.
  * See :ref:`dt-from-c` for a high-level guide to the new API, and
    :ref:`devicetree_api` for an API reference.
  * Use of the legacy macros now requires explicitly enabling
    :option:`CONFIG_LEGACY_DEVICETREE_MACROS`. See :ref:`dt-legacy-macros` for
    more information, including a link to a migration guide to the new API.

* Other

  * ``MACRO_MAP`` has been deprecated. Use ``FOR_EACH`` instead.
  * ``BUILD_ASSERT_MSG`` has been deprecated. Use ``BUILD_ASSERT`` instead.

Removed APIs in this release
============================

* The ``INLINE`` macro in ``util.h`` has been removed.
* ``STACK_ANALYZE``, ``stack_analyze`` and ``stack_unused_space_get`` have been
  removed.


Stable API changes in this release
==================================

* Bluetooth Mesh

  * The net_idx parameter has been removed from the Health Client model
    APIs since it can be derived (by the stack) from the app_idx parameter

* Networking

  * The NET_DEVICE_INIT(), NET_DEVICE_INIT_INSTANCE(), NET_DEVICE_OFFLOAD_INIT()
    and ETH_NET_DEVICE_INIT() macros changed and take a device power management
    function pointer parameter. If networking PM is not implemented for the
    specific network device, the device_pm_control_nop value can be used.

* Video

  * The video_dequeue() API call now takes a k_timeout_t for the timeout
    parameter. This reverts to s32_t if CONFIG_LEGACY_TIMEOUT_API is enabled.

* Floating Point Services

  * FLOAT and FP_SHARING Kconfig options have been renamed to FPU and FPU_SHARING,
    respectively.

Kernel
******

* A new general purpose memory allocator, sys_heap/k_heap, has been added
  to Zephyr with more conventional API/behavior, better space
  efficiency and higher performance than the pre-existing mem_pool.
  The older mem_pool APIs are, by default, wrappers around this new
  heap backend and will be deprecated in an upcoming release.  The
  original implementation remains available for this release via
  disabling CONFIG_MEM_POOL_HEAP_BACKEND.
* The timeout arguments to all kernel calls are now a "k_timeout_t"
  type instead of a 32 bit millisecond count.  These can be
  initialized in arbitrary time units (ns/us/ms, ticks), be
  interpreted relative to either current time or system start, and be
  expressed in 64 bit quantities.  This involves a minor change to the
  API, so the original API is still available in a completely
  source-compatible way via CONFIG_LEGACY_TIMEOUT_API.
* Simplified dummy thread implementation and properly name idle threads
* Centralized new thread priority check
* Refactored device structures and introduced struct init_entry which is
  a generic init end-point. SYS_INIT() generates only a struct init_entry via
  calling INIT_ENTRY_DEFINE(). Also removed struct deviceconfig leaving
  struct device to own everything now.

Architectures
*************

* ARC:

  * Changed to automatic generation of privilege stack for ARC MPU V2 to
    avoid the potential waste of memory When USERSPACE is configured
  * Enhanced runtime programming for the MPU v3 by making the gap-filling
    of kernel memory a user-configurable feature
  * Refactored the thread switch code in epilogue of irq and exception
  * Refactored the assembly codes for better maintenance
  * Fixed the behavior of ARC timer driver
  * Fixed the behavior of ARC SMP
  * Fixed the wrong configurations of ARC boards in Kconfig and DTS

* ARM:

  * CMSIS has been moved out of the main tree and now resides in its
    own standalone module repository
  * Updated CMSIS version to 5.7.0
  * Added CMSIS-DSP library integration
  * Added semihosting console support
  * Cleanups and improvements to the Cortex-M exception vector table
  * Fixed the behavior of Cortex-M spurious IRQ handler
  * Fixed parsing of Cortex-M MemManage Stacking Errors
  * Fixed the arch_cpu_idle() implementation for Cortex-M and Cortex-R
  * Renamed Cortex-R architecture port to cortex_a_r in preparation for the
    AArch32 Cortex-A architecture port
  * Added processor exception handling and reporting framework for Cortex-R
  * Added nested interrupt support on AArch32 Cortex-R and AArch64 Cortex-A
  * Refactored Cortex-R interrupt system to remove fake multi-level interrupt
    controller abstraction scheme


* POSIX:

  * Added support for building on ARM hosts

* RISC-V:

  * Added support for hard floating point for RISC-V
  * Added march and mabi options to Kconfig
  * Fixed compilation warning for platforms without PLIC

* x86:

  * Instrumented code for timing information
  * Added ability for SoC to add MMU regions
  * x86 FPU sharing symbols renamed
  * early_serial: extended to support MMIO UART

Boards & SoC Support
********************

* Added support for these SoC series:

  * Broadcom Viper BCM58402
  * Infineon XMC4500 SoC
  * Nordic nRF52820 SoC
  * NXP LPC55S16 SoC
  * SiLabs EFR32BG13P SoC
  * STM32L5 series of Ultra-low-power MCUs

* Added support for these ARM boards:

  * 96Boards AeroCore 2
  * Adafruit Feather nRF52840 Express
  * Adafruit Feather STM32F405 Express
  * Black STM32 F407VE Development Board
  * Black STM32 F407ZG Pro Development Board
  * Broadcom BCM958402M2
  * EFR32 BRD4104A (SLWRB4104A)
  * Infineon XMC45-RELAX-KIT
  * nRF52820 emulation on nRF52833 DK
  * nrf9160 INNBLUE21
  * nrf9160 INNBLUE22
  * NXP LPCXpresso55S16
  * SEGGER IP Switch Board
  * ST Nucleo H743ZI
  * ST Nucleo F303RE
  * ST Nucleo L552ZE-Q

* Made these changes in other boards

  * ``up_squared`` now defaults to the x86_64 architecture
  * ``intel_s1000`` now supports SMP

* Added support for these following shields:

  * Espressif ESP-8266 Module
  * MikroElektronika ADC Click
  * MikroElectronica Eth Click
  * ST X-NUCLEO-IKS02A1: MEMS Inertial and Environmental Multi sensor shield

Drivers and Sensors
*******************

* ADC

  * Added support for STM32G4, STM32L1 and STM32H7 series
  * Enabled internal voltage reference source on stm32
  * Added Microchip MCP320x driver

* Audio

  * N/A

* Bluetooth

  * Added an RX thread on stm32wb hci wrapper
  * Improved BLE support for rv32m1_vega:

    - Added Resolvable Private Address support
    - Enabled power saving support
    - Added 2 Mbps PHY support
    - Enabled controller-based privacy

* CAN

  * Converted can-primary alias to zephyr,can-primary chosen property
  * Converted loopback driver to use a thread to send frames

* Clock Control

  * Enabled MSI range config in PLL mode on stm32
  * Fixed AHB clock computation based on core on stm32h7

* Console

  * Fixed USB initialization
  * Added semihosting console

* Counter

  * Added support on stm32h7 and stm32l0
  * Fixed alarm tick count on stm32
  * Added Maxim DS3231 driver
  * Added NXP Kinetis LPTMR driver

* Crypto

  * Added driver for nRF ECB
  * Added CAP_NO_IV_PREFIX capability to stm32 driver

* DAC

  * Added stm32l0 series support
  * Added DAC shell
  * Added NXP Kinetis DAC and DAC32 drivers

* Debug

  * N/A

* Display

  * Added power management support to st7789v driver
  * Reworked controller memory initialization in ssd16xx driver
  * Updated st7789v driver to set x-offset and y-offset properties properly

* DMA

  * Enabled use of DMAMUX on stm32l4+ and stm32wb
  * Various fixes on stm32 dma management

* EEPROM

  * N/A

* Entropy

  * Removed Kconfig HAS_DTS_ENTROPY
  * Implemented ISR specific get entropy call in gecko driver

* ESPI

  * Various fixes in Microchip driver

* Ethernet

  * Added SAM E54 max queue count referencing
  * Added SAM0 family support to gmac driver
  * Added sam4e support to queue in gmac
  * Added network power management support to mcux
  * Added VLAN support to enc28j60
  * Added VLAN support to stm32
  * Added Ethernet cable link status support to gmac
  * Added support for i.MXRT1060 family to mcux
  * Added support for getting manual MAC address from devicetree
  * Added support for enabling random MAC address from devicetree
  * Various fixes to setup and cache handling in gmac
  * Fixed how unique MAC address is determined in mcux
  * Fixed Ethernet cable link detection in gecko
  * Fixed stm32 when receiving data during initialization

* Flash

  * Added logs on stm32
  * Fixed wrong bank erasing on stm32g4
  * Various fixes in nrf_qspi_nor driver
  * Added driver for AT456 compatible SPI flash chips
  * Enabled support for SAMV71

* GPIO

  * Added mcp23s17 driver
  * Added STM32L5 support to stm32 driver
  * Added interrupt support to sx1509b driver
  * Fixed interrupt handling in sifive, intel_apl, mchp_xec, mcux_igpio driver
  * Various fixes in intel_apl driver
  * Added MCP23S17 driver
  * Fixed port 1 interrupts in mcux lpc driver

* Hardware Info

  * Fixed ESP32 implementation
  * Updated byte order in all drivers

* I2C

  * Added support to stm32h7
  * Added write/read and bus recovery commands to shell
  * Added bus recovery function to gpio bitbang driver
  * Fixed fast and fast+ mode bus speeds in several drivers
  * Added mcux flexcomm driver

* I2S

  * Added I2S master DMA support and clock output to stm32 driver
  * Enabled SAMV71

* IEEE 802.15.4

  * Added Decawave DW1000 driver
  * Added "no auto start" option and local MAC address support to rf2xx
  * Added support for Frame Pending Bit (FPB) handling in nrf5
  * Added CSMA CA transmit capability to nrf5
  * Added PAN coordinator mode support to nrf5
  * Added support for promiscuous mode to nrf5
  * Added support for energy scan function to nrf5
  * Fixed RX timestamp handling in nrf5
  * Various fixes to rf2xx

* Interrupt Controller

  * Fixed PLIC register space
  * Added support for STM32L5 series
  * Added GIC V3 driver
  * Fixed ICFGRn access and config in GIC driver
  * Optimized the arc v2 interrupt unit driver

* IPM

  * Added CAVS DSP Intra-DSP Communication (IDC) driver

* Keyboard Scan

  * Added interrupt support to the ft5336 touch controller driver
  * Added SDL mouse driver

* LED

  * N/A

* LED Strip

  * N/A

* LoRa

  * Added a LoRa shell
  * Replaced counter driver usage with k_timer calls
  * Various fixes in sx1276 driver

* Modem

  * Added support for GSM 07.10 muxing protocol to generic GSM modem
  * Added support for modem commands that do not have a line ending
  * Added automatic detection of ublox-sara-r4 modem type
  * Added automatic setting of APN for ublox-sara-r4
  * Added sendmsg() support to ublox-sara-r4
  * Fixed UDP socket closing in ublox-sara-r4
  * Fixed RSSI calculation for Sara U201
  * Fixed TCP context release and RX socket src/dst port assignment in wncm14a2a
  * Changed PPP driver connection to generic GSM modem

* PECI

  * Added Microchip XEC driver

* Pinmux

  * Fixed compilation errors in rv32m1_vega pinmux

* PS/2

  * Tuned PS2 driver to support several mice brands

* PWM

  * Added support to stm32h7
  * Enhanced mcux ftm driver to configure pwm in ticks and allow configuring the clock prescaler
  * Added mcux tpm driver
  * Fixed nrfx driver to wait until PWM is stopped before restarting it

* Sensor

  * Added support for Analog Devices ADXL345 3-axis I2C accelerometer
  * Added Infineon DPS310 driver
  * Fixed temperature conversion in SI7006 driver
  * Added Honeywell MPR driver
  * Added BQ27421 driver
  * Added weighted average filter to NXP Kinetis temperature driver
  * Enabled single shot mode in ENS210 driver
  * Added forced sampling mode to BME280 driver
  * Added IIS2MDC magnetometer driver
  * Added IIS2DLPC accelerometer driver
  * Added ISM330DHCX IMU driver
  * Added MEC tachometer driver
  * Fixed I2C and SPI bus communication in LIS2DH driver

* Serial

  * Added uart_mux driver that is used in GSM 07.10 muxing protocol
  * Added support for parity setting from dts on stm32
  * Added support for stm32l5
  * Various fixes in ns16550 driver
  * Added XMC driver
  * Added interrupt and runtime configuration support to Xilinx driver
  * Fixed interrupt support in sifive driver
  * Enhanced nrfx driver TX only mode support
  * Added SAMV71 support to sam driver

* SPI

  * Added support for DMA client on stm32
  * Increased clock frequency in mcux flexcomm driver
  * Added power management support to cc13xx_cc26xx driver

* Timer

  * Various fixes in stm32_lptim driver
  * Removed RTC1 dependency from nrf driver
  * Various fixes in arcv2_timer0 driver
  * Fixed TICKLESS=n processing in nrf_rtc and stm32_lptim drivers
  * Added CAVS DSP wall clock timer driver
  * Implemented tickless support in xlnx_psttc_timer driver

* USB

  * Added experimental USB Audio implementation.
  * Added support to stm32wb
  * Fixed PMA leak at reset on stm32
  * Various fixes in usb_dc_nrfx driver
  * Refactored usb_dc_mcux_ehci driver

* Video

  * Added dedicated video init priority
  * Various fixes in sw_generator and mcux_csi
  * Fixed video buffer alignment

* Watchdog

  * Added support on stm32g0
  * Disabled iwdg at boot on stm32

* WiFi

  * Added scan completion indication to eswifi
  * Added support to ESP8266 and ESP32


Networking
**********

* Converted networking to use new k_timeout_t infrastructure
* Enhanced new TCP stack support
* Added minimal support for TFTP client (RFC 1350)
* Added support for network device driver power management
* Added support for socketpair() BSD socket API
* Added support for QEMU user networking (SLIRP)
* Added support to disable automatic network attachment in OpenThread
* Added support for Frame Pending Bit handling in OpenThread
* Added support for RX frame handling in OpenThread
* Added support for TX started notification in OpenThread
* Added support for HW CSMA CA in OpenThread
* Added support for promiscuous mode in OpenThread
* Added support for reading OPAQUE resources with OMA TLV in LWM2M
* Added config to enable PAN coordinator mode in IEEE 802.15.4
* Added config to enable promiscuous mode in IEEE 802.15.4
* Added support for subscribe in Azure cloud sample
* Added support for queue mode in lwm2m_client sample
* Added support to allow change of the QEMU Ethernet interface name
* Added support for PPP IPCP to negotiate used DNS servers
* Added support for setting hostname in DHCPv4 request
* Fixed binding AF_PACKET socket type multiple times
* Fixed LLDPDU data in sent LLDP packets
* Fixed and enhance Google IoT sample application documentation
* Fixed MQTT cloud sample when polling incoming messages
* Fixed LWM2M socket error handling, and pending and reply handling during start
* Fixed LWM2M retransmission logic
* Fixed LWM2M Cell ID resource initialization
* Fixed COAP pending and reply handling
* Fixed wpan_serial sample application and enable USB during initialization
* Fixed HTTP client payload issue on HTTP upload
* Fixed MQTT Websocket incoming data handling and accept packets only in RX
* Fixed MQTT Publish message length validation
* Fixed IEEE 802.15.4 received frame length validation
* IEEE 802.15.4: avoided ACK processing when not needed
* IEEE 802.15.4: Now allows energy detection scan unconditionally

Bluetooth
*********

* Host:

  * Support for LE Advertising Extensions has been added.
  * The Host is now 5.2 compliant, with support for EATT, L2CAP ECRED mode and
    all new GATT PDUs.
  * New application-controlled data length and PHY update APIs.
  * Legacy OOB pairing support has been added.
  * Multiple improvements to OOB data access and pairing.
  * The Host now uses the new thread analyzer functionality.
  * Multiple bug fixes and improvements

* BLE split software Controller:

  * The Controller is now 5.2 compliant.
  * A new HCI USB H4 driver has been added, which can interact with BlueZ's
    counterpart Host driver.
  * PHY support is now configurable.
  * Only control procedures supported by the peer are now used.
  * The Nordic nRF52820 IC is now supported
  * OpenISA/RV32M1:

    * 2 Mbps PHY support.
    * Radio deep sleep mode support.
    * Controller-based privacy support.

* BLE legacy software Controller:

  * The legacy Controller has been removed from the tree.

Build and Infrastructure
************************

* Zephyr CMake package

  * The Zephyr main repository now includes a Zephyr CMake package.
    This allows for registering Zephyr in the CMake user package registry and
    allows for easier integration into Zephyr applications, by using the CMake
    function, ``find_package(Zephyr ...)``.
    Registering the Zephyr CMake package in the CMake user package registry
    removes the need for setting of ``ZEPHYR_BASE``, sourcing ``zephyr-env.sh``,
    or running ``zephyr-env.cmd``.
  * A new ``west`` extension command, ``west zephyr-export`` is introduced for easy
    registration of Zephyr CMake package in the CMake user package registry.
  * Zephyr Build Configuration CMake package hook.
    Zephyr offers the possibility of configuring the Zephyr build system through
    a Zephyr Build Configuration package. A single Zephyr workspace
    ``ZephyrBuildConfig.cmake`` will be loaded if present in the Zephyr
    workspace. This allows users to configure the Zephyr build system on a per
    workspace setup, as an alternative to using a ``.zephyrrc`` system wide file.

* Devicetree

  * A new :ref:`devicetree_api` was added. This API is not generated, but is
    still included via ``<devicetree.h>``. The :ref:`dt-legacy-macros` are now
    deprecated; users should replace the generated macros with new API. The
    :ref:`dt-howtos` page has been extended for the new API, and a new
    :ref:`dt-from-c` API usage guide was also added.

Libraries / Subsystems
**********************

* Disk

  * Add stm32 sdmmc disk access driver, supports stm32f7 and stm32l4

* Random

  * Removed the ``rand32_timestamp`` driver.

* POSIX subsystem:

  * socketpair() function implemented.
  * eventfd() function (Linux-like extension) implemented.

* Power management:

  * Add system and device power management support on TI CC13x2/CC26x2.

HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.

Documentation
*************

* New API overview page added.
* Reference pages have been cleaned up and organized.
* The Devicetree documentation has been expanded significally.
* The project roles have been overhauled in the Contribution Guidelines pages.
* The documentation on driver-specific APIs has been simplified.
* Documentation for new APIs, boards and samples.

Tests and Samples
*****************

* Added samples for USB Audio Class.
* Added sample for using POSIX read()/write() with network sockets.

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.2.0 tagged
release:

* :github:`25991` - [net][net.socket.select][imx-rt series] test fails  (k_uptime_get_32() - tstamp <= FUZZ is false)
* :github:`25990` - tests/net/socket/select failed on sam_e70_xplained board.
* :github:`25960` - tests/net/socket/socketpair failed on mimxrt1050_evk and sam_e70_xplained.
* :github:`25948` - Function i2c_transfer stops execution for I2C_SAM0
* :github:`25944` - driver: timer: stm32_lptim: Extra ticks count
* :github:`25926` - k_cycle_get_32() returns 0 in native_posix
* :github:`25925` -  tests: net: socket: socketpair: fails due to empty message header name
* :github:`25920` - Compilation error when CONFIG_BOOTLOADER_MCUBOOT=y specified
* :github:`25904` - kernel: k_queue_get return NULL before timeout
* :github:`25901` - timer: nrf_rtc_timer: Subtraction underflow causing 8 minute time skips
* :github:`25895` - driver: timer: stm32_lptim: backup domain is reset
* :github:`25893` - Application syscalls in usermode gives bus fault with stacking error
* :github:`25887` - legacy timeout API does not work as expected
* :github:`25880` - stm32wb: Unable to use BLE and USB host simultaneously.
* :github:`25870` - tests/kernel/timer/timer_api fails conversion tests with large offset
* :github:`25863` - Where is the definition of SystemInit()?
* :github:`25859` - mesh example not working with switched off dcdc?
* :github:`25847` - Problems using math functions and double.
* :github:`25824` - Unpacked bt_l2cap_le_conn_rsp struct is causing corrupt L2CAP connection request responses on some platforms
* :github:`25820` - kernel: k_timer_start(timer, K_FOREVER, K_NO_WAIT) expires immediately
* :github:`25811` - K22F USB Console/Shell
* :github:`25797` - [Coverity CID :210607] Uninitialized scalar variable in tests/net/socket/socketpair/src/test_socketpair_happy_path.c
* :github:`25796` - [Coverity CID :210579] Uninitialized scalar variable in tests/net/socket/socketpair/src/test_socketpair_happy_path.c
* :github:`25795` - [Coverity CID :210564] Uninitialized scalar variable in tests/lib/cmsis_dsp/distance/src/u32.c
* :github:`25793` - [Coverity CID :210561] Resource leak in tests/net/socket/socketpair/src/test_socketpair_unsupported_calls.c
* :github:`25791` - [Coverity CID :210614] Explicit null dereferenced in tests/lib/cmsis_dsp/distance/src/f32.c
* :github:`25789` - [Coverity CID :210586] Explicit null dereferenced in tests/lib/cmsis_dsp/distance/src/f32.c
* :github:`25788` - [Coverity CID :210581] Dereference before null check in subsys/net/lib/sockets/socketpair.c
* :github:`25787` - [Coverity CID :210571] Explicit null dereferenced in tests/subsys/openthread/radio_test.c
* :github:`25785` - [Coverity CID :210549] Explicit null dereferenced in tests/subsys/openthread/radio_test.c
* :github:`25780` - [Coverity CID :210612] Negative array index read in samples/net/sockets/socketpair/src/socketpair_example.c
* :github:`25779` - [Coverity CID :209942] Pointer to local outside scope in subsys/net/ip/tcp2.c
* :github:`25774` - [Coverity CID :210615] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25773` - [Coverity CID :210613] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25772` - [Coverity CID :210609] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25771` - [Coverity CID :210608] Incompatible cast in tests/lib/cmsis_dsp/fastmath/src/f32.c
* :github:`25770` - [Coverity CID :210605] Incompatible cast in tests/lib/cmsis_dsp/filtering/src/misc_f32.c
* :github:`25769` - [Coverity CID :210603] Incompatible cast in tests/lib/cmsis_dsp/filtering/src/misc_f32.c
* :github:`25768` - [Coverity CID :210601] Incompatible cast in tests/lib/cmsis_dsp/fastmath/src/f32.c
* :github:`25767` - [Coverity CID :210600] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25766` - [Coverity CID :210592] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25765` - [Coverity CID :210591] Incompatible cast in tests/lib/cmsis_dsp/filtering/src/misc_f32.c
* :github:`25764` - [Coverity CID :210590] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25763` - [Coverity CID :210577] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25762` - [Coverity CID :210576] Incompatible cast in tests/lib/cmsis_dsp/filtering/src/misc_f32.c
* :github:`25761` - [Coverity CID :210574] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25760` - [Coverity CID :210572] Incompatible cast in tests/lib/cmsis_dsp/distance/src/f32.c
* :github:`25759` - [Coverity CID :210569] Incompatible cast in tests/lib/cmsis_dsp/bayes/src/f32.c
* :github:`25758` - [Coverity CID :210567] Incompatible cast in tests/lib/cmsis_dsp/fastmath/src/f32.c
* :github:`25757` - [Coverity CID :210565] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25756` - [Coverity CID :210563] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25755` - [Coverity CID :210560] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25754` - [Coverity CID :210556] Incompatible cast in tests/lib/cmsis_dsp/matrix/src/unary_f64.c
* :github:`25753` - [Coverity CID :210555] Incompatible cast in tests/lib/cmsis_dsp/support/src/barycenter_f32.c
* :github:`25752` - [Coverity CID :210551] Incompatible cast in tests/lib/cmsis_dsp/matrix/src/unary_f32.c
* :github:`25751` - [Coverity CID :210545] Incompatible cast in tests/benchmarks/cmsis_dsp/basicmath/src/f32.c
* :github:`25737` - [Coverity CID :210585] Unchecked return value in samples/net/sockets/socketpair/src/socketpair_example.c
* :github:`25736` - [Coverity CID :210583] Unchecked return value from library in samples/net/sockets/socketpair/src/socketpair_example.c
* :github:`25731` - [Coverity CID :210568] Argument cannot be negative in tests/net/socket/socketpair/src/test_socketpair_happy_path.c
* :github:`25730` - [Coverity CID :210553] Unchecked return value in tests/drivers/gpio/gpio_basic_api/src/test_deprecated.c
* :github:`25727` - [Coverity CID :210611] Logically dead code in subsys/net/lib/sockets/socketpair.c
* :github:`25702` - BSD socket sendmsg() did not verify params in usermode
* :github:`25701` - MPU FAULT in nvs test on nrf52840dk_nrf52840
* :github:`25698` - IPv6 prefix could be added multiple times to prefix timer list
* :github:`25697` - Example of Thread creation in documentation does not compile
* :github:`25694` - IPv6 RA prefix option invalid length
* :github:`25673` - Unable to use SPI1 when enabled without SPI0 on cc13xx/cc26xx
* :github:`25670` - Possible Null pointer dereferences in /subsys/logging/log_msg.c
* :github:`25666` - tests: kernel: mem_protect: syscalls: test_string_nlen fails
* :github:`25656` - shields: Can't use multiple shields anymore
* :github:`25635` - ARM: TLS pointer may not be set correctly
* :github:`25621` - ESWiFi does not populate info about remote when invoking callback
* :github:`25614` - fix longstanding error in pthread_attr_t definition
* :github:`25613` - USB: CDC adds set line coding callback
* :github:`25612` - ARM: Cortex-M: CPU is not reporting Explicit MemManage Stacking Errors correctly
* :github:`25597` - west sign fails to find header size or padding
* :github:`25585` - QEMU special key handling is broken on qemu_cortex_a53
* :github:`25578` - nrf: clock control: nrf5340: using CLOCK_CONTROL_NRF_K32SRC_RC results in build failure
* :github:`25568` - nrf: clock_control: Fatal error during initialization
* :github:`25561` - bluetooth: GATT lockup on split packets
* :github:`25555` - Unable to connect to Thread network (NRF52840DK)
* :github:`25527` - sample and writeup for socketpair
* :github:`25526` - Sanity Check Fails:
* :github:`25522` - settings: FCB back-end does not try to add record after the last compression attempt.
* :github:`25519` - wrong debug function cause kinds of building error
* :github:`25511` - arc em_starterkit_em11d failed in tests/kernel/timer/timer_api
* :github:`25510` - arc EMSDP failed in tests/kernel/gen_isr_table
* :github:`25509` - OpenThread SED set link mode fail
* :github:`25493` - devicetree: nRF5340 application core DTSI is missing cryptocell node
* :github:`25489` - drivers: modem_cmd_handler: uninitialized variable used
* :github:`25483` - Bluetooth: controller: split: feature exchange not conform V5.0 core spec
* :github:`25480` - Unconditional source of shield configs can mess up configuration
* :github:`25478` - settings_runtime_set() not populating bt/cf
* :github:`25477` - dts: arm: Incorrect GIC interrupt spec order for AArch64 SoCs
* :github:`25471` - disco_l475_iot1 don't write last small block
* :github:`25469` - Fix devicetree documentation for new API
* :github:`25468` - FRDM_K82F DTS missing information for ADC-0
* :github:`25452` - Some USB samples targeting stm32 are malfunctioning
* :github:`25448` - serial: uart_nrfx_uarte: poll & async TX infinite hang
* :github:`25447` - cf_set() returns 0 when no cfg is available
* :github:`25442` - Does Zephyr support USB host mode ?
* :github:`25437` - tests/lib/heap: sanitycheck timeout on STM32 boards
* :github:`25433` - Add vendor specific class custom usb device sample
* :github:`25427` - STM32 Ethernet driver build failure with CONFIG_ASSERT=1
* :github:`25408` - STM32 Ethernet Driver: Fix driver crash caused by RX IRQ trigger
* :github:`25390` - driver: timer: arm arch timer PPI configuration to be taken from dt
* :github:`25386` - boards: shields: esp_8266: There isn't CI tests enabled
* :github:`25379` - Bluetooth mesh example not working
* :github:`25378` - Installation problems
* :github:`25369` - tests/drivers/gpio/gpio_basic_api: test_gpio_deprecated step fails on STM32 boards
* :github:`25366` - tests/drivers/counter/counter_basic_api: instable test status on STM32 boards
* :github:`25363` - tests/drivers/counter/counter_basic_api: Assertion failed on STM32 boards
* :github:`25354` - Fails to compile when SYS_PM_DIRECT_FORCE_MODE is true
* :github:`25351` - test:mimxrt1050_evk:tests/subsys/usb/bos/: run failure
* :github:`25350` - Bluetooth: controller: Data transmission delayed by slave latency
* :github:`25349` - The b_l072z_lrwan1 board (STM32L0) doesn't support flashing of firmware larger than bank 0
* :github:`25348` - test:mimxrt10xx_evk:tests/kernel/mem_protect/stackprot: get unexpected Stacking error
* :github:`25346` - Timestamp in LOG jumps 00:08:32
* :github:`25337` - LED pins always configured as PWM outputs
* :github:`25334` - SPI won't build on microbit with I2C
* :github:`25332` - lib: updatehub: Don't build after conversion from DT_FLASH_AREA to FLASH_AREA macros
* :github:`25331` - test_timer_remaining() fails with assertion in timer_api test
* :github:`25319` - MMU and USERSPACE not working on upsquared
* :github:`25312` - samples:mimxrt1010_evk:samples/net/openthread/ncp: build error
* :github:`25289` - mcuboot incompatible with Nordic QSPI flash driver
* :github:`25287` - test/benchmarks/latency_measure fails on nucleo_f429zi and nucleo_f207zg
* :github:`25284` - spi: stm32: dma_client: Cannot use RX only configuration
* :github:`25276` - OpenThread not work after upgrade to latest version
* :github:`25272` - tests/drivers/gpio/gpio_basic_api failed on mec15xxevb_assy6853 board.
* :github:`25270` - fix userspace permissions in socketpair tests
* :github:`25263` - Can anyone tell me how can i use external qspi flash "mx25r64"(custom board with nrf52840 soc) for mcuboot slot1 and i'm using zephyr 2.2.0
* :github:`25260` - drivers: uart_ns16550: device config_info content mutated
* :github:`25251` - Post DT API migration review
* :github:`25247` - const qualifier lost on some device config_info casts
* :github:`25246` - SHELL_DEFAULT_TERMINAL_WIDTH should be configurable in Kconfig
* :github:`25241` - tests.drivers.spi_loopback stm32wb55x fails transferring multiple buffers with dma
* :github:`25240` - Building usb audio sample hangs the pre-processor
* :github:`25234` - kernel.timer.tickless test fails on atsamd21_xpro
* :github:`25233` - bad logic in test_busy_wait of tests/kernel/context
* :github:`25232` - driver: wifi: esp_offload.c: Missing new timeout API conversion
* :github:`25230` - Lib: UpdateHub: Missing new timeout API conversion
* :github:`25224` - benchmark.kernel.latency test fails on atsame54_xpro
* :github:`25221` - arch.arm.irq_advanced_features test fails on atsamd21_xpro
* :github:`25216` - cc13xx and cc26xx handler for IRQ invoked multiple times
* :github:`25210` - CI seems to be stuck for my pull request
* :github:`25204` - soc: apollo_lake: Disabling I2C support is not possible
* :github:`25200` - Build error in Sample App for OpenThread NCP
* :github:`25196` - tests: portability: cmsis_rtos_v2: hangs on nRF52, 53 and 91 nRF platforms
* :github:`25194` - tests: kernel: context: seems to be failing on Nordic platforms
* :github:`25191` - tests/drivers/console: drivers.console.semihost can't work
* :github:`25190` - West - init/update module SHA with --depth = 1
* :github:`25185` - Adding CONFIG_BT_SETTINGS creates errors on bt_hci_core & bt_gatt
* :github:`25184` - lldp: lldp_send includes bug
* :github:`25183` - west build error after while "getting started" on ESP32
* :github:`25180` - tests: drivers/i2s/i2s_api: Build failed on 96b_argonkey
* :github:`25179` - tests/kernel/timer/timer_api failed on iotdk board.
* :github:`25178` -  tests/kernel/sched/schedule_api failed on iotdk board.
* :github:`25177` - tests/drivers/counter/maxim_ds3231_api failed on frdm_k64f.
* :github:`25176` - tests/kernel/context failed on multiple platforms.
* :github:`25174` - qemu test failures when running sanitycheck
* :github:`25169` - soc/arm/infineon_xmc/4xxx/soc.h not found
* :github:`25161` - samples/cfb/display flickers with SSD1306
* :github:`25141` - Cannot use C++ on APPLICATION level initialization
* :github:`25140` - Unable to obtain dhcp lease
* :github:`25139` - USB HID mouse sample high input delay
* :github:`25130` - Bluetooth: controller: Incorrect version information
* :github:`25128` - Missing ``python3-dev`` dependency
* :github:`25123` - DAC is not described in soc of STM32L4xx series
* :github:`25109` - Flash tests fail on posix
* :github:`25101` - driver: gpio: mchp: GPIO initialization value doesn't get reflected when using new flags
* :github:`25091` - drivers: eSPI: Incorrect handling of OOB registers leads to report wrong OOB packet len
* :github:`25084` - LLDP: missing net_pkt_set_lldp in lldp_send
* :github:`25083` - Networking samples are not able to connect with the TCP under qemu_x86 after 9b055ec
* :github:`25067` - Insufficient ticker nodes for vendor implementations
* :github:`25057` - errors when running sanitycheck with tests/subsys/storage/stream/stream_flash
* :github:`25036` - kernel: pipe: read_avail / write_avail syscalls
* :github:`25032` - build failure on lpcxpresso55s16_ns
* :github:`25017` - [CI] m2gl025_miv in Shippable CI systematically fails some tests
* :github:`25016` - BT_LE_ADV_NCONN_NAME doesn't actually advertise name
* :github:`25015` - Bluetooth Isochronous Channels Support
* :github:`25012` - checkpatch.pl doesn't match the vendor string properly
* :github:`25010` - disco_l475_iot1 don't confirm MCUBoot slot-1 image
* :github:`24978` - RFC: use compatible name for prefix for device-specific API
* :github:`24970` - ieee802154 l2: no length check in frame validation
* :github:`24965` - RF2XX radio driver does automatic retransmission and OpenThread as well
* :github:`24963` - Slower OpenThread PSKc calculation
* :github:`24943` - Add a harness property to boards in sanitycheck's hardware_map
* :github:`24928` - Running Zephyr Bot tests on local machine
* :github:`24927` - stm32: Fix docs boards for doc generation
* :github:`24926` - Remove all uses of CONFIG_LEGACY_TIMEOUT_API  from the tree before 2.3
* :github:`24915` - accelerometer example no longer works for microbit
* :github:`24911` - arch: arm: aarch32: When CPU_HAS_FPU for Cortex-R5 is selected, prep_c.c uses undefined symbols
* :github:`24909` - ``find_package`` goes into an infinite loop on windows
* :github:`24903` - Python detection when building documentation fails
* :github:`24889` - stm32f469i discovery board and samples/display/lvgl fails
* :github:`24869` - qemu_x86: with icount enabled, crash in test_syscall_torture
* :github:`24853` - os: Precise data bus error with updatehub
* :github:`24842` - Support Building on Aarch64
* :github:`24840` - Unable to connect to OpenThread network after upgrade
* :github:`24805` - On x86, misalligned SSE accesses can occur when multithreading is enabled
* :github:`24784` - nRF: Busy wait clock is skewed vs. timer clock
* :github:`24773` - devicetree: allow generation of properties that don't have a binding
* :github:`24751` - What is purpose of the CONFIG_ADC_X
* :github:`24744` - k_thread_join() taking a very long time on qemu_cortex_m3
* :github:`24733` - Misconfigured environment
* :github:`24727` - Unable allocate buffer to send mesh message
* :github:`24722` - OnePlus 7T & peripheral_hr on NRF52 conn failure
* :github:`24720` - Build failure on intel_s1000_crb board for test case:” tests/kernel/smp”
* :github:`24718` - adc: stm32g4: Fix ADC instances naming
* :github:`24713` - ztest_test_fail() doesn't always work
* :github:`24706` - mcumgr: fail to upgrade nRF target using nRF Connect
* :github:`24702` - tests/drivers/counter/counter_basic_api failed on frdm_k64f board.
* :github:`24701` - tests/lib/cmsis_dsp/transform failed on frdm_k64f board.
* :github:`24695` - Board IP Can Not Be Set Manually
* :github:`24692` - FindPython3 has unexpected behavior on Windows
* :github:`24674` - Cannot generate code coverage report for unit tests using sanitycheck
* :github:`24665` - z_cstart memory corruption (ARM CortexM)
* :github:`24661` - sanitycheck incorrect judgement with tests/drivers/gpio/gpio_basic_api.
* :github:`24660` - tests/benchmarks/sys_kernel failed on nrf platforms
* :github:`24659` - tests/portability/cmsis_rtos_v2 failed on reel_board.
* :github:`24653` - device_pm: clarify and document usage
* :github:`24646` - Bluetooth: hci_uart broken on master
* :github:`24645` - naming consistency for kernel object initializer macros
* :github:`24642` - kernel: pipe: simple test fails for pipe write / read of 3 bytes
* :github:`24641` - inconsistent timer behavior on native platforms
* :github:`24635` - tests/counter/counter_basic_api fails on mps2_an385
* :github:`24634` - Invalid pin reported in gpio callback
* :github:`24626` - USB re-connection fails on SAM E70
* :github:`24612` - mimxrt1020_evk: total freeze
* :github:`24601` - Bluetooth: Mesh: Config Client's net_key_status pulls two key indexes, should pull one.
* :github:`24585` - How to read/write an big(>16K) file in littlefs shell sample on native posix board?
* :github:`24579` - Couldn't get test results from device serial on mimxrt1050_evk board.
* :github:`24576` - scripts/subfolder_list.py: Support long paths
* :github:`24571` - #include <new> is not available
* :github:`24564` - NRF51822 BLE ~400uA idle current consumption
* :github:`24554` - hal_infineon: Add new module for Infineon XMC HAL layer
* :github:`24553` - samples/subsys/shell/fs/ fail on native posix board
* :github:`24539` - How to complete userspace support for driver-specific API
* :github:`24534` - arch_mem_domain_max_partitions_get() returns equal number for all architectures
* :github:`24533` - devicetree: are some defines missing from the bindings?
* :github:`24509` - Ethernet Sample Echo Failed in Nucleo_f429zi - bisected
* :github:`24505` - Bluetooth: Mesh: Configuration Client: Add support for Model Subscription Get
* :github:`24500` - Failed to run the sample "Native Posix Ethernet"
* :github:`24497` - frdm_k64f fatal error while using flash and TLS features together
* :github:`24490` - SPI-NOR driver not found in spi_flash sample
* :github:`24485` - kernel: pipe: should return if >= min_xfer bytes transferred and timeout is K_FOREVER
* :github:`24484` - The file system shell example failed to build
* :github:`24479` - nrf-uarte problems with uart_irq_tx_disable() in handler
* :github:`24464` - drivers: espi: XEC: Incorrect eSPI channel status handling leading to missed interrupts and callbacks
* :github:`24462` - File not truncated to actual size after calling fs_close
* :github:`24457` - Common Trace Format - Failed to produce correct trace output
* :github:`24442` - samples/subsys/mgmt/mcumgr/smp_svr: should enable BT and FS for nrf52 boards
* :github:`24439` - LPCXpresso55S69_ns target : build failed
* :github:`24437` - smp_svr samle doesn't build for any target
* :github:`24431` - http_client assumes request payload is non-binary
* :github:`24426` - syscall for pipe(2)
* :github:`24409` - When the delay parameter of k_delayed_work_submit is K_FOREVER, the system will crash
* :github:`24399` - drivers: sam0_rtc_timer: DT_INST changes have broken this driver
* :github:`24390` - nsim_sem_normal target is broken
* :github:`24382` - disco_l475_iot1 not working with samples/net/wifi
* :github:`24376` - SPI (test) is not working for LPCXpresso54114
* :github:`24373` - NULL-pointer dereferencing in GATT when master connection fails
* :github:`24369` - tests/drivers/counter/counter_basic_api failure on nRF51-DK
* :github:`24366` - syscall for socketpair(2)
* :github:`24363` - nsim_hs_smp target doesn't work at all
* :github:`24359` - k_heap / sys_heap needs overview documentation
* :github:`24357` - NVS sample on STM32F4 fails even if the dts definition is correct
* :github:`24356` - MCUboot (and other users of DT_FLASH_DEV_NAME) broken with current zephyr master
* :github:`24355` - tests/drivers/uart/uart_basic_api configure and config_get fail because not implemented
* :github:`24353` - minnowboard hangs during boot of samples/hello_world
* :github:`24347` - Application Cortex M Systick driver broken by merge of #24012
* :github:`24340` - #24308 Broke python3 interpreter selection
* :github:`24339` - arm_gic_irq_set_priority - temporary variable overflow
* :github:`24325` - broken link in MinnowBoard documentation
* :github:`24324` - ST Nucleo F767ZI Ethernet Auto Negotiation problem
* :github:`24322` - IRQ_CONNECT and IRQ_DIRECT_CONNECT throw compile error with CONFIG_CPLUSPLUS
* :github:`24311` - LPN not receiving any message from Friend node after LPN device reset
* :github:`24306` - How to set up native posix board to allow connections to the Internet?
* :github:`24304` - Application crash #nrf52840 #ble
* :github:`24299` -  tests/subsys/storage/flash_map failed on frdm_k64f board.
* :github:`24294` - Problem using TMP116 sensor with platformio
* :github:`24291` - The button interrupt enters the spurious handler
* :github:`24283` - os:   Illegal use of the EPSR-disco_l475_iot1
* :github:`24282` - echo_client sample return: Cannot connect to TCP remote (IPv6): 110
* :github:`24278` - Function of "ull_conn_done" in "ull_conn.c"
* :github:`24277` - tests/kernel/workq/critical times out on ARC
* :github:`24276` - tests/kernel/context hangs on ARC in test_kernel_cpu_idle
* :github:`24275` - tests/kernel/mem_protect/syscalls fails on ARC in test_syscall_torture
* :github:`24252` - Python detection macro in cmake fails to detect highest installed version
* :github:`24243` - MCUBoot not working on disco_l475_iot1
* :github:`24241` - Build error when using MCHP ACPI HAL macros
* :github:`24237` - Fail to pass samples/subsys/nvs
* :github:`24227` - build hello_world sample failed for ESP32 board.
* :github:`24226` - [master]Bluetooth: samples/bluetooth/central_hr can't connect with samples/bluetooth/peripheral_hr
* :github:`24216` - Shell: Allow selecting command without subcommands
* :github:`24215` - Couldn't flash image into up_squared using misc.py script.
* :github:`24212` - lib: updatehub: Improve memory footprint
* :github:`24207` - tests/subsys/fs/fcb fails on nRF52840-DK
* :github:`24197` - Reduce snprintf and snprintk footprint
* :github:`24195` - question regarding c++
* :github:`24194` - Bluetooth: Mesh: Unknown message received by the node
* :github:`24193` - Issue with launching examples on custom board (after succesfull build)
* :github:`24187` - Remove the BLE Legacy Controller from the tree
* :github:`24183` - [v2.2] Bluetooth: controller: split: Regression slave latency during connection update
* :github:`24181` - Snprintk used at many place while dummy build if CONFIG_PRINTK is undef
* :github:`24180` - Parameter deprecation causes scanner malfunction on big-endian systems
* :github:`24178` - CI: extra_args from sanitycheck ``*.yaml`` do not propagate to cmake
* :github:`24176` - Where can I read PDR (packet delivery ratio)? Or number of TX/ACK packets?
* :github:`24162` - eSPI KConfig overrides espi_config API channel selection in eSPI driver
* :github:`24158` - gap in support for deprecated Nordic board names
* :github:`24156` - MQTT Websocket transport interprets all received data as MQTT messages
* :github:`24145` - File system shell example mount littleFS issue on nrf52840_pca10056
* :github:`24144` - deadlock potential in nrf_qspi_nor
* :github:`24136` - tests/benchmarks/latency_measure failed on mec15xxevb_assy6853 board.
* :github:`24122` - [nrf_qspi_nor] LittleFS file system fails to mount if LFS rcache buffer is not word aligned
* :github:`24108` - https GET request is failed for big file download.
* :github:`24104` - west sign usage help is missing key information
* :github:`24103` - USB Serial Number reverses bytes in hw identifier
* :github:`24101` - Bluetooth: Mesh: Transport Segment send failed lead to seg_tx un-free
* :github:`24098` - drivers: flash: flash_stm32: usage fault
* :github:`24089` - Zephyr/Openthread/MBEDTLS heap size/usage
* :github:`24086` - Bluetooth: SMP: Existing bond deleted on pairing failure
* :github:`24081` - le_adv_ext_report is not generating an HCI event
* :github:`24072` - tests/kernel/timer/timer_api failed on nucleo_stm32l152re board
* :github:`24068` - UART driver for sifive does not compile when configuring PORT_1
* :github:`24067` - cross-platform inconsistency in I2C bus speeds
* :github:`24055` - Add support for openocd on stm32g0 and stm32g4 targets
* :github:`24041` - [Coverity CID :209368] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24040` - [Coverity CID :209369] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24039` - [Coverity CID :209370] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24038` - [Coverity CID :209371] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24037` - [Coverity CID :209372] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24036` - [Coverity CID :209373] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24035` - [Coverity CID :209374] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24034` - [Coverity CID :209375] Side effect in assertion in tests/kernel/interrupt/src/prevent_irq.c
* :github:`24033` - [Coverity CID :209376] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24032` - [Coverity CID :209377] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24031` - [Coverity CID :209378] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24027` - [Coverity CID :209382] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24026` - [Coverity CID :209383] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`24016` - Fully support DTS on nrf entropy driver
* :github:`24014` - Bluetooth: Mesh: Friend node not cache for lpn which receiveing unknown app_idx
* :github:`24009` - Bluetooth: Mesh: Friend node not cache ALL_Node Address or different app_idx
* :github:`24008` - Build failure on intel_s1000_crb board.
* :github:`24003` - Couldn't generated code coverage report using sanitycheck
* :github:`24001` - tests/kernel/timer/timer_api failed on reel_board and mec15xxevb_assy6853.
* :github:`23998` - Infinite Reboot loop in Constructor C++
* :github:`23997` - flash sector erase fails on stm32l475
* :github:`23989` - Switching among different PHY Modes
* :github:`23986` - Possible use of uninitialized variable in subsys/net/ip/utils.c
* :github:`23980` - Nordic USB driver: last fragment sometimes dropped for OUT control endpoint
* :github:`23961` - CCC does not get cleared when CONFIG_BT_KEYS_OVERWRITE_OLDEST is enabled
* :github:`23953` - Question: How is pdata.tsize initialized in zephyr/subsys/usb/usb_transfer.c?
* :github:`23947` - soc: arm: atmel: sam4e: Enable FPU
* :github:`23946` - ARM soft FP ABI support is broken
* :github:`23945` - west flash don't flash right signed file when system build both hex and bin files
* :github:`23930` - Question: Cortex-M7 revision r0p1 errata
* :github:`23928` - Flash device FLASH_CTRL not found
* :github:`23922` - cmake 3.17 dev warning from FindPythonInterp.cmake
* :github:`23919` - sanitycheck samples/drivers/entropy/sample.drivers.entropy fails
* :github:`23907` - Shell overdo argument parsing in some cases
* :github:`23897` - Typo in linker.ld for NXP i.MX RT
* :github:`23893` - server to client ble coms: two characteristics with notifications failing to notify the right characteristics at the client
* :github:`23877` - syscall use of output buffers may be unsafe in some situations
* :github:`23872` - cmake find_package(ZephyrUnittest...) doesn't work
* :github:`23866` - sample hci_usb fails with zephyr 2.2.0 (worked with zephyr 2.1.0)
* :github:`23865` - nrf52840 and pyocd cannot program at addresses above 512k
* :github:`23853` - samples/boards/nrf/battery does not build
* :github:`23850` - Template with C linkage in util.h:52
* :github:`23824` - ARM Cortex-M7 MPU setting
* :github:`23805` - Bluetooth: controller: Switching to non conn adv fails for Mesh LPN
* :github:`23803` - nrf52840 ble error
* :github:`23800` - tests/drivers/counter/counter_cmos failed on up_squared platform
* :github:`23799` -  tests/subsys/logging/log_immediate failed on reel_board
* :github:`23777` - Problem with applying overlay for custom board in blinky example
* :github:`23763` - net: sockets: Wrong binding when connecting to ll address
* :github:`23762` - stm32: Revert nucleo_l152re to work at full speed
* :github:`23750` - eSPI API needs to be updated since it's passing parameters by value
* :github:`23718` - Getting started with zephyr OS
* :github:`23712` - Error in mounting the SD card
* :github:`23703` - Openthread on Zephyr cannot get On-Mesh Prefix address
* :github:`23694` - TEMP_KINETIS is forced enabled on frdm_k64f if SENSORS is enabled. But ADC is missing
* :github:`23692` - drivers: ublox-sara-r4: Add support for pin polarity
* :github:`23678` - drivers/flash: stm32: Error in device name
* :github:`23677` - SPI slave driver doesn't work correctly on STM32F746ZG; needs spi-fifo to be enabled in DT
* :github:`23674` - Openthread stop working after "Update OpenThread revision #23632"
* :github:`23673` - spi-nor driver fails to check for support of 32 KiBy block erase
* :github:`23669` -  ipv4 rx fragments: is zephyr support?
* :github:`23662` - Building blinky sample program goes wrong
* :github:`23637` - Wrong channel computation in stm32 pwm driver
* :github:`23624` - posix: clock: clock_gettime fault on userspace with CLOCK_REALTIME
* :github:`23623` - stm32 can2 not work properly
* :github:`23622` - litex_vexriscv: k_busy_wait() never returns if called with interrupts locked
* :github:`23618` - cmake: Export compile_commands.json for all generated code
* :github:`23617` - kernel: k_cpu_idle/atomic_idle() not tested for tick-less kernel
* :github:`23611` - Add QuickLogic EOS S3 HAL west module
* :github:`23600` - Differences in cycles between k_busy_wait and k_sleep
* :github:`23595` - RF2XX driver Openthread ACK handling
* :github:`23593` - Nested interrupt test is broken for RISC-V
* :github:`23588` - [Coverity CID :208912] Dereference after null check in tests/net/icmpv4/src/main.c
* :github:`23587` - [Coverity CID :208913] Resource leak in tests/net/socket/af_packet/src/main.c
* :github:`23586` - [Coverity CID :208914] Self assignment in drivers/peci/peci_mchp_xec.c
* :github:`23585` - [Coverity CID :208915] Out-of-bounds access in tests/net/icmpv4/src/main.c
* :github:`23584` - [Coverity CID :208916] Out-of-bounds read in drivers/sensor/adxl345/adxl345.c
* :github:`23583` - [Coverity CID :208917] Dereference after null check in tests/net/icmpv4/src/main.c
* :github:`23582` - [Coverity CID :208918] Side effect in assertion in tests/arch/arm/arm_interrupt/src/arm_interrupt.c
* :github:`23581` - [Coverity CID :208919] Out-of-bounds read in drivers/sensor/adxl345/adxl345.c
* :github:`23580` - [Coverity CID :208920] Resource leak in tests/net/socket/af_packet/src/main.c
* :github:`23579` - [Coverity CID :208921] Improper use of negative value in tests/net/socket/af_packet/src/main.c
* :github:`23577` - [Coverity CID :208923] Out-of-bounds read in drivers/sensor/adxl345/adxl345.c
* :github:`23576` - [Coverity CID :208924] Dereference after null check in tests/net/icmpv4/src/main.c
* :github:`23575` - [Coverity CID :208925] Unsigned compared against 0 in samples/drivers/espi/src/main.c
* :github:`23573` - [Coverity CID :208927] Dereference after null check in tests/net/icmpv4/src/main.c
* :github:`23571` - drivers: timer: nrf52: Question: Does nRF52840 errata 179 affect nrf_rtc_timer driver?
* :github:`23562` - build warnings when updating to master from 2.2.0
* :github:`23555` - STM32 SDMMC disk access driver (based on stm32 cube HAL)
* :github:`23544` - tests/kernel/mem_protect/syscalls failed on iotdk board.
* :github:`23541` - xilinx_zynqmp: k_busy_wait() never returns if called with interrupts locked
* :github:`23539` -  west flash --runner jlink returns KeyError: 'jlink'
* :github:`23529` - Convert STM32 drivers to new DT macros
* :github:`23528` - k64f dts flash0/storage_partition 8KiB -> 64KiB
* :github:`23507` - samples/subsys/shell/shell_module doesn't work on qemu_x86_64
* :github:`23504` - Build system dependency issue with syscalls
* :github:`23496` - Issue building & flashing a hello world project on nRF52840
* :github:`23494` - Bluetooth: LL/PAC/SLA/BV-01-C fails if Slave-initiated Feature Exchange is disabled
* :github:`23485` - BT: host: Service Change indication sent regardless of whether it is needed or not.
* :github:`23482` - 2M PHY + DLE and timing calculations on an encrypted link are wrong
* :github:`23476` - tests/kernel/interrupt failed on ARC
* :github:`23475` - tests/kernel/gen_isr_table failed on iotdk board.
* :github:`23473` - tests/posix/common failed on multiple ARM platforms.
* :github:`23468` - bluetooth: host: Runtime HCI_LE_Create_Connection timeout
* :github:`23467` - Import from linux to zephyr?
* :github:`23459` - tests: drivers: uart: config api has extra dependency in test 2
* :github:`23444` - drivers: hwinfo: shell command "hwinfo devid" output ignores endianness
* :github:`23441` - RFC: API change: Add I2C bus recovery API
* :github:`23438` - Cannot reset Bluetooth mesh device
* :github:`23435` - Missing documentation for macros in util.h
* :github:`23432` - Add PECI subsystem user space handlers
* :github:`23425` - Remote opencd
* :github:`23420` - PPP management don't build
* :github:`23418` - Building hello_world failed
* :github:`23415` - gen_defines does not resolve symbol values for devicetree.conf
* :github:`23414` - tests/benchmarks/timing_info  failed on mec15xxevb_assy6853 board.
* :github:`23395` - UART Console input does not work on SiFive HiFive1 on echo sample app
* :github:`23387` - [Question] Why does not zephyr use a toolchain file with cmake as -DCMAKE_TOOLCHAIN_FILE=.. ?
* :github:`23386` - SAM GMAC should support PHY link status detection
* :github:`23373` - ARM: Move CMSIS out of main tree
* :github:`23372` - arm: aarch32: spurious IRQ handler calling z_arm_reserved with wrong arguments' list
* :github:`23360` - Possible NULL dereference in  zephyr/arch/arm/include/aarch32/cortex_m/exc.h
* :github:`23353` - nrf51_ble400.dts i2c pins inverted
* :github:`23346` - bl65x_dvk boards do not reset after flashing
* :github:`23339` - tests/kernel/sched/schedule_api failed on mps2_an385 with v1.14 branch.
* :github:`23337` - USB DFU device + Composite Device with ACM Serial - Windows Fails
* :github:`23324` - TinyCBOR is not linked to application files unless CONFIG_MCUMGR is selected
* :github:`23311` - Sanitycheck flash error on frdm_k64f board.
* :github:`23309` - Sanitycheck generated incorrect acrn.xml on acrn platform
* :github:`23299` - Some bugs or dead codes cased by possible NULL pointers
* :github:`23295` - [Coverity CID :208676] Overlapping buffer in memory copy in subsys/usb/class/mass_storage.c
* :github:`23294` - [Coverity CID :208677] Unchecked return value in drivers/sensor/lis3mdl/lis3mdl_trigger.c
* :github:`23284` - driver: ethernet: Add support for a second Ethernet controller in the MCUX driver
* :github:`23280` - Bluetooth: hci_usb fails to connect to two devices with slow advertising interval
* :github:`23278` - uart_basic_api test fails for SAM family devices
* :github:`23274` - power: subsystem: Application hangs when logging is enabled after entering deep sleep
* :github:`23247` - Bluetooth LE: Add feature to allow profiles to change ADV data at RPA updates
* :github:`23246` - net: tx_bufs are not freed when NET_TCP_BACKLOG_SIZE is too high
* :github:`23226` - Bluetooth: host: Peer not resolved when host resolving is used
* :github:`23225` - Bluetooth: Quality of service: Adaptive channel map
* :github:`23222` - Bluetooth: host: Unable to pair when privacy feature is disabled by application
* :github:`23207` - tests/kernel/mem_pool/mem_pool_concept failed on mec15xxevb_assy6853 board.
* :github:`23193` - Allow overriding get_mac() function in ieee802154 drivers
* :github:`23187` - nrf_rtc_timer.c  timseout setting mistake.
* :github:`23184` - mqtt_connect fails with return -2
* :github:`23156` - App determines if Bluetooth host link request is allowed
* :github:`23153` - Binding AF_PACKET socket second time will fail with multiple network interfaces
* :github:`23133` - boards: adafruit_feather_m0: don't throw compiler warnings on using custom sercom config
* :github:`23117` - Unable to flash hello_world w/XDS-110 & OpenOCD
* :github:`23107` - Convert SAM SoC drivers to DT_INST
* :github:`23106` - timer_api intermittent failures on Nordic nRF
* :github:`23070` - Bluetooth: controller: Fix ticker implementation to avoid catch up
* :github:`23026` - missing ISR locking in UART driver?
* :github:`23001` - Implement SAM E5X GMAC support
* :github:`22997` - Add GMAC device tree definition
* :github:`22964` - Define a consistent naming convention for device tree defines
* :github:`22948` - sanitycheck --build-only followed by --test-only fails
* :github:`22911` - [Coverity CID :208407] Unsigned compared against 0 in drivers/modem/modem_pin.c
* :github:`22910` - [Coverity CID :208408] Unsigned compared against 0 in drivers/modem/modem_pin.c
* :github:`22909` - [Coverity CID :208409] Unchecked return value in tests/drivers/gpio/gpio_basic_api/src/test_deprecated.c
* :github:`22908` - [Coverity CID :208410] Unsigned compared against 0 in drivers/modem/modem_pin.c
* :github:`22907` - si7006 temperature conversion offset missing
* :github:`22903` - mcuboot/samples/zephyr (make test-good-rsa) doesn't work
* :github:`22887` - Atomic operations on pointers
* :github:`22860` - Highly accurate synchronized clock distribution for BLE mesh network
* :github:`22780` - Sanitycheck hardware map integration caused some tests failure.
* :github:`22777` - Sanitycheck hardware map integration failed with some tests timeout.
* :github:`22745` - schedule_api  fails with slice testing on frdmkw41z board on v2.2.0_rc1
* :github:`22738` - crashes in tests/kernel/mem_protect/userspace case pass_noperms_object on x86_64
* :github:`22732` - IPv6 address and prefix timeout failures
* :github:`22701` - Implement I2C driver for lpcxpresso55s69
* :github:`22679` - MQTT publish causes unnecessary TCP segmentation
* :github:`22670` - Implement GIC-based ARM interrupt tests
* :github:`22643` - [Coverity CID :208206] Unsigned compared against 0 in samples/sensor/fxos8700-hid/src/main.c
* :github:`22625` - tests/subsys/canbus/isotp/conformance fails on frdm_k64f and twr_ke18f boards
* :github:`22622` - tests/drivers/gpio/gpio_basic_api failed on multiple ARM platforms
* :github:`22561` - tests/kernel/mem_protect/syscalls fails test_string_nlen on nsim_sem
* :github:`22555` - Add support to device tree generation support for DT_NODELABEL_<node-label>_<FOO> generation
* :github:`22554` - Add support to device tree generation support for DT_PATH_<path>_<FOO> generation
* :github:`22541` - hal_nordic: nrf_glue.h change mapped assert function
* :github:`22521` - intermittent crash in tests/portability/cmsis_rtos_v2 on qemu_x86
* :github:`22502` - USB transfer warnings
* :github:`22452` - not driver found in can bus samples for olimexino_stm32
* :github:`22441` - [Coverity CID :207967] Invalid type in argument to printf format specifier in samples/drivers/spi_flash/src/main.c
* :github:`22431` - [Coverity CID :207984] Sizeof not portable in drivers/counter/counter_handlers.c
* :github:`22429` - [Coverity CID :207989] Dereference after null check in drivers/sensor/sensor_shell.c
* :github:`22421` - mbed TLS: Inconsistent Kconfig option names
* :github:`22356` - An application hook for early init
* :github:`22348` - LIS2DH SPI Support
* :github:`22270` - wrong total of testcases when sanitycheck is run with a single test
* :github:`22264` - drivers: serial: nrf_uart & nrf_uarte infinite hang
* :github:`22222` - Enabling OpenThread SLAAC
* :github:`22158` - flash_img: support arbitrary flash devices
* :github:`22083` - stm32: spi: Infinite loop of RXNE bit check
* :github:`22078` - stm32: Shell module sample doesn't work on nucleo_l152re
* :github:`22034` - Add support for USB device on STM32L1 series
* :github:`21984` - i2c_4 not working on stm32f746g_disco
* :github:`21955` - usb: tests/subsys/usb/device fails on all NXP RT boards
* :github:`21932` - Current consumption on nrf52_pca10040, power_mgr sample
* :github:`21917` - cmake error with CONFIG_COUNTER and CONFIG_BT both enabled (nrf52 board)
* :github:`21899` - STM32F769I-DISCO > microSD + FatFS > failed in "samples/subsys/fs/fat_fs" > CMD0 and 0x01
* :github:`21877` - tests/drivers/uart/uart_async_api fails on qemu_cortex_m0
* :github:`21833` - SRAM not sufficient when building BT Mesh developer guide build on BBC Micro-bit
* :github:`21820` - docs: "Crypto Cipher" API isn't available in the docs
* :github:`21755` - tests/drivers/adc/adc_api  failed on  mec15xxevb_assy6853 board.
* :github:`21706` - Link to releases in README.rst give a 404 error
* :github:`21701` - [Coverity CID :206600] Logically dead code in drivers/crypto/crypto_mtls_shim.c
* :github:`21677` - [Coverity CID :206388] Unrecoverable parse warning in subsys/cpp/cpp_new.cpp
* :github:`21675` - [Coverity CID :206390] Unrecoverable parse warning in subsys/cpp/cpp_new.cpp
* :github:`21514` - Logging - strange behaviour with RTT on nRF53
* :github:`21513` - NULL parameter checks in Zephyr APIs
* :github:`21500` - RFC: k_thread_join()
* :github:`21469` - ARC SMP is mostly untested in sanitycheck
* :github:`21455` - driver: subsys: sdhc: USAGE FAULT trace and no cs control
* :github:`21441` - Add UART5 on B-port to H7 pinmux
* :github:`21426` - civetweb triggers an error on Windows with Git 2.24
* :github:`21390` - BLE Incomplete Connect results in subsquent encryption failures
* :github:`21372` - cc26x2r1_launchxl build passed, but can't flash
* :github:`21369` - devicetree: clearly define constraints on identifier/property name conflicts
* :github:`21321` - error update for project civetweb
* :github:`21305` - New Kernel Timeout API
* :github:`21253` - 2.2 Release Checklist
* :github:`21201` - ARM: Core Stack Improvements/Bug fixes for 2.2 release
* :github:`21200` - Replace IWDG_STM32_START_AT_BOOT by WDT_DISABLE_AT_BOOT
* :github:`21158` - Giving Semaphore Limit+1 can cause limit+1 takes
* :github:`21156` - Interrupts do not work on UP Squared board
* :github:`21107` - LL_ASSERT and 'Imprecise data bus error' in LL Controller
* :github:`21093` - put sys_trace_isr_enter/sys_trace_isr_exit to user care about ISR instead of every ISR
* :github:`21088` - Bluetooth: Mesh: Send Model Message shouldn't require explicit NetKey Index
* :github:`21068` - Conflicting documentation for device initialization
* :github:`20993` - spinlock APIs need documentation
* :github:`20991` - test_timer_duration_period fails with stm32 lptimer
* :github:`20945` - samples/synchronization fails on nsim_hs_smp and nsim_sem_normal
* :github:`20876` - [Coverity CID :205820] Memory - corruptions in tests/crypto/tinycrypt/src/cmac_mode.c
* :github:`20875` - [Coverity CID :205840] Memory - corruptions in tests/benchmarks/mbedtls/src/benchmark.c
* :github:`20874` - [Coverity CID :205805] Memory - corruptions in tests/benchmarks/mbedtls/src/benchmark.c
* :github:`20873` - [Coverity CID :205782] Memory - corruptions in tests/benchmarks/mbedtls/src/benchmark.c
* :github:`20835` - [Coverity CID :205797] Control flow issues in drivers/flash/spi_nor.c
* :github:`20825` - stm32: dma: enable dma with peripheral using DMAMUX
* :github:`20699` - Each board should have a list of Kconfig options supported
* :github:`20632` - call to bt_gatt_hids_init influences execution time of work queue
* :github:`20604` - log will be discarded before logging_thread scheduled once
* :github:`20585` - z_clock_announce starvation with timeslicing active
* :github:`20492` - [Coverity CID :205653]Control flow issues in /drivers/dma/dma_stm32_v1.c
* :github:`20491` - [Coverity CID :205644]Control flow issues in /drivers/dma/dma_stm32_v1.c
* :github:`20348` - Convert remaining entropy to Devicetree
* :github:`20330` - devicetree Arduino bindings do not support identification of bus controllers
* :github:`20301` - tests/drivers/watchdog/wdt_basic_api failed on mec15xxevb_assy6853 board.
* :github:`20259` - Bluetooth: Mesh: Network management
* :github:`20137` - posix: undefined reference with --no-gc-sections
* :github:`20136` - kernel: undefined reference with --no-gc-sections
* :github:`20068` - Application doesn't start when SHELL-UART is enabled and UART is not connected on STM32F0
* :github:`19869` - Implement tickless capability for xlnx_psttc_timer
* :github:`19852` - Add support for GPIO AF remap on STM32F1XX
* :github:`19837` - SS register is 0 when taking exceptions on qemu_x86_long
* :github:`19813` - tests/crypto/rand32 failed on sam_e70 board on v1.14 branch.
* :github:`19763` - tests/subsys/usb/device/ failed on mimxrt1050_evk board.
* :github:`19614` - Make zephyr_library out of hal_stm32 and hal_st
* :github:`19550` - drivers/pcie: ``pcie_get_mbar()`` should return a ``void *`` not ``u32_t``
* :github:`19487` - tests/kernel/fifo/fifo_usage GPF crash on qemu_x86_long
* :github:`19456` - arch/x86: make use of z_bss_zero() and z_data_copy()
* :github:`19353` - arch/x86: QEMU doesn't appear to support x2APIC
* :github:`19307` - _interrupt_stack is defined in the kernel, but declared in arch headers
* :github:`19285` - devicetree: fixed non-alias reference to specific nodes
* :github:`19235` - move drivers/timer/apic_timer.c to devicetree
* :github:`19219` - drivers/i2c/i2c_dw.c is not 64-bit clean
* :github:`19144` - arch/x86: CONFIG_BOOT_TIME_MEASUREMENT broken
* :github:`19075` - k_delayed_work_submit() does not handle long delays correctly
* :github:`19067` - non-overlapping MPU gap-filling needs to be optional
* :github:`19038` - [zephyr branch 1.14 and master -stm32-netusb]:errors when i view RNDIS Device‘s properties on Windows 10
* :github:`18956` - memory protection for x86 dependent on XIP
* :github:`18940` - Counter External Trigger
* :github:`18808` - Docs for gpmrb board incorrectly refer to up_squared board
* :github:`18787` - arch/x86: retire loapic_timer.c driver in favor of new apic_timer.c
* :github:`18657` - drivers/timer/hpet.c should use devicetree, not CONFIG_* for MMIO/IRQ data
* :github:`18614` - same70 hsmci interface
* :github:`18568` - Support for Particle Photon
* :github:`18435` - [Coverity CID :203481]API usage errors in /tests/crypto/tinycrypt/src/test_ecc_utils.c
* :github:`18425` - [Coverity CID :203498]Memory - corruptions in /tests/application_development/gen_inc_file/src/main.c
* :github:`18422` - [Coverity CID :203415]Memory - illegal accesses in /subsys/shell/shell_telnet.c
* :github:`18389` - [Coverity CID :203396]Null pointer dereferences in /subsys/bluetooth/mesh/access.c
* :github:`18386` - [Coverity CID :203443]Memory - corruptions in /subsys/bluetooth/host/rfcomm.c
* :github:`18263` - flash sector erase fails on stm32f412
* :github:`18207` - tests/bluetooth/hci_prop_evt fails with code coverage enabled in qemu_x86
* :github:`18124` - synchronization example fails to build for SMP platforms
* :github:`18118` - samples/subsys/console doesn't work with qemu_riscv32
* :github:`18106` - Only 1 NET_SOCKET_OFFLOAD driver can be used
* :github:`18085` - I2C log level ignored
* :github:`18050` - BT Host - Advertisement extensions support
* :github:`18047` - BT Host: Advertising Extensions - Advertiser
* :github:`18046` - BT Host: Advertising Extensions - Scanner
* :github:`18044` - BT Host: Advertising Extensions - Periodic Advertisement Synchronisation (Rx)
* :github:`18042` - Only corporate members can join the slack channel
* :github:`17892` - arch/x86: clean up segmentation.h
* :github:`17888` - arch/x86: remove IAMCU ABI support
* :github:`17775` - Microchip XEC rtos timer should be using values coming from DTS
* :github:`17755` - ARC privilege mode stacks waste memory due to alignment requirements
* :github:`17735` - abolish Z_OOPS() in system call handlers
* :github:`17543` - dtc version 1.4.5 with ubuntu 18.04 and zephyr sdk-0.10.1
* :github:`17508` - RFC: Change/deprecation in display API
* :github:`17443` - Kconfig: move arch-specific stack sizes to arch trees?
* :github:`17430` - arch/x86: drivers/interrupt_controller/system_apic.c improperly classifies IRQs
* :github:`17415` - Settings Module - settings_line_val_read() returning -EINVAL instead of 0 for deleted setting entries
* :github:`17361` - _THREAD_QUEUED overlaps with x86 _EXC_ACTIVE in k_thread.thread_state
* :github:`17324` - failing bluetooth tests with code coverage enabled in qemu_x86
* :github:`17323` - failing network tests with code coverage enabled in qemu_x86
* :github:`17240` - add arc support in Zephyr's openthread
* :github:`17234` - CONFIG_KERNEL_ENTRY appears to be superfluous
* :github:`17166` - arch/x86: eliminate support for CONFIG_REALMODE
* :github:`17135` - Cannot flash LWM2M example for ESP32
* :github:`17133` - arch/x86: x2APIC EOI should be inline
* :github:`17104` - arch/x86: fix -march flag for Apollo Lake
* :github:`17064` - drivers/serial/uart_ns16550: CMD_SET_DLF should be removed
* :github:`16988` - Packet isn't received by server during stepping
* :github:`16902` - CMSIS v2 emulation assumes ticks == milliseconds
* :github:`16886` - Bluetooth Mesh: Receive segmented message multiple times
* :github:`16721` - PCIe build warnings from devicetree
* :github:`16720` - drivers/loapic_timer.c is buggy, needs cleanup
* :github:`16649` - z_init_timeout() ignores fn parameter
* :github:`16587` - build failures with gcc 9.x
* :github:`16436` - Organize generated include files
* :github:`16385` - watch dog timer causes the reboot on SAME70 board
* :github:`16330` - LPCXpresso55S69 secure/non-secure configuration
* :github:`16196` - display_mcux_elcdif driver full support frame buffer features
* :github:`16122` - Detect first block in LWM2M firmware updates.
* :github:`16096` - Sam gmac Ethernet driver should be able to detect the carrier state
* :github:`16072` - boards/up_squared: k_sleep() too long with local APIC timer
* :github:`15903` - Documentation missing for SPI and ADC async operations
* :github:`15680` - "backport v1.14 branch" label: update description and doc
* :github:`15565` - undefined references to ``sys_rand32_get``
* :github:`15504` -  Can I use one custom random static bd_addr before provision?
* :github:`15499` - gpio_intel_apl: gpio_pin_read() pin value doesn't match documentation
* :github:`15463` - soc/x86/apollo_lake/soc_gpio.h: leading zeros on decimal constants
* :github:`15449` - tests/net/ieee802154/crypto: Assertion Failure:  ds_test(dev) is false
* :github:`15343` - tests/kernel/interrupt: Assertion Failure in test_prevent_interruption
* :github:`15304` - merge gen_kobject_list.py and gen_priv_stacks.py
* :github:`15202` - tests/benchmarks/timing_info measurements are suddenly higher than previous values on nrf52_pca10040
* :github:`15181` - ztest issues
* :github:`15177` - samples/drivers/crypto:  CBC and CTR mode not supported
* :github:`14972` - samples: Create README.rst
* :github:`14790` - google_iot_mqtt sample does not work with qemu_x86 out of the box
* :github:`14763` - PCI debug logging cannot work with PCI-enabled NS16550
* :github:`14749` - Verify all samples work as intended
* :github:`14647` - IP: Zephyr replies to broadcast ethernet packets in other subnets on the same wire
* :github:`14591` - Infineon Tricore architecture support
* :github:`14540` - kernel: message queue MACRO not compatible with C++
* :github:`14302` - USB MSC fails USB3CV tests
* :github:`14173` - Configure QEMU to run independent of the host clock
* :github:`14122` - CONFIG_FLOAT/CONFIG_FP_SHARING descriptions are confusing and contradictory
* :github:`14099` - Minnowboard doesn't build tests/kernel/xip/
* :github:`13963` - up_squared: evaluate removal of SBL-related special configurations
* :github:`13821` - tests/kernel/sched/schedule_api: Assertion failed for test_slice_scheduling
* :github:`13783` - tests/kernel/mem_protect/stackprot failure in frdm_k64f due to limited privilege stack size
* :github:`13569` - ZTEST: Add optional float/double comparison support
* :github:`13468` - tests/drivers/watchdog/wdt_basic_api/testcase.yaml: Various version of "Waiting to restart MCU"
* :github:`13353` - z_timeout_remaining should subtract z_clock_elapsed
* :github:`12872` - Update uart api tests with configure/configure_get apis
* :github:`12775` - USB audio isochronous endpoints
* :github:`12553` - List of tests that keep failing sporadically
* :github:`12478` - tests/drivers/ipm/peripheral.mailbox failing sporadically on qemu_x86_64 (timeout)
* :github:`12440` - Device discovery of direct advertising devices is not working
* :github:`12385` - Support touch button
* :github:`12264` - kernel: poll: outdated check for expired timeout
* :github:`11998` - intermittent failures in tests/kernel/common: test_timeout_order: (poll_events[ii].state not equal to K_POLL_STATE_SEM_AVAILABLE)
* :github:`11928` - nRF UART nrfx drivers (nRF UARTE 0) won't build
* :github:`11916` - ISR table (_sw_isr_table) generation is fragile and can result in corrupted binaries
* :github:`11745` - logging: never leaves panic mode on fatal thread exception
* :github:`11261` - ARM Cortex-M4 (EFR32FG1P) MCU fails to wake up from sleep within _sys_soc_suspend()
* :github:`11149` - subsys/bluetooth/host/rfcomm.c: Missing unlock
* :github:`11016` - nRF52840-PCA10056/59: Cannot bring up HCI0 when using HCI_USB sample
* :github:`9994` - irq_is_enabled not available on nios2
* :github:`9962` - Migrate sensor drivers to device tree
* :github:`9953` - wrong behavior in pthread_barrier_wait()
* :github:`9741` - tests/kernel/spinlock:kernel.multiprocessing.spinlock_bounce crashing on ESP32
* :github:`9711` - RFC: Zephyr should provide a unique id interface
* :github:`9608` - Bluetooth: different transaction collision
* :github:`9566` - Unclear definition of CONFIG_IS_BOOTLOADER
* :github:`8139` - Driver for BMA400 accelerometer
* :github:`7868` - Support non-recursive single-toolchain multi-image builds
* :github:`7564` - dtc: define list of acceptable warnings (and silent them with --warning -no<warnign-name> option)
* :github:`6648` - Trusted Execution Framework: practical use-cases (high-level overview)
* :github:`6015` - PWM on 32bit arch can get 0 pulse_cycle because of 64bit calculation
* :github:`5857` - net: TCP retransmit queue implementation is broken
* :github:`5408` - Improve docs & samples on device tree overlay
* :github:`4985` - TEE support for ARMv8-M
* :github:`4911` - Filesystem support for qemu
* :github:`4832` - disco_l475_iot1: Provide 802.15.4 Sub-GHz
* :github:`4475` - Add support for Rigado BMD-3XX-EVAL boards
* :github:`4412` - Replace STM32 USB driver with DWC
* :github:`4326` - Port Zephyr to Cypress PSoC 6 MCU's
* :github:`3909` - Add Atmel SAM QDEC Driver
* :github:`3730` - ESP32: DAC Driver support
* :github:`3729` - ESP32 ADC Driver Support
* :github:`3727` - ESP32: SPI Driver Support
* :github:`3726` - ESP32: DMA Driver Support
* :github:`3694` - i2c: Drivers are not thread safe
* :github:`3668` - timeslice reset is not tested for interrupt-induced swaps
* :github:`3564` - Requires more UART samples for STM32 Nucleo/similar boards
* :github:`3285` - Allow taking advantage of HW-based AES block cipher
* :github:`3232` - Add ksdk dma shim driver
* :github:`3076` - Add support for DAC (Digital to Analog Converter) drivers
* :github:`2585` - Support for LE legacy out-of-band pairing
* :github:`2566` - Create a tool for finding out stack sizes automatically.
* :github:`1900` - Framework for Trusted Execution Environment
* :github:`1894` - Secure Key Storage
* :github:`1333` - Provide build number in include/generated/version.h
