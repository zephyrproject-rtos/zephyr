:orphan:

.. _zephyr_1.13:

Zephyr Kernel 1.13.0 (DRAFT)
############################

We are pleased to announce the release of Zephyr kernel version 1.13.0.

Major enhancements with this release include:

-

The following sections provide detailed lists of changes by component.

Kernel
******

* Detail

Architectures
*************

* arch: arm: stm32: enable instruction and data caches on STM32F7
* arch: arm: implement ARMv8-M MPU driver
* irq: Fix irq_lock api usage
* arch: arm: macro API for defining non-secure entry functions
* arch: arm: allow processor to ignore/recover from faults
* arm: nxp: mpu: Consolidate k64 mpu regions
* arm: Print NXP MPU error information in BusFault dump
* arch: ARM: Change the march used by cortex-m0 and cortex-m0plus
* arch: arm: clean up MPU code for ARM and NXP
* arch: arm: Set Zero Latency IRQ to priority level zero
* arch/arm: Fix locking in __pendsv

Boards
******

* Added support for the following Arm boards:

  * efr32_slwstk6061a
  * nrf52_adafruit_feather
  * nrf52810_pca10040
  * nrf52840_pca10059
  * nucleo_f207zg
  * reel_board
  * stm32f723e_disco
  * stm32f746g_disco
  * stm32f769i_disco
  * udoo_neo_full_m4
  * warp7_m4

Drivers and Sensors
*******************

* adc: Introduced reworked API and updated Nordic, NXP, Atmel, and Designware
  drivers
* audio: Added TLV320DAC310x audio DAC driver
* can: Added can support for STM32L432
* clock_control: Added STM32F7 family clock control
* entropy: Added support for STM32F7
* eth: Enabled gPTP support in mcux and gmac drivers
* eth: Added promiscuous mode support to native_posix
* eth: mcux: Added an option for randomized, but stable MAC address
* gpio: Added STM32F7 GPIO support
* interrupt_controller: Added STM32F7 EXTI support
* i2c: Added support for STM32F7
* i2c: Added i.MX shim driver
* i2c: Implemented slave support for stm32_v2
* i2c: Added EEPROM I2C slave driver
* i2c: Added shims for nrfx TWI and TWIM drivers
* i2s: Exposed i2s APIs to user mode
* led: Added TI LP5562 and NXP PCA9633 drivers
* modem: Added Wistron WNC-M14A2A LTE-M Modem driver
* modem: Added modem receiver (tty) driver
* pinmux: Added STM32F7 pinmux support
* pwm: Added i.MX shim driver
* pwm: Added shim for nrfx PWM HW driver
* serial: Added power management to nRF UART driver
* serial: Added STM32F7 UART support
* serial: Allow to pass arbitrary user data to irq callback
* serial: Added UARTE driver for the nRFx family
* sensor: Added adxl372, mma8451q, adt7420 drivers
* sensor: lis2dh: Fix I2C burst read/write operations
* rtc: Added support for STM32
* usb: Added support for OTG FS on STM32F2 and STM32F7
* usb: Added High Speed support for DesignWare USB
* wifi: Added SimpleLink WiFi Offload Driver (wifi_mgmt only)

Networking
**********

* Introduce system calls for BSD socket APIs.
* Add IPv4 autoconf support. This adds support for IPv4 link-local addresses
  (169.254.*.*)
* Add TLS and DTLS support to BSD socket API. They are configured via
  setsockopt() API.
* Add support for IEEE 802.1AS-2011 generalized Precision Time Protocol (gPTP)
  for ethernet networks. A sample application is created to show how to interact
  with gPTP code.
* Add support for PTP clock driver. This driver will be used by gPTP supported
  ethernet drivers.
* Add Link Layer Discovery Protocol (LLDP) TX support.
* Add support for managing Qav credit-based shaper algorithm.
* Add generic TX timestamping support.
* Add carrier detection support to ethernet L2 driver.
* Add support for having vendor specific ethernet statistics.
* Add getter support to ethernet management interface.
* Add promiscuous mode support to network interface. A sample application is
  created that shows how to use the user API for getting all network packets.
  The native_posix ethernet driver supports promiscuous mode at this point.
* Add support for Link Layer Multicast Name Resolution (LLMNR). LLMNR is used in
  Microsoft Windows networks for local name resolution.
* Add API to net_pkt to prefill a network packet to a pre-defined value.
* Add IEEE 802.1Qav support to Atmel GMAC ethernet driver.
* Add hardware TX timestamping support to Atmel GMAC ethernet driver.
* Add multiple hardware queue support to Atmel GMAC ethernet driver.
* Add gPTP support to Atmel GMAC ethernet driver.
* Add support for TI SimpleLink WiFI offload driver.
* Add support for randomized but stable MAC address in NXP MCUX ethernet driver.
* Add extra prints to net-shell for ethernet based network interfaces. The
  supported features and priority queue information is printed.
* Add and fix string to integer conversions in net-shell.
* Allow user to configure MAC address filters into ethernet devices.
* Catch network interface ON and OFF events in DHCPv4 and renew address lease if
  we re-connected.
* Remove forever timeouts when waiting a new network buffer to be available.
* Relay network interface up/down command from net-shell to Linux host for
  native_posix ethernet driver.
* No need to join IPv6 solicited node multicast group for Bluetooth IPSP
  supported nodes.
* Allow external program to be started for native_posix ethernet driver. This
  allows for example startup of wireshark when zeth is created.
* Network packet priority and traffic class fixes and clarifications.
* Lower memory consumption in net by using packed enums when applicable.
* Correctly notify net_app server when TCP is disconnected.
* Register OpenThread used unicast and multicast IPv6 addresses for network
  interface.
* Enable Fast Connect policy for TI SimpleLink ethernet driver.
* Fix ieee802154 simulator driver channel/tx power settings.
* Handle large IPv6 packets properly.
* Enable gPTP support in native_posix, NXP mcux and Atmel GMAC ethernet drivers.
  The native_posix ethernet driver gPTP support is only for testing purposes.
* Network configuration (net_config) library split from the net_app library.
  (This change requires updating application configs to refer to corresponding
  NET_CONFIG_* options instead of NET_APP_*).
* Moving all layer 2 (L2) network code into subsys/net/l2 directory.
* Add MSS option on sending TCP SYN request.
* Fix TCP by processing zero window probes when our receive window is 0.
* IPv4, IPv6, ICMPv6, ARP code refactoring and cleanup.
* IPv6 address lifetime fixes.
* IPv6 fragmentation fixes.
* ARP fixes when using VLAN.
* Timeout too long lasting ARP requests.
* DHCPv4 fixes and timeout management refactoring.
* TCP retry, RST packet handling, and memory leak fixes.
* IP address print function enhancements.
* HTTP fix when sending the last chunk.
* MQTT fixes.
* LWM2M cleanups and fixes.
* Fix cache support in Atmel GMAC ethernet driver.
* Fix NXP MCUX ethernet driver to detect carrier lost event.
* Port native API echo-server/echo-client samples to BSD sockets API, with
  TLS/DTLS support.
* Handle out-of-buf situations gracefully in echo-client and echo-server sample
  applications.

Bluetooth
*********

* New user-friendly service population using a refreshed BT_GATT_CHARACTERISTIC
  macro.
* Added support for Bluetooth hardware in the native_posix board, allowing
  developers to use the native POSIX architecture with Bluetooth.
* Added a new helper API to parse advertising data.
* Added a new flag, BT_LE_ADV_OPT_USE_NAME, to include the Bluetooth Device
  Name in the advertising data.
* Added support for fixed passkeys to use in bonding procedures.
* Added a new Bluetooth shell command to send arbitrary HCI commands to the
  controller.
* Added a new feature to support multiple local identities using a single
  controller.
* Added a new, board-specific mesh sample for the nRF52x series that
  implements the following models:

  - Generic OnOff client and server.
  - Generic Level client and server.
  - Generic Power OnOff client and server.
  - Light Lightness client and server.
  - Light CTL client and server.
  - Vendor Model.
* Controller: Added a TX Power Kconfig option.
* Controller: Use the newly available nrfx utility functions to access the
  nRF5x hardware.
* Controller: Multiple bug fixes.
* Controller: Added support for the nRF52810 SoC from Nordic Semiconductor.
* New HCI driver quirks API to support controllers that need uncommon reset
  sequences.
* Host: Multiple bug fixes for GATT and SMP.
* Mesh: Multiple bug fixes.

Build and Infrastructure
************************


Libraries / Subsystems
***********************


HALs
****


Documentation
*************

* Simplified and more maintainable theme applied to documentation.
  Latest and previous four releases regenerated and published to
  http://docs.zephyrproject.org
* Updated contributing guidelines
* General organization cleanup and spell check on docs including content
  generated from Kconfig files and doxygen API comments.
* General improvements to documentation following code,
  implementation changes, and in support of new features, boards, and
  samples.
* Documentation generation now supported on Windows host systems
  (previously only linux doc generation was supported).
* PDF version of documentation can now be created


Tests and Samples
*****************


Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.12.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title
