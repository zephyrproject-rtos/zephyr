:orphan:

.. _zephyr_2.7:

Zephyr 2.7.0
############

We are pleased to announce the release of Zephyr RTOS version 2.7.0 (LTS2).

Major enhancements since v2.6.0 include:

* Bluetooth Audio, Direction Finding, and Mesh improvements
* Support for Bluetooth Advertisement PDU Chaining
* Added support for armclang / armlinker toolchain
* Added support for MWDT C / C++ toolchain
* Update to CMSIS v5.8.0 (Core v5.5.0, DSP v1.9.0)
* Support for M-Profile Vector Extensions (MVE) on ARMv8.1-M
* Improved thread safety for Newlib and C++ on SMP-capable systems
* IEEE 802.15.4 Software Address Filtering
* New Action-based Power Management API
* USB Device Framework now includes all Chapter 9 defines and structures
* Generic System Controller (``syscon``) driver and emulator
* Linker Support for Tightly-Coupled Memory in RISC-V
* Additional Blocking API calls for LoRa
* Support for extended PCI / PCIe capabilities, improved MIS-X support
* Added Support for Service Type Enumeration (STE) with mDNS / DNS Service Discovery
* Added Zephyr Thread Awareness for OpenOCD to West
* EEPROM now can be emulated in flash
* Added both Ethernet MDIO and Ethernet generic PHY drivers

Additional Major enhancements since v1.14.0 (LTS1) include:

* The kernel now supports both 32- and 64-bit architectures
* We added support for SOCKS5 proxy
* Introduced support for 6LoCAN, a 6Lo adaption layer for Controller Area Networks
* We added support for Point-to-Point Protocol (PPP)
* We added support for UpdateHub, an end-to-end solution for over-the-air device updates
* We added support for ARM Cortex-R Architecture
* Normalized APIs across all architectures
* Expanded support for ARMv6-M architecture
* Added support for numerous new boards and shields
* Added numerous new drivers and sensors
* Added BLE support on Vega platform
* Memory size improvements to Bluetooth host stack
* We added initial support for 64-bit ARMv8-A architecture
* CANopen protocol support through 3rd party CANopenNode stack
* LoRa support was added along with the SX1276 LoRa modem driver
* A new Zephyr CMake package has been introduced
* A new Devicetree API which provides access to virtually all DT nodes and properties
* The kernel timeout API has been overhauled
* A new k_heap/sys_heap allocator, with improved performance
* Zephyr now integrates with the TF-M (Trusted Firmware M) PSA-compliant framework
* The Bluetooth Low Energy Host now supports LE Advertising Extensions
* The CMSIS-DSP library is now included and integrated
* Introduced initial support for virtual memory management
* Added Bluetooth host support for periodic advertisement and isochronous channels.
* Added a new TCP stack which improves network protocol testability
* Introduced a new toolchain abstraction with initial support for GCC and LLVM/Clang
* Moved to using C99 integer types and deprecate Zephyr integer types
* Introduced support for the SPARC architecture and the LEON implementation
* Added Thread Local Storage (TLS) support
* Added support for per thread runtime statistics
* Added support for building with LLVM on X86
* Added new synchronization mechanisms using Condition Variables
* Add support for demand paging, initial support on X86
* Logging subsystem overhauled
* Added support for 64-bit ARCv3
* Split ARM32 and ARM64, ARM64 is now a top-level architecture
* Added initial support for Arm v8.1-m and Cortex-M55
* Removed legacy TCP stack support which was deprecated in 2.4
* Tracing subsystem overhaul / added support for Percepio Tracealyzer
* Device runtime power management (PM) completely overhauled
* Automatic SPDX SBOM generation has been added to West
* Added an example standalone Zephyr application

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2021-3510: `Zephyr project bug tracker GHSA-289f-7mw3-2qf4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-289f-7mw3-2qf4>`_.


Known issues
************

You can check all currently known issues by listing them using the GitHub
interface and listing all issues with the `bug label
<https://github.com/zephyrproject-rtos/zephyr/issues?q=is%3Aissue+is%3Aopen+label%3Abug>`_.

API Changes
***********

Deprecated in this release

* :c:macro:`DT_ENUM_TOKEN` and :c:macro:`DT_ENUM_UPPER_TOKEN`,
  were deprecated in favor of utilizing
  :c:macro:`DT_STRING_TOKEN` and :c:macro:`DT_STRING_UPPER_TOKEN`

* :c:macro:`BT_CONN_ROLE_MASTER` and :c:macro:`BT_CONN_ROLE_SLAVE`
  have been deprecated in favor of
  :c:macro:`BT_CONN_ROLE_CENTRAL` and :c:macro:`BT_CONN_ROLE_PERIPHERAL`

* :c:macro:`BT_LE_SCAN_OPT_FILTER_WHITELIST`
  has been deprecated in favor of
  :c:macro:`BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST`

* The following whitelist functions have been deprecated:
  :c:func:`bt_le_whitelist_add`
  :c:func:`bt_le_whitelist_rem`
  :c:func:`bt_le_whitelist_clear`
  in favor of
  :c:func:`bt_le_filter_accept_list_add`
  :c:func:`bt_le_filter_accept_list_remove`
  :c:func:`bt_le_filter_accept_list_clear`

Modified in this release

* The following Bluetooth macros and structures in :file:`hci.h` have been
  modified to align with the inclusive naming in the v5.3 specification:

  * ``BT_LE_FEAT_BIT_SLAVE_FEAT_REQ`` is now ``BT_LE_FEAT_BIT_PER_INIT_FEAT_XCHG``
  * ``BT_LE_FEAT_BIT_CIS_MASTER`` is now ``BT_LE_FEAT_BIT_CIS_CENTRAL``
  * ``BT_LE_FEAT_BIT_CIS_SLAVE`` is now ``BT_LE_FEAT_BIT_CIS_PERIPHERAL``
  * ``BT_FEAT_LE_SLAVE_FEATURE_XCHG`` is now ``BT_FEAT_LE_PER_INIT_FEAT_XCHG``
  * ``BT_FEAT_LE_CIS_MASTER`` is now ``BT_FEAT_LE_CIS_CENTRAL``
  * ``BT_FEAT_LE_CIS_SLAVE`` is now ``BT_FEAT_LE_CIS_PERIPHERAL``
  * ``BT_LE_STATES_SLAVE_CONN_ADV`` is now ``BT_LE_STATES_PER_CONN_ADV``
  * ``BT_HCI_OP_LE_READ_WL_SIZE`` is now ``BT_HCI_OP_LE_READ_FAL_SIZE``
  * ``bt_hci_rp_le_read_wl_size`` is now ``bt_hci_rp_le_read_fal_size``
  * ``bt_hci_rp_le_read_wl_size::wl_size`` is now ``bt_hci_rp_le_read_fal_size::fal_size``
  * ``BT_HCI_OP_LE_CLEAR_WL`` is now ``BT_HCI_OP_LE_CLEAR_FAL``
  * ``BT_HCI_OP_LE_ADD_DEV_TO_WL`` is now ``BT_HCI_OP_LE_REM_DEV_FROM_FAL``
  * ``bt_hci_cp_le_add_dev_to_wl`` is now ``bt_hci_cp_le_add_dev_to_fal``
  * ``BT_HCI_OP_LE_REM_DEV_FROM_WL`` is now ``BT_HCI_OP_LE_REM_DEV_FROM_FAL``
  * ``bt_hci_cp_le_rem_dev_from_wl`` is now ``bt_hci_cp_le_rem_dev_from_fal``
  * ``BT_HCI_ROLE_MASTER`` is now ``BT_HCI_ROLE_CENTRAL``
  * ``BT_HCI_ROLE_SLAVE`` is now ``BT_HCI_ROLE_PERIPHERAL``
  * ``BT_EVT_MASK_CL_SLAVE_BC_RX`` is now ``BT_EVT_MASK_CL_PER_BC_RX``
  * ``BT_EVT_MASK_CL_SLAVE_BC_TIMEOUT`` is now ``BT_EVT_MASK_CL_PER_BC_TIMEOUT``
  * ``BT_EVT_MASK_SLAVE_PAGE_RSP_TIMEOUT`` is now ``BT_EVT_MASK_PER_PAGE_RSP_TIMEOUT``
  * ``BT_EVT_MASK_CL_SLAVE_BC_CH_MAP_CHANGE`` is now ``BT_EVT_MASK_CL_PER_BC_CH_MAP_CHANGE``
  * ``m_*`` structure members are now ``c_*``
  * ``s_*`` structure members are now ``p_*``

* The ``CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY`` Kconfig option is now
  :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_LATENCY`
* The ``CONFIG_BT_CTLR_SLAVE_FEAT_REQ_SUPPORT`` Kconfig option is now
  :kconfig:option:`CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG_SUPPORT`
* The ``CONFIG_BT_CTLR_SLAVE_FEAT_REQ`` Kconfig option is now
  :kconfig:option:`CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG`

Changes in this release
==========================

Removed APIs in this release

* Removed support for the deprecated ``DEVICE_INIT`` and ``DEVICE_AND_API_INIT`` macros.
* Removed support for the deprecated ``BUILD_ASSERT_MSG`` macro.
* Removed support for the deprecated ``GET_ARG1``, ``GET_ARG2`` and ``GET_ARGS_LESS_1`` macros.
* Removed support for the deprecated Kconfig ``PRINTK64`` option.
* Removed support for the deprecated ``bt_set_id_addr`` function.
* Removed support for the Kconfig ``USB`` option. Option ``USB_DEVICE_STACK``
  is sufficient to enable USB device support.

* Removed ``CONFIG_OPENTHREAD_COPROCESSOR_SPINEL_ON_UART_ACM`` and
  ``CONFIG_OPENTHREAD_COPROCESSOR_SPINEL_ON_UART_DEV_NAME`` Kconfig options
  in favor of chosen node ``zephyr,ot-uart``.
* Removed ``CONFIG_BT_UART_ON_DEV_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,bt-uart``.
* Removed ``CONFIG_BT_MONITOR_ON_DEV_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,bt-mon-uart``.
* Removed ``CONFIG_MODEM_GSM_UART_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,gsm-ppp``.
* Removed ``CONFIG_UART_MCUMGR_ON_DEV_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,uart-mcumgr``.
* Removed ``CONFIG_UART_CONSOLE_ON_DEV_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,console``.
* Removed ``CONFIG_UART_SHELL_ON_DEV_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,shell-uart``.

============================

Stable API changes in this release
==================================

* Bluetooth

  * Added :c:struct:`multiple` to the :c:struct:`bt_gatt_read_params` - this
    structure contains two members: ``handles``, which was moved from
    :c:struct:`bt_gatt_read_params`, and ``variable``.

* Networking

  * Added IPv4 address support to the multicast group join/leave monitor. The
    address parameter passed to the callback function was therefore changed from
    ``in6_addr`` to ``net_addr`` type.

Kernel
******


Architectures
*************

* ARC

  * Add SMP support to ARCv3 HS6x
  * Add MWDT C library support
  * Add basic C++ support with MWDT toolchain
  * Add MPUv3 and MPUv6 support
  * Remove dead PM code from ARC core interrupt controller driver
  * Add updating arc connect debug mask dynamically


* ARM

  * AARCH32

     * Updated CMSIS version to 5.8.0
     * Added support for FPU in QEMU for Cortex-M, allowing to build and execute
       tests in CI with FPU and FPU_SHARING options enabled.
     * Added MPU support for Cortex-R


  * AARCH64


* RISC-V

  * Added support to RISC-V CPU devicetree compatible bindings
  * Added support to link with ITCM & DTCM sections


* x86


Bluetooth
*********

* Updated all APIs and internal implementation to be conformant with the new
  inclusive terminology in version 5.3 of the Bluetooth Core Specification

* Audio

  * Added the Microphone Input Control Service and client.
  * Changed the connected isochronous API to use a group-based opaque struct
  * Split the configuration options into connected and broadcast groups
  * Added support for a new sent callback to be notified when an SDU has been
    transmitted

* Direction Finding

  * Added configurability for conditional CTE RX support
  * Added support for CTE periodic advertising chain transmissions

* Host

  * Added support for setting more than 251 bytes of advertising data
  * Added new callbacks on ATT MTU update
  * Added a new call to retrieve the handle of an advertising set
  * Fixed key overwrite algorithm when working with multiple connections
  * Added configuration support for GATT security re-establishment
  * Added support for writing a long device name
  * OTS: Added object name write capability

* Mesh

  * Added return value for opcode callback
  * Added support for OOB Public Key in the provisionee role
  * Added a new API to manually store pending RPL entries
  * Added support for application access to mesh messages
  * Refactored the Mesh Model Extensions

* Bluetooth LE split software Controller

  * Added support for advertising PDU chaining, implementing advertising trains
    for Direction Finding
  * Added support for adding or removing the ACAD field in Common Extended
    Header Format to support BIGInfo
  * Refactored the legacy, extended and periodic advertising time reservation
    slot calculations
  * Implemented CSA#2 in Extended Advertising and Broadcast ISO sub-events
  * Added support for Extended Active Scanning
  * Added support for advertising on the S2 and S8 coding schemes
  * Added support for the Periodic Advertising channel map update indication

* HCI Driver

  * Removed all ``CONFIG_BT_*_ON_DEV_NAME`` Kconfig options, use Devicetree
    instead

Boards & SoC Support
********************

* Added support for these SoC series:

  * Added STM32U5 basic SoC support

* Removed support for these SoC series:


* Made these changes in other SoC series:

  * Added Atmel SAM0 pinctrl support
  * Added Atmel SAM4L USBC device controller
  * Added Atmel GMAC support for MDIO driver
  * Added Atmel GMAC support to use generic PHY driver
  * Added Atmel SAM counter (TC) Driver
  * Added Atmel SAM DAC (DACC) driver
  * Enabled Atmel SAM ``clock-frequency`` support from devicetree
  * Free Atmel SAM TRACESWO pin when unused
  * Enabled Cypress PSoC-6 Cortex-M4 support
  * Added low power support to STM32L0, STM32G0 and STM32WL series
  * STM32: Enabled ART Flash accelerator by default when available (F2, F4, F7, H7, L5)
  * STM32: Added Kconfig option to enable STM32Cube asserts (CONFIG_USE_STM32_ASSERT)
  * NXP FRDM-K82F: Added arduino_i2c and arduino_spi aliases
  * NXP i.MX RT series: Added support for flash controller with XIP
  * NXP i.MX RT series: Added TRNG support
  * NXP i.MX RT1170: Added LPSPI driver support
  * NXP i.MX RT1170: Added ADC driver support
  * NXP i.MX RT1170: Enabled Segger RTT/SystemView
  * NXP i.MX RT1170: Added MCUX FlexCan support
  * NXP i.MX RT1064: Added watchdog driver support
  * NXP i.MX RT1064: Added DMA driver support
  * NXP i.MX RT600: Added arduino serial port
  * NXP i.MX RT600: Add mcuboot flash partitions
  * NXP i.MX RT600: Added counter support
  * NXP i.MX RT600: Added PWM support
  * NXP i.MX RT600: Added disk driver support
  * NXP i.MX RT600: Added USB driver support
  * NXP i.MX RT600: Added LPADC driver support
  * NXP i.MX RT600: Added CTimer Counter support
  * NXP KE1xF: Added SoC Power Management support
  * NXP LPC55s69: Added USB driver support
  * NXP LPC55s69: Added ctimer driver support
  * NXP LPC55s69: Added I2S driver support


* Changes for ARC boards:

  * Implement 'run' command for SMP nSIM simulation board
  * Enable upstream verification on QEMU ARCv3 HS6x board (qemu_arc_hs6x)
  * Implement creg GPIO driver and add it to hsdk and em_starterkit boards


* Changes for ARM boards:

  * Added SPI support on Arduino standard SPI when possible

* Added support for these ARM boards:

  * Dragino NBSN95 NB-IoT Sensor Node
  * Seeedstudio LoRa-E5 Dev Board
  * ST B_U585I_IOT02A Discovery kit
  * ST Nucleo F446ZE
  * ST Nucleo U575ZI Q
  * ST STM32H735G Discovery
  * PJRC Teensy 4 Board

* Added support for these ARM64 boards:


* Removed support for these ARM boards:


* Removed support for these X86 boards:


* Made these changes in other boards:

  * arduino_due: Added support for TC driver
  * atsame54_xpro: Added support for PHY driver
  * sam4l_ek: Added support for TC driver
  * sam4e_xpro: Added support for PHY driver
  * sam4e_xpro: Added support for TC driver
  * sam4s_xplained: Added support for TC driver
  * sam_e70_xplained: Added support for DACC driver
  * sam_e70_xplained: Added support for PHY driver
  * sam_e70_xplained: Added support for TC driver
  * sam_v71_xult: Added support for DACC driver
  * sam_v71_xult: Added support for PHY driver
  * sam_v71_xult: Added support for TC driver
  * sam_v71_xult: Enable pwm on LED0
  * cy8ckit_062_ble: Added arduino's nexus map


* Added support for these following shields:

  * 4.2inch epaper display (GDEW042T2)
  * X-NUCLEO-EEPRMA2 EEPROM memory expansion board

Drivers and Sensors
*******************

* ADC

  * Added STM32WL ADC driver
  * STM32: Added support for oversampling
  * Added driver for Microchip MEC172x

* Audio

  * Added DMIC driver for nRF PDM peripherals

* Bluetooth


* CAN

  * Renesas R-Car driver added


* Clock Control


* Console


* Counter

  * Add Atmel SAM counter (TC) Driver
  * Added STM32WL RTC counter driver

* Crypto

  * STM23: Add support for SOCs with AES hardware block (STM32G0, STM32L5 and STM32WL)

* DAC

  * Added Atmel SAM DAC (DACC) driver
  * Added support for Microchip MCP4725
  * Added support for STM32F3 series

* Disk

  * Added SDMMC support on STM32L4+
  * STM32 SDMMC now supports SDIO enabled devices
  * Added USDHC support for i.MX RT685

* Display

  * Added support for ST7735R

* DMA

  * Added Atmel SAM XDMAC reload support
  * Added support on STM32F3, STM32G0, STM32H7 and STM32L5
  * STM32: Reviewed bindings definitions, "st,stm32-dma-v2bis" introduced.


* EEPROM

  * Added support for EEPROM emulated in flash.

* Entropy

  * Added support for STM32F2, STM32G0, STM32WB and STM32WL

* ESPI

  * Added support for Microchip eSPI SAF

* Ethernet

  * Added Atmel SAM/SAM0 GMAC devicetree support
  * Added Atmel SAM/SAM0 MDIO driver
  * Added MDIO driver
  * Added generic PHY driver


* Flash

  * Added STM32F2, STM32L5 and STM32WL Flash driver support
  * STM32: Max erase time parameter was moved to device tree
  * Added quad SPI support for STM32F4

* GPIO


* Hardware Info


* I2C


* I2S

  * Added Atmel SAM I2S driver support to XDMAC reload
  * Added driver for nRF I2S peripherals

* IEEE 802.15.4

* IPM

  * STM32: Add HSEM based IPM driver for STM32H7 series

* Interrupt Controller


* LED


* LoRa

  * lora_send now blocks until the transmission is complete. lora_send_async
    can be used for the previous, non-blocking behaviour.
  * Enabled support for STM32WL series

* MEMC

  * Added STM32F4 support


* Modem

  * Added gsm_ppp devicetree support

* PCI/PCIe

  * Fixed an issue that MSI-X was used even though it is not available.
  * Improved MBAR retrieval for MSI-X.
  * Added ability to retrieve extended PCI/PCIe capabilities.

* Pinmux

  * Added Atmel SAM0 pinctrl support
  * STM32: Deprecated definitions like 'STM32F2_PINMUX_FUNC_PA0_UART4_TX'
    are now removed.


* PWM

  * Property "st,prescaler" of binding "st,stm32-pwm" now defaults to "0".
  * Added driver for ITE IT8XXX2 series
  * Added driver for NXP LPC devices
  * Added driver for Telink B91

* Sensor

  * Refactored various drivers to use ``_dt_spec``.
  * Refactored various drivers to support multiple instances.
  * Enhanced TI HDC20XX driver to support DRDY/INT pin.
  * Updated temperature conversion formula in TI HDC20XX driver.
  * Enhanced MS5607 pressure sensor driver to support I2C.
  * Fixed temperature compensation in MS5607 pressure sensor driver.
  * Refactored ST LIS2DW12 driver to move range, power, and trigger
    configuration from Kconfig to dts.
  * Enhanced TI BQ274XX fuel gauge driver to support power management.
  * Aligned ST sensor drivers to use STMEMC HAL I/F v2.00.
  * Added Sensirion SGP40 multipixel gas sensor driver.
  * Added Sensirion SHTCX humidity sensor driver.
  * Added Sensirion SHT4X temperature and humidity sensor driver.
  * Added SiLabs SI7270 hall effect magnetic position and temperature sensor
    driver.
  * Added ST I3G4250D gyroscope driver.
  * Added TI INA219 and INA23X current/power monitor drivers.
  * Added TI LM75 and LM77 temperature sensor drivers.
  * Added TI HDC20XX humidity and temperature sensor driver.

* Serial

  * Added kconfig to disable runtime re-configuration of UART
    to reduce footprint if so desired.
  * Added ESP32-C3 polling only UART driver.
  * Added ESP32-S2 polling only UART driver.
  * Added Microchip XEC UART driver.

* SPI


* Timer


* USB

  * Added Atmel SAM4L USBC device controller driver
  * Added support for NXP LPC USB controller
  * Adapted drivers use new USB framework header

* Watchdog

  * Added STM32L5 watchdog support


* WiFi


Networking
**********

* 802.15.4 L2:

  * Fixed a bug, where the net_pkt structure contained invalid LL address
    pointers after being processed by 802.15.4 L2.
  * Added an optional destination address filtering in the 802.15.4 L2.

* CoAP:

  * Added ``user_data`` field to the :c:struct:`coap_packet` structure.
  * Fixed processing of out-of-order notifications.
  * Fixed :c:func:`coap_packet_get_payload` function.
  * Converted CoAP test suite to ztest API.
  * Improved :c:func:`coap_packet_get_payload` function to minimize number
    of RNG calls.
  * Fixed retransmissions in the ``coap_server`` sample.
  * Fixed observer removal in the ``coap_server`` sample (on notification
    timeout).

* DHCPv4:

  * Fixed a bug, where DHPCv4 library removed statically configured gateway
    before obtaining a new one from the server.

* DNS:

  * Fixed a bug, where the same IP address was used to populate the result
    address info entries, when multiple IP addresses were obtained from the
    server.

* DNS-SD:

  * Added Service Type Enumeration support (``_services._dns_sd._udp.local``)

* HTTP:

  * Switched the library to use ``zsock_*`` API, to improve compatibility with
    various POSIX configurations.
  * Fixed a bug, where ``HTTP_DATA_FINAL`` notification was triggered even for
    intermediate response fragments.

* IPv6:

  * Multiple IPv6 fixes, addressing failures in IPv6Ready compliance tests.

* LwM2M:

  * Added support for notification timeout reporting to the application.
  * Fixed a bug, where a multi instance resource with only one active instance
    was incorrectly encoded on reads.
  * Fixed a bug, where notifications were generated on changes to non-readable
    resources.
  * Added mutex protection  for the state variable of the ``lwm2m_rd_client``
    module.
  * Removed LWM2M_RES_TYPE_U64 type, as it's not possible to encode it properly
    for large values.
  * Fixed a bug, where large unsigned integers were incorrectly encoded in TLV.
  * Multiple fixes for FLOAT type processing in the LwM2M engine and encoders.
  * Fix a bug, where IPSO Push Button counter resource was not triggering
    notification on incrementation.
  * Fixed a bug, where Register failures were reported as success to the
    application.

* Misc:

  * Added RX/TX timeout on a socket in ``big_http_download`` sample.
  * Introduced :c:func:`net_pkt_remove_tail` function.
    Added IEEE 802.15.4 security-related flags to the :c:struct:`net_pkt`
    structure.
  * Added bridging support to the Ethernet L2.
  * Fixed a bug in mDNS, where an incorrect address type could be set as a
    response destination.
  * Added an option to suppress ICMP destination unreachable errors.
  * Fixed possible assertion in ``net nbr`` shell command.
  * Major refactoring of the TFTP library.

* MQTT:

  * Added an option to register a custom transport type.
  * Fixed a bug in :c:func:`mqtt_abort`, where the function could return without
    releasing a lock.

* OpenThread:

  * Update OpenThread module up to commit ``9ea34d1e2053b6b2a80e1d46b65a6aee99fc504a``.
    Added several new Kconfig options to align with new OpenThread
    configurations.
  * Added OpenThread API mutex protection during initialization.
  * Converted OpenThread thread to a dedicated work queue.
  * Implemented missing :c:func:`otPlatAssertFail` platform function.
  * Fixed a bug, where NONE level OpenThread logs were not processed.
  * Added possibility to disable CSL sampling, when used.
  * Fixed a potential bug, where invalid error code could be returned by the
    platform radio layer to OpenThread.
  * Reworked UART configuration in the OpenThread Coprocessor sample.

* Socket:

  * Added microsecond accuracy in :c:func:`zsock_select` function.
  * Reworked :c:func:`zsock_select` into a syscall.
  * Fixed a bug, where :c:func:`poll` events were not signalled correctly
    for socketpair sockets.
  * Fixed a bug, where socket mutex could be used after being initialized by a
    new owner after being deallocated in :c:func:`zsock_close`.
  * Fixed a possible assert after enabling CAN sockets.
  * Fixed IPPROTO_RAW usage in packet socket implementation.

* TCP:

  * Fixed a bug, where ``unacked_len`` could be set to a negative value.
  * Fixed possible assertion failure in :c:func:`tcp_send_data`.
  * Fixed a bug, where [FIN, PSH, ACK] was not handled properly in
    TCP_FIN_WAIT_2 state.

* TLS:

  * Reworked TLS sockets to use secure random generator from Zephyr.
  * Fixed busy looping during DTLS handshake with offloaded sockets.
  * Fixed busy looping during TLS/DTLS handshake on non blocking sockets.
  * Reset mbed TLS session on timed out DTLS handshake, to allow a retry without
    closing a socket.
  * Fixed TLS/DTLS :c:func:`sendmsg` implementation for larger payloads.
  * Fixed TLS/DTLS sockets ``POLLHUP`` notification.

* WebSocket:

  * Fixed :c:func:`poll` implementation for WebSocket, which did not work
    correctly with offloaded sockets.
  * Fixed :c:func:`ioctl` implementation for WebSocket, which did not work
    correctly with offloaded sockets.

USB
***

* Added new header file where all defines and structures from Chapter 9
  (USB Device Framework) should be included.
* Revised configuration of USB device support.
  Removed Kconfig option ``CONFIG_USB`` and introduced Kconfig option
  ``CONFIG_USB_DEVICE_DRIVER`` to enable USB device controller drivers,
  which is selected when option ``CONFIG_USB_DEVICE_STACK`` is enabled.
* Enhanced verification of the control request in device stack, classes, and samples.
* Added support to store alternate interface setting.
* Added ``zephyr_udc0`` nodelabel to all boards with USB support to allow
  generic USB device support samples to be build.
* Reworked descriptors, config, and data definitions macros in CDC ACM class.
* Changed CDC ACM UART implementation to get configuration from devicetree.
  With this change, many ``CONFIG_*_ON_DEV_NAME`` options were removed and
  applications revised. See :ref:`usb_device_cdc_acm` for more information.

Build and Infrastructure
************************

* Devicetree API

  * New "for-each" macros which work like existing APIs, but take variable
    numbers of arguments: :c:macro:`DT_FOREACH_CHILD_VARGS`,
    :c:macro:`DT_FOREACH_CHILD_STATUS_OKAY_VARGS`,
    :c:macro:`DT_FOREACH_PROP_ELEM_VARGS`,
    :c:macro:`DT_INST_FOREACH_CHILD_VARGS`,
    :c:macro:`DT_INST_FOREACH_STATUS_OKAY_VARGS`,
    :c:macro:`DT_INST_FOREACH_PROP_ELEM_VARGS`

  * Other new "for-each" macros: :c:macro:`DT_FOREACH_STATUS_OKAY`,
    :c:macro:`DT_FOREACH_STATUS_OKAY_VARGS`

  * New macros for converting strings to C tokens: :c:macro:`DT_STRING_TOKEN`,
    :c:macro:`DT_STRING_UPPER_TOKEN`

  * New :ref:`devicetree-pinctrl-api` helper macros

* Devicetree tooling

  * Errors are now generated when invalid YAML files are discovered while
    searching for bindings. See :ref:`dt-where-bindings-are-located` for
    information on the search path.

  * File names ending in ``.yml`` are now considered YAML files when searching
    for bindings.

  * Errors are now generated if invalid node names are used. For example, the
    node name ``node?`` now generates an error message ending in ``node?: Bad
    character '?' in node name``. The valid node names are documented in
    "2.2.2 Node Names" of the Devicetree specification v0.3.

  * Warnings are now generated if a :ref:`compatible property
    <dt-important-props>` in the ``vendor,device`` format uses an unknown
    vendor prefix. This warning does not apply to the root node.

    Known vendor prefixes are defined in
    :file:`dts/bindings/vendor-prefixes.txt` files, which may appear in any
    directory in :ref:`DTS_ROOT <dts_root>`.

    These may be upgraded to errors using the edtlib Python APIs; Zephyr's CI
    now generates such errors.

* Devicetree bindings

  * Various bindings had incorrect vendor prefixes in their :ref:`compatible
    <dt-important-props>` properties; the following changes were made to fix
    these.

    .. list-table::
       :header-rows: 1

       - * Old compatible
         * New compatible
       - * ``nios,i2c``
         * :dtcompatible:`altr,nios2-i2c`
       - * ``cadence,tensilica-xtensa-lx4``
         * :dtcompatible:`cdns,tensilica-xtensa-lx4`
       - * ``cadence,tensilica-xtensa-lx6``
         * :dtcompatible:`cdns,tensilica-xtensa-lx6`
       - * ``colorway,lpd8803``
         * :dtcompatible:`greeled,lpd8803`
       - * ``colorway,lpd8806``
         * :dtcompatible:`greeled,lpd8806`
       - * ``grove,light``
         * :dtcompatible:`seeed,grove-light`
       - * ``grove,temperature``
         * :dtcompatible:`seeed,grove-temperature`
       - * ``max,max30101``
         * :dtcompatible:`maxim,max30101`
       - * ``ublox,sara-r4``
         * :dtcompatible:`u-blox,sara-r4`
       - * ``xtensa,core-intc``
         * :dtcompatible:`cdns,xtensa-core-intc`
       - * ``vexriscv,intc0``
         * :dtcompatible:`vexriscv-intc0`

    Out of tree users of these bindings will need to update their
    devicetrees.

    You can support multiple versions of Zephyr with one devicetree by
    including both the old and new values in your nodes' compatible properties,
    like this example for the LPD8803::

        my-led-strip@0 {
                compatible = "colorway,lpd8803", "greeled,lpd8803";
                ...
        };

  * Other new bindings in alphabetical order: :dtcompatible:`andestech,atcgpio100`,
    :dtcompatible:`arm,gic-v3-its`, :dtcompatible:`atmel,sam0-gmac`,
    :dtcompatible:`atmel,sam0-pinctrl`, :dtcompatible:`atmel,sam-dac`,
    :dtcompatible:`atmel,sam-mdio`, :dtcompatible:`atmel,sam-usbc`,
    :dtcompatible:`cdns,tensilica-xtensa-lx7`,
    :dtcompatible:`espressif,esp32c3-uart`,
    :dtcompatible:`espressif,esp32-intc`,
    :dtcompatible:`espressif,esp32s2-uart`, :dtcompatible:`ethernet-phy`,
    :dtcompatible:`fcs,fxl6408`, :dtcompatible:`ilitek,ili9341`,
    :dtcompatible:`ite,it8xxx2-bbram`, :dtcompatible:`ite,it8xxx2-kscan`,
    :dtcompatible:`ite,it8xxx2-pinctrl-conf`, :dtcompatible:`ite,it8xxx2-pwm`,
    :dtcompatible:`ite,it8xxx2-pwmprs`, :dtcompatible:`ite,it8xxx2-watchdog`,
    :dtcompatible:`lm75`, :dtcompatible:`lm77`, :dtcompatible:`meas,ms5607`,
    :dtcompatible:`microchip,ksz8863`, :dtcompatible:`microchip,mcp7940n`,
    :dtcompatible:`microchip,xec-adc-v2`, :dtcompatible:`microchip,xec-ecia`,
    :dtcompatible:`microchip,xec-ecia-girq`,
    :dtcompatible:`microchip,xec-gpio-v2`,
    :dtcompatible:`microchip,xec-i2c-v2`, :dtcompatible:`microchip,xec-pcr`,
    :dtcompatible:`microchip,xec-uart`, :dtcompatible:`nuvoton,npcx-bbram`,
    :dtcompatible:`nuvoton,npcx-booter-variant`,
    :dtcompatible:`nuvoton,npcx-ps2-channel`,
    :dtcompatible:`nuvoton,npcx-ps2-ctrl`, :dtcompatible:`nuvoton,npcx-soc-id`,
    :dtcompatible:`nxp,imx-ccm-rev2`, :dtcompatible:`nxp,lpc-ctimer`,
    :dtcompatible:`nxp,lpc-uid`, :dtcompatible:`nxp,mcux-usbd`,
    :dtcompatible:`nxp,sctimer-pwm`, :dtcompatible:`ovti,ov2640`,
    :dtcompatible:`renesas,rcar-can`, :dtcompatible:`renesas,rcar-i2c`,
    :dtcompatible:`reserved-memory`, :dtcompatible:`riscv,sifive-e24`,
    :dtcompatible:`sensirion,sgp40`, :dtcompatible:`sensirion,sht4x`,
    :dtcompatible:`sensirion,shtcx`, :dtcompatible:`silabs,si7055`,
    :dtcompatible:`silabs,si7210`, :dtcompatible:`snps,creg-gpio`,
    :dtcompatible:`st,i3g4250d`, :dtcompatible:`st,stm32-aes`,
    :dtcompatible:`st,stm32-dma`, :dtcompatible:`st,stm32-dma-v2bis`,
    :dtcompatible:`st,stm32-hsem-mailbox`, :dtcompatible:`st,stm32-nv-flash`,
    :dtcompatible:`st,stm32-spi-subghz`,
    :dtcompatible:`st,stm32u5-flash-controller`,
    :dtcompatible:`st,stm32u5-msi-clock`, :dtcompatible:`st,stm32u5-pll-clock`,
    :dtcompatible:`st,stm32u5-rcc`, :dtcompatible:`st,stm32wl-hse-clock`,
    :dtcompatible:`st,stm32wl-subghz-radio`, :dtcompatible:`st,stmpe1600`,
    :dtcompatible:`syscon`, :dtcompatible:`telink,b91`,
    :dtcompatible:`telink,b91-flash-controller`,
    :dtcompatible:`telink,b91-gpio`, :dtcompatible:`telink,b91-i2c`,
    :dtcompatible:`telink,b91-pinmux`, :dtcompatible:`telink,b91-power`,
    :dtcompatible:`telink,b91-pwm`, :dtcompatible:`telink,b91-spi`,
    :dtcompatible:`telink,b91-trng`, :dtcompatible:`telink,b91-uart`,
    :dtcompatible:`telink,b91-zb`, :dtcompatible:`ti,hdc2010`,
    :dtcompatible:`ti,hdc2021`, :dtcompatible:`ti,hdc2022`,
    :dtcompatible:`ti,hdc2080`, :dtcompatible:`ti,hdc20xx`,
    :dtcompatible:`ti,ina219`, :dtcompatible:`ti,ina23x`,
    :dtcompatible:`ti,tca9538`, :dtcompatible:`ti,tca9546a`,
    :dtcompatible:`ti,tlc59108`,
    :dtcompatible:`xlnx,gem`, :dtcompatible:`zephyr,bbram-emul`,
    :dtcompatible:`zephyr,cdc-acm-uart`, :dtcompatible:`zephyr,gsm-ppp`,
    :dtcompatible:`zephyr,native-posix-udc`

* West (extensions)

    * openocd runner: Zephyr thread awareness is now available in GDB by default
      for application builds with :kconfig:option:`CONFIG_DEBUG_THREAD_INFO` set to ``y``
      in :ref:`kconfig`. This applies to ``west debug``, ``west debugserver``,
      and ``west attach``. OpenOCD version later than 0.11.0 must be installed
      on the host system.


Libraries / Subsystems
**********************

* Disk


* Management


* CMSIS subsystem


* Power management

  * The APIs to set/clear/check if devices are busy from a power management
    perspective have been moved to the PM subsystem. Their naming and signature
    has also been adjusted to follow common conventions. Below you can find the
    equivalence list.

    * ``device_busy_set`` -> ``pm_device_busy_set``
    * ``device_busy_clear`` -> ``pm_device_busy_clear``
    * ``device_busy_check`` -> ``pm_device_is_busy``
    * ``device_any_busy_check`` -> ``pm_device_is_any_busy``

  * The device power management callback (``pm_device_control_callback_t``) has
    been largely simplified to work based on *actions*, resulting in simpler and
    more natural implementations. This principle is also used by other OSes like
    the Linux Kernel. As a result, the callback argument list has been reduced
    to the device instance and an action (e.g. ``PM_DEVICE_ACTION_RESUME``).
    Other improvements include specification of error codes, removal of some
    unused/unclear states, or guarantees such as avoid calling a device for
    suspend/resume if it is already at the right state. All these changes
    together have allowed simplifying multiple device power management callback
    implementations.

  * Introduced a new API to allow devices capable of wake up the system
    register themselves was wake up sources. This permits applications to
    select the most appropriate way to wake up the system when it is
    suspended. Devices marked as wake up source are not suspended by the kernel
    when the system is idle. It is possible to declare a device wake up capable
    direct in devicetree like this example::

        &gpio0 {
                compatible = "zephyr,gpio-emul";
                gpio-controller;
                wakeup-source;
        };

    * Removed  ``PM_DEVICE_STATE_FORCE_SUSPEND`` device power state.because it
      is an action and not a state.

    * Removed ``PM_DEVICE_STATE_RESUMING`` and ``PM_DEVICE_STATE_SUSPENDING``.
      They were transitional states and only used in device runtime. Now the
      subsystem is using device flag to keep track of a transition.

    * Implement constraint API as weak symbols so applications or platform
      can override them. Platforms can have their own way to
      set/release constraints in their drivers that are not part of
      Zephyr code base.


* Logging

* MODBUS

  * Changed server handler to copy Transaction and Protocol Identifiers
    to response header.

* Random

  * xoroshiro128+ PRNG deprecated in favor of xoshiro128++

* Shell


* Storage


* Task Watchdog


* Tracing


* Debug

* OS


HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.


Trusted Firmware-m
******************

* Renamed psa_level_1 sample to psa_crypto. Extended the use of the PSA Cryptography
  1.0 API in the sample code to demonstrate additional crypto functionality.
* Added a new sample to showcase the PSA Protecter Storage service.

Documentation
*************

* Kconfig options need to be referenced using the ``:kconfig:option:`` Sphinx role.
  Previous to this change, ``:option:`` was used for this purpose.
* Doxygen alias ``@config{}`` has been deprecated in favor of ``@kconfig{}``.

Tests and Samples
*****************


Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.6.0 tagged
release:

* :github:`39443` - Be more inclusive
* :github:`39419` - STM32WL55 not found st/wl/stm32wl55jcix-pinctrl.dtsi
* :github:`39413` - warnings when using newlibc and threads
* :github:`39409` - runners: canopen: program download fails with slow flash access and/or congested CAN nets
* :github:`39389` - http_get, big_http_download samples fails to build
* :github:`39388` - GSM Modem sample fails to build
* :github:`39378` - Garbage IQ Data Reports are generated if some check in hci_df_prepare_connectionless_iq_report fails
* :github:`39294` - noticing stm32 clock domain naming changes
* :github:`39291` - Bluetooth: Periodic advertising
* :github:`39284` - mdns + dns_sd: fix regression that breaks ptr queries
* :github:`39281` - Undefined references to k_thread_abort related tracing routines
* :github:`39270` - example-application CI build fails
* :github:`39263` - Bluetooth: controller: DF: wrong handling of max_cte_count
* :github:`39260` - [backport v2.7-branch] backport of #38292 failed
* :github:`39240` - ARC Kconfig allows so select IRQ configuration which isn't supported in SW
* :github:`39206` - lwm2m: send_attempts field does not seem to be used?
* :github:`39205` - drivers: wifi: esp_at: cannot connect to open (unsecure) WiFi networks
* :github:`39195` - USB: netusb: example echo_server not working as expected
* :github:`39190` - tests/subsys/logging/log_core_additional/logging.add.log2 fails
* :github:`39188` - tests/bluetooth/mesh/bluetooth.mesh.ext_adv fails
* :github:`39185` - tests/subsys/logging/log_core_additional/logging.add.user fails on several platforms
* :github:`39180` - samples/subsys/mgmt/osdp/peripheral_device & samples/subsys/mgmt/osdp/control_panel fail to build
* :github:`39170` - Can not run correctly on NXP MIMXRT1061 CVL5A.
* :github:`39135` - samples/compression/lz4 build failed (lz4.h: No such file or directory)
* :github:`39132` - subsys/net/ip/tcp2: Missing feature to decrease Receive Window size sent in the ACK messge
* :github:`39123` - ztest:  Broken on NRF52840 Platform
* :github:`39115` - sensor: fdc2x1x: warnings and compilation errors when PM_DEVICE is used
* :github:`39086` - CMake warning during build - depracated roule CMP0079
* :github:`39085` - Ordering of device_map() breaks PCIe config space mapping on ARM64
* :github:`39075` - IPv6 address not set on loopback interface
* :github:`39051` - Zephyr was unable to find the toolchain. Is the environment misconfigured?
* :github:`39036` - Multicast packet forwarding not working for the coap_server sample and Openthread
* :github:`39022` - [backport v2.7-branch] backport of #38834 failed
* :github:`39011` - Bluetooth: Mesh: Model extensions walk stops before last model
* :github:`39009` - Nordic PWM causing lock up due to infinte loop
* :github:`39008` - tests: logging.add.user: build failure on STM32H7 targets
* :github:`38999` - [backport v2.7-branch] backport of #38407 failed
* :github:`38996` - There is no way to leave a ipv6 multicast group
* :github:`38994` - ARP: Replies are sent to multicast MAC address rather than senders MAC address.
* :github:`38970` - LWM2M Client Sample with DTLS enabled fail to connect
* :github:`38966` - Please add STM32F412VX
* :github:`38961` - tests: kernel: sched: schedule_api: instable on disco_l475_iot1
* :github:`38959` - ITE RISCV I2C driver returning positive values for error instead of negative values
* :github:`38943` - west: update hal_espressif failure
* :github:`38938` - Bluetooth tester application should be able return L2CAP ECFC credits on demand
* :github:`38930` - Low Power mode not functional on nucleo_l073rz
* :github:`38924` - twister: cmake: Misleading error in Twister when sdk-zephyr 0.13.1 not used
* :github:`38904` - [backport v2.7-branch] backport of #38860 failed
* :github:`38902` - i2c_nrfx_twim: Error 0x0BAE0002 if sensor is set in trigger mode and reset with nrf device
* :github:`38899` - There is no valid date setting function in the RTC driver of the LL Library of STM32
* :github:`38893` - g0b1re + spi_flash_at45 + flash_shell: First write always fails with ``CONFIG_PM_DEVICE``
* :github:`38886` - devicetree/memory.h probably should not exist as-is
* :github:`38877` - Running the zephyr elf natively on an arm a53 machine (ThunderX2) with KVM emulation
* :github:`38870` - stm32f1: Button callback not fired
* :github:`38853` - Bluetooth: host: bt_unpair failed because function [bt_conn_set_state] wont work as expected
* :github:`38849` - drivers: i2c: nrf: i2c error with burst write
* :github:`38829` - net_buf issue leads to unwanted elem free
* :github:`38826` - tests/lib/cmsis_dsp: malloc failed on 128K SRAM targets
* :github:`38818` - driver display display_st7789v.c build error
* :github:`38815` - kernel/mem_domain: Remove dead case in check_add_partition()
* :github:`38807` - stm32: Missing header in power.c files
* :github:`38804` - tests\kernel\threads\thread_stack test fail with ARC
* :github:`38799` - BLE central_ht only receives 7 notifications
* :github:`38796` - Failure building the zephyr\tests\subsys\cpp\libcxx project
* :github:`38791` - Example code_relocation not compiling.
* :github:`38790` - SD FatFS Sample Build Failure
* :github:`38784` - stm32: pm: Debug mode not functional on G0
* :github:`38782` - CONFIG_BT_CTLR_DATA_LENGTH_MAX=250 causes pairing compatibility issues with many devices
* :github:`38769` - mqtt: the size of a mqtt payload is limited
* :github:`38765` - samples: create an OLED example
* :github:`38764` - CBPRINTF_FP_SUPPORT does not work after NEWLIB_LIBC enabled
* :github:`38761` - Does zephyr_library_property defines -DTRUE in command-line?
* :github:`38756` - Twister: missing testcases with error in report
* :github:`38745` - Bluetooth when configured for extended advertising does not limit advertisement packet size if a non-extended avertisement is used
* :github:`38737` - drivers: syscon: missing implementation
* :github:`38735` - nucleo_wb55rg: Flash space left to M0 binary is not sufficient anymore
* :github:`38731` - test-ci: ptp_clock_test :  test failure on frdm_k64f platform
* :github:`38727` - [RFC] Add hal_gigadevice to support GigaDevice SoC Vendor
* :github:`38716` - modem: HL7800: does not work with IPv6
* :github:`38702` - Coap server not properly removing observers
* :github:`38701` - Observable resource of coap server seems to not support a restart of an observer
* :github:`38700` - Observable resource of coap server seems to not support 2 observers simultaneously
* :github:`38698` - stm32f4_disco: Socket CAN sample not working
* :github:`38697` - The coap_server sample is missing the actual send in the retransmit routine
* :github:`38694` - Disabling NET_CONFIG_AUTO_INIT does not require calling net_config_init() manually in application as mentioned in Zephyr Network Configuration Library documentation
* :github:`38692` - samples/tfm_integration: Compilation fails ("unexpected keyword argument 'rom_fixed'")
* :github:`38691` - MPU fault with mcumgr bluetooth FOTA started whilst existing FOTA is in progress
* :github:`38690` - Wrong initialisation priority on different display drivers (eg. ST7735r) cause exception when using lvgl.
* :github:`38688` - bt_gatt_unsubscribe does not remove subscription from internal list/returning BT_GATT_ITER_STOP causes bt_gatt_subscribe to return -ENOMEM / -12
* :github:`38675` - DTS binding create devicetree_unfixed.h build error at v2.7.0
* :github:`38673` - DNS-SD library does not support ``_services._dns-sd._udp.local`` meta-query for service enumeration
* :github:`38668` -  ESP32‘s I2S
* :github:`38667` - ST LSM6DSO polling mode does not work on nRF52dk_nrf52832
* :github:`38655` - Failing Tests for Regulator API
* :github:`38653` - drivers: modem: gsm_ppp: Add support for Quectel modems
* :github:`38646` - SIMD Rounding bug while running Assembly addps instruction on Zephyr
* :github:`38641` - Arm v8-M '_ns' renaming was applied inconsistently
* :github:`38635` - USDHC driver broken on RT10XX after 387e6a676f86c00d1f9ef018e4b2480e0bcad3c8 commit
* :github:`38622` - subsys/usb: CONFIG_USB_DEVICE_STACK resulted in 10kb increase in firmware size
* :github:`38621` - Drivers: spi: stm32: Transceive lock forever
* :github:`38620` - STM32 uart driver prevent system to go to deep sleep
* :github:`38617` - HL7800 PSM not working as intended
* :github:`38613` - BLE connection parameters updated with inconsistent values
* :github:`38612` - Fault with assertions enabled prevents detailed output because of ISR() assertion check in shell function
* :github:`38602` - modem gsm
* :github:`38601` - nucleo_f103rb: samples/posix/eventfd/ failed since "retargetable locking" addition
* :github:`38593` - using RTT console to print along with newlib C library in Zephyr
* :github:`38591` - nucleo_f091rc: Linking issue since "align __data_ram/rom_start/end linker" (65a2de84a9d5c535167951bf1cf610c4f7967ea5)
* :github:`38586` - olimexino_stm32: "no DEVICE_HANDLE_ENDS inserted" builld issue (samples/subsys/usb/audio/headphones_microphone)
* :github:`38581` - tests-ci : kernel: scheduler: multiq test failed
* :github:`38582` - tests-ci : kernel: scheduler:  test failed
* :github:`38578` - STM32L0X ADC hangs
* :github:`38572` - Builds with macOS SDK are failing
* :github:`38571` - bug: drivers: ethernet: build as static library breaks frdm_k64f gptp sample application
* :github:`38563` - ISO broadcast cannot send with callback if CONFIG_BT_CONN=n
* :github:`38560` - log v2 with 64-bit integers and threads causes invalid 64-bit value output
* :github:`38559` - Shell log backend may hang on qemu_x86_64
* :github:`38558` - CMake warning: CMP0079
* :github:`38554` - tests-ci : kernel: scheduler:  test failed
* :github:`38552` - stm32: g0b1: garbage output in log and suspected hard fault when configuring modem
* :github:`38536` - samples: tests: display: Sample for display.ft800 causes end in timeout
* :github:`38535` - drivers: modem: bg9x: Kconfig values compiled into ``autoconf.h`` even if it isn't being used
* :github:`38534` - lwm2m: add api to inspect observation state of resource/object
* :github:`38532` - samples: audio: tests: Twister fails on samples/drivers/audio/dmic
* :github:`38527` - lwm2m: re-register instead of removing observer on COAP reset answer to notification
* :github:`38520` - Bluetooth:Host:Scan: "bt_le_per_adv_list_add" function doesn't work
* :github:`38519` - stm32: g0b1re: Log/Shell subsys with serial uart buggy after #38432
* :github:`38516` - subsys: net: ip: packet_socket: always returning of NET_CONTINUE caused access to unreferred pkt and causing a crash/segmentation fault
* :github:`38514` - mqtt azure sample failing with net_tcp "is waiting on connect semaphore"
* :github:`38512` - stm32f7: CAN: STM32F645VE CAN signal seems upside down.
* :github:`38500` - tests/kernel/device/kernel.device.pm fails to build on TI platforms
* :github:`38498` - net: ipv6: nbr_lock not initialized with CONFIG_NET_IPV6_ND=n
* :github:`38480` - Improve samples documentation
* :github:`38479` - "west flash" command exiting with error
* :github:`38477` - json: JSON Library Orphaned, Request to Become a Maintainer
* :github:`38474` - command exited with status 63: nrfjprog --ids
* :github:`38463` - check_compliance gives very many Kconfig warnings
* :github:`38452` - Some STM32 series require CONFIG_PM_DEVICE if CONFIG_PM=y
* :github:`38442` - test-ci: can: twr_ke18f: all can driver test fails with BUS Fault
* :github:`38438` - test-ci: test_flash_map:twr_ke18f: test failure
* :github:`38437` - stm32: g0b1re: Serial UART timing issue after MCU entered deep sleep
* :github:`38433` - gpio_pin_set not working on STM32 with CONFIG_PM_DEVICE_RUNTIME
* :github:`38428` - http_client response callback always reports final_data == HTTP_DATA_FINAL
* :github:`38427` - mimxrt1050_evk and mimxrt1020_evk boards fail to boot some sample applications
* :github:`38421` - HardFault regression detected on Cortex-M0+ following Cortex-R introduction
* :github:`38418` - twister: Remove toolchain-depandat filter for native_posix
* :github:`38417` - Add support for WeAct-F401CC board
* :github:`38414` - Build of http client fails if CONFIG_POSIX_API=y
* :github:`38405` - samples/philosophers/sample.kernel.philosopher.stacks fails on xtensa
* :github:`38403` - Cleanup ``No SOURCES given to Zephyr library`` warnings
* :github:`38402` - module: MCUboot module missing fixes available upstream
* :github:`38401` - Builds fail due to a proxy error by launchpadlibrarian
* :github:`38400` - mec15xxevb_assy6853: arm_ramfunc and arm_sw_vector_relay tests timeout after the build
* :github:`38398` - DT_N_INST error for TMP116 sample
* :github:`38396` - RISC-V privilege SoC initialisation code skips the __reset vector
* :github:`38382` - stm32 uart finishes Tx before going to PM
* :github:`38365` - drivers: gsm_ppp: gsm_ppp_stop fails to lock tx_sem after some time
* :github:`38362` - soc: ti cc13x2-cc26x2: PM standby + radio interaction regression
* :github:`38354` - stm32: stm32f10x JTAG realated gpio repmap didn't works
* :github:`38351` - Custom radio protocol
* :github:`38349` - XCC compilation fails on Intel cAVS platforms
* :github:`38348` - Bluetooth: Switch to inclusive terminology from the 5.3 specification
* :github:`38340` - Bluetooth:DirectionFinding: Disabling the MPU causes some compilation errors
* :github:`38332` - stm32g0: power hooks should be define as weak
* :github:`38323` - Can not generate code coverage report by running samples/subsys/tracing
* :github:`38316` - Synchronize multiple DF TX devices in the DF Connectionless RX Example "Periodic Advertising list"
* :github:`38309` - ARC context switch to interrupted thread busted with CONFIG_ARC_FIRQ=y and CONFIG_NUM_IRQ_PRIO_LEVELS=1
* :github:`38303` - The current BabbleSim tests build system based on bash scripts hides warnings
* :github:`38290` - net_buf_add_mem() hard-faults when adding buffer from external SDRAM
* :github:`38279` - Bluetooth: Controller: assert LL_ASSERT(!radio_is_ready()) in lll_conn.c
* :github:`38277` - soc: stm32h7: Fails to boot with LDO power supply, if soc has SMPS support.
* :github:`38276` - LwM2M: RD Client: Wrong state if registration fails
* :github:`38273` - Support UART4 on STM32F303Xe
* :github:`38272` - "west flash" stopped working
* :github:`38271` - Expose emulator_get_binding function
* :github:`38264` - Modbus over RS485 on samd21g18a re-gpios turning on 1 byte too early
* :github:`38259` - subsys/shell: ``[JJ`` escape codes in logs after disabling colors
* :github:`38258` - newlib: first malloc call may fail on Xtensa depending on image size
* :github:`38246` - samples: drivers: flash_shell: fails on arduino_due due to compilation issue
* :github:`38245` - board: bl654_usb project: samples/basic/blinky does not blink LED
* :github:`38240` - Connected ISO does not disconnect gracefully
* :github:`38237` - [backport v2.6-branch] backport of #37479 failed
* :github:`38235` - Please add stm32h723Xe.dtsi to dts/arm/st/h7/
* :github:`38234` - Newlib retargetable lock init fails on qemu_xtensa
* :github:`38233` - Build newlib function read() and write() failed when enable userspace
* :github:`38219` - kernel: Z_MEM_SLAB_INITIALIZER MACRO not compatible with C++
* :github:`38216` - nxp_adsp_imx8 fails to build a number of tests
* :github:`38214` - xtensa builds fail in CI due to running out of ram to link
* :github:`38207` - Use of unaligned noinit data hangs qemu_arc_hs
* :github:`38202` - mbedtls and littlefs on a STM32L4
* :github:`38197` - Invalid NULL check for ``iso`` in bt_iso_connected
* :github:`38196` - net nbr command might crash
* :github:`38191` - Unable to connect multiple MQTT clients
* :github:`38186` - i.MX RT10xx boards fail to initialize when Ethernet is enabled
* :github:`38181` - tests/drivers/uart/uart_basic_api/drivers.uart.cdc_acm fails to build
* :github:`38177` - LORA Module crashes SHT3XD sensor.
* :github:`38173` - STM32WB: Low power modes entry blocked by C2 when CONFIG_BLE=n
* :github:`38172` - modem_context_sprint_ip_addr returns pointer to stack array
* :github:`38170` - Shell argument in second position containing a question mark is ignored
* :github:`38168` - aarch32: flags value collision between base IRQ layer and GIC interrupt controller driver
* :github:`38162` - Upgrade to 2.6 GPIO device_get_binding("GPIO_0") now returns null
* :github:`38154` - Error building example i2c_fujitsu_fram
* :github:`38153` - Zephyr Native POSIX select() implementation too frequent wakeup on pure timeout based use
* :github:`38145` - [backport v2.6-branch] backport of #37787 failed
* :github:`38144` - [backport v2.6-branch] backport of #37787 failed
* :github:`38141` - Wrong output from printk() with CONFIG_CBPRINTF_NANO=y
* :github:`38138` - [Coverity CID: 239554] Out-of-bounds read in /zephyr/include/generated/syscalls/log_msg2.h (Generated Code)
* :github:`38137` - [Coverity CID: 239555] Unchecked return value in subsys/mgmt/hawkbit/hawkbit.c
* :github:`38136` - [Coverity CID: 239557] Out-of-bounds read in /zephyr/include/generated/syscalls/kernel.h (Generated Code)
* :github:`38135` - [Coverity CID: 239560] Out-of-bounds access in subsys/modbus/modbus_core.c
* :github:`38134` - [Coverity CID: 239563] Logically dead code in subsys/bluetooth/host/id.c
* :github:`38133` - [Coverity CID: 239564] Side effect in assertion in subsys/bluetooth/controller/ll_sw/nordic/lll/lll.c
* :github:`38132` - [Coverity CID: 239565] Unchecked return value in drivers/sensor/adxl372/adxl372_trigger.c
* :github:`38131` - [Coverity CID: 239568] Out-of-bounds access in subsys/modbus/modbus_core.c
* :github:`38130` - [Coverity CID: 239569] Out-of-bounds access in subsys/bluetooth/host/id.c
* :github:`38129` - [Coverity CID: 239572] Out-of-bounds read in /zephyr/include/generated/syscalls/kernel.h (Generated Code)
* :github:`38127` - [Coverity CID: 239579] Logically dead code in drivers/flash/nrf_qspi_nor.c
* :github:`38126` - [Coverity CID: 239581] Out-of-bounds access in subsys/modbus/modbus_core.c
* :github:`38125` - [Coverity CID: 239582] Unchecked return value in drivers/display/ssd1306.c
* :github:`38124` - [Coverity CID: 239583] Side effect in assertion in subsys/bluetooth/controller/ll_sw/nordic/lll/lll.c
* :github:`38123` - [Coverity CID: 239584] Improper use of negative value in subsys/logging/log_msg2.c
* :github:`38122` - [Coverity CID: 239585] Side effect in assertion in subsys/bluetooth/controller/ll_sw/nordic/lll/lll.c
* :github:`38121` - [Coverity CID: 239586] Side effect in assertion in subsys/bluetooth/controller/ll_sw/nordic/lll/lll.c
* :github:`38120` - [Coverity CID: 239588] Unchecked return value in subsys/bluetooth/host/id.c
* :github:`38119` - [Coverity CID: 239592] Dereference before null check in subsys/ipc/rpmsg_multi_instance/rpmsg_multi_instance.c
* :github:`38118` - [Coverity CID: 239597] Explicit null dereferenced in tests/net/context/src/main.c
* :github:`38117` - [Coverity CID: 239598] Unchecked return value in drivers/sensor/adxl362/adxl362_trigger.c
* :github:`38116` - [Coverity CID: 239601] Untrusted loop bound in subsys/bluetooth/host/sdp.c
* :github:`38115` - [Coverity CID: 239605] Logically dead code in drivers/flash/nrf_qspi_nor.c
* :github:`38114` - [Coverity CID: 239607] Missing break in switch in subsys/usb/class/dfu/usb_dfu.c
* :github:`38113` - [Coverity CID: 239609] Out-of-bounds access in subsys/random/rand32_ctr_drbg.c
* :github:`38112` - [Coverity CID: 239612] Out-of-bounds read in /zephyr/include/generated/syscalls/log_ctrl.h (Generated Code)
* :github:`38111` - [Coverity CID: 239615] Out-of-bounds access in subsys/net/lib/sockets/sockets_tls.c
* :github:`38110` - [Coverity CID: 239619] Out-of-bounds access in subsys/net/lib/sockets/sockets_tls.c
* :github:`38109` - [Coverity CID: 239623] Out-of-bounds access in subsys/net/lib/sockets/sockets_tls.c
* :github:`38108` - nxp: usb driver build failure due to d92d1f162af3ba24963f1026fc0a304f1a44d1f3
* :github:`38104` - kheap buffer own section attribute causing memory overflow in ESP32
* :github:`38101` - bt_le_adv_update_data() assertion fail
* :github:`38093` - preempt_cnt not reset in each test case in tests/lib/ringbuffer/libraries.data_structures
* :github:`38090` - LPS22HH: int32_t overflow in pressure calculations
* :github:`38082` - Hawkbit (http request) and MQTT can't seem to work together
* :github:`38078` - RT6XX I2S test fails after d92d1f162af3ba24963f1026fc0a304f1a44d1f3
* :github:`38069` - stm32h747i_disco M4 not working following merge of 9fa5437447712eece9c88e728ac05ac10fb01c4a
* :github:`38065` - Bluetooth: Direction Finding: Compiler warning when included in other header files
* :github:`38059` - automount configuration in nrf52840dk_nrf52840.overlay causes error: mount point already exists!! in subsys/fs/littlefs sample
* :github:`38054` - Bluetooth: host: Local Host terminated but send host number of completed Packed
* :github:`38047` - twister: The --board-root parameter doesn't appear to work
* :github:`38046` - twister: The --device-serial only works at 115200 baud
* :github:`38044` - tests: newlib: Scenarios from tests/lib/newlib/thread_safety fail on nrf9160dk_nrf9160_ns
* :github:`38031` - STM32WB - Problem with data reception on LPUART when PM and LPTIM are enabled
* :github:`38026` - boards: bl654_usb: does not support samples/bluetooth/hci_uart
* :github:`38022` - thread: k_float_enable() API can't build on x86_64 platforms, fix that API and macro documentation
* :github:`38019` - nsim_sem_mpu_stack_guard board can't run
* :github:`38017` - [Coverity CID: 237063] Untrusted value as argument in tests/net/lib/coap/src/main.c
* :github:`38016` - [Coverity CID: 238375] Uninitialized pointer read in subsys/bluetooth/mesh/shell.c
* :github:`38015` - [Coverity CID: 237072] Uninitialized pointer read in subsys/bluetooth/controller/ll_sw/ull_adv_aux.c
* :github:`38014` - [Coverity CID: 237071] Unexpected control flow in subsys/bluetooth/host/keys.c
* :github:`38013` - [Coverity CID: 237070] Unchecked return value in subsys/bluetooth/shell/gatt.c
* :github:`38012` - [Coverity CID: 236654] Unchecked return value in subsys/bluetooth/host/gatt.c
* :github:`38011` - [Coverity CID: 236653] Unchecked return value in drivers/sensor/bmi160/bmi160_trigger.c
* :github:`38010` - [Coverity CID: 236652] Unchecked return value in drivers/sensor/fxas21002/fxas21002_trigger.c
* :github:`38009` - [Coverity CID: 236651] Unchecked return value in drivers/sensor/bmg160/bmg160_trigger.c
* :github:`38008` - [Coverity CID: 236650] Unchecked return value in drivers/sensor/fxos8700/fxos8700_trigger.c
* :github:`38007` - [Coverity CID: 236649] Unchecked return value in drivers/sensor/adt7420/adt7420_trigger.c
* :github:`38006` - [Coverity CID: 236648] Unchecked return value in drivers/sensor/sx9500/sx9500_trigger.c
* :github:`38005` - [Coverity CID: 236647] Unchecked return value in drivers/sensor/bmp388/bmp388_trigger.c
* :github:`38004` - [Coverity CID: 238360] Result is not floating-point in drivers/sensor/sgp40/sgp40.c
* :github:`38003` - [Coverity CID: 238343] Result is not floating-point in drivers/sensor/sgp40/sgp40.c
* :github:`38002` - [Coverity CID: 237060] Out-of-bounds access in subsys/bluetooth/host/gatt.c
* :github:`38001` - [Coverity CID: 238371] Negative array index read in tests/lib/cbprintf_package/src/test.inc
* :github:`38000` - [Coverity CID: 238347] Negative array index read in tests/lib/cbprintf_package/src/test.inc
* :github:`37999` - [Coverity CID: 238383] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37998` - [Coverity CID: 238381] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37997` - [Coverity CID: 238380] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37996` - [Coverity CID: 238379] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37995` - [Coverity CID: 238378] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37994` - [Coverity CID: 238377] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37993` - [Coverity CID: 238376] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37992` - [Coverity CID: 238374] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37991` - [Coverity CID: 238373] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37990` - [Coverity CID: 238372] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37989` - [Coverity CID: 238370] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37988` - [Coverity CID: 238369] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37987` - [Coverity CID: 238368] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37986` - [Coverity CID: 238367] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37985` - [Coverity CID: 238366] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37984` - [Coverity CID: 238364] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37983` - [Coverity CID: 238363] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37982` - [Coverity CID: 238362] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37981` - [Coverity CID: 238361] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37980` - [Coverity CID: 238359] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37979` - [Coverity CID: 238358] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37978` - [Coverity CID: 238357] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37977` - [Coverity CID: 238356] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37976` - [Coverity CID: 238355] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37975` - [Coverity CID: 238354] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37974` - [Coverity CID: 238353] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37973` - [Coverity CID: 238352] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37972` - [Coverity CID: 238351] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37971` - [Coverity CID: 238350] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37970` - [Coverity CID: 238349] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37969` - [Coverity CID: 238348] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37968` - [Coverity CID: 238346] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37967` - [Coverity CID: 238345] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37966` - [Coverity CID: 238344] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37965` - [Coverity CID: 238342] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37964` - [Coverity CID: 238341] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37963` - [Coverity CID: 238340] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37962` - [Coverity CID: 238339] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37961` - [Coverity CID: 238337] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37960` - [Coverity CID: 238336] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37959` - [Coverity CID: 238335] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37958` - [Coverity CID: 238334] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37957` - [Coverity CID: 238333] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37956` - [Coverity CID: 238332] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37955` - [Coverity CID: 238331] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37954` - [Coverity CID: 238330] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37953` - [Coverity CID: 238328] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37952` - [Coverity CID: 238327] Logically dead code in tests/bluetooth/tester/src/mesh.c
* :github:`37951` - [Coverity CID: 238365] Logical vs. bitwise operator in drivers/i2s/i2s_nrfx.c
* :github:`37950` - [Coverity CID: 237067] Division or modulo by zero in tests/benchmarks/latency_measure/src/heap_malloc_free.c
* :github:`37949` - [Coverity CID: 238382] Dereference before null check in subsys/bluetooth/mesh/cfg_cli.c
* :github:`37948` - [Coverity CID: 238338] Dereference before null check in subsys/bluetooth/mesh/cfg_cli.c
* :github:`37947` - [Coverity CID: 237069] Dereference before null check in subsys/bluetooth/host/att.c
* :github:`37946` - [Coverity CID: 237066] Calling risky function in tests/lib/c_lib/src/main.c
* :github:`37945` - [Coverity CID: 237064] Calling risky function in tests/lib/c_lib/src/main.c
* :github:`37944` - [Coverity CID: 237062] Calling risky function in tests/lib/c_lib/src/main.c
* :github:`37940` - Unconsistent UART ASYNC API
* :github:`37927` - tests-ci: net-lib: test/net/lib : build missing drivers__net and application has no console output
* :github:`37916` - [Coverity CID :219656] Uninitialized scalar variable in file /tests/kernel/threads/thread_stack/src/main.c
* :github:`37915` - led_pwm driver not generating correct linker symbol
* :github:`37896` - samples: bluetooth: mesh: build failed for native posix
* :github:`37876` - Execution of twister in makefile environment
* :github:`37865` - nRF Battery measurement issue
* :github:`37861` - tests/lib/ringbuffer failed on ARC boards
* :github:`37856` - tests: arm: uninitialized FPSCR
* :github:`37852` - RISC-V machine timer time-keeping question
* :github:`37850` - Provide macros for switching off Zephyr kernel version
* :github:`37842` - TCP2 statemachine gets stuck in TCP_FIN_WAIT_2 state
* :github:`37839` - SX1272 LoRa driver is broken and fails to build
* :github:`37838` - cmake 3.20 not supported (yet) by recent Ubuntu
* :github:`37830` - intel_adsp_cavs15: run queue testcases failed on ADSP
* :github:`37827` - stm32h747i_disco M4 not working, if use large size(>1KB) global array
* :github:`37821` - pm: ``pm_device_request`` incorrectly returns errors
* :github:`37797` - Merge vendor-prefixes.txt from all modules with build.settings.dts_root in zephyr/module.yml
* :github:`37790` - Bluetooth: host: Confusion about periodic advertising interval
* :github:`37786` - Example for tca9546a multiplexor driver
* :github:`37784` - MPU6050 accel and gyro values swapped
* :github:`37781` - nucleo_l496zg lpuart1 driver not working
* :github:`37779` - adc sam0 interrupt mapping, RESRDY maps to second interrupt in samd5x.dtsi
* :github:`37772` - samples: subsys: usb: mass: Use &flash0 storage_partition for USB mass storage
* :github:`37768` - tests/lib/ringbuffer/libraries.data_structures fails to build on number of platforms due to CONFIG_SYS_CLOCK_TICKS_PER_SEC=100000
* :github:`37765` - cmake: multiple ``No SOURCES given to Zephyr library:`` warnings
* :github:`37746` - qemu_x86_64 fails samples/hello_world/sample.basic.helloworld.uefi in CI
* :github:`37735` - Unsigned types are incorrectly serialized when TLV format is used in LWM2M response
* :github:`37734` - xtensa xcc build spi_nor.c fail
* :github:`37720` - net: dtls: received close_notify alerts are not properly handled by DTLS client sockets
* :github:`37718` - Incompatible (u)intptr_t type and PRIxPTR definitions
* :github:`37709` - x86 PCIe ECAM does not work as expected
* :github:`37701` - stm32:  conflicts with uart serial DMA
* :github:`37696` - Modbus TCP: wrong transaction id in response
* :github:`37694` - Update CMSIS-DSP version to 1.9.0 (CMSIS 5.8.0)
* :github:`37693` - Update CMSIS-Core version to 5.5.0 (CMSIS 5.8.0)
* :github:`37691` - samples/subsys/canbus/isotp/sample.subsys.canbus.isotp fails to build on mimxrt1170_evk_cm7
* :github:`37687` - Support MVE on ARMv8.1-M
* :github:`37684` - Add State Machine Framework to Zephyr
* :github:`37676` - tests/kernel/device/kernel.device.pm (and tests/subsys/pm/power_mgmt/subsys.pm.device_pm) fails to build on mec172xevb_assy6906 & mec1501modular_assy6885
* :github:`37675` - tests/kernel/device/kernel.device.pm fails on bt510/bt6x0
* :github:`37672` - Board qemu_x86 is no longer working with shell
* :github:`37665` - File system: wrong type for ssize_t in fs.h for CONFIG_ARCH_POSIX
* :github:`37660` - Changing zephyr,console requires a clean build
* :github:`37658` - samples: boards/stm32/backup_sram : needs backup sram enabled in DT to properly display memory region
* :github:`37652` - bluetooth: tests/bluetooth/bsim_bt/bsim_test_advx reported success but still reported failed.
* :github:`37637` - Infinite configuring loop for samples\drivers\led_ws2812 sample
* :github:`37619` - RT6xx TRNG reports error on first request after reset
* :github:`37611` - Bluetooth: host: Implement L2CAP ecred reconfiguration request as initiator
* :github:`37610` - subsys/mgmt/hawkbit: Unable to parse json if the payload is split into 2 packets
* :github:`37600` - Invalidate TLB after ptables swap
* :github:`37597` - samples: bluetooth: scan_adv
* :github:`37586` - get_maintainer.py is broken
* :github:`37581` - Bluetooth: controller: radio: Change CTE configuration method
* :github:`37579` - PWM: Issue compiling project when CONFIG_PWM and CONFIG_PWM_SAM is used with SAME70
* :github:`37571` - Bluetooth: Extended advertising assertion
* :github:`37556` - Schedule or timeline of LE audio in zephyr
* :github:`37547` - Bluetooth: Direction Finding: Channel index of received CTE packet is incorrect
* :github:`37544` - Change partition name using .overlay
* :github:`37543` - Using STM32Cube HAL function results in linker error
* :github:`37536` - _pm_devices() skips the very first device in the list and suspend() is not called.
* :github:`37530` - arc smp build failed with mwdt toolchain.
* :github:`37527` - Replace mqtt-azure example with azure-sdk-for-c
* :github:`37526` - ehl_crb:  edac tests are failing
* :github:`37520` - Is zephyr can run syscall or extrenal program
* :github:`37519` - friend.c:unseg_app_sdu_decrypt causes assert: net_buf_simple_tailfroom(buf) >= len when payload + opcode is 10 or 11 bytes long
* :github:`37515` - drivers: flash_sam: Random failures when writing large amount of data to flash
* :github:`37502` - OPENTHREAD_CUSTOM_PARAMETERS does not seem to work
* :github:`37495` - mcuboot: Booting an image flashed on top of a Hawkbit updated ones results in hard fault
* :github:`37491` - wrong documentation format on DMA peripheral API reference
* :github:`37482` - 'cmd.exe' is not recognized as an internal or external command, operable program or batch file.
* :github:`37475` - twister: wrong test statuses in json report
* :github:`37472` - Corrupted timeout on poll for offloaded sockets
* :github:`37467` - Bluetooth: host: Incorrect advertiser timeout handling when using Limited Discoverable flag
* :github:`37465` - samples/bluetooth/iso_receive fails on nrf5340dk target
* :github:`37462` - Bluetooth: Advertising becomes scannable even if BT_LE_ADV_OPT_FORCE_NAME_IN_AD is set
* :github:`37461` - Schedule of LE audio in zephyr
* :github:`37460` - tests/kernel/sched/schedule_api/kernel.scheduler and tests/kernel/fifo/fifo_timeout/kernel.fifo.timeout failed on nsim_hs_smp board
* :github:`37456` - script: Unaccounted size in ram/rom reports
* :github:`37454` - Sensor driver: sht4x, sgp40, invalid include path
* :github:`37446` - Sensor driver: ST LPS22HH undeclared functions and variables
* :github:`37444` - MSI-X: wrong register referenced in map_msix_table()
* :github:`37441` - Native POSIX Flash Storage Does not Support Multiple Instances
* :github:`37436` - Delayed startup due to printing over not ready console
* :github:`37412` - IQ samples are not correct during the "reference period" of CTE signal
* :github:`37409` - Allow dual controller on usb
* :github:`37406` - ISO disconnect complete event doesn't reach the application
* :github:`37400` - esp32 build
* :github:`37396` - DHCP issue with events not triggering on network with microsoft windows DHCP server
* :github:`37395` - stm32h747i_disco board M4 core not working
* :github:`37391` - Bluetooth: 4 Bits of IQ Samples Are Removed (Direction Finding Based on CTE)
* :github:`37386` - bt_vcs_register() enhancement for setting default volume and step
* :github:`37379` - drivers: adc for stm32h7 depends on the version for oversampling
* :github:`37376` - samples/subsys/usb/dfu/sample.usb.dfu fails on teensy41/teensy40
* :github:`37375` - tests/drivers/adc/adc_api/drivers.adc fails to build on nucleo_h753zi
* :github:`37371` - logging.log2_api_deferred_64b_timestamp tests fails running on several qemu platforms
* :github:`37367` - Bluetooth: Host: Support setting long advertising data
* :github:`37365` - STM32 :DTCM: incorrect buffer size utilization
* :github:`37346` - STM32WL LoRa increased the current in "suspend_to_idle" state
* :github:`37338` - west flash to teensy 41 fail, use blinky with west build
* :github:`37332` - Increased power consumption for STM32WB55 with enabled PM and Bluetooth
* :github:`37327` - subsys/mgmt/hawkbit: hawkbit run can interrupt a running instance
* :github:`37319` - West 0.11.0 fails in Zephyr doc build under other manifest repo & renamed Zephyr fork
* :github:`37309` - ARC: add MPU v6 (and others) support
* :github:`37307` - Use XOSHIRO random number generator on NXP i.MX RT platform
* :github:`37306` - revert commit with bogus commit message
* :github:`37305` - Bluetooth Direction Finding Support of "AoA RX 1us"
* :github:`37304` - k_timer_status_sync can lock forever on qemu_x86_64
* :github:`37303` - tests: drivers: i2s: drivers.i2s.speed scenario fails on nrf platforms
* :github:`37294` - RTT logs not found with default west debug invocation on jlink runner
* :github:`37293` - Native POSIX MAC addresses are not random and are duplicate between multiple instances
* :github:`37272` - subsys/mgmt/hawkbit: Falsely determine that an update is installed successfully
* :github:`37270` - stm32l4 System Power Management issue
* :github:`37264` - tests-ci : can: isotp: implemmentation test  report FATAL ERROR when do not connect can loopback test pins
* :github:`37265` - tests-ci : kernel: scheduler: multiq test failed
* :github:`37266` - tests-ci : kernel: memory_protection: userspace test Timeout
* :github:`37267` - tests-ci : kernel: threads: apis test Timeout
* :github:`37263` - lib: timeutil: conversion becomes less accurate as time progresses
* :github:`37260` - STM32WL does not support HSE as RCC source and HSEDiv
* :github:`37258` - symmetric multiprocessing failed in user mode
* :github:`37254` - Run Coverity / Generate GitHub Issues
* :github:`37253` - west flash is failed with openocd  for on macOS
* :github:`37236` - ESP32 will not start when CONFIG_ASSERT=y is enabled
* :github:`37231` - BME280 faulty measurement after power cycle
* :github:`37228` - Bluetooth SMP does not complete pairing
* :github:`37226` - PM: soc: Leftover in conversion of PM hooks to __weak
* :github:`37225` - subsys/mgmt/hawkbit & sample: Bugs/improvements
* :github:`37222` - k_queue data corruption, override user data after reserved heading word
* :github:`37221` - nRF5340: SPIM4 invalid clock frequency selection
* :github:`37213` - ESP32: can't write to SD card over SPI (CRC error)
* :github:`37207` - drivers: serial: convert uart_altera_jtag_hal to use devicetree
* :github:`37206` - counter: stm32: Missing implementation of set_top_value
* :github:`37205` - openocd: Configure thread awareness by default
* :github:`37202` - esp32c3 build error
* :github:`37189` - Bug "Key 'name', 'cmake-ext' and 'kconfig-ext' were not defined" when build a zephyr application
* :github:`37188` - Get an error  of "Illegal load of EXC_RETURN into PC" when print log in IO interrupt callback
* :github:`37182` - cmsis_v1 osSignalWait doesn't clear the signals properly when any signal mask is set
* :github:`37180` - Led driver PCA9633 does nok take chip out from sleep
* :github:`37175` - nucleo-f756zg: rtos aware debugging not working
* :github:`37174` - Zephyr's .git directory is 409 MiB, can it be squashed?
* :github:`37173` - drivers: clock_control: stm32: AHB prescaler usable for almost all stm32 series
* :github:`37170` - LwM2M lwm2m_rd_client_stop() not working when called during bootstrapping/registration
* :github:`37160` - [Moved] Bootloader should provide the version of zephyr, mcuboot and a user defined version to the application
* :github:`37159` - osThreadTerminate does not decrease the instances counter
* :github:`37153` - USB serial number is not unique for STM32 devices
* :github:`37145` - sys: ring_buffer: ring_buf_peek() and ring_buf_size_get()
* :github:`37140` - Twister: Cmake error wrongly counted in the report
* :github:`37135` - Extend the HWINFO API to provide variable length unique ID
* :github:`37134` - Add support for the Raspberry Pi Compute Module 4
* :github:`37132` - Assert on enabling Socket CAN
* :github:`37120` - Documentation on modules
* :github:`37119` - tests: kernel tests hardfault on nucleo_l073rz
* :github:`37115` - tests/bluetooth/shell fails to builds on a lot of platforms
* :github:`37109` - Zephyr POSIX layer uses swapping with an interrupt lock held
* :github:`37105` - mcumgr: BUS FAULT when starting dfu with mcumgr CLI
* :github:`37104` - tests-ci : kernel: scheduler: multiq test failed
* :github:`37075` - PlatformIO: i cannot use the Wifi Shield ESP8266 to build the sample wifi project with the Nucleo F429ZI
* :github:`37070` - NXP mcux ADC16 reading 65535
* :github:`37057` - PWM-blinky for Silabs MCU
* :github:`37038` - stm32f4 - DMA tx interrupt doesn't trigger
* :github:`37032` - document: API reference missing: In clock of zephyr document
* :github:`37029` - drivers: sensor: sensor_value_to_double requieres non const sensor_value pointer
* :github:`37028` - ipv6 multicast addresses vanish after iface down/up sequence
* :github:`37024` - Compile error if we only use VCS without VOCS and AICS
* :github:`37023` - zephyr_prebuilt.elf and zephyr.elf has inconsistent symbol address in RISC-V platform
* :github:`37007` - Problem with out of tree driver
* :github:`37006` - tests: kernel: mem_protect: stack_random: enable qemu_riscv32
* :github:`36998` - TF-M: does not allow PSA Connect to proceed with IRQs locked
* :github:`36990` - Memory misalignment ARM Cortex-M33
* :github:`36971` - ESP32: wifi station sample does not get IP address by DHCP4
* :github:`36967` - Bluetooth: public API to query controller public address
* :github:`36959` - Direction Finding - CTE transmitted in connectionless mode has wrong length
* :github:`36953` - <err> lorawan: MlmeConfirm failed : Tx timeout
* :github:`36948` - Cluttering of logs on USB Console in Zephyr when CDC Shell is enabled
* :github:`36947` - Tensorflow: Dedicated tflite-micro repository
* :github:`36929` - Failure to build OpenThread LwM2M client on nrf52840dk
* :github:`36928` - Disconnecting ISO mid-send giver error in hci_num_completed_packets
* :github:`36927` - LWM2M: Writing to Write-Only resource causes notification
* :github:`36926` - samples/boards/nrf/system_off wouldn't compile on Particle Xenon board
* :github:`36924` - embARC Machine Learning Inference Library from Synopsys
* :github:`36917` - Runtime device PM is broken on STM32
* :github:`36909` - Shell log` commands crash the system if CONFIG_SHELL_LOG_BACKEND is not defined
* :github:`36896` - tests: net: select: still failing occasionally due to FUZZ
* :github:`36891` - Significant TCP perfomance regression
* :github:`36889` - string.h / strcasestr() + strtok()
* :github:`36885` - Update ISO API to better support TWS
* :github:`36882` - MCUMGR: fs upload fail for first time file upload
* :github:`36873` - USB AUDIO Byte alignment issues
* :github:`36869` - Direction Finding Connectionless porting to nrf52811
* :github:`36866` - CONFIG_NO_OPTIMIZATIONS=y MPU fault on Zephyr 2.6
* :github:`36865` - k_work_q seems to check uninitialized flag on start
* :github:`36859` - Possible Advertising PDU Corruption if bt_enable called in SYS_INIT function
* :github:`36858` - Static object constructors execute twice on the NATIVE_POSIX target
* :github:`36857` - i2c_samd0.c burst write not compatible with ssd1306.c
* :github:`36851` - FS logging backend assumes littlefs
* :github:`36823` - Build excludes paths to standard C++ headers when using GNUARMEMB toolchain variant
* :github:`36819` - qemu_leon3 samples/subsys/portability/cmsis_rtos_v2 samples failing
* :github:`36814` - Wrong format type for uint32_t
* :github:`36811` - Clarify ``Z_`` APIs naming conventions and intended scope
* :github:`36802` - MCUboot doesn't work with encrypted images on external flash
* :github:`36796` - Build failure: samples/net/civetweb/http_server using target stm32h735g_disco
* :github:`36794` - Build failure: tests/drivers/adc using stm32l562e_dk
* :github:`36790` - sys: ring_buffer: correct space calculation when tail is less than head
* :github:`36789` - [ESP32] samples blinky / gpio / custom board
* :github:`36783` - drivers: modem: hl7800 gpio init failed with interrupt flags
* :github:`36782` - drivers: serial: nrfx: Enforced pull-ups on RXD and CTS conflict on many custom boards
* :github:`36781` - source_periph incorrectly set in dma_stm32
* :github:`36778` - firmware update using mcumgr displays information for only slot 0 and not slot 1.
* :github:`36770` - doc：Missing description for deadline scheduling
* :github:`36769` - Zephyr assumes Interrupt Line config space register is RW, while ACRN hardwired it to 0.
* :github:`36767` - tests-ci :arch.arm.irq_advanced_features.arm_irq_target_state : test failed
* :github:`36768` - tests-ci :coredump.logging_backend : test failed
* :github:`36765` - [PCI] ACRN sets Interrupt Line config space register to 0 and ReadOnly.
* :github:`36764` - Bluetooth Require paired after disconnected work with iphone
* :github:`36755` - NTP client faults module when it fails
* :github:`36748` - Zephyr IP Stack Leaks when using PROMISCUOUS socket along with POSIX sockets based implementation.
* :github:`36747` - Adding Board Support for STEVAL-STWINKT1B
* :github:`36745` - Zephyr IP Stack Limited to 1514 bytes on the wire - no ICMPs beyond this limit
* :github:`36739` - coap_packet_get_payload() returns the wrong size
* :github:`36737` - Cortex M23: "swap_helper.S:223: Error: invalid offset, value too big (0x0000009C)"
* :github:`36736` - kernel: SMP global lock (and therefore irq_lock) works incorrectly on SMP platforms
* :github:`36718` - st_ble_sensor sample references wrong attribute
* :github:`36716` - zephyr - ADC - ATSAMD21G18A
* :github:`36713` - nrf5 ieee802154 driver does not compile and breaks CI
* :github:`36711` - Enable "template repository" for zephyrproject-rtos/example-application
* :github:`36696` - Json on native_posix_64 board
* :github:`36695` - net: ieee802154: cc13xx_26xx: Sub-GHz RF power saving
* :github:`36692` - Release Notes for 2.6.0 not useful (BLE API changes)
* :github:`36679` - Bluetooth - notifications not sending (bonded, CONFIG_BT_MAX_CONN=4, after disconnection then reconnection)
* :github:`36678` - Zephyr Throws Exception for Shell "log status" command when Telnet is shell backend and log is UART backend
* :github:`36668` - LittleFS example overwrite falsh memory
* :github:`36667` - logger: Filesystem backend doesn't work except for first time boot
* :github:`36665` - l2cap cids mixed up in request
* :github:`36661` - xtensa xcc does not support "-Warray-bounds"
* :github:`36659` - samples/net/sockets small bugs
* :github:`36655` - twister:  sometimes the twister fails because the error ``configparser.NoSectionError: No section: 'manifest'``
* :github:`36652` - deadlock in pthread implementation on SMP platforms
* :github:`36646` - sample.shell.shell_module.minimal_rtt fails to build on mimxrt1170_evk_cm4/mimxrt1170_evk_cm7
* :github:`36644` - Toolchain C++ headers can be included when libstdc++ is not selected
* :github:`36631` - Turn on GPIO from DTS
* :github:`36625` - compilation fails while building samples/net/openthread/coprocessor for Arduino nano 33 ble
* :github:`36613` - LoRaWAN - Provide method to register a callback for received data
* :github:`36609` - could not mount fatfs on efm32pg_stk3402a
* :github:`36608` - Unable to compile USB console example with uart_mux
* :github:`36606` - Regression in udp socket performance from zephyr v2.3.0 to v2.6.0
* :github:`36600` - Bluetooth: Peripheral: Bond issue when using secure connection
* :github:`36598` - Lora driver TX done wait/synchronous call
* :github:`36593` - Failing IPv6 Ready compliance (RFC 2460)
* :github:`36590` - NVS sector size above 65535 not supported
* :github:`36578` - net: ip: Assertion fails when tcp_send_data() with zero length packet
* :github:`36575` - Modbus RTU Client on ESP32
* :github:`36572` - kernel: Negative mutex lock_count value
* :github:`36570` - Use a custom role for Kconfig configuration options
* :github:`36569` - '.. only:' is not working as expected in documentation
* :github:`36568` - net: lib: sockets: Assertion fails when zsock_close()
* :github:`36565` - ehl_crb: Only boot banner is printed but not the test related details for multiple tests due to PR #36191 is not backported to v2.6.0 release
* :github:`36553` - LoRaWAN Sample: join accept but "Join failed"
* :github:`36552` - Bluetooth v2.6.0 connectable advertising leak/loss
* :github:`36540` - LoRaWAN otaa.app_key belongs to mib_req.Param.AppKey
* :github:`36524` - HSE clock doesn't initialize and blinky doesn't run on custom board when moving from zephyr v2.3.0 to v2.6.0
* :github:`36520` - tests/kernel/timer/timer_api/kernel.timer.tickless fails to build on npcx9m6f_evb
* :github:`36500` - espressif: cannot install toolchain on Darwin-arm64
* :github:`36496` - bluetooth: only the first Extended Advertising Report with data status "incomplete, more data to  come" is issued
* :github:`36495` - dtc generates missing #address-cells in interrupt provider warning
* :github:`36486` - LOG2 - self referential macro
* :github:`36467` - runner mdb-hw not work with arc hsdk board
* :github:`36466` - tests/kernel/mem_protect/mem_protect failed with arcmwdt toolchain
* :github:`36465` - samples/compression/lz4 failed with arcmwdt toolchian
* :github:`36462` - [bluetooth stack][limited_discoverable_advertising timeout] Some problems about the lim_adv_timeout
* :github:`36448` - samples: subsys: fs: fat_fs: adafruit_2_8_tft_touch_v2: buildkite compilation failed when no i2c defined
* :github:`36447` - net: socket: socketpair: Poll call resetting all events
* :github:`36435` - RFC: API Change: Mesh: Add return value for opcode callback
* :github:`36427` - test: kernel.common.nano32: zephyr-v2.6.0-286-g46029914a7ac: mimxrt1060_evk: test fails
* :github:`36419` - test-ci: net.ethernet_mgmt: zephyr-v2.6.0-286-g46029914a7ac: frdm_k64f: test fails
* :github:`36418` - test-ci: net.socket.tls: zephyr-v2.6.0-286-g46029914a7ac: frdm_k64f: test fail
* :github:`36417` - tests-ci :coredump.logging_backend : zephyr-v2.6.0-286-g46029914a7ac: lpcxpresso55s28: test failed
* :github:`36416` - tests-ci :arch.arm.irq_advanced_features.arm_irq_target_state : zephyr-v2.6.0-286-g46029914a7ac: lpcxpresso55s28: test failed
* :github:`36414` - ESP32 with samples/net/wifi gives: net_if: There is no network interface to work with!
* :github:`36412` - Blinky on ESP32: Unsupported board: led0 devicetree alias is not defined"
* :github:`36410` - board: cc1352r_sensortag: add dts entry for hdc2080
* :github:`36408` - ARM_MPU on boards ``stm32_min_dev_*`` without MPU enabled
* :github:`36398` - [Video API] Erroneous function pointer validation
* :github:`36390` - net: ip: Negative TCP unacked_len value
* :github:`36388` - ARM: Architecture Level user guide
* :github:`36382` - segfault when hardware isn't emulated
* :github:`36381` - Bluetooth ASSERTION FAIL [evdone] Zephyr v2.6.0
* :github:`36380` - missing auto-dependency on CONFIG_EMUL
* :github:`36357` - tests: samples: watchdog: sample.subsys.task_wdt fails on nrf platforms
* :github:`36356` - Network fails to transmit STM32H747DISC0 board zephyr v2.6.0
* :github:`36351` - nRF: we do not always guarantee that SystemInit is inlined
* :github:`36347` - Zephyr Wifi IoT device - whats a good board to start with?
* :github:`36344` - Zephyr 2.6.0 st_ble_sensor sample is broken when compiled for nucleo_wb55rg
* :github:`36339` - samples/subsys/logging/dictionary doesn't build under MS Windows environment
* :github:`36329` - Support for CC3120 WiFi module
* :github:`36324` - add project groups to upsteam west manifest
* :github:`36323` - Don't set TFM_CMAKE_BUILD_TYPE_DEBUG by default on LPC55S69-NS if DEBUG_OPTIMIZATIONS
* :github:`36319` - Help: Asking for Help Tips page gets 404 error
* :github:`36318` - [Coverity CID: 236600] Unused value in drivers/ieee802154/ieee802154_nrf5.c
* :github:`36317` - [Coverity CID: 236599] Unused value in drivers/ieee802154/ieee802154_nrf5.c
* :github:`36316` - [Coverity CID: 236597] Unused value in drivers/ieee802154/ieee802154_nrf5.c
* :github:`36315` - [Coverity CID: 236604] Untrusted value as argument in subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`36314` - [Coverity CID: 236610] Uninitialized pointer read in subsys/bluetooth/mesh/proxy.c
* :github:`36313` - [Coverity CID: 236602] Unchecked return value in drivers/modem/gsm_ppp.c
* :github:`36312` - [Coverity CID: 236608] Out-of-bounds access in subsys/bluetooth/audio/mics_client.c
* :github:`36311` - [Coverity CID: 236598] Out-of-bounds access in subsys/bluetooth/audio/mics_client.c
* :github:`36310` - [Coverity CID: 236607] Missing break in switch in drivers/ieee802154/ieee802154_nrf5.c
* :github:`36309` - [Coverity CID: 236606] Missing break in switch in drivers/ieee802154/ieee802154_nrf5.c
* :github:`36308` - [Coverity CID: 236601] Missing break in switch in drivers/ieee802154/ieee802154_nrf5.c
* :github:`36307` - [Coverity CID: 236605] Logically dead code in subsys/bluetooth/audio/mics.c
* :github:`36306` - [Coverity CID: 236596] Logically dead code in subsys/bluetooth/audio/mics.c
* :github:`36305` - [Coverity CID: 236595] Logically dead code in samples/drivers/eeprom/src/main.c
* :github:`36304` - [Coverity CID: 236609] Explicit null dereferenced in subsys/bluetooth/audio/mics_client.c
* :github:`36303` - [Coverity CID: 236603] Dereference after null check in subsys/bluetooth/audio/vcs_client.c
* :github:`36301` - soc: cypress: Port Zephyr to Cypress CYW43907
* :github:`36298` - TF-M integration: add a brief user guide
* :github:`36291` - ADC and math library functions use for stm32l496
* :github:`36289` - eswifi gets a deadlock on b_l4s5i_iot01a target
* :github:`36282` - Overwrite mode for RTT logging
* :github:`36278` - ARM: Cortex-M: SysTick priority is not initialized if the SysTick is not used
* :github:`36276` - NULL pointer access in check_used_port()
* :github:`36270` - TF-M: introduce uniformity in Non-Secure target names
* :github:`36267` - net: ieee802154: software address filtering
* :github:`36263` - up_squared: kernel.memory_protection.mem_map.x86_64 failed.
* :github:`36256` - SPI4 & 3 MISO not working on nRF5340
* :github:`36255` - tests/subsys/logging/log_core failed on hsdk board
* :github:`36254` - Zephyr shell subsystem not work with ARC hardware boards.
* :github:`36250` - tests/subsys/cpp/cxx - doesn't compile on native_posix when CONFIG_EXCEPTIONS=y
* :github:`36247` - samples: usb: testusb: Problems with using with cdc-acm
* :github:`36242` - Zephyr Upstream + sdk-nrf BLE NUS SHELL LOG/CBPRINTF build problem.
* :github:`36238` - net_if.c: possible mutex deadlock
* :github:`36237` - fs_open returns 0 on existing file with FS_O_CREATE | FS_O_WRITE
* :github:`36197` - BOSSA flashing on Arduino Nano 33 BLE (NRF52840)
* :github:`36185` - CMP0116 related warnings
* :github:`36172` - net: ieee802154: LL src/dst address is lost from received net_pkt (when using 6LO)
* :github:`36163` - nvs no longer supports the use of id=0xffff
* :github:`36131` - Occasionally unable to scan for extended advertisements when connected
* :github:`36117` - toolchain: The added abstraction for llvm, breaks builds with off-tree llvm based toolchains
* :github:`36107` - ehl_crb: Multiple tests are failing and board is not booting up.
* :github:`36101` - tfm related build rebuild even if nothing changes
* :github:`36100` - pb_gatt buf_send does not call callback
* :github:`36095` - drivers: pwm: sam: compilation failure for sam_v71b_xult
* :github:`36094` - BLE wrong connections intervals on multible connections
* :github:`36093` - Fix dt_compat_enabled_with_label behavior (or usage)
* :github:`36089` - intel_adsp_cavs25: support more than 2 DSP cores
* :github:`36088` - intel_adsp_cavs25: secondary boot fails in arch_start_cpu()
* :github:`36084` - Arduino Nano 33 BLE: USB gets disconnected after flashing
* :github:`36078` - coredump.logging_backend: lpcxpresso55s28: test failure assertion fail
* :github:`36077` - net: lib: coap: Impossible to get socket info from incoming packet
* :github:`36075` - drivers: can: stm32fd: can2 does not work
* :github:`36074` - LoRaWAN: sx126x: infinite loop on CRC error
* :github:`36061` - Undefined reference to ``z_priq_rb_lessthan(rbnode*, rbnode*)`` when using k_timer_start in cpp file.
* :github:`36057` - Zephyr Shell Console and Logging Targeting Isolated Different Device Interfaces
* :github:`36048` - Cannot establish ISO CIS connection properly after ACL disconnected several times
* :github:`36038` - iotdk: the testcase samples/modules/nanopb can't build
* :github:`36037` - bt_init returning success when Bluetooth initialization does not get finalized.
* :github:`36035` - struct devices should be allocated in ROM, not RAM
* :github:`36033` - Mere warnings slow down incremental documentation build from seconds to minutes
* :github:`36030` - West warnings (and others?) are ignored when building documentation
* :github:`36028` - More Description in Example Documentation
* :github:`36026` - wolfssl / wolfcrypt
* :github:`36022` - Wrong channel index in connectionless IQ samples report
* :github:`36014` - stm32g050: Missing closing parenthesis for soc prototype
* :github:`36013` - arm: qemu: run cmsis-dsp tests on the qemu target with FPU
* :github:`35999` - Unexpected Bluetooth disconnection and removal of bond
* :github:`35992` - stm32f303k8 device tree missing DACs
* :github:`35986` - POSIX: multiple definition of posix_types
* :github:`35983` - [backport v1.14-branch] backport of #35935 failed
* :github:`35978` - ESP32 SPI send data hangup
* :github:`35972` - C++ exceptions do not work when building with GNU Arm Embedded
* :github:`35971` - ehl_crb: test_nop  is failing under tests/kernel/common/
* :github:`35970` - up_squared: samples/boards/up_squared/gpio_counter/ is failing
* :github:`35964` - shell_uart hangs when putting UART into PM_LOW_POWER_STATE / PM_DEVICE_STATE_LOW_POWER
* :github:`35962` - drivers using deprecated Kconfigs
* :github:`35955` - Bluetooth: Controller: Regression in connection setup
* :github:`35949` - can: mcan: sjw-data devicetree configuration is not written correctly
* :github:`35945` - SPI4 on nRF5340 not working when using k_sleep() in main
* :github:`35941` - subsys: tracing: sysview: No SEGGER_SYSVIEW.h in path
* :github:`35939` - enc424j600 driver unusable/broken on stm32l552
* :github:`35931` - Bluetooth: controller: Assertion in ull_master.c
* :github:`35930` - nRF Dongle as BLE Central Unstable Connectivity at Long-ish Range
* :github:`35926` - Shell tab-completion with more than two levels of nested dynamic commands fails
* :github:`35916` - drivers: TI cc13xx_cc26xx: build error when PM is enabled (serial, entropy, spi, i2c modules)
* :github:`35908` - Stopping DHCP with network interface goes down leaves networking state in a broken state
* :github:`35897` - Bluetooth: PTS Tester on native posix
* :github:`35890` - Build system ignores explicit ZephyrBuildConfiguration_ROOT variable
* :github:`35880` - PSA tests run indefinitely when CONFIG_TFM_IPC=y
* :github:`35870` - Build failure with gcc 11.x on native_posix
* :github:`35857` - intel_adsp_cavs15: run msgq testcases failed on ADSP
* :github:`35856` - intel_adsp_cavs15: run semaphore testcases failed on ADSP
* :github:`35850` - the sample kernel/metairq_dispatch fails on nucleo_g474re
* :github:`35835` - ADC support for STM32l496_disco board
* :github:`35809` - sample: USB audio samples are not working on STM32
* :github:`35793` - kernel.scheduler.multiq: Failed since #35276 ("cooperative scheduling only" special cases removal)
* :github:`35789` - sockets_tls: receiving data on offloaded socket before handshake causes pollin | pollerr and failed recvfrom (SARA-R4)
* :github:`35721` - Atmel sam0 Async and/or DMA may not work
* :github:`35720` - tests:kernel timer fails on test_sleep_abs with TICKLESS_KERNEL and PM on nucleo_wb55rg
* :github:`35718` - Excessive error messages from filesystem interface
* :github:`35711` - net: sockets: dtls: handshake not reset as it ought to be
* :github:`35707` - AssertionError: zephyr/tests/kernel/common test case is failing with gcc-11 (Yocto)
* :github:`35703` - posix_apis: fails at test_posix_realtime for mimxrt1024_evk
* :github:`35681` - Unable to get output for samples/subsys/logging/logger and samples/philosophers
* :github:`35668` - The channel selection of auxiliary advertisments in extended advertisments
* :github:`35663` - STM32H7: Support memory protection unit(MPU) to enable shared memory
* :github:`35658` - arch.interrupt.arm.irq_vector_table.arm_irq_vector_table: MPU FAULT Halting system for mximxrt685_evk_cm33
* :github:`35656` - arch.interrupt.arm.arm_interrupt: hangs on mimxrt685_evk_cm33
* :github:`35581` - stm32 SPI problems with DMA and INTR set-up
* :github:`35550` - nRF91: DPS310 I2C driver not working
* :github:`35532` - SSL Handshake error with modified http(s) client example
* :github:`35529` - STM32: STM32H7 ADC calibration must be performed on startup
* :github:`35429` - subsys: settings: Encryption
* :github:`35377` - add creg_gpio driver for ARC HSDK board
* :github:`35354` - Adding support for measurement of Ultraviolet(UV) Light.
* :github:`35293` - Sporadic boot failure
* :github:`35256` - DOC:  DATA PASSING TABLE MISSING THE OBJECT QUEUES
* :github:`35250` - Twister is not reading the serial line output completely
* :github:`35244` - twister: build failure for native_posix with GNU binutils 2.35
* :github:`35238` - ieee802.15.4 support for stm32wb55
* :github:`35229` - twister log mixing between tests
* :github:`35190` - echo_server sample non-functional rails all CPUs on native_posix_64 board build
* :github:`35055` - STM32L432KC Nucleo Reference board SWD problem after programming with Zephyr
* :github:`34917` - arch.interrupt.arm| arch.interrupt.extra_exception_info: lpcxpresso55s28 series: test failure
* :github:`34913` - ModuleNotFoundError: No module named 'elftools'
* :github:`34879` - mec15xxevb_assy6853: 2 GPIO test failures
* :github:`34855` - FANSTEL BT840X
* :github:`34832` - Coding Guideline - MISRA rule 14.4 not applied properly
* :github:`34829` - Bluetooth: ISO: Don't attempt to remove the ISO data path of a disconnected ISO channel
* :github:`34767` - C++ support on ESP boards
* :github:`34760` - Hawkbit not downloading large files
* :github:`34659` - Bluetooth: HCI cmd response timeout
* :github:`34571` - Twister mark successfully passed tests as failed
* :github:`34557` - upgrade fatfs to 0.14b
* :github:`34554` - Settings FS: Duplicate finding is extremely slow when dealing with larger number of settings entries
* :github:`34544` - lib: gui: lvgl: buffer overflow bug on misconfiguration
* :github:`34543` - STM32F1 failed to compile with CONFIG_UART_ASYNC_API
* :github:`34392` - [backport v2.5-branch] backport of #34237 failed
* :github:`34391` - [backport v1.14-branch] backport of #34237 failed
* :github:`34390` - i2s: bitrate is wrongly configured on STM32
* :github:`34372` - CPU Lockups when using own Log Backend
* :github:`34354` - Please investigate adding DMA support to STM32 I2C!
* :github:`34324` - RTT is not working on STM32
* :github:`34315` - BMI270 configuration file sending to I2C seems to be not handling the last part of the configuration properly.
* :github:`34305` - Shell [modem send] command causes shell to hang after about 10 seconds, Sara R4 - Particle Boron
* :github:`34282` - HAL Module Request: hal_telink
* :github:`34273` - mqtt_publisher: Unable to connect properly on EC21 modem with bg9x driver
* :github:`34269` - LOG_MODE_MINIMAL BUILD error
* :github:`34268` - Bluetooth: Mesh: Sample is stuck in init process on disco_l475_iot
* :github:`34259` - Problem running code with memory domain
* :github:`34239` - Call settings_save_one in the system work queue, which will cause real-time performance degradation.
* :github:`34236` - External source code integration request: Raspberry Pi Pico SDK
* :github:`34231` - uzlib (decompression library)
* :github:`34226` - Compile error when building civetweb http_server sample for posix_native
* :github:`34222` - Commit related to null pointer exception detection causing UART issues
* :github:`34218` - Civetweb server crashing when trying to access invalid resource
* :github:`34204` - nvs_write: Bad documented return value.
* :github:`34192` - Sensor BME680: Add support for SPI operation
* :github:`34134` - USB do not works if bootloader badly use the device before
* :github:`34131` - TFTP client ignores incoming data packets
* :github:`34121` - Unable to generate pdf according to the documentation steps on windows
* :github:`34105` - Convert tests/kernel/workq to new kwork API
* :github:`34049` - Nordic nrf9160 switching between drivers and peripherals
* :github:`34015` - cfb sample "Device not found" for esp32 when SSD1306 is enabled
* :github:`33994` - kscan_ft5336 doesn't provide proper up/down information when polling, and hogs resources in interrupt mode
* :github:`33960` - Zephyr for Briey SoC
* :github:`33937` - [backport v1.14-branch] backport of #26712 failed
* :github:`33932` - [backport v1.14-branch] backport of #26083 failed
* :github:`33910` - sam_v71_xult -> I2C_1 hang during scanning i2c devices
* :github:`33901` - tests: interrupt: irq_enable() and irq_disable() do not work with direct and regular interrupt on x86
* :github:`33895` - Device tree: STM32L412 and STM32L422 are missing nodes
* :github:`33883` - [backport v2.5-branch] backport of #33340 failed
* :github:`33876` - Lora sender sample build error for esp32
* :github:`33873` - arm_arch_timer: Too many clock announcements with CONFIG_TICKLESS_KERNEL=n on SMP
* :github:`33862` - [backport v2.5-branch] backport of #33771 failed
* :github:`33753` - LVGL output doesn't match the LVGL TFT simulator for gauge widget
* :github:`33652` - Monitoring the BLE connection
* :github:`33573` - JSON_OBJ_DESCR_ARRAY_ARRAY is dangerously broken
* :github:`33554` - Request to add OM13056 board (LPC1500 family or specifically SoC LPC1519) support to Zephyr
* :github:`33544` - ehl_crb: portability.posix.common.posix_realtime failed.
* :github:`33485` - Issue with DMA transfers outside of the Zephyr DMA driver on STM32F767
* :github:`33483` - TIMESLICE and PM interaction and expected behavior
* :github:`33449` - Remove deprecated items in 2.7
* :github:`33440` - lsm6dso sensor driver not working on nRF5340
* :github:`33435` - armclang / armlinker
* :github:`33337` - twister: Find and fix all "dead" samples/tests
* :github:`33275` - ehl_crb: samples/subsys/shell/shell_module does not work
* :github:`33265` - Power Management Overhaul
* :github:`33192` - LoRaWAN - Application fails to start if module is not powered
* :github:`33113` - Improve code coverage for new feature or code change in kernel
* :github:`33104` - Updating Zephyr to fix Work Queue Problems
* :github:`33099` - ppp: termination packet not sent
* :github:`33052` - [Coverity CID :219624] Untrusted loop bound in subsys/bluetooth/host/sdp.c
* :github:`33041` - [Coverity CID :219645] Untrusted loop bound in subsys/bluetooth/host/sdp.c
* :github:`33016` - spi_nor: CONFIG_SPI_NOR_SFDP_RUNTIME leaves flash in Standby after spi_nor_configure()
* :github:`33015` - spi_nor driver: SPI_NOR_IDLE_IN_DPD breaks SPI_NOR_SFDP_RUNTIME
* :github:`32997` - Improve documentation search experience
* :github:`32990` - FS/littlefs: it is possible to write to already deleted file
* :github:`32984` - West: openocd runner: Don't let debug mode on by default
* :github:`32875` - Benchmarking Zephyr vs. RIOT-OS
* :github:`32836` - Remaining integration failures on intel_adsp_cavs15
* :github:`32822` - Code doesn't compile after changing the PWM pin on example "blinky_pwm" on NRF52
* :github:`32803` - Extend mcux uart drivers to support async API
* :github:`32789` - USB DFU support w/o MPU support
* :github:`32733` - RS-485 support
* :github:`32669` - [Bluetooth] sample code for Periodic Advertising Sync Transfer
* :github:`32603` - acrn_ehl_crb: test case of arch.interrupt.prevent_interruption failed
* :github:`32564` - net_buf reference count not protected
* :github:`32545` - It seems that CONFIG_IMG_MGMT_VERBOSE_ERR does not work
* :github:`32531` - get_maintainer.py cannot parse MAINTAINERS.yml
* :github:`32293` - Zephyr 2.6 Release Checklist
* :github:`32289` - USDHC: Fails after reset
* :github:`32282` - x86 ACPI images are much too large
* :github:`32261` - problem with CONFIG_STACK_SENTINEL
* :github:`32133` - Current atomics are subtly broken on AArch64 due to memory ordering
* :github:`32111` - Zephyr build fail with LLVM on Windows
* :github:`32035` - Bluetooth: application notification when MTU updated
* :github:`31993` - Add west extension to parse yml file
* :github:`31985` - riscv: Long execution time when TICKLESS_KERNEL=y
* :github:`31943` - drivers: flash: stm32: harmonization of flash erase implementation across STM32 series
* :github:`31739` - Convert CoAP unit tests to use ztest API
* :github:`31593` - civetweb hangs when there are no free filedescriptors
* :github:`31499` - lwm2m : Add visibility into observer notification success/fail
* :github:`31475` - TCP keepalive
* :github:`31473` - Failed phy request not retried and may prevent DLE procedure during auto-initiation
* :github:`31447` - MQTT idling gets disconnected when using TCP2
* :github:`31290` - dts: arm: st: standardize pwm default property st,prescaler to 0
* :github:`31253` - lis3dh driver support is confusing
* :github:`31162` - Mapping between existing and new system power management states
* :github:`31107` - libc: minimal: add qsort routine
* :github:`31043` - Infinite loop in modem cmd_handler_process
* :github:`30921` - west flash failed with an open ocd error
* :github:`30861` - drivers: uart: increase timeout precision in uart_rx_enable
* :github:`30635` - cpu_stats: Change from printk to ``LOG_*``
* :github:`30429` - Thread Border Router with NRC/RCP sample and nrf52840dk not starting
* :github:`30367` - TCP2 does not send our MSS to peer
* :github:`30245` - Bluetooth: controller: event scheduling pipeline preemption by short schedule
* :github:`30244` - Bluetooth: controller: Extended scan window time reservation prevents auxiliary channel reception
* :github:`30243` - Bluetooth: controller: IRK resolution in extended scanning breaks auxiliary PDU reception
* :github:`30236` - Main thread sometimes looping forever before user application is reached when using UDP and IPv6 on Nucleo F767ZI
* :github:`30209` - TCP2 : How to add MSS option on sending [SYN, ACK] to client?
* :github:`30066` - CI test build with RAM overflow
* :github:`30026` - Can not make multiple BLE IPSP connection to the same host
* :github:`29545` - samples: tfm_integration: tfm_ipc: No module named 'cryptography.hazmat.primitives.asymmetric.ed25519'
* :github:`29535` - riscv: stack objects are mis-aligned
* :github:`29520` - make k_current_get() work without a system call
* :github:`29397` - Build all tests of module mcuboot
* :github:`28872` - Support ESP32 as Bluetooth controller
* :github:`28819` - memory order and consistency promises for Zephyr atomic API?
* :github:`28729` - ARM: Core Stack Improvements/Bug fixes for 2.6 release
* :github:`28716` - 2.5 Release Checklist
* :github:`28312` - Add option to enable ART Accelerator on STM32 FLASH controller
* :github:`27992` - stm32f7: usb: Bursting HID Get and Set report requests leads to unresponding Control endpoint.
* :github:`27525` - Including STM32Cube's USB PD support to Zephyr
* :github:`27415` - Decide if we keep a single thread support (CONFIG_MULTITHREADING=n) in Zephyr
* :github:`27176` - [v1.14] Restore socket descriptor permission management
* :github:`27015` - Add custom transport support for MQTT
* :github:`26981` - Problem with PPP + GSM MUX with SIMCOM7600E
* :github:`26585` - IPv4 multicast datagrams can't be received for mimxrt1064_evk board (missing ethernet API)
* :github:`26256` - NRF51822 BLE Micro module: hangs on k_msleep() (RTC counter not working)
* :github:`26136` - CMake Error in Windows Environment
* :github:`26051` - shell: uart: Allow a change in the shell initalisation to let routing it through USB UART
* :github:`25832` - [test][kernel][lpcxpresso55s69_ns] kernel cases meet ESF could not be retrieved successfully
* :github:`25182` - Raspberry Pi 4B Support
* :github:`25015` - Bluetooth Isochronous Channels Support
* :github:`24854` - docs: Using third-party libraries not well documented in Memory partitions docs
* :github:`24733` - Misconfigured environment
* :github:`24200` - USB GET_INTERFACE response always 0, even when an alternate setting is used
* :github:`24051` - double to sensor_val
* :github:`23745` - Align PS/2 handlers with the handlers found in other drivers
* :github:`23723` - Poor sinf/cosf performance compared to the Segger math libraries
* :github:`23349` - Question: How to add external soc, board, DTS, drivers and libs?
* :github:`22731` - Improve docker CI documentation
* :github:`22705` - Implement counter driver for lpcxpresso55s69
* :github:`22702` - Implement I2S driver for lpcxpresso55s69
* :github:`22455` - How to assign USB endpoint address manually in stm32f4_disco for CDC ACM class driver
* :github:`22210` - Bluetooth -  bt_gatt_get_value_attr_by_uuid
* :github:`22131` - ARM Cortex_R: CONFIG_USERSPACE: external interrupts are disabled during system calls
* :github:`21869` - IPv6 neighbors get added too eagerly
* :github:`21648` - improve documentation on meta-IRQ threads
* :github:`21519` - RFC: libc: thread-safe newlib
* :github:`21339` - Expired IPv6 router causes an infinite loop
* :github:`21293` - adding timeout the I2C read/write functions for the stm32 port
* :github:`21205` - get_device_list only available if power management invoked
* :github:`21167` - libraries.libc.newlib test fails
* :github:`20576` - DTS overlay files must include full path name
* :github:`20409` - USB: Create webusb shell
* :github:`20236` - usb: api: Cleanup of current inclusion path for USB
* :github:`20171` - support external spi nor flash on mimxrt1060-evk
* :github:`19882` - Add support for multiple channel sampling to STM32 ADC driver
* :github:`19328` - Logger could block in thread at certain log message pool usage
* :github:`18960` - [Coverity CID :203908]Error handling issues in /lib/libc/newlib/libc-hooks.c
* :github:`18896` - Concurrent Multi-Protocol Support NRF52840
* :github:`18850` - Bluetooth: controller: Advertiser following directed advertiser will have corrupt data
* :github:`18386` - [Coverity CID :203443]Memory - corruptions in /subsys/bluetooth/host/rfcomm.c
* :github:`18351` - logging: 32 bit float values don't work.
* :github:`18316` - Support for unregistering bt_conn callbacks
* :github:`18042` - Only corporate members can join the slack channel
* :github:`17748` - stm32: clock-control: Remove usage of SystemCoreClock
* :github:`17692` - Proper way for joining a multicast group (NRF52840/OpenThread)
* :github:`17375` - Add VREF, TEMPSENSOR, VBAT internal channels to the stm32 adc driver
* :github:`17021` - revise concurrency control in kernel/userspace.c
* :github:`16761` - nrf52840 usb driver with openthread
* :github:`16671` - ideas for future of the settings
* :github:`16231` - Add CONFIG_UART_DYNAMIC_SETTINGS option
* :github:`15841` - Support AT86RF233
* :github:`15793` - Unable to load binaries into iotdk
* :github:`15676` - Support instrumentation for time spent in various power states
* :github:`15555` - Counter Docs Missing Callback Context Note
* :github:`14308` - Better integration between system and device power modes.
* :github:`12405` - add test to catch issues fixed in PR  #12384
* :github:`11773` - Add Bluetooth support for Silicon Labs EFR32MG12
* :github:`11702` - Add support for nrfx i2s driver
* :github:`11519` - Add at least build test for cc1200
* :github:`11193` - ARM V8M Trusted Execution Environments and Zephyr
* :github:`11028` - CONFIG_LOAPIC_SPURIOUS_VECTOR not being tested
* :github:`11000` - USB 2.0 high-speed support in Zephyr
* :github:`10930` - Extending string formatting function
* :github:`10676` - Feature Required: DFU over Thread network
* :github:`10378` - watchdog: Limitation with the current watchdog API for Nordic devices
* :github:`10324` - Publish PDF with the release doc build
* :github:`10198` - Add support for FRDM-STBC-AGM01 sensor shield
* :github:`8876` - Adapt net/l2/ieee802154 subsystem to new shell subsystem
* :github:`8275` - when zephyr can support popular IDE develop?
* :github:`7001` - ST Sensors: Driver factorization
* :github:`6777` - Add copyright handling to contributing doc
* :github:`6657` - Question: Is Bluetooth avrcp supported in Zephyr? Or any plan?
* :github:`6493` - need APIs for ranged random number generation
* :github:`6450` - Several devices of same type on same bus - how to address?
* :github:`6117` - Make sanitycheck aware of DTS and HW support
* :github:`4911` - Filesystem support for qemu
* :github:`1392` - No module named 'elftools'
* :github:`3886` - Add mutual authentication to net/crypto examples
* :github:`3885` - Add real entropy to crypto-based net samples
* :github:`3884` - Improve the TLS and DTLS examples to use best practices on security
* :github:`3879` - k_thread_abort vs k_thread->fn_abort()
* :github:`3677` - Implement HCI Zephyr extensions
* :github:`3199` - xtensa: simplify linker scripts
* :github:`2811` - Investigate having timeout code track tick deadlines instead of deltas
* :github:`2619` - Define APIs for hashing/ Message Authentication
* :github:`2248` - Split LE Controller: style fixes
