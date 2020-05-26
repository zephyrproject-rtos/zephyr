:orphan:

.. _zephyr_2.3:

Zephyr 2.3.0 (Working Draft)
############################

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

Removed APIs in this release
============================


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

* A new general purpose memory allocator, sys_heap/k_heap, was added
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

  * Add support for hard floating point for RISC-V
  * Add march and mabi options to Kconfig
  * Fix compilation warning for platforms without PLIC

* x86:

  * <TBD>

Boards & SoC Support
********************

* Added support for these SoC series:

.. rst-class:: rst-columns

   * Broadcom Viper BCM58402
   * Infineon XMC4500 SoC
   * Nordic nRF52820 SoC
   * NXP LPC55S16 SoC
   * SiLabs EFR32BG13P SoC
   * STM32L5 series of Ultra-low-power MCUs

* Added support for these ARM boards:

  .. rst-class:: rst-columns

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

* Added support for these following shields:

  .. rst-class:: rst-columns

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

* Neural Net

  * N/A

* PCIe

  * N/A

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

* Convert networking to use new k_timeout_t infrastructure
* Enhance new TCP stack support
* Add minimal support for TFTP client (RFC 1350)
* Add support for network device driver power management
* Add support for socketpair() BSD socket API
* Add support for QEMU user networking (SLIRP)
* Add support to disable automatic network attachment in OpenThread
* Add support for Frame Pending Bit handling in OpenThread
* Add support for RX frame handling in OpenThread
* Add support for TX started notification in OpenThread
* Add support for HW CSMA CA in OpenThread
* Add support for promiscuous mode in OpenThread
* Add support for reading OPAQUE resources with OMA TLV in LWM2M
* Add config to enable PAN coordinator mode in IEEE 802.15.4
* Add config to enable promiscuous mode in IEEE 802.15.4
* Add support for subscribe in Azure cloud sample
* Add support for queue mode in lwm2m_client sample
* Add support to allow change of the QEMU Ethernet interface name
* Add support for PPP IPCP to negotiate used DNS servers
* Add support for setting hostname in DHCPv4 request
* Fix binding AF_PACKET socket type multiple times
* Fix LLDPDU data in sent LLDP packets
* Fix and enhance Google IoT sample application documentation
* Fix MQTT cloud sample when polling incoming messages
* Fix LWM2M socket error handling, and pending and reply handling during start
* Fix LWM2M retransmission logic
* Fix LWM2M Cell ID resource initialization
* Fix COAP pending and reply handling
* Fix wpan_serial sample application and enable USB during initialization
* Fix HTTP client payload issue on HTTP upload
* Fix MQTT Websocket incoming data handling and accept packets only in RX
* Fix MQTT Publish message length validation
* Fix IEEE 802.15.4 received frame length validation
* Fix IEEE 802.15.4 and avoid ACK processing when not needed
* Fix IEEE 802.15.4 and allow energy detection scan unconditionally

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

  * <TBD>

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

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title
