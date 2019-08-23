:orphan:

.. _zephyr_2.0:

Zephyr Kernel 2.0.0 (Working Draft)
####################################

We are pleased to announce the release of Zephyr kernel version 2.0.0.

Major enhancements with this release include:

* The kernel now supports both 32- and 64-bit architectures.
* We added support for SOCKS5 proxy. SOCKS5 is an Internet protocol that
  exchanges network packets between a client and server through a proxy server.
* Introduced support for 6LoCAN, a 6Lo adaption layer for Controller Area
  Networks.
* We added support for :ref:`Point-to-Point Protocol (PPP) <ppp>`. PPP is a
  data link layer (layer 2) communications protocol used to establish a direct
  connection between two nodes.
* We added support for UpdateHub which is an end-to-end solution for large scale
  over-the-air update of devices.
* We added support for ARM Cortex-R Architecture (EXPERIMENTAL).

The following sections provide detailed lists of changes by component.

Kernel
******

* New kernel API for per-thread disabling of Floating Point Services for
  ARC, ARM Cortex-M, and x86 architectures.

Architectures
*************

* ARM:

  * Added initial support for ARM Cortex-R architecture (EXPERIMENTAL)
  * We enhanced the support for Floating Point Services in Cortex-M
    architecture, implementing and enabling lazy-stacking for FPU
    capable threads and fixing stack overflow detection for FPU
    capable supervisor threads
  * Added Qemu support for ARMv8-M Mainline architecture
  * Optimized the IRQ locking time in thread context switch
  * Fixed several critical bugs in User Mode implementation
  * Added test coverage for ARM-specific kernel features
  * Improved support for linking TrustZone Secure Entry functions into
    Non-Secure firmware


* POSIX:

  * Fix race condition with terminated threads which had never been
    scheduled by kernel. On very loaded systems it could cause swap errors.

Boards & SoC Support
********************

* Add native_posix_64: A 64 bit variant of native_posix

* Added support for the following ARC boards:

  * emdsp
  * hsdk

* Added support for the following ARM boards:

  * atsamr21_xpro
  * cc1352r1_launchxl
  * cc26x2r1_launchxl
  * holyiot_yj16019
  * lpcxpresso55s69
  * mec15xxevb_assy6853
  * mikroe_mini_m4_for_stm32
  * mimxrt1015_evk
  * mps2_an521
  * nrf51_pca10031
  * nrf52811_pca10056
  * nucleo_g071rb
  * nucleo_wb55rg
  * qemu_cortex_r5
  * stm32h747i_disco
  * stm32mp157c_dk2
  * twr_ke18f
  * v2m_musca_b1
  * 96b_avenger96
  * 96b_meerkat96
  * 96b_wistrio

* Added support for the following RISC-V boards:

  * hifive1_revb
  * litex_vexriscv
  * qemu_riscv64

* Added support for the following x86 boards:

  * gpmrb

* Added support for the following shield boards:

  * frdm_cr20a
  * link_board_can
  * sparkfun_sara_r4
  * wnc_m14a2a
  * x_nucleo_iks01a3

* Removed support for the following boards:

  * arduino_101
  * arduino_101_sss
  * curie_ble
  * galileo
  * quark_d2000_crb
  * quark_se_c1000_devboard
  * quark_se_c1000_ss_devboard
  * quark_se_c1000_ble
  * tinytile
  * x86_jailhouse

Drivers and Sensors
*******************

* ADC

  * Added API to support calibration
  * Enabled ADC on STM32WB
  * Removed Quark D2000 ADC driver
  * Added NXP ADC12 and SAM0 ADC drivers
  * Added ADC shell

* Audio

  * Added support for 2 microphones (stereo) in mpxxdtyy driver

* CAN

  * Added support for canbus Ethernet translator
  * Added 6LoCAN implementation
  * Added MCP2515, NXP FlexCAN, and loopback drivers
  * Added CAN shell

* Clock Control

  * Added NXP Kinetis MCG, SCG, and PCC drivers
  * Removed Quark SE driver
  * Added STM32H7, STM32L1X, and STM32WB support

* Counter

  * Added optional flags to alarm configuration structure and extended set channel alarm flags
  * Added top_value setting configuration structure to API
  * Enabled counter for STM32WB
  * Added NXP GPT, "CMOS" RTC, SiLabs RTCC, and SAM0 drivers
  * Removed Quark D2000 support from QMSI driver

* Display

  * Added ST7789V based LCD driver
  * Renamed ssd1673 driver to ssd16xx
  * Added framebuffer driver with multiboot support
  * Added support for Seeed 2.8" TFT touch shield v2.0

* DMA

  * Added API to retrieve runtime status
  * Added SAM0 DMAC driver
  * Removed Quark SE C1000 support from QMSI driver

* Entropy

  * Added TI CC13xx / CC26xx driver

* ESPI

  * Added Microchip XEC driver

* Ethernet

  * Added LiteEth driver

* Flash

  * Removed Quark SE C1000 driver
  * Removed support for Quark D2000 from QMSI driver
  * Added STM32G0XX and STM32WB support to STM32 driver
  * Added RV32M1 and native POSIX drivers

* GPIO

  * Added stm32f1x SWJ configuration
  * Removed Quark SE C1000 and D2000 support from DesignWare driver
  * Added support for STM32H7, STM32L1X, and STM32WB to STM32 driver
  * Added Microchip XEC and TI CC13x2 / CC26x2 drivers
  * Added HT16K33 LED driver
  * Added interrupt support to SAM0 driver

* Hardware Info

  * Added ESP32 and SAM0 drivers

* I2C

  * Added support for STM32MP1, STM32WB, and STM32L1X to STM32 driver
  * Added STM32F10X slave support
  * Added power management to nrf TWI and TWIM drivers
  * Added TI CC13xx / CC26xx, Microchip MEC, SAM0, and RV32M1 drivers
  * Rewrote DesignWare driver for PCI(e) support

* IEEE 802.15.4

  * Fixed KW41z fault and dBm mapping

* Interrupt Controller

  * Added initial support for ARC TCC
  * Added GIC400, LiteX, and SAM0 EIC drivers
  * Added support for STM32G0X, STM32H7, STM32WB, and STM32MP1 to STM32 driver
  * Removed MVIC driver

* IPM

  * Removed Quark SE driver
  * Added MHU and STM32 drivers

* LED

  * Added Holtek HT16K33 LED driver

* Modem

  * Introduced socket helper layer
  * Introduced command handler and UART interface driver layers
  * Introduced modem context helper driver
  * Added u-blox SARA-R4 modem driver

* Pinmux

  * Added SPI support to STM32MP1
  * Enabled ADC, PWM, I2C, and SPI pins on STM32WB
  * Added Microchip XEC and TI CC13x2 / CC26x2 drivers

* PWM

  * Added NXP PWM driver
  * Added support for STM32G0XX to STM32 driver

* Sensor

  * Added STTS751 temperature sensor driver
  * Added LSM6DSO and LPS22HH drivers
  * Renamed HDC1008 driver to ti_hdc and added support for 1050 version
  * Added LED current, proximity pulse length, ALS, and proximity gain configurations to APDS9960 driver
  * Reworked temperature and acceleration conversions, and added interrupt handling in ADXL362 driver
  * Added BME680 driver and AMS ENS210 drivers

* Serial

  * Added Xilinx ZynqMP PS, LiteUART, and TI CC12x2 / CC26x2 drivers
  * Added support for virtual UARTS over RTT channels
  * Added support for STM32H7 to STM32 driver
  * Removed support for Quark D2000 from QMSI driver
  * Enabled interrupts in LPC driver
  * Implemented ASYNC API in SAM0 driver
  * Added PCI(e) support to NS16550 driver

* SPI

  * Added support for STM32MP1X and STM32WB to STM32 driver
  * Removed support for Quark SE C1000 from DesignWare driver
  * Added TI CC13xx / CC26xx driver
  * Implemented ASYNC API in SAM0 driver

* Timer

  * Added Xilinx ZynqMP PS ttc driver
  * Added support for SMP to ARC V2 driver
  * Added MEC1501 32 KHZ, local APIC timer, and LiteX drivers
  * Replaced native POSIX system timer driver with tickless support
  * Removed default selection of SYSTICK timer for ARM platforms

* USB

  * Added NXP EHCI driver
  * Implemented missing API functions in SAM0 driver

* WiFi

  * Implemented TCP listen/accept and UDP support in eswifi driver

Networking
**********

* Add support for `SOCKS5 proxy <https://en.wikipedia.org/wiki/SOCKS>`__.
  See also `RFC1928 <https://tools.ietf.org/html/rfc1928>`__ for details.
* Add support for 6LoCAN, a 6Lo adaption layer for Controller Area Networks.
* Add support for :ref:`Point-to-Point Protocol (PPP) <ppp>`.
* Add support for UpdateHub which is an end-to-end solution for large scale
  over-the-air update of devices.
  See `UpdateHub.io <https://updatehub.io/>`__ for details.
* Add support to automatically register network socket family.
* Add support for ``getsockname()`` function.
* Add SO_PRIORITY support to ``setsockopt()``
* Add support for VLAN tag stripping.
* Add IEEE 802.15.4 API for ACK configuration.
* Add dispatching support to SocketCAN sockets.
* Add user mode support to PTP clock API.
* Add user mode support to network interface address functions.
* Add AF_NET_MGMT socket address family support. This is for receiving network
  event information in user mode application.
* Add user mode support to ``net_addr_ntop()`` and ``net_addr_pton()``
* Add support for sending network management events when DNS server is added
  or deleted.
* Add LiteEth Ethernet driver.
* Add support for ``sendmsg()`` API.
* Add `civetweb <https://civetweb.github.io/civetweb/>`__ HTTP API support.
* Add LWM2M IPSO Accelerometer, Push Button, On/Off Switch and Buzzer object
  support.
* Add LWM2M Location and Connection Monitoring object support.
* Add network management L4 layer. The L4 management events are used
  when monitoring network connectivity.
* Allow net-mgmt API to pass information length to application.
* Remove network management L1 layer as it was never used.
* By default a network interface is set to UP when the device starts.
  If this is not desired, then it is possible to disable automatic start.
* Allow collecting network packet TX throughput times in the network stack.
  This information can be seen in net-shell.
* net-shell Ping command overhaul.
* Accept UDP packet with missing checksum.
* 6lo compression rework.
* Incoming connection handling refactoring.
* Network interface refactoring.
* IPv6 fragmentation fixes.
* TCP data length fixes if TCP options are present.
* SNTP client updates.
* Trickle timer re-init fixes.
* ``getaddrinfo()`` fixes.
* DHCPv4 fixes.
* LWM2M fixes.
* gPTP fixes.
* MQTT fixes.
* DNS fixes for non-compressed answers.
* mDNS resolver fixes.
* LLMNR resolver fixes.
* Ethernet ARP fixes.
* OpenThread updates and fixes.
* Network device driver enhancements:

  - Ethernet e1000 fixes.
  - Ethernet enc28j60 fixes.
  - Ethernet mcux fixes.
  - Ethernet stellaris fixes.
  - Ethernet gmac fixes.
  - Ethernet stm32 fixes.
  - WiFi eswifi fixes.
  - IEEE 802.15.4 kw41z fixes.
  - IEEE 802.15.4 nrf5 fixes.

Bluetooth
*********

* Host:

  * GATT: Added support for database hashes
  * GATT: Added support for Ready Using Characteristic UUID
  * GATT: Added support for static services
  * GATT: Added support for disabling the dynamic database
  * GATT: Added support for notifying and indicating by UUID
  * GATT: Simplified the bt_gatt_notify_cb() API
  * GATT: Added additional attributes to the Device Information Service
  * GATT: Several protocol and database fixes
  * Settings: Transitioned to new, optimized settings model
  * Settings: Support for custom backends
  * Completed support for directed advertising
  * Completed support for Out-Of-Band (OOB) pairing
  * Added support for fine-grained control of security establishment, including
    forcing a pairing procedure in case of key loss
  * Switched to separate, dedicated pools for discardable events and number of
    completed packets events
  * Extended and improved the Bluetooth shell with several commands
  * BLE qualification up to the 5.1 specification
  * BLE Mesh: Several fixes and improvements

* BLE split software Controller:

  * The split software Controller is now the default
  * Added support for the Data Length Update procedure
  * Improved the ticker packet scheduler for improved conflict resolution
  * Added documentation for the ticker packet scheduler
  * Added support for out-of-tree user-defined commands and events
  * Added support for Zephyr Vendor Specific Commands
  * Added support for user-defined protocols
  * Converted several control procedures to be queuable
  * Nordic: Added support for Controller-based privacy
  * Nordic: Decorrelated address generation from resolution
  * Nordic: Added support for fast encryption setup
  * Nordic: Added support for RSSI
  * Nordic: Added support for low-latency ULL processing of messages
  * Nordic: Added support for the nRF52811 IC BLE radio
  * Nordic: Added support for PA/LNA on Port 1 GPIO pins
  * Nordic: Added support for radio event abort
  * BLE qualification up to the 5.1 specification
  * Several bug fixes

* BLE legacy software Controller:

  * BLE qualification up to the 5.1 specification
  * Multiple control procedures fixes and improvements

Build and Infrastructure
************************

* TBD

Libraries / Subsystems
***********************

* TBD

HALs
****

* TBD

Documentation
*************

* TBD

Tests and Samples
*****************

* TBD

Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.14.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`99999` - issue title
