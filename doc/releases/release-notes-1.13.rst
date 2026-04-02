:orphan:

.. _zephyr_1.13:

Zephyr Kernel 1.13.0
####################

We are pleased to announce the release of Zephyr kernel version 1.13.0.

Major enhancements with this release include:

* Extensible and Pluggable Tracing Support
* Compartmentalized application memory organization
* Logging System Overhaul
* Introduce system calls for BSD socket APIs
* Support for IEEE 802.1AS-2011 generalized Precision Time Protocol (gPTP)
* Link Layer Discovery Protocol (LLDP) TX support
* Support for TLS and DTLS using BSD socket API
* Support for Link Layer Multicast Name Resolution (LLMNR)
* Introduced reworked ADC API and updated Nordic, NXP, Atmel, and
  Synopsys DesignWare drivers
* Support OS driven Power Management framework
* Basic support for Arm TrustZone in Armv8-M

The following sections provide detailed lists of changes by component.

Kernel
******

* Remove kernel event manager, replaced by generic tracing interface
* Enhanced Timeout and Tick handling in kernel
* Compartmentalized application memory organization
* Fix errno access for user mode

Architectures
*************

* arch: arc: improve the reset code
* arch: arc: use a separate stack for exception handling
* arch: arc: refactor the arc stack check support
* arch: arm: stm32: enable instruction and data caches on STM32F7
* arch: arm: implement ARMv8-M MPU driver
* irq: Fix irq_lock api usage
* arch: arm: macro API for defining non-secure entry functions
* arch: arm: allow processor to ignore/recover from faults
* arm: nxp: mpu: Consolidate k64 mpu regions
* arm: Print NXP MPU error information in BusFault dump
* arch: ARM: Change the march used by cortex-m0 and cortex-m0plus
* arch: arm: integrate ARM CMSE with CMake
* arch: arm: basic Arm TrustZone-M functionality for Cortex-M23 and Cortex-M33
* arch: arm: built-in stack protection using Armv8-M SPLIM registers
* arch: arm: API for using TT intrinsics in Secure/Non-Secure Armv8-M firmware
* arch: arm: clean up MPU code for ARM and NXP
* arch: arm: Set Zero Latency IRQ to priority level zero
* arch/arm: Fix locking in __pendsv

Boards & SoC Support
********************

* x86: add SoC configuration for Apollo Lake
* x86: add support for UP Squared (Pentium/Celeron)
* arc: Support Synopsys ARC nSim instruction set simulator
* riscv32: riscv-privilege: Microsemi Mi-V support
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

* adc: Introduced reworked API and updated Nordic, NXP, Atmel, and
  Synopsys DesignWare drivers
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
* Kconfig: Remove redundant 'default n' properties
* cmake: replace PROJECT_SOURCE_DIR with ZEPHYR_BASE
* Kconfig: Switch to improved globbing statements


Libraries / Subsystems
***********************
* Tracing: Basic support SEGGER systemview
* Logging: Introduce a new logging subsystem
* fs/nvs: Improved nvs for larger blocksizes
* subsys: console: Refactor code to allow per-UART "tty" wrapper


HALs
****
* ext/hal: stm32cube: STM32L4: Enable legacy CAN API
* ext: Import Atmel SAMD20 header files from ASF library
* ext: gecko: Add Silabs Gecko SDK for EFR32FG1P SoCs
* drivers: add i.MX I2C driver shim
* hal: stm32f2x: Add HAL for the STM32F2x series
* ext: stm32cube: update stm32l4xx cube version
* ext: stm32cube: update stm32f7xx cube version
* ext: stm32cube: update stm32f4xx cube version
* ext: stm32cube: update stm32f3xx cube version
* ext: stm32cube: update stm32f1xx cube version
* ext: hal: nordic: Update nrfx to version 1.1.0
* net: drivers: wifi: SimpleLink WiFi Offload Driver (wifi_mgmt only)
* ext/hal/nxp/imx: Import the nxp imx6 freertos bsp

Documentation
*************
* Simplified and more maintainable theme applied to documentation.
  Latest and previous four releases regenerated and published to
  https://docs.zephyrproject.org
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
* Enhanced benchmarks to support userspace
* Improve test coverage for the kernel


Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.12.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`9862` - tests/drivers/build_all#test_build_sensors_a_m @ quark_se_c1000_devboard:x86 BUILD failed
* :github:`9857` - tests/cmsis_rtos_v1 - test_signal_events_signalled results in Assertion failure on all targets with PR#9856
* :github:`9840` - doc: potential broken link when referencing latest doc version
* :github:`9833` - Bluetooth Mesh incorrect reference to CONFIG_BT_SETTINGS
* :github:`9788` - update to mbedTLS 2.12.0
* :github:`9786` - arch: xtensa: build failure due to extra #endif
* :github:`9785` - Bluetooth: bt_gatt_service_register() assumes sc delayed work handler is initialized
* :github:`9772` - Test application hangs without any console output on x86/ARC based boards @arduino_101:arc
* :github:`9768` - [Coverity CID :187902] Memory - illegal accesses in /subsys/net/ip/ipv6_fragment.c
* :github:`9766` - [Coverity CID :187904] Integer handling issues in /tests/benchmarks/timing_info/src/semaphore_bench.c
* :github:`9753` - ESP32: Failing to build project
* :github:`9746` - zephyr networking non socket client server, qemu_x86  issue
* :github:`9744` - tests/kernel/mbox/mbox_usage/testcase.yaml#kernel.mailbox crashes on ESP32
* :github:`9727` - Bluetooth: IPSP Sample Doc no match for new path
* :github:`9723` - tests/drivers/adc/adc_api/ fails on sam_e70_xplained
* :github:`9718` - The test suite test_spi (spi_loopback) when built and run on the nrf52832_pca10040 board
* :github:`9701` - Suggestion: Turn warnings into errors in extract_dts_includes.py
* :github:`9689` - Multiple tests are failing on sam_e70_xplained once the cache is enabled
* :github:`9684` - tests/posix/ fails on sam_e70_xplained
* :github:`9683` - Multiple testcases in tests/kernel/mem_protect/mem_protect, tests/kernel/alert, tests/kernel/mem_pool test fails on sam_e70_xplained due to commit c090776
* :github:`9682` - tests/kernel/init: kernel.common.init.verify_bootdelay fails on sam_e70_xplained
* :github:`9680` - tests/mem_slab/mslab, tests/mem_slab/mslab_api and tests/mem_slab/mslab_threadsafe tests are crashing on sam_e70_xplained
* :github:`9677` - tests:cmsis_rtos_v1: test_mutex crashes with bus fault on sam_e70_xplained
* :github:`9676` - benchmark.timing.userspace not working on nrf52840 with v1.13.0-rc1
* :github:`9671` - Zephyr with WNC-M14A2A not compiling
* :github:`9670` - Bluetooth: Mesh: Persistent Storage: AppKey not restored
* :github:`9667` - LwM2M: Writeable parameter /3311/0/5850 doesn't persist write
* :github:`9665` - tests/drivers/watchdog/wdt_basic_api crashes on Quark D2k / SE and ESP32
* :github:`9664` - tests/kernel/threads/thread_apis/kernel.threads.user_mode crases on QEMU-x86
* :github:`9652` - gen_isr_table@mimxrt1050_evk runs failure on R1.13_RC1.
* :github:`9649` - readme of LPCxpresso54114_mo core needs update for R1.13
* :github:`9646` - sanitycheck: crashes after test execution summary report are not caught
* :github:`9644` - [Coverity CID :187817] Error handling issues in /tests/benchmarks/timing_info/src/msg_passing_bench.c
* :github:`9643` - [Coverity CID :187818] Error handling issues in /tests/benchmarks/timing_info/src/msg_passing_bench.c
* :github:`9642` - [Coverity CID :187819] Memory - illegal accesses in /subsys/logging/log_msg.c
* :github:`9641` - [Coverity CID :187820] Memory - illegal accesses in /subsys/bluetooth/host/hci_core.c
* :github:`9640` - [Coverity CID :187821] Memory - illegal accesses in /subsys/bluetooth/host/hci_core.c
* :github:`9639` - [Coverity CID :187822] Null pointer dereferences in /subsys/net/ip/tcp.c
* :github:`9638` - [Coverity CID :187823] Memory - corruptions in /samples/net/coap_server/src/coap-server.c
* :github:`9637` - [Coverity CID :187824] Integer handling issues in /lib/cmsis_rtos_v1/cmsis_thread.c
* :github:`9636` - [Coverity CID :187825] Error handling issues in /subsys/net/ip/udp.c
* :github:`9635` - [Coverity CID :187826] Error handling issues in /tests/benchmarks/timing_info/src/msg_passing_bench.c
* :github:`9634` - [Coverity CID :187827] Null pointer dereferences in /subsys/logging/log_msg.c
* :github:`9633` - [Coverity CID :187828] Error handling issues in /tests/benchmarks/timing_info/src/msg_passing_bench.c
* :github:`9630` - STM32L4: something wrong with GPIO interrupts
* :github:`9623` - tests/net/lib/tls_credentials/ crashed on sam_e70_xplained and frdm_k64f
* :github:`9622` - tests/net/mgmt/ crashed on sam_e70_xplained
* :github:`9621` - tests/net/promiscuous crashed on sam_e70_xplained
* :github:`9619` - tests/net/socket/getaddrinfo/ - crashes on sam_e70_xplained and  frdm_k64f
* :github:`9618` - tests/net/udp/ - MPU fault on sam_e70_xplained
* :github:`9617` - tests/net/websocket/ - passed on QEMUx86 but the target crashed after that
* :github:`9614` - tests/net/socket/ faults on sam_e70_xplained and frdm_k64f
* :github:`9611` - tests/kernel/sched/schedule_api/testcase.yaml#kernel.sched.slice_reset fails on nrf52840_pca10056, sam_e70_xplained, nrf52_pca10040
* :github:`9609` - tests/kernel/mem_protect/stack_random: kernel.memory_protection.stack_random fails on emsk7d_v22
* :github:`9598` - tests/power/power_states fail on arduino101:x86
* :github:`9597` - tests/subsys/fs/fat_fs_api assertion fail on arduino101
* :github:`9591` - @hci.h use of magic-number in bluetooth addr struct (Missing define in @bluetooth.h)
* :github:`9580` - peripheral_hids does not remember bonds
* :github:`9575` - Network NULL pointer reference when enable net/dhcpv4 debug
* :github:`9574` - tests/cmsis_rtos_v1 - test_mutex_lock_timeout results in Assertion failure on all targets with PR#9569
* :github:`9561` - Question: Does it support passing the bootloader(mcuboot) parameter to the kernel(zephyr)?
* :github:`9558` - DTC 1.4.7 breaks at least FRDM_K64F builds
* :github:`9537` - ENC28J60 can't receive packets properly
* :github:`9536` - console: missing kernel.h include in header
* :github:`9535` - broken callback handling in nrfx gpio driver
* :github:`9530` - Bluetooth/gatt: bt_gatt_notify never return -ENOMEM, undocumented return value.
* :github:`9527` - tests/kernel/sched/schedule_api/testcase.yaml#kernel.sched.unlock_preemptible fails on nrf52840_pca10056, sam_e70_xplained, nrf52_pca10040
* :github:`9523` - tests/kernel/mem_protect/stackprot hangs without any console output on nrf51/52
* :github:`9494` - Nordic nrf52810_pca10040 is missing default bluetooth configuration options
* :github:`9487` - tests/cmsis_rtos_v1 - test_kernel_systick results in Assertion failure on nrf51/52
* :github:`9486` - sanitycheck filter rules does not work
* :github:`9471` - soc: efr32fg1p: hello_world sample app hangs when started by MCUboot
* :github:`9470` - LWM2M: TLV encoding of read result is wrong
* :github:`9468` - tests/kernel/mem_pool/mem_pool_concept/testcase.yaml#kernel.memory_pool fails on nrf52840_pca10056, nrf52_pca10040 and  nrf51_pca10028
* :github:`9466` - tests/kernel/context/testcase.yaml#kernel.common.k_sleep fails on nrf52_pca10040 and nrf52840_pca10056
* :github:`9465` - tests/net/ptp/clock: PTP clock test are failing on FRDM_K64f and same_e70_xplained platforms
* :github:`9462` - [Coverity CID :187670] Integer handling issues in /tests/net/ethernet_mgmt/src/main.c
* :github:`9461` - [Coverity CID :187671] Uninitialized variables in /tests/net/iface/src/main.c
* :github:`9460` - [Coverity CID :187672] Uninitialized variables in /tests/net/iface/src/main.c
* :github:`9459` - tests/posix/timer fails on nRF51/52
* :github:`9452` - Error parsing DTS 'compatible' property list
* :github:`9446` - CI didn't report failure due to ARC_INIT issue
* :github:`9444` - sanitycheck not able to run due to CONFIG_ARC_INIT=n
* :github:`9441` - tests/kernel/gen_isr_table fails on mimxrt1050_evk
* :github:`9413` - tests/cmsis_rtos_v1 - test_signal_events_signalled results in Assertion failure on nrf51/52
* :github:`9402` - samples/drivers/watchdog fails on frdm_k64f
* :github:`9396` - ./loop-socat.sh not running
* :github:`9392` - samples/bluetooth/hci_uart ninja flash - UnicodeDecodeError: 'ascii' codec can't decode byte 0xe2 in position 360: ordinal not in range(128)
* :github:`9389` - ESP32 support: setting env var ESP_DEVICE not working
* :github:`9356` - Test tests/crypto/rand32 hangs on nrf51_pca10028
* :github:`9348` - samples: net: echo_client/echo_server does not work with IPv4 qemu_x86
* :github:`9310` - nRF52_PCA10040: Failing test_slice_reset
* :github:`9297` - [Coverity CID :187318] Error handling issues in /tests/posix/pthread_key/src/pthread_key.c
* :github:`9296` - [Coverity CID :187319] Control flow issues in /subsys/net/lib/sockets/sockets.c
* :github:`9295` - [Coverity CID :187320] Control flow issues in /drivers/ethernet/eth_sam_gmac.c
* :github:`9294` - [Coverity CID :187321] Possible Control flow issues in /samples/net/sockets/big_http_download/src/big_http_download.c
* :github:`9293` - [Coverity CID :187322] Incorrect expression in /tests/posix/pthread_key/src/pthread_key.c
* :github:`9292` - [Coverity CID :187323] Control flow issues in /subsys/net/ip/net_if.c
* :github:`9291` - [Coverity CID :187324] Control flow issues in /subsys/net/lib/sockets/sockets.c
* :github:`9287` - net/dhcpv4: Fix single byte buffer filling madness
* :github:`9273` - k_pipe_alloc_init() api is failing on qemu_x86
* :github:`9270` - cmake: kconfig: menuconfig is not writing zephyr/.config
* :github:`9262` - tests/kernel/mem_protect/userspace.access_other_memdomain fails on sam_e70_xplained and nrf52840_pca10056
* :github:`9238` - Get POSIX board compliant with default configuration guidelines
* :github:`9234` - Get ARC boards compliant with default configuration guidelines
* :github:`9224` - sam_e70_xplained fails to build several tests
* :github:`9221` - calloc memory data is not initialized to zero for MINIMAL_LIBC
* :github:`9198` - Out-of-Tree YAML and DTS support
* :github:`9196` - optimize gen_kobject_list.py
* :github:`9160` - net: openthread: Mesh Local IPv6 is not in zephyr stack
* :github:`9148` - samples/net/http_server: Failed to respond back to CURL command on http Client
* :github:`9135` - Failure : "integer overflow in exp" on Altera-Max 10 platform
* :github:`9134` - Build failure with SAM_e70 platform
* :github:`9131` - samples/net/coaps_server: Failed to send response to coaps_client
* :github:`9128` - doc build fails if no reST reference to file
* :github:`9113` - Enabling various thread options causes failures on cortex-M0 boards
* :github:`9108` - Which board is suit with esidon??
* :github:`9098` - Doc build failure not noticed by CI test system
* :github:`9081` - dynamic thread objects do not have a thread ID assigned
* :github:`9067` - Failed tests: posix.sema and posix_checks on em_starterkit_em7d_v22
* :github:`9061` - sanitycheck not printing QEMU console in some cases
* :github:`9058` - Kconfig default on BT_ACL_RX_COUNT can be 1, but range is 2-64
* :github:`9054` - Build failures with mimxrt1050_evk board
* :github:`9044` - "logging: Remove log.h including in headers limitation" breaks logging
* :github:`9032` - net/sockets/echo_async crashes after several connections (qemu_x86)
* :github:`9028` - STM32 SPI/I2S: LSB bit corrupted for the received data
* :github:`9019` - cmsis Include/ version mismatch
* :github:`9006` - Create driver for the MMA8451Q accelerometer sensor on FRDM-KL25Z
* :github:`9002` - [Coverity CID :187063] Control flow issues in /subsys/net/l2/ethernet/ethernet_mgmt.c
* :github:`9001` - [Coverity CID :187064] Control flow issues in /subsys/bluetooth/host/mesh/cfg_srv.c
* :github:`9000` - [Coverity CID :187065] Memory - corruptions in /subsys/net/l2/ethernet/gptp/gptp_mi.c
* :github:`8998` - [Coverity CID :187068] Memory - illegal accesses in /subsys/bluetooth/host/mesh/cfg_srv.c
* :github:`8997` - [Coverity CID :187069] Memory - illegal accesses in /subsys/logging/log_msg.c
* :github:`8996` - [Coverity CID :187070] Control flow issues in /drivers/bluetooth/hci/spi.c
* :github:`8995` - [Coverity CID :187071] Insecure data handling in /subsys/net/l2/ethernet/gptp/gptp_mi.c
* :github:`8994` - [Coverity CID :187072] Error handling issues in /samples/net/sockets/echo_server/src/udp.c
* :github:`8993` - [Coverity CID :187073] Null pointer dereferences in /subsys/net/ip/utils.c
* :github:`8992` - [Coverity CID :187074] Incorrect expression in /samples/net/traffic_class/src/main.c
* :github:`8991` - [Coverity CID :187075] Memory - corruptions in /subsys/net/l2/ethernet/gptp/gptp_mi.c
* :github:`8990` - [Coverity CID :187077] Memory - corruptions in /samples/net/rpl_border_router/src/http.c
* :github:`8989` - [Coverity CID :187078] Control flow issues in /subsys/net/l2/ethernet/gptp/gptp_md.c
* :github:`8988` - [Coverity CID :187079] Integer handling issues in /subsys/net/l2/ethernet/gptp/gptp.c
* :github:`8987` - [Coverity CID :187080] Control flow issues in /subsys/net/l2/ethernet/gptp/gptp_mi.c
* :github:`8982` - tests/drivers/watchdog/wdt_basic_api results in FATAL EXCEPTION on esp32
* :github:`8977` - CMake Error
* :github:`8976` - nordic: watchdog: Cannot be initialized - circular dependency
* :github:`8968` - The tests/kernel/tickless/tickless_concept fails on nRF5x
* :github:`8963` - tests/net/trickle, utils and icmpv6 hangs on sam_e70_xplained:arm
* :github:`8960` - Tcp connection not connecting
* :github:`8950` - ARM fault dumping code does too much, assumes all faults are fatal, and doesn't work under some configurations
* :github:`8949` - nsim_sem board does not work
* :github:`8933` - doc: build WARNING on windows 7
* :github:`8931` - STM32L4 CAN sample project does not compile
* :github:`8924` - Get rid of -fno-strict-overflow
* :github:`8906` - zsock_getaddrinfo is not reentrant
* :github:`8899` - Failed test: kernel.common.timing.sleep on nrf52 (tests/kernel/sleep/kernel.common.timing)
* :github:`8898` - Failed test: kernel.timer.timer_periodicity on nrf51/nrf52
* :github:`8897` - Failed test: kernel.tickless.tickless_slice on nrf51/nrf52
* :github:`8896` - Failed test: kernel.sched.slice_reset and kernel.sched.slice_scheduling (tests/kernel/sched/schedule_api/kernel.sched) on nrf51/nrf52
* :github:`8895` - Failed test: kernel.common.timing.pending on nrf51_pca10028 and nrf52_pca10040 (tests/kernel/pending/kernel.common.timing)
* :github:`8888` - http client example fails on mimxrt1050_evk
* :github:`8887` - Ping command crash on mimxrt1050_evk
* :github:`8871` - drivers: can: Compiling error due to stm23Cube update
* :github:`8866` - Failed test: net.arp.arp (tests/net/arp) on sam_e70_xplained
* :github:`8865` - Failed test: net.udp.udp (tests/net/udp/)  on sam_e70_xplained
* :github:`8864` - ARM MPU _arch_buffer_validate allowing reads to kernel memory
* :github:`8860` - GATT MTU Callback
* :github:`8849` - Allow application to define its own DTS bindings
* :github:`8833` - OpenThread: Minimal Thread Device (MTD) option is not building
* :github:`8829` - BLE "device name" characteristic of Generic Access Service is read only
* :github:`8820` - wifi_winc1500 driver socket id stored in net_context->user_data may be overwritten at socket layer
* :github:`8815` - Nordic: Directly accessing GPIOTE might create unstable firmware (GPIO, PWM, BLE)
* :github:`8800` - cmake errors with menuconfig
* :github:`8798` - k_cycle_get_32() implementation on nrf series is too slow.
* :github:`8791` - Request supporting OTG_HS port on STM32F4/F7 SoCs
* :github:`8790` - K64F/Kinetis: extract_dts_includes.py warnings when building sample
* :github:`8752` - net: ARP is broken after PR #8608
* :github:`8732` - tests/subsys/usb/bos/ fails randomly
* :github:`8727` - Network stack cleanup: DHCPv4
* :github:`8720` - Network stack cleanup: IPv4
* :github:`8717` - posix:  Memory is not returned to mempool when a pthread complete its execution
* :github:`8715` - buffer-overflow in tests/net/tx_timestamp
* :github:`8713` - add DTS gpio support for NRF51
* :github:`8705` - Out of the box error in samples/subsys/nvs with nRF52-PCA10040
* :github:`8700` - [Coverity CID :186841] Null pointer dereferences in /subsys/usb/usb_descriptor.c
* :github:`8699` - [Coverity CID :186842] Memory - illegal accesses in /drivers/interrupt_controller/plic.c
* :github:`8698` - [Coverity CID :186843] Parse warnings in /tests/kernel/mem_protect/mem_protect/src/mem_domain.c
* :github:`8697` - [Coverity CID :186844] Parse warnings in /tests/net/ieee802154/fragment/src/main.c
* :github:`8696` - [Coverity CID :186845] Parse warnings in /tests/net/ieee802154/l2/src/ieee802154_test.c
* :github:`8695` - [Coverity CID :186846] Null pointer dereferences in /tests/net/ptp/clock/src/main.c
* :github:`8694` - [Coverity CID :186847] Parse warnings in /tests/kernel/mem_protect/mem_protect/src/inherit.c
* :github:`8693` - [Coverity CID :186848] Parse warnings in /tests/kernel/mem_protect/mem_protect/src/mem_domain.c
* :github:`8692` - [Coverity CID :186849] Parse warnings in /tests/kernel/mem_protect/mem_protect/src/mem_domain.c
* :github:`8691` - [Coverity CID :186850] Parse warnings in /tests/kernel/mem_protect/mem_protect/src/mem_domain.c
* :github:`8690` - [Coverity CID :186851] Error handling issues in /tests/bluetooth/mesh/src/microbit.c
* :github:`8689` - [Coverity CID :186852] Parse warnings in /tests/kernel/mem_protect/mem_protect/src/mem_domain.c
* :github:`8669` - fault during my timer testing
* :github:`8668` - net: ARP is broken in master (at least) on STM32
* :github:`8658` - tests/net/trickle fails on FRDM k64f
* :github:`8657` - tests/net/ptp fails on QEMU x86
* :github:`8646` - CONFIG_NET_OFFLOAD defined in subsys/net/l2/, but not referenced there
* :github:`8643` - Add SAADC driver for nRF52
* :github:`8642` - ieee802154 tests fail to build
* :github:`8636` - MCUboot firmware update issue
* :github:`8611` - RT1050EVK: MPU FAULT with Zephyr OS v1.12.0-360-gf3d1b22 using ztest
* :github:`8610` - USB: Setup stage in control transfers
* :github:`8605` - mbedtls_ssl_close_notify was called after DTLS context released
* :github:`8602` - master broken for stm32 ARM boards
* :github:`8600` - Not able to bind the adc device structure for nrf52832 controller
* :github:`8598` - [Coverity CID :186057] - Out of bounds write in samples/net/rpl_border_router/src/coap.c
* :github:`8596` - drivers: dma_cavs: NULL pointer exception when DMA start called after DMA stop
* :github:`8593` - samples/mpu/mem_domain_apis_test/kernel.memory_protection.memory_domains fails to build
* :github:`8587` - ZTEST should support multiple calls to mocked function
* :github:`8584` - ToolchainCapabilityDatabase.cmake:93 error in PR #8579
* :github:`8576` - there have a error in doc
* :github:`8567` - Can't parse json
* :github:`8563` - Compilation warning/error on stm32l4: "__weak" redefined
* :github:`8529` - tests/kernel/common/kernel.common fails for native_posix on Ubuntu 16.04
* :github:`8528` - rpl-mesh-qemu sample, the net inface init failed.
* :github:`8511` - nrf52_blenano2 tmp112 sensor sample build failed - redefined I2C
* :github:`8506` - tests/subsys/fs/fat_fs_api - test_fat_mount results into assertion failure on Arduino_101 - FS init failed (-19)
* :github:`8502` - Compiling for native_posix with newlib is missing various math symbols
* :github:`8501` - I think there is a issue about shell.
* :github:`8470` - Broken Arduino 101 Bluetooth Core flashing
* :github:`8466` - k_sleep on mimxrt1050_evk board broken
* :github:`8464` - sdk_version file missing
* :github:`8462` - non-ASCII / non-UTF-8 files in ext/
* :github:`8452` - ieee802154: csma-ca: random backoff factor looks wrong
* :github:`8444` - "make clean" removes include directory
* :github:`8438` - cmake: Propagation of library specific compile flag
* :github:`8434` - Networking Problems, Size Missmatch 15 vs 13
* :github:`8431` - mqtt: unimplemented MQTT_UNSUBACK in mqtt_parser function in mqtt.c file
* :github:`8424` - HID example broken
* :github:`8416` - [Coverity CID :186580] Uninitialized variables in /drivers/can/stm32_can.c
* :github:`8415` - [Coverity CID :186581] Memory - corruptions in /subsys/bluetooth/host/gatt.c
* :github:`8414` - [Coverity CID :186582] Memory - corruptions in /subsys/bluetooth/host/gatt.c
* :github:`8413` - [Coverity CID :186583] Error handling issues in /samples/net/sockets/dumb_http_server/src/socket_dumb_http.c
* :github:`8393` - ``CONFIG_MULTITHREADING=n`` builds call main() with interrupts locked
* :github:`8391` - nrf52_blenano2 tmp112 sensor sample build failed.
* :github:`8390` - bluetooth: request APIs to notify application that pairing is complete or not
* :github:`8388` - Assigning to promptless symbols should have a better error message
* :github:`8385` - Missing documentation on bt_conn_auth_cb(...)
* :github:`8382` - ESP32: add support for ESP-IDF bootloader
* :github:`8380` - cmake: ninja clean tries to remove include folders
* :github:`8378` - subsys: settings: Idea for a very simple settings system
* :github:`8371` - nRF5: enable UARTE peripheral support
* :github:`8367` - fs: nvs: auto restore FS on writing while power down error.
* :github:`8366` - mcumgr: unable to perform 2nd update
* :github:`8365` - mcumgr: improper response to "image list" command after update.
* :github:`8361` - __ASSERT() triggers with ``CONFIG_MULTITHREADING=n``
* :github:`8358` - Flashing Target Device FAIL
* :github:`8357` - bluetooth: request the capability to change gap device name programmatically
* :github:`8356` - Failed test: kernel.common.bitfield (tests/kernel/common) on Altera Max10
* :github:`8355` - CMake prints a spammy warning about "policy CMP0000"
* :github:`8350` - bluetooth: request BLE stack to support pre-set passkey for pairing
* :github:`8334` - nrf52840.dtsi contains "0x" in device label
* :github:`8329` - qustion: build-system: How to generate a preprocess file
* :github:`8327` - CONFIG_SPI_FLASH_W25QXXDV_MAX_DATA_LEN doesn't work in proj.conf
* :github:`8322` - LwM2M: Occasional registration updates fail with 4.4 error
* :github:`8313` - Enable hardware stack checking for ARC em_starterkit_em7d (Secure mode)
* :github:`8311` - tests/benchmarks/sys_kernel fails on frdm_k64f, sam_e70
* :github:`8309` - lpcxpresso54114_m4: when i configed system clock from 48M to 96M the target can't work.
* :github:`8302` - Failed test: peripheral.adc.adc on quark_se
* :github:`8300` - Failed test: kernel.memory_protection.userspace.access_after_revoke (in tests/kernel/mem_protect/userspace)
* :github:`8299` - Failed test: kernel.memory_pool.mpool_alloc_free_isr (in tests/kernel/mem_pool/mem_pool_api)
* :github:`8298` - Failed test: kernel.alert.isr_alert_consumed (in tests/kernel/alert/) on quark_se_c1000_ss
* :github:`8293` - ARM: MPU faults should indicate faulting memory address
* :github:`8292` - Rework ARC exception stack
* :github:`8287` - LwM2M: Cancelling an observation doesn't work
* :github:`8286` - LwM2M: Observe of not allowed value still creates observer
* :github:`8284` - Documentation build on Windows
* :github:`8283` - Failed test: kernel.mailbox.msg_receiver_unlimited (tests/kernel/mbox/mbox_usage/) on ESP32
* :github:`8262` - [Bluetooth] MPU FAULT on sdu_recv
* :github:`8255` - [RFC] Add support for system suspend/resume handling from kernel
* :github:`8252` - GPIO interrupt only called once on nRF52832
* :github:`8240` - ESP32: update to recent ESP-IDF
* :github:`8235` - nxp_lpc54102: how to add lpc54102 support?
* :github:`8231` - GATT Macro Confusion
* :github:`8226` - drivers: can: stm32_can: various issues
* :github:`8225` - Error mbedtls_pk_verify MBEDTLS_ERR_RSA_VERIFY_FAILED
* :github:`8215` - Update watchdog driver sample to new API
* :github:`8210` - Always rebuilding even though there are no changes.
* :github:`8206` - Stray files in libapp.a
* :github:`8203` - Implement system calls for the new socket APIs
* :github:`8199` - Tests: Crypto: rand32 faults on nrf51_pca10028 and nrf52_pca10040
* :github:`8188` - net: TCP: FIN packets aren't queued for retransmission, loss leads to TCP timeout on peer's side
* :github:`8183` - zsock_getaddrinfo() is not reentrant
* :github:`8173` - Driver tests failing with an assertion on frdm_k64f
* :github:`8138` - Unsatisfactory kernel benchmark results on SAM E-70 Xplained
* :github:`8128` - scheduler: threads using k_sleep can be _swap()'d back too early
* :github:`8125` - About BMI160 reading issue.
* :github:`8090` - tests/sched/schedule_api fails to build on EMSK7d
* :github:`8041` - arm: NXP MPU does not report faulting address for Stacking Errors
* :github:`8039` - tests/shell failing on Arduino 101 / Quark SE arc
* :github:`8026` - Verify TLS server side operation
* :github:`8019` - ARP: should drop any packet pended when timeout
* :github:`8013` - Open-AMP power on can not communicate
* :github:`7999` - HCI UART with Linux host cannot connect to nrf52 6lowpan peripheral
* :github:`7978` - SSE and SSE_FP_MATH are set on frdm_k64f, which doesn't have it, triggering Kconfig warnings
* :github:`7977` - ARC_INIT is set on boards that don't have it, triggering Kconfig warnings
* :github:`7966` - Move k_thread_foreach() tests to tests/kernel/threads
* :github:`7924` - mcu_mgmt: Memory corruption (cborattr suspected) - test case with smp_svr
* :github:`7906` - tests/benchmarks/sys_kernel fails on Arduino Due
* :github:`7884` - tests/power/power_states never completes on Arduino 101's arc core
* :github:`7882` - tests/dfu/mcuboot.test_bank_erase fails on nrf52840_pca10056
* :github:`7869` - Improve Zero Latency IRQ on ARM
* :github:`7848` - CONFIG_BMM150_SET_ATTR not defined (and now removed), giving dead code
* :github:`7800` - ext/lib/mgmt/mcumgr/cmd/log_mgmt/Kconfig references MDLOG, but MDLOG doesn't exist
* :github:`7758` - sanitycheck error with --coverage
* :github:`7705` - nxp_kinetis/k6x boot MPU regions are configured incorrectly
* :github:`7703` - NUM_KERNEL_OBJECT_FILES is too small
* :github:`7685` - API for 802.1Qav parameters configuration
* :github:`7678` - Unstable ping RTT with ethernet ipv4 networking
* :github:`7658` - [RFC] net: Split off net_app_settings lib to a separate directory under subsys/net/lib/
* :github:`7596` - API to communicate list of MAC addresses to the Ethernet controller
* :github:`7595` - Promiscuous mode and receiving all packets at applications level
* :github:`7571` - IP stack can't recover from a packet overload
* :github:`7570` - usb: update bcdUSB to 2.00
* :github:`7553` - DHCP client does not notice missing link
* :github:`7509` - [Coverity CID :185398] Memory - corruptions in /samples/net/mbedtls_sslclient/src/mini_client.c
* :github:`7502` - samples/mbedtls_sslclient: Discards TLS records, handshake does not work
* :github:`7473` - Bluetooth: Support for multiple local identity addresses
* :github:`7423` - samples: net: echo_client: sample runs failed with prj_qemu_x86_tls.conf configuration file
* :github:`7384` - ARM MPU region configuration possibly out of bounds
* :github:`7372` - Create socket options for certificates and ciphers
* :github:`7371` - Move TLS connection data out from net_context
* :github:`7370` - Add Kconfig options to handle certificates and ciphers.
* :github:`7367` - Doxygen warnings about device.h macros
* :github:`7314` - Generate SPDX TagValue document as part of 1.13 release
* :github:`7310` - Provide signed Zephyr releases
* :github:`7243` - BLE DTM ll_test does not set correct TXPower
* :github:`7230` - The guidelines for whether something should be in DTS or Kconfig are too vague
* :github:`7173` - Difference between the ZEPHYR_BASE and PROJECT_SOURCE_DIR CMake variables is unclear
* :github:`7145` - Configuration file for Cross Toolchain on macOS
* :github:`7112` - ARMv8-M: API for checking permissions using ARMv8-M TT intrinsics
* :github:`7106` - tests: obj_tracing: Test fails on ESP32, semaphore count is more than what is created in the application
* :github:`7042` - Ethernet network management interface additions for MAC filtering
* :github:`6982` - STM32F746G DISCOVERY board support
* :github:`6981` - STM32F7 series MCUs support
* :github:`6866` - build: requirements: No module named yaml and elftools
* :github:`6846` - need console subsystem abstraction for console syscalls
* :github:`6785` - Fail to compile when OT l2 debug is enabled.
* :github:`6778` - Push latest docs down into a "latest" folder
* :github:`6775` - Simplify left nav index on technical docs
* :github:`6749` - kconfig: The error message is misleading when values are out-of-range
* :github:`6730` - ARMv8-M: internal low-level (TrustZone) API & implementation for configuring IRQ target
* :github:`6727` - k_mem_pool crash with larger values of n_max
* :github:`6681` - [Coverity CID: 183051] Error handling issues in /tests/benchmarks/app_kernel/src/memmap_b.c
* :github:`6678` - [Coverity CID: 183054] Memory - corruptions in /tests/lib/c_lib/src/main.c
* :github:`6676` - [Coverity CID: 183056] Memory - corruptions in /tests/kernel/common/src/atomic.c
* :github:`6673` - [Coverity CID: 183059] Memory - corruptions in /samples/net/mbedtls_dtlsclient/src/dtls_client.c
* :github:`6593` - Allow configuring the USB serial number string in runtime
* :github:`6533` - 1.12 Release Checklist
* :github:`6522` - Should have a "dumb" O(N) scheduler
* :github:`6514` - samples/drivers/i2c_fujitsu_fram: Data comparison on data written and data read fails randomly
* :github:`6399` - How to using the PPI chanels from 20-31 in Nrf5 chip?
* :github:`6373` - ARMv8-M: Implement stack limit checking for Secure/Non-secure stack pointers
* :github:`6188` - doc: Merge non-apache contributing into CONTRIBUTING
* :github:`6132` - [RFC] Restructuring and cleanup of mbedTLS configurations
* :github:`5980` - NRF5 I2C standard speed 250kHz
* :github:`5939` - NRF5 I2C (TWI) driver
* :github:`5900` - net: Prototype a TLS convenience API based on sockets
* :github:`5896` - Accidentally using MSYS's python from native windows leads to obscure error messages
* :github:`5833` - Script to import mcux sdk
* :github:`5733` - single threaded applications fail when asserts are enabled
* :github:`5732` - sanitycheck fails with gcc 7 as the host compiler
* :github:`5725` - Ninja: Running sanitycheck has byproducts outside of sanity-out
* :github:`5723` - cmake: Accept CONFIG_XX overrides from command line
* :github:`5524` - reorg documentation structure on website (docs.zephyrproject.org)
* :github:`5445` - Shadowed declarations in bluetooth stack
* :github:`5371` - [Coverity CID: 180698] Null pointer dereferences in /tests/bluetooth/tester/src/gatt.c
* :github:`5366` - Document zephyr-app-commands usage
* :github:`5357` - CII Badge: Generate list of externally maintained dependencies
* :github:`5153` - [RFC] Discussion of "cmake" vs "make" variables, aka "build environment" vs "work environment" setup
* :github:`5132` - Soft real-time "tasklets" in kernel
* :github:`4963` - Convert NIOS2 boards to device tree
* :github:`4957` - Add build targets for each explicit debug/flash option
* :github:`4883` - Should command line examples be "cut and paste" ready?
* :github:`4829` - device tree: gpio
* :github:`4767` - USB: assign endpoints at runtime
* :github:`4762` - [nrf][power][Sample] nrf52 exits from Low Power Mode immedately
* :github:`4590` - [CID: 178238] Parse warnings in samples/mpu/mem_domain_apis_test/src/main.c
* :github:`4283` - kconfig warning are being ignored by sanitycheck
* :github:`4060` - net: NET_CONTEXT_SYNC_RECV relevant
* :github:`4047` - [nrf] nrf GPIO does not have sense configuration value
* :github:`4018` -  zephyr.git/tests/net/mld/testcase.yaml#test  :evalution failed
* :github:`3995` - net tcp retry triggers assert in kernel/sem.c:145
* :github:`3993` - Enabling  Low Power Mode on nordic based platforms(nrf52/51)
* :github:`3980` - Remove adc_enable/adc_disable functions
* :github:`3947` - multiple build failures with XCC toolchain
* :github:`3935` - Bluetooth sample setup docs mentions unknown "btproxy" tool
* :github:`3903` - Static code scan (coverity) issues seen
* :github:`3845` - Enable Sphinx option doc_role='any' for improved reference linking
* :github:`3826` - RISCV32 {__irq_wrapper} exception handling error under compressed instruction mode?
* :github:`3770` - mbedtls build error when CONFIG_DEBUG=y
* :github:`3754` - Support static BT MAC address
* :github:`3666` - schedule_api test uses zassert without cleaning up properly
* :github:`3631` - program text should be in its own memory region
* :github:`3602` - power_mgr and power_states: need build option to keep the app exiting in "active" state
* :github:`3583` - NUCLEO-L073RZ/NUCLEO-L053R8 Dev Board Support
* :github:`3458` - Port Zephyr to Silabs EFM32ZG-STK3200
* :github:`3395` - Provide a sample app that demonstrates VLANs
* :github:`3394` - Support basic VLAN tags
* :github:`3393` - VLAN: Expose through virtual network interfaces
* :github:`3377` - Missing le_param_updated callback when conn param update request fails
* :github:`3363` - Missing board documentation for nios2/qemu_nois2
* :github:`3354` - Missing board documentation for x86/se_c1000_devboard
* :github:`3263` - improve Galileo flashing process
* :github:`3233` - LLDP Transmitting Agent
* :github:`3222` - No negative response if remote enabled encryption too soon
* :github:`3221` - re-pairing with no-bond legacy pairing results in using all zeros LTK
* :github:`3187` - frdm_k64f: Ethernet networking starts to respond ~10s after boot
* :github:`3173` - k_cpu_atomic_idle failed @ARM
* :github:`3150` - Si1153 Ambient Light Sensor, Proximity, and Gesture detector support
* :github:`3149` - Add support for ADXRS290
* :github:`3073` - Add Atmel SAM family DAC (DACC) driver
* :github:`3071` - Add Atmel SAM family Timer Counter (TC) driver
* :github:`3067` - Support Precision Time Protocol (PTP)
* :github:`3056` - arch-specific inline functions cannot manipulate _kernel
* :github:`3025` - Implement _tsc_read equivalent for NiosII
* :github:`3024` - Implement _tsc_read equivalent for ARM
* :github:`3007` - Provide board documentation for all boards
* :github:`2991` - Enable NXP Cortex-M SoCs with MCUXpresso SDK
* :github:`2975` - add arc nSIM simulator build target
* :github:`2972` - extend sanitycheck to support ARC simulator
* :github:`2956` - I2C Slave Driver
* :github:`2954` - nRF5x interrupt-driven TX UART driver
* :github:`2952` - ADC: ADC fails to work when fetch multiple sequence entries
* :github:`2934` - Ecosystem and Tool Support
* :github:`2879` - ARC: Interrupt latency too large
* :github:`2645` - create DRAM_BASE_ADDRESS and SIZE config parameters
* :github:`2623` - nRF52 UART behaviour sensitive to timing of baud rate initialization.
* :github:`2568` - Have the kernel give the leftover memory to the IP stack
* :github:`2422` - O(1) pend queue support
* :github:`2353` - nRF5x: Refactor gpio_nrf5.c to use the MDK headers
* :github:`1678` - support edge/pulse interrupts on ARC
* :github:`1662` - Problem sourcing the project environment file from zsh
* :github:`1600` - Could you give me BTP upper tester demo which can work on PC
* :github:`1464` - SYS_CLOCK_HW_CYCLES_PER_SEC is missing a default value
