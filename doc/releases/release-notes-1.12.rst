:orphan:

.. _zephyr_1.12:

Zephyr Kernel 1.12.0
####################

We are pleased to announce the release of Zephyr kernel version 1.12.0.

Major enhancements with this release include:

- Asymmetric multiprocessing (AMP) via integration of OpenAMP
- Persistent storage support for Bluetooth Low Energy including Mesh
- 802.1Q - Virtual Local Area Network (VLAN) traffic on an Ethernet network
- Support multiple concurrent filesystem devices, partitions, and FS types
- Ethernet network management interface
- Networking traffic prioritization on a per-connection basis
- Support for Ethernet statistical counters
- Support for TAP net device on the native POSIX port
- Command-line Zephyr meta-tool "west"
- SPI slave support
- Runtime non-volatile configuration data storage system (settings)


The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

* A suitably sized k_malloc() request can result in a smaller than
  requested buffer.  Use of that buffer could result in writes to
  unallocated memory.  Proper overflow checks were added to fix this
  issue in k_malloc and k_calloc.

  * kernel: mempool: Check for overflow in k_malloc()
  * kernel: mempool: Always check for overflow in k_calloc()
  * tests: mempool: Add overflow checks

Kernel
******

* Added k_thread_foreach API
* kernel/sched: Fix preemption logic
* kernel/sched: Fix SMP scheduling
* kernel/sched: Don't preempt cooperative threads
* kernel: Scheduler rewrite
* kernel: Fix sloppy wait queue API
* kernel/mempool: Handle transient failure condition
* kernel: handle early entropy issues
* kernel: Earliest-deadline-first scheduling policy
* kernel: Add "meta IRQ" thread priorities
* kernel: introduce initial stack randomization
* k_poll: expose to user mode
* k_queue: allow user mode access via allocators
* mempool: add API for malloc semantics
* userspace: add support for dynamic kernel objects

Architectures
*************

* arch: arc: refactor the arc stack check support
* arch: arc: add the support of STACK_SENTINEL
* arch: arc: optimize the _SysFatalErrorHandler
* arch: arc: bug fixes in irq_load
* arch: arc: bug fixes and optimization in exception handling
* arch: arm: Fix zero interrupt latency priority level
* arch: arm: refactor default _FaultDump to provide fatal error code
* arch: arm: Define & implement API for test target (Non-Secure)
* arch: arm: thread built-in stack guard implementation
* arch: arm: lpc: Added support for Cortex-M0+ on lpc54114 soc
* arch: arm: Secure fault handling for Cortex-M23
* arch: arm: SecureFault Handling for Cortex-M33A
* arch: arm: Change method of __swap processing
* arm_mpu: reduce boot MPU regions for various soc
* arm: userspace: fix initial user sp location
* arm: userspace: Rework system call arguments
* arm: syscalls: fix some register issues
* dts: nios2-qemu: add device tree support
* dts: nios2f: Add device tree support
* dts: x86: derive RAM and ROM size from dts instead of Kconfig
* dts: xtensa: Add device tree support for xtensa
* newlib: fix heap user mode access for MPU devices
* nxp_imx/mcimx7_m4: Added support for i.MX7 Cortex M4 core
* x86: minnowboard: Enable the userspace mode
* arch: x86: Unwind the stack on fatal errors
* xtensa: provide XCC compiler support for Xtensa

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
  prioritization of incoming or outgoing network traffic. Both Rx and Tx can
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

* settings-based persistent storage functionality for BLE (including CCC) and
  Mesh
* Mesh-specific optimizations to avoid flash wear
* Added a new API to set the identity address from the application
* Old bt_storage API removed from the codebase
* Rewrote the HCI SPI driver to comply with the new API
* Added BLE support for the standard entropy driver via an ISR-friendly call
* Multiple BLE Mesh bugfixes and improvements
* Added option to use the identity address for advertising even when using
  privacy
* Added support for L2CAP dynamically allocated PSM values
* GATT CCC handling fixes
* GATT attribute declaration macros reworked for clarity
* Fixed handlng of connection cancellation in the controller
* Fixed a potential assertion failure in the controller related to white list
  handling

Build and Infrastructure
************************

* build: use git version and hash for boot banner
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

* Added description for kernel test cases through extensive doxygen comments
* Discovered some API docs were missing, and fixed
* Documentation added covering system calls and userspace, kernel, and
  threading APIs, POSIX compability, VLANs, network traffic
  classification, and the sanitycheck script used by CI.
* Documented writing guidelines and local doc generation process
* Improved Sphinx search results output (removed markup)
* Improved configuration options auto-generated documentation
* Significantly reduced local doc regeneration time

Tests and Samples
*****************
* Added test for POSIX mutex
* Added Apple iBeacon sample application
* Enhanced threads test suite
* Added tests for memory domain

Issue Related Items
*******************

These GitHub issues were closed since the previous 1.11.0 tagged release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`1420` - LXR for Zephyr
* :github:`1582` - USB: Add support for MS OS Descriptors
* :github:`1620` - BT 4.2 Controller-based link-layer privacy
* :github:`1651` - ARC: remove tickless idle dependency on SW ISR table
* :github:`1655` - clean up how internal APIs are used
* :github:`1799` - Provide an interface for cpu/soc id and version
* :github:`1882` - SMP - Multi-core
* :github:`1896` - Thread Protocol
* :github:`2001` - Add support in I2C shim driver for DMA transfer mode
* :github:`2002` - Add support in SPI shim driver for DMA transfer mode
* :github:`2341` - Thread requirements in RFC1122
* :github:`2342` - Thread requirements in RFC2460
* :github:`2343` - Thread requirements in RFC4291
* :github:`2344` - Thread requirements in RFC4443
* :github:`2345` - Thread requirements in RFC4944
* :github:`2346` - Thread Requirements on RFC6282
* :github:`2360` - Review ALL device driver APIs and enhance them to support variety of devices and MCUs
* :github:`2410` - Create APIs for app to create and mount FS
* :github:`2446` - Different address used for advertising in non-connectable mode from oob data
* :github:`2452` - Add framework for provisioning-style device configuration
* :github:`2529` - GPIO API Update
* :github:`2593` - Consider subdividing applications into different categories
* :github:`2613` - Extend USB stack so it covers multiple device classes switchable on runtime
* :github:`2654` - Encoding/Decoding Libraries
* :github:`2860` - Customer: GUI for Zephyr configuration
* :github:`2916` - event logger: context switch event is not supported at ARC
* :github:`2977` - Install nSIM on CI machines
* :github:`2978` - Add Support for Multiple Simultaneous Backends/Partitions for FS
* :github:`2987` - Add support for computing IP, TCP, UDP checksums in hardware
* :github:`3065` -  Asymmetric multiprocessing (AMP)
* :github:`3152` - Support Atmel WINC1500 Wifi module
* :github:`3167` - consolidate all quark se c1000 arc boards into one
* :github:`3234` - 802.1Q - Virtual LANs
* :github:`3282` - Quick Kernel Resume
* :github:`3339` - IoTivity Import and Interoperability with CoAP and DTLS
* :github:`3348` - Missing board documentation for arc/quark_se_c1000_ss_devboard
* :github:`3369` - BSD Sockets API layer
* :github:`3373` - Atmel board/ Driver Support
* :github:`3384` - DataReady triggers failed to stop on BMI160 when both Accel/Gyro is enabled
* :github:`3388` - Power management-Idle State
* :github:`3389` - Power management-Device State
* :github:`3433` - Nordic SPI driver
* :github:`3471` - Espressif ESP Architecture Support
* :github:`3479` - Implement complete set of stm32cube driver based on LL API for STM32 SoCs
* :github:`3482` - Standardize stm32 SoCs porting
* :github:`3500` - ESP8266 Architecture Configuration
* :github:`3516` - Arduino 101 filesystem fails test
* :github:`3624` - Memory protection: define allocators for kernel objects
* :github:`3637` - Xtensa build is producing huge zephyr.bin
* :github:`3650` - no board target for Risc-V Freedom E310 SOC
* :github:`3703` - Doesn't compile if XCHAL_HAVE_ICACHE_DYN_WAYS and XCHAL_HAVE_DCACHE_DYN_WAYS are defined for an Xtensa processor
* :github:`3711` - RPL root node Grounded flag should be followed by client node
* :github:`3739` - linker: implement MPU alignment constraints
* :github:`3744` - Improve configuration tools for Windows developers
* :github:`3782` - SPI Slave support
* :github:`3819` - Add 802.15.4 Sub-Ghz TI CC1200 driver
* :github:`3824` - Add RPL border router functionality to Zephyr
* :github:`3849` - Reduce the overall memory usage of the LwM2M library
* :github:`3869` - Verify thath echo_server and echo_client examples work over Thread network
* :github:`3910` - IEEE 802.15.4 MCR20A driver problem sending packets
* :github:`3994` - Watchdog API update
* :github:`4012` - echo_server with DTLS drops lots of packets on frdm_k64f
* :github:`4052` - Coverity issue seen with CID: 177217 , in file: /tests/subsys/dfu/img_util/src/main.c
* :github:`4053` - Coverity issue seen with CID: 177216 , in file: /tests/subsys/dfu/mcuboot/src/main.c
* :github:`4066` - Function typedef issue when generating htmldocs.
* :github:`4213` - samples/net/: unexpected communication happens between echo_client and echo_server sample applications
* :github:`4217` - samples:net:sockets:echo_async : unexpected communication blocks between client and server after few packets transmission
* :github:`4309` - xtensa: GDB: Unable to debug
* :github:`4533` - IPv6/DAD: Things should be synchronized around net if up status and ipv6 addr add
* :github:`4684` - mtls and tinycrypt crypto drivers not returning number of bytes written to output buffer
* :github:`4713` - SPI: Update drivers to drop support of the legacy API
* :github:`4959` - Failure to install on Mac OS High Sierra (return of the same error)
* :github:`4963` - Convert NIOS2 boards to device tree
* :github:`4964` - Convert ARC boards to device tree
* :github:`5149` - Runtime non-volatile configuration system
* :github:`5254` - missing test for kernel event logger
* :github:`5274` - Issue with pinmux combination for FRDK-K64F setup for cc2520
* :github:`5327` - 1.11 Release Checklist
* :github:`5344` - samples/net/http_client: unable to send the proper http request to Apache server  in IPv6
* :github:`5482` - net: RFC: Move TCP-related code from net_context.c to tcp.c
* :github:`5554` - Support maintaining subsystems out-of-tree
* :github:`5577` - Document interaction between mcuboot and Zephyr
* :github:`5622` - Use the kconfiglib in scripts/kconfig for generating Kconfig documentation
* :github:`5633` - Optimize the CMake configuration time by reducing the time spent testing the toolchain
* :github:`5653` - STM32 boards: Generic guidelines for pin configuration
* :github:`5658` - Clicking on intra-page links broken on docs.zephyrproject.org
* :github:`5714` - 15.4 features required for OpenThread certification
* :github:`5718` - sanitycheck doesn't work on Windows (mkfifo not supported)
* :github:`5738` - [Coverity CID: 182194] Control flow issues in /subsys/storage/flash_map/flash_map.c
* :github:`5739` - [Coverity CID: 182193] Control flow issues in /subsys/storage/flash_map/flash_map.c
* :github:`5742` - [Coverity CID: 181921] Incorrect expression in /subsys/bluetooth/controller/ll_sw/ctrl.c
* :github:`5757` - native: fix -T warning while linking
* :github:`5767` - docs: Zephyr OpenOCD documentation is weak/non-existent
* :github:`5797` - SPI subsystem API & features updates
* :github:`5807` - Can't build Arduino_101 on Mac following instructions
* :github:`5839` - SPI API improvements proposals
* :github:`5847` - make menuconfig not supported on Windows
* :github:`5891` - [Coverity CID: 182585] Integer handling issues in /arch/x86/core/thread.c
* :github:`5892` - [Coverity CID: 182584] Integer handling issues in /kernel/sched.c
* :github:`5942` - OT: add framing part to OT build for the radio drivers with dependence on AR flag
* :github:`5953` - Build system: typedef not fit with zephyr type when CONFIG_NEWLIB_LIBC is enabled
* :github:`5956` - samples/net/coap_server: Failed to send response to coap client
* :github:`5978` - "$ ninja kconfig-usage" is out-of-date
* :github:`6007` - native: Add support for TAP net device
* :github:`6022` - Consistent feature dependency checks based on ARMvX-M
* :github:`6029` - doc: Zephyr sphinx/pygments support DTS
* :github:`6038` - Update Zephyr Licensing page
* :github:`6093` - [Coverity CID: 182778] Error handling issues in /samples/net/sockets/dumb_http_server/src/socket_dumb_http.c
* :github:`6094` - [Coverity CID: 182777] Error handling issues in /samples/net/sockets/dumb_http_server/src/socket_dumb_http.c
* :github:`6095` - [Coverity CID: 182776] Uninitialized variables in /tests/net/socket/udp/src/main.c
* :github:`6096` - [Coverity CID: 182775] Error handling issues in /tests/net/socket/udp/src/main.c
* :github:`6097` - [Coverity CID: 182774] Resource leaks in /tests/net/socket/udp/src/main.c
* :github:`6098` - [Coverity CID: 182773] Error handling issues in /samples/net/sockets/http_get/src/http_get.c
* :github:`6099` - [Coverity CID: 182772] Error handling issues in /tests/net/socket/udp/src/main.c
* :github:`6100` - [Coverity CID: 182771] Error handling issues in /samples/net/sockets/dumb_http_server/src/socket_dumb_http.c
* :github:`6101` - [Coverity CID: 182770] Error handling issues in /samples/net/sockets/http_get/src/http_get.c
* :github:`6103` - [Coverity CID: 182768] Error handling issues in /samples/net/sockets/dumb_http_server/src/socket_dumb_http.c
* :github:`6104` - [Coverity CID: 182767] Error handling issues in /tests/net/socket/udp/src/main.c
* :github:`6105` - [Coverity CID: 182766] Uninitialized variables in /tests/net/socket/udp/src/main.c
* :github:`6106` - [Coverity CID: 182765] Error handling issues in /tests/net/socket/udp/src/main.c
* :github:`6107` - [Coverity CID: 182764] Resource leaks in /tests/net/socket/udp/src/main.c
* :github:`6108` - [Coverity CID: 182763] Uninitialized variables in /tests/net/socket/udp/src/main.c
* :github:`6109` - [Coverity CID: 182762] Control flow issues in /subsys/storage/flash_map/flash_map.c
* :github:`6230` - Bluetooth: controller: refactor to use min/max macro
* :github:`6258` - [Coverity CID: 182894] Error handling issues in /samples/net/nats/src/main.c
* :github:`6259` - [Coverity CID: 182892] Various in /tests/ztest/src/ztest.c
* :github:`6260` - [Coverity CID: 182890] Null pointer dereferences in /tests/net/net_pkt/src/main.c
* :github:`6262` - [Coverity CID: 182886] Error handling issues in /subsys/bluetooth/controller/hal/nrf5/ticker.c
* :github:`6287` - runtime allocation of kernel objects
* :github:`6288` - better heap APIs for user mode
* :github:`6307` - Unaligned access in networking code causes unaligned exception on Nucleo-F429ZI
* :github:`6338` - Bluetooth: mesh: Node Identity Advertising issue
* :github:`6342` - echo server: incorrect Ethernet FCS and checksum in echo response when running in QEMU
* :github:`6347` - dhcpv4_client sample on spi_api_rework branch with board olimexino_stm32 does not work well
* :github:`6356` - samples/net/http_server causes an endless loop with wget
* :github:`6370` - I can't find adc name which is f429zi board
* :github:`6372` - ARMv8-M: implement & integrate SecureFault Handling
* :github:`6384` - Native (POSIX) zephyr.exe command line options not documented
* :github:`6388` - entropy_native_posix doesn't follow "entropy" contract and is thus security risk
* :github:`6400` - samples/net/http_client: Failed to connect to samples/net/http_server
* :github:`6413` - net_mgmt.h API event set data structure leads to undesirable behavior
* :github:`6424` - tests/kernel/mem_protect/x86_mmu_api: crashes on Arduino_101
* :github:`6450` - Several devices of same type on same bus - how to address?
* :github:`6511` - simics/qemu_x86_nommu: testscases with CONFIG_BOOT_DELAY !=0 do not boot
* :github:`6513` - arch: arc: the stack_sentinel is not supported in arc
* :github:`6514` - samples/drivers/i2c_fujitsu_fram: Data comparison on data written and data read fails randomly
* :github:`6515` - boards: em_starterkit:  the reset mechanism is not stable
* :github:`6534` - coap-server: Canceling Observation not working
* :github:`6559` - boards with i2c child nodes fail to build on windows
* :github:`6564` - samples/net/echo_client: Failed to connect to samples/net/echo_server for IPV4 test
* :github:`6565` - samples/net/sockets/echo: Failed to connect to samples/net/echo_server for IPV6 test
* :github:`6577` - sam0: SPI CS released too early
* :github:`6583` - samples/net/http_client: Failed to connect to
* :github:`6588` - Traffic prioritization on per-connection basis
* :github:`6594` - usb: replace "unicode" with "utf16le"
* :github:`6611` - Make sanitycheck run on Windows
* :github:`6616` - Non-detected/delayed sanitycheck failures due to ROM/RAM overflow
* :github:`6621` - newlib expects HEAP to be in CONFIG_SRAM_*, on arc there isn't always SRAM
* :github:`6623` - Request to support Application's Kconfig tree
* :github:`6625` - stm32: pwm: PWM 3 typo
* :github:`6635` - tests/net/websocket/test doesnt build on qemu_xtensa
* :github:`6640` - Ethernet network management interface additions
* :github:`6643` - usb: nrf52 returns empty configuration responses
* :github:`6644` - Bluetooth: Add reason parameter to L2CAP Channel disconnected callback
* :github:`6646` - usb: protocol field in descriptor for CDC ACM should default to zero
* :github:`6651` - sanity tries to compile things when it should not
* :github:`6657` - Question: Is Bluetooth avrcp supported in Zephyr? Or any plan?
* :github:`6660` - [Coverity CID: 183072] Incorrect expression in /tests/lib/c_lib/src/main.c
* :github:`6661` - [Coverity CID: 183071] Incorrect expression in /tests/kernel/static_idt/src/static_idt.c
* :github:`6662` - [Coverity CID: 183070] Uninitialized variables in /tests/posix/timer/src/posix_timer.c
* :github:`6663` - [Coverity CID: 183068] Incorrect expression in /tests/kernel/fatal/src/main.c
* :github:`6665` - [Coverity CID: 183067] Incorrect expression in /tests/lib/c_lib/src/main.c
* :github:`6666` - [Coverity CID: 183066] Error handling issues in /tests/kernel/mbox/mbox_api/src/test_mbox_api.c
* :github:`6667` - [Coverity CID: 183065] Integer handling issues in /tests/posix/timer/src/posix_timer.c
* :github:`6668` - [Coverity CID: 183064] Incorrect expression in /tests/kernel/common/src/intmath.c
* :github:`6669` - [Coverity CID: 183063] Null pointer dereferences in /tests/net/websocket/src/server.c
* :github:`6670` - [Coverity CID: 183062] Error handling issues in /samples/net/sockets/big_http_download/src/big_http_download.c
* :github:`6671` - [Coverity CID: 183061] Incorrect expression in /tests/kernel/mem_pool/mem_pool/src/main.c
* :github:`6672` - [Coverity CID: 183060] Incorrect expression in /tests/kernel/fatal/src/main.c
* :github:`6674` - [Coverity CID: 183058] Incorrect expression in /tests/kernel/static_idt/src/static_idt.c
* :github:`6675` - [Coverity CID: 183057] Memory - illegal accesses in /subsys/net/lib/websocket/websocket.c
* :github:`6677` - [Coverity CID: 183055] Concurrent data access violations in /kernel/posix/pthread.c
* :github:`6679` - [Coverity CID: 183053] Memory - corruptions in /samples/net/ws_echo_server/src/ws.c
* :github:`6680` - [Coverity CID: 183052] Memory - corruptions in /tests/net/app/src/main.c
* :github:`6682` - [Coverity CID: 183050] Memory - illegal accesses in /subsys/net/lib/websocket/websocket.c
* :github:`6683` - [Coverity CID: 183049] Incorrect expression in /tests/lib/c_lib/src/main.c
* :github:`6684` - [Coverity CID: 183048] Program hangs in /tests/posix/pthread_rwlock/src/posix_rwlock.c
* :github:`6685` - [Coverity CID: 183047] Uninitialized variables in /kernel/smp.c
* :github:`6686` - [Coverity CID: 183046] Memory - corruptions in /drivers/console/uart_console.c
* :github:`6687` - [Coverity CID: 183045] Error handling issues in /tests/drivers/spi/spi_loopback/src/spi.c
* :github:`6688` - [Coverity CID: 183044] Memory - corruptions in /tests/net/app/src/main.c
* :github:`6689` - [Coverity CID: 183043] Incorrect expression in /tests/kernel/common/src/intmath.c
* :github:`6690` - [Coverity CID: 183042] Program hangs in /tests/posix/pthread_rwlock/src/posix_rwlock.c
* :github:`6691` - [Coverity CID: 183041] Memory - corruptions in /tests/net/websocket/src/server.c
* :github:`6692` - [Coverity CID: 183040] Incorrect expression in /tests/kernel/static_idt/src/static_idt.c
* :github:`6693` - [Coverity CID: 183039] Error handling issues in /tests/kernel/mem_slab/mslab_threadsafe/src/test_mslab_threadsafe.c
* :github:`6694` - [Coverity CID: 183038] Integer handling issues in /kernel/posix/timer.c
* :github:`6697` - [Coverity CID: 183035] Null pointer dereferences in /tests/net/udp/src/main.c
* :github:`6698` - [Coverity CID: 183034] Error handling issues in /tests/net/websocket/src/main.c
* :github:`6699` - [Coverity CID: 183033] Program hangs in /tests/posix/pthread_rwlock/src/posix_rwlock.c
* :github:`6700` - [Coverity CID: 183032] Error handling issues in /tests/net/websocket/src/main.c
* :github:`6701` - [Coverity CID: 183031] Error handling issues in /tests/posix/semaphore/src/sem.c
* :github:`6702` - [Coverity CID: 183030] Memory - corruptions in /drivers/console/uart_console.c
* :github:`6719` - compilation problems with posix/unistd.h
* :github:`6726` - setting locale breaks MacOS X builds
* :github:`6749` - kconfig: The error message is misleading when values are out-of-range
* :github:`6755` - stm32: Compile error if 2-nd UART console enabled
* :github:`6757` - kernel:the API of k_mem_pool_alloc need try again to -EAGAIN[bug]
* :github:`6759` - sanitycheck in shippable states faillure but reports 0 errors
* :github:`6764` - ARC EMSK dts enhancements
* :github:`6779` - websocket API documentation missing
* :github:`6792` - [Coverity CID: 183443] Memory - corruptions in /subsys/bluetooth/controller/ll_sw/ctrl.c
* :github:`6793` - [Coverity CID: 183442] Null pointer dereferences in /subsys/net/ip/icmpv6.c
* :github:`6802` - unexpected user mode stack overflows on ARM
* :github:`6811` - Add ReST/Sphinx usage guide to our contributing documentation
* :github:`6814` - user mode does not work with newlib
* :github:`6821` - Simplify rendering of Kconfig variable doc
* :github:`6822` - Document how to flash with openocd on windows
* :github:`6831` - Update template docs with build example
* :github:`6833` - Question: BLE 5.0 extended advertising feature support for HCI
* :github:`6844` - Update Kconfiglib to improve generated documentation
* :github:`6849` - Some Kconfig symbols select choice symbols, which is a no-op
* :github:`6851` - 'make html' in doc/ fails with lexer.DtsLexer import error
* :github:`6854` - 'make html' in doc/ gives an error if doc.log is missing or empty
* :github:`6866` - build: requirements: No module named yaml and elftools
* :github:`6874` - Not able to join OpenThread BorderRouter or a ot-ftd-cli network
* :github:`6879` - Display symbols with multiple defs. with the right properties in the Kconfig documentation
* :github:`6881` - [Coverity CID: 183487] Control flow issues in /subsys/net/ip/net_core.c
* :github:`6882` - [Coverity CID: 183486] Null pointer dereferences in /tests/net/traffic_class/src/main.c
* :github:`6883` - [Coverity CID: 183485] Memory - illegal accesses in /subsys/net/ip/net_tc.c
* :github:`6884` - [Coverity CID: 183484] Null pointer dereferences in /tests/net/checksum_offload/src/main.c
* :github:`6885` - [Coverity CID: 183482] Memory - illegal accesses in /subsys/net/ip/net_tc.c
* :github:`6886` - [Coverity CID: 183481] Insecure data handling in /ext/lib/crypto/mbedtls/library/pkparse.c
* :github:`6887` - [Coverity CID: 183480] Null pointer dereferences in /tests/net/checksum_offload/src/main.c
* :github:`6888` - [Coverity CID: 183479] Insecure data handling in /ext/lib/crypto/mbedtls/library/pkparse.c
* :github:`6889` - [Coverity CID: 183478] Error handling issues in /tests/net/ipv6/src/main.c
* :github:`6890` - tests: kernel: arm_irq_vector_table: Usage fault on nrf52_pca10040
* :github:`6891` - jlink flashing is broken in windows
* :github:`6893` - http_client: Struct data is erased for no apparent reason
* :github:`6896` - too many static MPU regions on many ARM targets
* :github:`6897` - Can't build i586 on Mac following instructions
* :github:`6899` - support Ethernet statistical counters
* :github:`6902` - k_call_stacks_analyze needs to be reimplemented and optimized
* :github:`6907` - driver_api structs should have all function pointers defined
* :github:`6908` - shippable: console limit error (Console size exceeds 16 MB limit)
* :github:`6909` - Number of regions in arm_core_mpu_buffer_validate() can overflow
* :github:`6911` - xtools build hard-coded to use IAMCU compiler for all x86 targets
* :github:`6912` - OS X: setup instructions are wrong
* :github:`6929` - Make slab allocator work on user mode
* :github:`6937` - Add option for configuring P0.9 and P0.10 as GPIO
* :github:`6948` - Kconfig choice symbols could not be assigned in Kconfig.* files
* :github:`6957` - NRF52840: I2C, SPI driver
* :github:`6972` - RFC: kernel heap requests on behalf of syscalls
* :github:`6973` - bad magic number in 'kconfiglib' error when generating docs
* :github:`6978` - Fix issues with running Python's curses module on Windows
* :github:`6980` - extended Advertising BLE-5
* :github:`6983` - top level CMakeList.txt test if LINKER_SCRIPT points to existing file
* :github:`6984` - include pthread in app -> compilation failed
* :github:`6988` - test checksum_offload fail on native_posix
* :github:`6992` - extern object declarations interfere with kernel object detection
* :github:`6996` - buffer management issues with k_pipe syscalls
* :github:`6997` - buffer management issues with k_msgq syscalls
* :github:`7009` - LSM6DSL: Isse with spi_config
* :github:`7020` - tests/kernel/smp: Test fails, scheduler schedules the threads on only one core
* :github:`7022` - HTTP Server crashes on native posix
* :github:`7026` - i2c based sensor test cases fails on arc core
* :github:`7032` - Have Sphinx search display txt not ReST as results
* :github:`7033` - tests:fp_sharing: Test takes almost an hour on frdm_k64f
* :github:`7044` - Boot banner not correct for application builds outside of zephyr
* :github:`7050` - tests: sys_mem_pool: Bus fault occurs on ARM boards (frdmk64f and nrf52840_pca10056)
* :github:`7055` - tests: fatal: Stack protection fatal test fails on ARC core
* :github:`7067` - scripts: extract_dts_includes: fails on multiple includes in yaml bindings file
* :github:`7070` - lwm2m: possible buffer overflow in LwM2M engine debug output
* :github:`7073` - Full persistent storage support for Bluetooth
* :github:`7075` - No such file or directory: CMAKE_READELF: 'CMAKE_READELF-NOTFOUND'
* :github:`7076` - NRF52840: I2C Sensor (SHT3XD) driver issue
* :github:`7078` - [Coverity CID: 185286] Error handling issues in /subsys/settings/src/settings_store.c
* :github:`7079` - [Coverity CID: 185285] Error handling issues in /subsys/settings/src/settings_fcb.c
* :github:`7080` - [Coverity CID: 185284] Error handling issues in /subsys/settings/src/settings_fcb.c
* :github:`7081` - [Coverity CID: 185283] Control flow issues in /subsys/fs/nffs_fs.c
* :github:`7082` - [Coverity CID: 185282] Error handling issues in /subsys/settings/src/settings_init.c
* :github:`7083` - [Coverity CID: 185281] Null pointer dereferences in /kernel/posix/mqueue.c
* :github:`7084` - [Coverity CID: 185280] Error handling issues in /tests/posix/pthread_rwlock/src/posix_rwlock.c
* :github:`7085` - [Coverity CID: 185279] Resource leaks in /tests/net/socket/getaddrinfo/src/main.c
* :github:`7086` - [Coverity CID: 185278] Null pointer dereferences in /samples/net/coap_server/src/coap-server.c
* :github:`7087` - [Coverity CID: 185277] Null pointer dereferences in /samples/net/coap_server/src/coap-server.c
* :github:`7088` - [Coverity CID: 185276] Uninitialized variables in /tests/posix/posix_checks/src/posix_checks.c
* :github:`7089` - [Coverity CID: 185275] Integer handling issues in /kernel/posix/pthread_common.c
* :github:`7090` - [Coverity CID: 185274] Error handling issues in /subsys/settings/src/settings_store.c
* :github:`7091` - [Coverity CID: 185273] Resource leaks in /tests/net/socket/getaddrinfo/src/main.c
* :github:`7097` - doc build kconfig warning for XOROSHIRO_RANDOM_GENERATOR
* :github:`7103` - Unpatched upstream vulnerabilities in mbedTLS
* :github:`7107` - Crash while running echo_server with openthread
* :github:`7115` - doc/subsystems/settings/settings.rst references non-existing variables
* :github:`7127` - STM32 ethernet driver crashes without connected cable
* :github:`7128` - msp uninitialized on reset leading to usage fault for non-XIP targets
* :github:`7137` - I2C Driver does not compile for nrf5 boards
* :github:`7144` - SDK Openocd stm32f4discovery.cfg is incorrect for new versions of the STM32F407G-DISC1
* :github:`7146` - scripts/sanitycheck will delete any folder given to --outdir argument
* :github:`7155` - DTS: qemu_x86.dts: Warning (unit_address_format): Node /flash@00001000 unit name should not have leading 0s
* :github:`7159` - Kconfig.defconfig is undocumented and unclear
* :github:`7170` - zassert: Confusing rules and actual usage for messages in zassert_*() calls
* :github:`7172` - Mcr20a initialization crashes with frdm_k64f board
* :github:`7184` - List of supported boards is incorrect when $BOARD_ROOT is set by user.
* :github:`7186` - settings_load() never returns when called
* :github:`7198` - sanitycheck issue w/ztest
* :github:`7200` - Commit 'tests: kernel: mem_protect: tests for userspace mode' breaks scripts/sanitycheck
* :github:`7207` - cmake fails when zephyr is used as submodule
* :github:`7208` - ztest_test_fail() not failing?
* :github:`7219` - printk output with gdbserver?
* :github:`7227` - /subsys/storage/flash_map/flash_map_default.c missing declarations.
* :github:`7236` - Sample Http_Client  is deprecated
* :github:`7245` - EMSK 7d: can't build after 60ec8be309cb84d72c5fc61330abc968eb62333e
* :github:`7246` - esp32 fails to build with xtensa-esp32-elf-gcc: error: unrecognized command line option '-no-pie'
* :github:`7248` - i2c: Seems issue in dts
* :github:`7249` - Arduino 101 / ARC:  tests/kernel/fifo/fifo_api/ fails after 3d9ba10b5c903265d870a9f24065340d93e7d465
* :github:`7254` - [Coverity CID :185402] Code maintainability issues in /drivers/spi/spi_dw.c
* :github:`7255` - [Coverity CID :185401] Integer handling issues in /drivers/spi/spi_mcux_dspi.c
* :github:`7256` - [Coverity CID :185400] Null pointer dereferences in /drivers/spi/spi_dw.c
* :github:`7257` - [Coverity CID :185399] Integer handling issues in /subsys/usb/usb_device.c
* :github:`7258` - [Coverity CID :185398] Memory - corruptions in /samples/net/mbedtls_sslclient/src/mini_client.c
* :github:`7259` - [Coverity CID :185397] Null pointer dereferences in /tests/net/ipv6_fragment/src/main.c
* :github:`7260` - [Coverity CID :185395] Memory - corruptions in /samples/net/mbedtls_sslclient/src/mini_client.c
* :github:`7261` - [Coverity CID :185394] Null pointer dereferences in /subsys/net/ip/l2/ethernet/arp.c
* :github:`7262` - [Coverity CID :185393] Memory - illegal accesses in /drivers/interrupt_controller/plic_fe310.c
* :github:`7263` - [Coverity CID :185392] Null pointer dereferences in /drivers/spi/spi_dw.c
* :github:`7264` - [Coverity CID :185391] Incorrect expression in /tests/lib/rbtree/src/main.c
* :github:`7266` - Zephyr's C Kconfig tools do not support <, <=, >, >=, due to being too old
* :github:`7269` - /samples/net/http_client: Error using https in http_cliente sample
* :github:`7280` - if we have two flash on board?
* :github:`7285` - buffer management issues with k_stack syscalls
* :github:`7287` - Git describe broken with older versions of Git
* :github:`7308` - qemu_xtensa cannot be debugged with SDK
* :github:`7309` - minnowboard DTS is not configured correctly
* :github:`7311` - FCB: CRC write size in append_finish doesn't honor flash min write size
* :github:`7327` - Interrupt stack is not initialized for Xtensa target
* :github:`7329` - is there has anyone who having portting stm32f2?
* :github:`7336` - USB DFU: this area can not be overwritten
* :github:`7340` - DISCUSS: usb_device.c: If condition judgment
* :github:`7342` - samples: net/dns_resolve does not build when activating dhcp
* :github:`7349` - Add STM32L0 USB support
* :github:`7364` - kernel crash: USB ECM: echo_server
* :github:`7365` - net: Regression in multiple client connection handling with samples/net/sockets/dumb_http_server (QEMU/SLIP)
* :github:`7377` - net: Regression in multiple client connection handling with samples/net/sockets/dumb_http_server (frdm_k64f/Ethernet)
* :github:`7378` - TOCTOU in spi_transceive syscall handler
* :github:`7379` - TOCTOU in adc_read() handler
* :github:`7380` - dma_stm32f4x possible access out of bounds in start/stop handlers
* :github:`7388` - nxp_mpu: suspicious ENDADDR_ROUND() macro
* :github:`7389` - t1_adc108s102 buffer overflow due to chan->buf_idx growth
* :github:`7412` - Mismatch between 'uint32_t' (and `off_t`) definitions in minimal libc and newlib
* :github:`7434` - bluetooth: host: sample applications can't set BT address w/o using an FS
* :github:`7437` - Zephyr's mailing list archives were hard to find
* :github:`7442` - menuconfig should perform fuzzy string matching for symbols
* :github:`7447` - net tests: valgrind detected issues
* :github:`7452` - nRF52, NXP kinetis, ARM Beetle and  STM `MPU` option appear for every MPU-equipped device
* :github:`7453` - Bluetooth mesh message context API needs to expose DST address of RX messages
* :github:`7459` - net: Multiple inconsistent settings to configure TIME_WAIT delay in the IP stack
* :github:`7460` - Unable to view PR #6391
* :github:`7475` - LwM2M: UDP local port setting not obeyed, random port doesn't work
* :github:`7478` - tests: valgrind detected issues
* :github:`7480` - pthread_attr_init( ) does not conform to POSIX specification
* :github:`7482` - [Coverity CID :185523]  Out-of-bounds read in lsm6dsl driver
* :github:`7495` - cmake: extensions: ToolchainCapabilityDatabase.cmake parse error in shippable
* :github:`7500` - DHCP: when CONFIG_NET_APP_MY_IPV4_ADDR is IP stack needs to rebind on DHCP aquire
* :github:`7508` - [Coverity CID :185523] Memory - illegal accesses in /drivers/sensor/lsm6dsl/lsm6dsl.c
* :github:`7511` - [Coverity CID :185391] Incorrect expression in /tests/lib/rbtree/src/main.c
* :github:`7519` - Verify CODEOWNERS (not) including subfolders is intended
* :github:`7521` - Website: git clone instructions refer to old (v1.10.0) tag
* :github:`7536` - tests: kernel.timer: fails on riscv32
* :github:`7541` - arm: struct k_thread->entry is overwritten once the thread is scheduled
* :github:`7565` - zephyr_library_ifdef has unexpected behaviour
* :github:`7569` - test: posix/pthread_rwlock
* :github:`7608` - ARC objdump crash when creating zephyr.lst for one test
* :github:`7610` - tests/lib/c_lib fails on native_posix on FC28
* :github:`7613` - OTA:an issue about OTA/mcumgr
* :github:`7644` - k_mem_slab_free triggers rescheduling even when no threads are pending
* :github:`7651` - nRF5x console broken
* :github:`7655` - Invalid argument passed to k_sleep
* :github:`7656` - Invalid argument passed to k_sleep
* :github:`7657` - Invalid argument passed to k_sleep
* :github:`7661` - LwM2M error: invisible error during registration
* :github:`7663` - Sample ipsp: bluetooth: not functional on disco_l475_iot1
* :github:`7666` - NVS API documentation is missing
* :github:`7671` - NVS broken for write-align > 4
* :github:`7673` - Eliminate recursive make in OpenAMP integration
* :github:`7676` - buildsystem: 'make flash' failed
* :github:`7677` - mcuboot-master imgtool.py sign error
* :github:`7692` - Kernel tests failing at runtime on frdm_k64f
* :github:`7694` - Have RTC binding for QMSI utilize base rtc.yaml
* :github:`7698` - Kernel tests failing at runtime on frdm_kw41z
* :github:`7699` - drivers: i2s: intel_s1000: I2S BCLK cannot be a fraction of reference clock
* :github:`7704` - nrf52_pca10040:tests/bluetooth/init/test_controller_dbg fails build with CONFIG_USERSPACE=y
* :github:`7709` - native_posix: hello_world fails to link on Fedora 28
* :github:`7712` - [Coverity CID :186063] Null pointer dereferences in /subsys/disk/disk_access.c
* :github:`7713` - [Coverity CID :186062] Error handling issues in /samples/net/sockets/big_http_download/src/big_http_download.c
* :github:`7714` - [Coverity CID :186061] Memory - corruptions in /drivers/usb/device/usb_dc_kinetis.c
* :github:`7715` - [Coverity CID :186059] Memory - illegal accesses in /drivers/usb/device/usb_dc_kinetis.c
* :github:`7716` - [Coverity CID :186058] Null pointer dereferences in /tests/kernel/fifo/fifo_timeout/src/main.c
* :github:`7717` - [Coverity CID :186057] Memory - corruptions in /samples/net/rpl_border_router/src/coap.c
* :github:`7718` - [Coverity CID :186056] Null pointer dereferences in /subsys/disk/disk_access.c
* :github:`7719` - [Coverity CID :186055] Memory - corruptions in /drivers/usb/device/usb_dc_kinetis.c
* :github:`7720` - [Coverity CID :186053] Code maintainability issues in /samples/net/rpl_border_router/src/http.c
* :github:`7721` - [Coverity CID :186051] Memory - illegal accesses in /drivers/usb/device/usb_dc_kinetis.c
* :github:`7722` - [Coverity CID :186049] Memory - corruptions in /samples/subsys/mgmt/mcumgr/smp_svr/src/main.c
* :github:`7723` - [Coverity CID :186048] Memory - illegal accesses in /drivers/usb/device/usb_dc_kinetis.c
* :github:`7724` - [Coverity CID :186047] Null pointer dereferences in /tests/net/arp/src/main.c
* :github:`7725` - [Coverity CID :186046] Memory - corruptions in /drivers/usb/device/usb_dc_kinetis.c
* :github:`7726` - [Coverity CID :186045] Null pointer dereferences in /subsys/disk/disk_access.c
* :github:`7727` - [Coverity CID :186044] Memory - illegal accesses in /drivers/usb/device/usb_dc_kinetis.c
* :github:`7728` - [Coverity CID :186043] Incorrect expression in /tests/posix/fs/src/test_fs_dir.c
* :github:`7729` - [Coverity CID :186042] Program hangs in /tests/posix/mutex/src/posix_mutex.c
* :github:`7730` - [Coverity CID :186041] Memory - corruptions in /samples/net/rpl_border_router/src/http.c
* :github:`7731` - [Coverity CID :186040] Resource leaks in /tests/posix/fs/src/test_fs_dir.c
* :github:`7732` - [Coverity CID :186039] Control flow issues in /subsys/net/ip/connection.c
* :github:`7733` - [Coverity CID :186037] Memory - corruptions in /lib/posix/fs.c
* :github:`7734` - [Coverity CID :186036] Memory - corruptions in /drivers/usb/device/usb_dc_kinetis.c
* :github:`7735` - [Coverity CID :186035] Incorrect expression in /drivers/ipm/ipm_mcux.c
* :github:`7736` - [Coverity CID :186034] Memory - corruptions in /tests/net/udp/src/main.c
* :github:`7737` - [Coverity CID :186033] Control flow issues in /subsys/mgmt/smp.c
* :github:`7738` - [Coverity CID :186032] Memory - illegal accesses in /drivers/usb/device/usb_dc_kinetis.c
* :github:`7739` - [Coverity CID :186030] Error handling issues in /subsys/bluetooth/host/settings.c
* :github:`7740` - [Coverity CID :186029] Null pointer dereferences in /subsys/disk/disk_access.c
* :github:`7741` - [Coverity CID :186028] Incorrect expression in /drivers/gpio/gpio_imx.c
* :github:`7742` - [Coverity CID :186027] Null pointer dereferences in /subsys/disk/disk_access.c
* :github:`7753` - security: ARM does not scrub registers when returning from system calls
* :github:`7754` - tests/kernel/threads/lifecycle/thread_init register corruption on ARM with user mode enabled
* :github:`7755` - getchar sample not working on nRF5x
* :github:`7761` - ARM: failed syscalls do not report site of faulting syscall
* :github:`7766` - tests/subsys/fs/fat_fs_api - fat_fs_basic_test hangs in fatfs_mount
* :github:`7776` - possible unaligned memory access to struct _k_object's perms
* :github:`7780` - Using latest Openthread in Zephyr
* :github:`7782` - tests/kernel/mem_protect/stack_random hangs without any console output on frdm_k64f
* :github:`7787` - tests: kernel: smp fatal exception observed on ESP32
* :github:`7789` - Reset sequence broken on nRF5x chips
* :github:`7793` - samples\sensor: bme280 and tmp112 hangs without any console output on quark_se_c1000_devboard
* :github:`7795` - STM32 related Kconfig symbols don't exist anymore or aren't referenced anywhere
* :github:`7797` - subsys/net/ip/Kconfig references NET_L2_OFFLOAD but that doesn't exist
* :github:`7798` - nonexistent Kconfig symbol in defconfig arch/x86/soc/intel_quark/quark_x1000/Kconfig.defconfig.series
* :github:`7799` - nonexistent Kconfig symbol in defconfig boards/x86/quark_se_c1000_devboard/Kconfig.defconfig
* :github:`7802` - Add RTC support for all NXP MCUX platforms
* :github:`7804` - samples/sensor/apds9960 fails with assertion "Fatal fault in essential thread" on quark_se_c1000_ss_devboard
* :github:`7811` - CAVS Interrupt controller - using undefined Kconfig symbols
* :github:`7812` - tests: Crypto tests fail on nrf52 boards after enabling user mode
* :github:`7814` - networking - Cleanup undefined but referenced Kconfig
* :github:`7815` - bluetooth - cleanup undefined Kconfig symbols
* :github:`7819` - build breakage due to enabling USER_SPACE by default
* :github:`7821` - net: Loopback broken: Packets sent locally to loopback address are now dropped
* :github:`7858` - Test for k_thread_foreach() is failing on some boards
* :github:`7862` - rpl_border_router sample bus_faults on frdm_k64f
* :github:`7873` - cc2650_sensortag_defconfig assigned missing Kconfig symbols (now dead code)
* :github:`7877` - tests: kernel/mem_protect/mem_protect is failed on nucleo_f429zi/disco_l475_iot1
* :github:`7882` - tests/dfu/mcuboot.test_bank_erase fails on nrf52840_pca10056
* :github:`7885` - em_starterkit_em7d_v22 failing multiple tests in user mode
* :github:`7891` - tests/posix/timer.test_timer fails on nrf51_pca10028
* :github:`7907` - tests/benchmarks/latency_measure fails on sam_e70_xplained
* :github:`7927` - West runner command doesn't work for em-starterkit
* :github:`7931` - Bluetooth controller nrf52 - connection event status
* :github:`7933` - drivers: can: stm32_can: bitrate ignored
* :github:`7942` - tests: benchmarks: build fail in arm/core/fault.c for frdm_k64f in footprint/min
* :github:`7954` - make flash failing for altera_max10
* :github:`7959` - shell: backspace doesn't work in minicom
* :github:`7972` - Bluetooth: Mesh: adv bearer does not use correct interval
* :github:`7974` - extract_dts_includes: Binding merge warning should be enhanced
* :github:`7979` - drivers: dma: dma_cavs: DMA driver does not support per-channel callbacks
* :github:`7989` - eth: ping: can't ping others from zephyr console
* :github:`8005` - FRDM-K64F boot hang w/ mcuboot + lwm2m client
* :github:`8009` - POSIX `clock_gettime()` is discontinuous
* :github:`8015` - Driver:spi_flash_w25qxxdv.c init mistake and can not be erased
* :github:`8032` - _SysFatalErrorHandler not working properly for arc on quark_se_c1000_ss_devboard
* :github:`8033` - tests/crypto/mbedtls/ results in exception on frdm_k64f
* :github:`8038` - tests/subsys/settings/fcb system.settings.fcb fails on nrf52
* :github:`8049` - kernel: scheduler tries to make polling threads active
* :github:`8054` - Ethernet initialization is unreliable and gets stuck on frdm-k64f
* :github:`8062` - [Coverity CID :186196] Error handling issues in /samples/sensor/mcp9808/src/main.c
* :github:`8063` - [Coverity CID :186190] Null pointer dereferences in /tests/kernel/fifo/fifo_timeout/src/main.c
* :github:`8064` - arm: multiple MemManage status flags may be set simultaneously
* :github:`8065` - tests/subsys/fs/fat_fs_api - test_fat_file and test_fat_dir results into Assertion failure on Arduino_101 due to spi_flash changes
* :github:`8069` - mem_slab/mslab_threadsafe testcase fails in CI sporadically
* :github:`8070` - table broken in S1000 documentation
* :github:`8073` - Zero Latency IRQ masked by interrupt locking
* :github:`8083` - Bluetooth ATT trying to access invalid pointer after disconnect
* :github:`8085` - tests/subsys/logging/logger-hook crashes on sam_e70_explained
* :github:`8086` - tests/net/ieee802154/crypto fails on Quark SE / x86
* :github:`8087` - tests/misc/test_build fails to build on esp32
* :github:`8088` - tests/kernel/xip fails on QEMU riscv32 with no output
* :github:`8090` - tests/sched/schedule_api fails to build on EMSK7d
* :github:`8092` - tests/kernel/fatal crashes on Quark SE / ARC
* :github:`8093` - tests/kernel/common fails to build on xtensa / ESP32
* :github:`8094` - tests/drivers/watchdog/wdt_basic_api fails to build on esp32 / xtensa
* :github:`8096` - tests/drivers/watchdog/wdt_basic_api fails on Quark SE / ARC with no output
* :github:`8098` - tests/drivers/rtc/rtc_basic_api fails on Quark SE / x86
* :github:`8099` - tests/drivers/rtc/rtc_basic_api assertion failure on Arduino 101 / ARC
* :github:`8111` - kconfiglib warning "quotes recommended around default value for string symbol SOC_SERIES"
* :github:`8117` - tests/kernel/errno crashes on minnowboard
* :github:`8118` - x86 may expose private kernel data to user mode
* :github:`8129` - scheduler: in update_cache() thread from next_up() and _current are the same
* :github:`8132` - stm3210c_eval.dts_compiled: Warning
* :github:`8142` - GPIO API not shown on docs.zephyrproject.org
* :github:`8145` - samples/subsys/usb/dfu: Build failure, Reference to non-existent node or label "slot0_partition"
* :github:`8150` - Doc: Update Zephyr security overview
* :github:`8171` - Tests failing with a stacking error on frdm_k64f
* :github:`8172` - Networking tests failing with an assertion on frdm_k64f
* :github:`8180` - objcopy bug？
* :github:`8182` - Problem with obtaining hop_limit from a received packet
* :github:`8189` - lwm2m: Quickly running out of resources when using observe
* :github:`8192` - MPU Fault on some platforms after THREAD_MONITOR "fix"
* :github:`8193` - STM32 config BUILD_OUTPUT_HEX fail
* :github:`8198` - Tests: fifo_timeout fails on nrf51_pca10028
* :github:`8200` - Tests: arm_irq_vector_table: Assertion failure on nrf52840_pca10056
* :github:`8202` - question: is the irq_lock is necessary in console_putchar
* :github:`8213` - Failed test: usb.device.dfu.bank_erase
* :github:`8214` - Failed test: kernel.threads.customdata_get_set_coop
* :github:`8222` - tests/drivers/watchdog/wdt_basic_api crashes on multiple platforms
* :github:`8232` - Failed test: kernel.memory_protection.create_new_essential_thread_from_user
* :github:`8250` - UDP socket may lose data
* :github:`8274` - Make flash doesn't work on nrf51_pca10028
* :github:`8275` - when zephyr can support popular IDE develop?
* :github:`8280` - [Coverity CID :186491] Memory - corruptions in /lib/posix/fs.c
* :github:`8292` - Rework ARC exception stack
* :github:`8298` - Failed test: kernel.alert.isr_alert_consumed (in tests/kernel/alert/) on quark_se_c1000_ss
* :github:`8299` - Failed test: kernel.memory_pool.mpool_alloc_free_isr (in tests/kernel/mem_pool/mem_pool_api)
* :github:`8302` - Failed test: peripheral.adc.adc on quark_se
* :github:`8311` - tests/benchmarks/sys_kernel fails on frdm_k64f, sam_e70
