:orphan:

.. _zephyr_1.12:

Zephyr Kernel 1.12.0 (DRAFT)
############################

We are pleased to announce the release of Zephyr kernel version 1.12.0.

Major enhancements with this release include:

- 802.1Q - Virtual Local Area Network (VLAN) traffic on an Ethernet network
- Support multiple concurrent filesystem devices, partitions, and FS types
- Support for TAP net device on the the native POSIX port
- SPI slave support
- Runtime non-volatile configuration data storage system


The following sections provide detailed lists of changes by component.

Kernel
******

* Added k_thread_foreach API

Architectures
*************

* nxp_imx/mcimx7_m4: Added support for i.MX7 Cortex M4 core

Boards
******

* Added support for the following Arm boards:

  * 96b_argonkey
  * adafruit_feather_m0_basic_proto
  * colibri_imx7d_m4
  * dragino_lsn50
  * lpcxpresso54114_m0
  * nrf51_ble400
  * nrf52_pca20020
  * nucleo_f070rb
  * nucleo_f446re
  * nucleo_l053r8
  * nucleo_l073rzA
  * olimex_stm32_h407
  * stm32f0_disco

* Added support for the following RISC-V boards:

  * hifive1

* Added support for the following Xtensa boards:

  * intel_s1000_crb

* arc: Added device tree support for all ARC SoCs
* arm: Renamed lpcxpresso54114 to lpcxpresso54114_m4
* nios2: Added device tree support for qemu_nios2 and altera_max10
* Continued adding dts support for device drivers (gpio, spi, i2c, sensors, usb)

Drivers and Sensors
*******************

* can: Added CAN driver support for STM32 SoCs
* display: Added ILI9340 LCD display driver
* dma: Added dma driver for Nios-II MSGDMA core
* dma: Introduce Intel CAVS DMA
* ethernet: Added ethernet driver for native posix arch
* gpio: Added support for i.MX GPIO
* gpio: Added driver for SX1509B
* gpio: Added GPIO for SAM family
* gpio: Added GPIO driver for stm32l0x
* i2s: Introduce CAVS I2S
* ieee802154: Added OpenThread modifications to KW41Z driver
* interrupts: introduce CAVS interrupt logic
* interrupts: Introduce Designware interrupt controller
* ipm: Added mcux ipm driver for LPC SoCs
* led: Added new public API and driver support for TI LP3943
* pinmux: Added pinmux driver for stm32l0x
* rtc: Added mcux RTC driver for Kinetis SoCs
* sensor: Added sensorhub support to lsm6dsl driver
* sensor: Added trigger support to lsm6dsl
* serial: Added support for i.MX UART interface
* spi: Added shims for nrfx SPIS and SPIM drivers
* spi: Updated mcux shim driver to new SPI API
* spi: Updated sensor and radio drivers to new SPI API
* usb: Added usb device driver for Kinetis USBFSOTG controller
* usb: Added usb support for stml072/73, stm32f070/72
* usb: Enable usb2.0 on intel_s1000
* usb: Added nRF52840 USB Device Controller Driver
* watchdog: Added mcux watchdog driver for Kinetis SoCs
* watchdog: Added nrfx watchdog driver for NRF SoCs
* wifi: Added winc1500 WiFi driver

Networking
**********

* Minimal server side websocket support.
* Add network support to syslog.
* Reducing net_pkt RAM usage.
* TCP code refactoring. TCP code is now in one place in tcp.c
* Support MSG_DONTWAIT and MSG_PEEK in recvfrom() socket call.
* Support MSG_DONTWAIT in sendto() socket call.
* Add support for freeaddrinfo() API.
* Allow empty service in getaddrinfo() API.
* Add PRIORITY support to net_context. This is working same way as SO_PRIORITY
  in BSD sockets API.
* Add network traffic classification support to Rx and Tx paths. This allows
  priorization of incoming or outgoing network traffic. Both Rx and Tx can
  have max 8 network queues.
* Add network interface up/down command to net-shell.
* Create ethernet driver for native_posix board. The driver is enabled
  automatically if networking is active when compiling for native_posix board.
* Support network packet checksum calculation offloading. This is available for
  ethernet based boards.
* Add support for ethernet virtual LANs (VLAN). Following ethernet drivers
  support VLANs: frdm_k64f, sam_e70_explained, native_posix and qemu.
* Allow network statistics collection / network interface.
* Add network management support to ethernet sub-system.
* Add network capabilities support to ethernet network drivers. This is used
  for management purposes.
* Allow collection of ethernet statistics. Currently only native_posix ethernet
  driver supports this.
* Add OpenThread support for KW41Z driver.
* Add initial WiFi management API definitions.
* Add a shell module for controlling WiFi devices.
* Add dedicated net mgmt hooks for WiFi offload devices.
* Use proper IPv4 source address when sending IPv4 packets.
* Add support for energy detection scan on IEEE 802.15.4 driver API.
* Add support for filtering source short IEEE 802.15.4 addresses.
* Add RPL border router sample application.
* LWM2M code refactoring.
* LWM2M OPTIONAL resource fixes.
* LWM2M source port fixes.
* LWM2M resource usage enhancements.
* Fixing network management event ordering.
* Fix ENC28J70 ethernet driver.
* CoAP sample application fixes.
* Network timeout fixes.
* ICMPv6 error check fixes.
* Net-app API port number fixes.
* WPAN USB driver and sample application fixes.
* BSD socket sample application fixes.
* Fix IPv4 echo-request (ping) in net-shell when having multiple network
  interfaces.
* Fixing IPv6 compile error in certain configuration.

Bluetooth
*********


Build and Infrastructure
************************

* kconfig: Drop support for CONFIG_TOOLCHAIN_VARIANT
* kconfig: Remove the C Kconfig implementation
* scripts: kconfig: Add a Python menuconfig implementation
* scripts: west: introduce common runner configuration
* scripts: debug, debugserver and flash scripts for intel_s1000
* xtensa: provide XCC compiler support for Xtensa

Libraries / Subsystems
***********************

* subsys/disk: Added support for multiple disk interfaces
* subsys/fs: Added support for multiple instances of filesystem
* subsys/fs: Added Virtual File system Switch (VFS) support
* lib/posix: Added POSIX Mutex support
* lib/posix: Added POSIX semaphore support
* crypto: Updated mbedTLS to 2.9.0
* Imported libmetal and OpenAMP for IPC

HALs
****

* altera: Add modular Scatter-Gather DMA HAL driver
* atmel: Added winc1500 driver from Atmel
* cmsis: Update ARM CMSIS headers to version 5.3.0
* nordic: Import SVD files for nRF5 SoCs
* nordic: Update nrfx to version 1.0.0
* nxp: imported i.MX7 FreeRTOS HAL
* nxp: Added dual core startup code for lpc54114 based on mcux 2.3.0
* stm32l0x: Add HAL for the STM32L0x series

Documentation
*************


Tests and Samples
*****************
* Added test for POSIX mutex
* Added Apple iBeacon sample application

Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.11.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title
