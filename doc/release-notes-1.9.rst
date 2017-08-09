:orphan:

.. _zephyr_1.9:

Zephyr Kernel 1.9.0 (WIP)
#########################

We are pleased to announce the release of Zephyr kernel version 1.9.0
(planned for release in August 2017).

Major enhancements planned with this release include:

* POSIX API Layer
* BSD Socket Support
* Expand Device Tree support to more architectures
* BLE Mesh
* Full Bluetooth 5.0 Support
* Expand LLVM Support to more architectures
* Revamp Testsuite, Increase Coverage
* Zephyr SDK NG
* Eco System: Tracing, debugging support through 3rd party tools

These enhancements are planned, but may move out to a future release:

* LWM2M
* Thread Protocol (initial drop)
* MMU/MPU (Cont.): Thread Isolation, Paging
* Build and Configuration System (CMake)


The following sections provide detailed lists of changes by component.

Kernel
******

* kernel: add early init routines for app RAM
* kernel: add config for app/kernel split
* kernel: queue
* kernel: remove gdb_server
* kernel: make K_.*_INITIALIZER private to kernel


Architectures
*************

* arm: stm32f1: Add support for STM32F103x8 SoC
* arm: soc: stm32: f3: add MPU capability
* arm: soc: stm32: l4: add MPU capability for series
* arm: soc: stm32: make mpu f4 config useable for other family
* arm: Add support for TI's CC2650 SoC.
* arm: Modify linker script to accomodate need for flash footer
* arm: nxp: mpu: Fix region descriptor 0 attributes
* arm: Add build time consistency check for irq priority defines
* arm: nxp: k6x: Fix typo in partition offset
* arm: nxp: k6x: Add default partition table.
* arch: xtensa: Use CONFIG_SIMULATOR_XTENSA to set XT_{BOARD
* arch: xtensa: Use Zephyr configuration options
* arch: xtensa: Add ESP32 SoC
* linker: arm: Split out application from kernel
* arm: dts: Modified Atmel SAM family processor's UART to DTS.


Boards
******

* boards: frdm_k64f: enable ethernet for networking
* boards: qemu: enable test random generator
* boards: arm: carbon/l475/f401re/olimexino: enable I2C
* boards: arm: Add support for the VBLUno51 board
* boards: arm: Add STM32F3DISCOVERY board
* pinmux: boards: add I2C to carbon/lf33fr8/f401re/olimexino
* pinmux: stm32 F1X/F3X/F4X: add I2C
* boards; cc2650_sensortag: Get building with sanitycheck
* boards: arm: Add support for STM32 Minimum Development Board
* boards: disco_l475_iot1: enable MPU
* boards: nucleo_l476rg: enable MPU
* boards: sam_e70_xplained: allow flashing via JTAG header
* board: frdm_k64f: allow overriding default debug/flash scripts
* boards: 96b_nitrogen: Add support for flash/debug with pyOCD
* boards: add board meta-data
* boards: esp32: Do not use undefined CONFIG_BOARD_XTENSA
* boards: xtensa: Add ESP32 board
* boards: arm: olimex_stm32_e407: Initial Olimex STM32-E407 BSP
* board: cc2650_sensortag: Add zephyr
* boards: introduce usb_device support tag
* boards: mark boards with built-in networking support
* boards: arm: stm32f3_disco: update yaml to spec ram
* dts: arduino101: Add device tree support for arduino101 board
* dts: arm: STM32 boards use DT to configure I2C
* sensortag: Add TI's SensorTag board
* timer: xtensa_sys: Cleanup use of C99 types
* xtensa: esp32: configure default UART using ROM functions
* xtensa: esp32: place .rodata into DRAM
* dts: nucleo_f401re: add partition support for bootloader
* dts: x86: Add dts support for x86
* x86: place application data in its own sections
* qemu_x86: ia32: fix ROM size with XIP enabled
* x86: implement bss zero and data copy for application	
* dts: arduino101: Add device tree support for arduino101 board


Drivers and Sensors
*******************

* drivers: slip: move doxygen header
* drivers: ataes132a: Fix Kconfig name
* drivers: i2c: stm32 LL F1/F4 (v1) STM32 F3/L4X (v2)
* drivers: usb: use generic option name for log level
* Drivers: flash: NRF5x: synchronous mode for co-operation with BLE radio
* drivers: serial: Add device support for serial driver
* drivers: esp32: Add minimal UART driver based on ROM routines
* drivers: remove unused headers for nsim serial
* drivers: serial: uart_stellaris: Remove UART_IRQ_FLAGS
* drivers: timer: init earlier in boot sequence
* drivers: stm32: random: Initial STM32 random number generator driver
* drivers: ieee802154: kw41z: add support for KW40Z
* drivers: ieee802154: kw41z: fix issue in RX sequence
* drivers: ieee802154: kw41z: remove unnecessary cast for PKT_BUFFER_RX
* ext: mcux: import XCVR driver for KW40Z
* ext: mcux: update KW40Z files for building XCVR driver
* ext: mcux: add makefiles for building KW40Z XCVR driver
* ext: mcux: add minimal v2.2 interface for KW40Z XCVR driver
* arch: intel_quark: use DW device driver when USB is selected
* i2c: stm32: Cleanup how we enable the specific I2C driver
* spi: add SPI driver for STM32 family
* serial: nsim: Add missing SERIAL_HAS_DRIVER in Kconfig
* bluetooth: shell: add module for testing NRF5x flash driver
* cc2650: Add pinmux driver.
* cc2650: Add GPIO driver.
* uart: Use DTS labels for Stellaris driver.
* pinmux: pinmux_dev_k64 driver and related references are removed.


Networking
**********

* net: kconfig: fix help message for SLIP
* net: fix wrong Kconfig
* net: app: prevent setting semaphore limit to 0
* net: app: fix typo
* net: app: Create support for network application API
* net: ipv4: fix icmp checksum calculation
* net: samples: Convert echo-client to use network app API
* net: samples: Convert echo-server to use network app API
* net: sockets: Explicitly flush conn/pkt queue on close()
* net: sockets: Implement recv() for DGRAM sockets
* net: sockets: Implement recv() for STREAM sockets
* net: sockets: Implement send()
* net: sockets: Add POSIX compat defines for inet_ntop
* net: sockets: Implement bind()
* net: context: Allow to put context into FIFO at expense of user_data
* net: sockets: Add configurable option to provide raw POSIX API names
* net: sockets: Bootstrap Sockets API implementation
* net: http: Remove mbedtls heap setting from http library
* net: zoap: add missing response code for zoap_header_get_code()
* net: context: Go back to LISTEN state when receiving RST
* net: context: Close connection fast if TIME_WAIT support is off
* net: buf: Move net_buf_pool objects to dedicated linker area
* net: pkt: Fix net_pkt_split()
* net: arp: Do not try to access NULL pointer
* net: sockets: Implement non-blocking mode.
* net: Comment false positives reported by Coverity
* net: context: Fix use of k_delayed_work_cancel with SYN backlog
* net: tcp: Fix sequence number validator
* net: ipv6: Increase default multicast address count to 3
* net: tcp: Add FIN timer when doing active close
* net: tcp: Fix passive close ACK timer
* net: Fix NULL pointer access
* net: bt: Check return of bt_conn_get_info
* net: tcp: Recalculate the tcp checksum when changing tcp header.
* net: context: Use K_NO_WAIT instead of 0 for timeout
* net: rpl: Ignore consistent DIO messages
* net: rpl: Fix invalid access of IPv6 nbr link metric
* net: rpl: Fix byte order conversion while preparing message
* net: rpl: Fix byte order conversion of sender rank
* net: Avoid printing non-error cases when parsing packet
* net: tcp: Remove NET_TCP_HDR() macro and direct access to net_buf
* net: udp: Remove NET_UDP_HDR() macro and direct access to net_buf
* net: icmp: Remove NET_ICMP_HDR() macro and direct access to net_buf
* net: ipv6: Handle PAD1 extension header properly
* net: utils: Helper to check if protocol header fits in fragment
* net: utils: Rework the IP packet checksum calculation
* net: net_app: fix syntax error when accessing remote from default_ctx
* net: tcp: Reset context->tcp to NULL after net_tcp_release.
* net: route: Do not try to access null link layer address
* net: route: Do not try to del null route
* net: context: Remove tcp struct SYN-ACK timer handling
* net: context: Add TCP SYN-ACK timer handling
* net: tcp: Implement TCP backlog for incoming connections
* net: tcp: Make initial sequence number calculation public
* net: app: Fix dual IPv4 and IPv6 support
* net: context: Remove useless asserts when setting net_buf pools


Bluetooth
*********

* Bluetooth: hci_uart: Set the UART TX size correctly
* Bluetooth: Clean up struct bt_conn
* Bluetooth: GATT Introduce BT_GATT_CCC_MAX helper define
* Bluetooth: GATT: Reorder bt_gatt_ccc_cfg to eliminate padding
* Bluetooth: Kconfig: let MAX_PAIRED be 0 if SMP is not supported
* Bluetooth: controller: Fix HCI remote version structure
* Bluetooth: controller: Use RL indices in adv ISR
* Bluetooth: controller: Fix advertisement event lengths
* Bluetooth: controller: Rename chl_* to chan_*
* Bluetooth: controller: Fix Event Mask Page 2 handling
* Bluetooth: controller: Filter Auth Payload timeout exp
* Bluetooth: controller: Add support for Event Mask Page 2
* Bluetooth: controller: Reset filters correctly
* Bluetooth: tests: Remove unused prj_nimble.conf files
* Bluetooth: controller: Increase ticker operations in thread mode
* Bluetooth: monitor: Remove interrupt locking from monitor_console_out()
* Bluetooth: controller: Privacy filtering in advertiser
* Bluetooth: controller: Fix missing re-initialization of ret_cb
* Bluetooth: controller: Fix for scanner Rx window hang
* Bluetooth: controller: Fix scanner to use correct slot ticks
* Bluetooth: samples: scan_adv: Select BROADCASTER and OBSERVER
* Bluetooth: hci_ecc: Use ATOMIC_DEFINE() for the flags
* Bluetooth: controller: Fix hardfault
* Bluetooth: Introduce new BT_LE_ADV_OPT_ONE_TIME advertising option
* Bluetooth: Fix KEEP_ADVERTISING flag treatment in bt_le_adv_stop()
* Bluetooth: shell: Fix compile warnings when CONFIG_DEBUG=y	
* Bluetooth: shell: Fix incorrect #define
* Bluetooth: controller: Temporarily disable scan req notification
* bluetooth: shell: add module for testing NRF5x flash driver
* Bluetooth: controller: support for code exe. in co-operation with radio
* Bluetooth: controller: Keep track of IRK to RL indices
* Bluetooth: controller: Add wl bit in resolving list
* Bluetooth: controller: Add device match ID radio API
* Bluetooth: controller: Generate RPAs for scanning and initating
* Bluetooth: controller: Be fair when pre-empting a ticker
* Bluetooth: controller: Do not skip one-shot tickers with slot
* Bluetooth: controller: Coding style and refactoring
* Bluetooth: Move PRNG initialization a bit later in HCI init
* Bluetooth: controller: Optimize RL addition
* Bluetooth: controller: Handle Privacy Modes
* Bluetooth: controller: Add RL filter
* Bluetooth: controller: Properly remove peer IRKs from RL
* Bluetooth: controller: Add whitelist population with privacy
* Bluetooth: controller: Fix supported cmds and states
* Bluetooth: Fortify role dependency
* Bluetooth: conn: Add pending tx before calling bt_send()
* Bluetooth: controller: Fix assert due to stale tick count
* Bluetooth: controller: Correct adv
* Bluetooth: Cleanup use of C99 types
* Bluetooth: Fix use of uint32_t in nRF5 radio timings abstractions
* Bluetooth: controller: Add radio fast ramp feature
* Bluetooth: controller: nRF5 radio timings abstractions
* Bluetooth: Set hci_uart RAM config to fit privacy
* Bluetooth: controller: Fix OCF data size
* Bluetooth: controller: Fix directed adv timeout and disable
* Bluetooth: controller: Add connect ind PDU address fields check
* Bluetooth: controller: Fix conn context leak under directed adv
* Bluetooth: GATT: Fix aligment of bt_gatt_ccc_cfg and _bt_gatt_ccc
* Bluetooth: controller: Refactor whitelist handling (v2)
* Bluetooth: GATT: Rename bt_gatt_unregister_service
* Bluetooth: controller: Add inital support for Controller-based privacy
* Bluetooth: Correctly select RPA and TINYCRYPT options
* Bluetooth: controller: Rename mem function that checks all-zero mem
* Bluetooth: controller: Use find_lsb_set instead of custom ffs
* bluetooth: hci: Kconfig: Fix dependency
* Bluetooth: Shell: Add gatt-show-db command
* Bluetooth: Shell: Add gatt-unregister-service command
* Bluetooth: GATT: Add bt_gatt_unregister_service
* Bluetooth: GATT: Add bt_gatt_register_service
* Bluetooth: GATT: Indicate Service Changed when a service is added
* Bluetooth: GATT: Add GATT service by default
* Bluetooth: GATT: Add GAP service by default
* Bluetooth: Kconfig: Add CONFIG_BLUETOOTH_DEVICE_APPEARANCE
* Bluetooth: Remove CONFIG_BLUETOOTH_GATT_DYNAMIC_DB
* Bluetooth: ATT: Fix adding timeout for NULL request
* Bluetooth: controller: Move scan response data swap outside tIFS
* Bluetooth: controller: Avoid adv data set on ADV_EXT_IND PDU	
* Bluetooth: controller: Fix ChSel bit be not used in ADV_EXT_IND
* Bluetooth: Enforce cooperative priorities in Bluetooth threads
* Bluetooth: Shell: Add second vendor service
* Bluetooth: Shell: Implement support for gatt-metrics off
* Bluetooth: GATT: Rework Service Changed indications
* Bluetooth: controller: Rename whitelist arrays
* Bluetooth: controller: Fix resolving list error handling
* Bluetooth: drivers: Make RX thread priority consistent
* Bluetooth: Decrease Rx processing priority
* Bluetooth: conn: Fix notifying all pending tx packets in send_frag()
* Bluetooth: Fix bogus decoding of 8-bit num_handles as 16-bit
* Bluetooth: conn: Switch order of tx_notify & tx_queue
* Bluetooth: controller: Fix first interval to be within 16us


Build and Infrastructure
************************

* build: place app objects in libapplication.a

Libraries
*********

* ext: lib: mbedtls: Optimize example config for Zephyr
* ext: lib: mbedtls: Add Kconfig option to enable mbedtls debugging
* ext: lib: mbedtls: Enable and set heap size at device startup
* libc: minimal: Add empty sys/cdefs.h


HALs
****

* ext: hal: stm32f1x and stm32f4x: disable i2c HAL
* interrupt_controller: add STM32F303XC cc3220sdk I lines number
* dts: Rename k64sim to nxp
* rts: Kconfig: Add QMSI dependency on RTC options
* i2c: stm32: Cleanup how we enable the specific I2C driver
* pinmux: stm32: nucleo_l432kc: Add SPI pins
* pinmux: stm32: nucleo_l476rg: Fix SPI Pinmux
* pinmux: stm32: nucleo_f334r8: add support for SPI
* pinmux: stm32: nucleo_f401re: Add support for SPI
* pinmux: stm32f4: Add SPI2 pins on PB12
* pinmux: stm32f4: Add SPI1 pins on PA4
* stm32cube: build stm32xxx_ll_spi if CONFIG_SPI
* stm32cube: Fix warning when SPI LL API is compiled
* soc: stm32: F1/F3/F4/L4: enable I2C LL
* interrupt_controller: add STM32F303XC cc3220sdk I lines number
* pinmux: stm32 F1X/F3X/F4X: add I2C
* clock: stm32: Cleanup use of C99 types
* flash: stm32l4x: Cleanup use of C99 types
* flash: stm32: distinguish read/write for flash range valid
* flash: stm32: fix for l4 writing wrong data
* dts: yaml: Add yaml files for STM32 I2C support to DT
* i2c: stm32: fix build issue


Documentation
*************

* boards: nucleo_l476rg: Document default SPI pinmux
* boards: sensortag: Add documentation.
* doc: boards: qemu: Mention support for SLIP networking
* doc: add qemu_xtensa board instructions
* doc: update qemu_cortex_m3 instructions
* doc: update qemu_x86 instructions
* doc: fix sidebar nav issues with board docs
* doc: fix doc errors in stm32_min_dev.rst (take 2)
* doc: update network buffers push/pull examples
* doc: update sanitycheck doc to clarify some details
* doc: add documentation about sanitycheck
* ztest: update documentation for yaml
* doc: fix references to moved files
* doc: add MPU samples to index
* doc: fix headings in device tree doc
* doc: update release-notes index page
* doc: change UTF-8 chars to sphinx inline replaces
* doc: Add link to v1.8 documentation
* doc: fix doxygen errors in bt include files
* doc: add 1.8 release notes
* boards: stm32373c_eval: fix trailing whitespace in docs
* boards: provide STM32373C-EVAL development board's documentation
* boards: provide STM3210C-EVAL development board's documentation
* boards: provide Nucleo-64 F334R8 development board's documentation
* doc: fix wiki board references in non .rst files
* boards: arm: doc: Add I2C feature and serial port section
* doc: add placeholder for 1.9 release notes


Tests and Samples
*****************

Bluetooth: tests: Fix left-over issues from bt_gatt_service() removal
tests: mbedtls: cleanup prj.conf
tests: crypto: do not build when DEBUG is enabled
tests: remove build_only tag
tests: move spi test under tests/
tests/ieee802154: Fix how is initialized the driver lock in l2 test
tests: crypto: include back riscv32 arch for ecc_dh
tests: tickless: fix building of test
tests: boot_time: cleanup boot_time test to work on ARM
tests: shell: Filter on UART_CONSOLE support to enable more boards
tests: Remove camel case and fix coding style
tests: fix PCI test using 'supported' keyword
tests: replace filters in testcase files
tests: convert testcase.ini to new format
tests: net: Add test for Sockets API UDP socket/bind/connect/send/recv
tests: do not exclude quark_d2000_crb
tests: samples: remove duplicate filtering
tests: dma: fix chan_blen_transfer
tests: protection: add testcase.yaml
tests: protection: convert to testcase.yaml
tests: Add a self-protection test suite
tests/ztest: Add ztest_test_pass()
tests: dns: do not set as slow test
tests: net: context: Do not print error for passed test
tests: net: tcp: Fix tests and run them automatically
tests: net: mld: Fix tests and run them automatically
tests: net: ipv6: Fix tests and run them automatically
tests: net: buf: Run the net_buf tests automatically
ztest: update documentation for yaml
tests: remove testcase.ini files
tests: samples: convert testcase files to yaml
tests: pipe: fix style
tests: pipe: rename test directory
tests: timer_monotonic: fix style
tests: timer_api: fix style
tests: schedule_api: fix style
tests: thread_init: fix style
tests: rename cdata -> custom_data
tests: threads_scheduling ->  threads/scheduling
tests: threads_lifecycle/ -> threads/lifecycle
tests: threads_customdata/ -> threads/customdata
tests: queue: fix code style
tests: profiling_api: fix code style
tests: poll: fix code style
tests: pending: fix code style
tests: obj_tracing: fix code style
tests: mutex_api: fix code style
tests: mutex: fix code style
tests: mbox: fix code style
tests: move ipm test to drivers
tests: ipm: fix code style
tests: fp_sharing: fix code style
tests: fifo: fix code style
tests: critical: fix code style
tests: common: fix code style
tests: bitfield: fix code style
tests: arm_runtime_nmi: fix code style
tests: arm_irq_vector_table: fix code style
tests: alert_api: fix code style
tests: sleep: rename test directory
tests: put tickless tests together
tests: workq_api: rename test directory
tests: work_queue: fix code style
tests: workq_api: fix code style
tests: mem_heap: fix code style
tests: mem_pool_threadsafe: fix code style
tests: mem_pool_api: fix code style
tests: mem_pool: fix code style
tests: mslab_threadsafe: fix code style
tests: mslab_concept: fix code style
tests: mslab_api: fix style
tests: rename test directory for mem_pool
tests: rename test directory for mem_slab
tests: rename test directory for fifo
tests: rename test directory for lifo
tests: rename test directory for alert
test: fix misspellings
tests: rename test directory test_bluetooth -> bluetooth
tests: rename test directory test_loop_transfer -> loop_transfer
tests: rename test directory test_chan_blen_transfer
tests: rename test directory test_sha256 -> sha256
tests: rename test directory test_mbedtls -> mbedtls
tests: rename test directory test_hmac_prng -> hmac_prng
tests: rename test directory test_hmac -> hmac
tests: rename test directory test_ecc_dh -> ecc_dh
tests: rename test directory test_ecc_dsa -> ecc_dsa
tests: rename test directory test_ctr_prng -> ctr_prng
tests: rename test directory test_ctr_mode -> ctr_mode
tests: rename test directory test_cmac_mode -> cmac_mode
tests: rename test directory test_ccm_mode -> ccm_mode
tests: rename test directory test_cbc_mode -> cbc_mode
tests: rename test directory test_aes -> aes
tests: net: ipv6_fragment: Fix llvm compiler warning
tests: benchmark: boot_time: Reading time stamps made arch agnostic
tests: benchmarks: footprint: really enable floating point on arm
tests: benchmarks: footprint: build on ARM
tests: benchmarks: footprint: fix tag typo
tests: net: 6lo: Remove CONFIG_MAIN_STACK_SIZE setting
tests: protection: don't do exec tests on x86
tests: net: remove overriden RAM size
tests: net: route: fix semaphore usage
tests/ieee802154: Fix accessing unmapped memory area
tests/ieee802154: Fix a null pointer deferencing
tests: net: ip-addr: Fix null pointer access in the test
tests: crypto: fix coding style
tests: crypto: sha256: limit to systems with >48k of memory
samples: remove reference to nimble
samples: mqtt_publisher: fixed typo
samples: ipm_mailbox: fix thread creation
samples/spi: Enable test on STM32 SoCs
samples/spi: Permit specifying low test frequency
samples/spi: Make async test optional
Bluetooth: samples: scan_adv: Select BROADCASTER and OBSERVER
samples: net: https: Increase the RAM for client and server
samples: net: http: Remove net_app_init() calls
samples: net: wpan: No need to define IP addresses
samples: nfc_hello: limit to uarts that support interrupts
samples: hello_world: remove min_ram requirement
samples: net Documented QEMU_INSTANCE usage
samples: net: Multiple instances of QEMU
samples: Fix filtering on UART_CONSOLE
samples: net: Add socket-based echo server example
samples: net: mbedtls: Fix server compilation
samples: coaps_server: Fix platforms to build coap_server test on
samples: drivers/net: apps: Resolve Kconfig dependency
samples: hci_uart: optimize testcase definition
samples: environmental_sensing: update filtering
samples: wpanusb: remove unnecessary condition for ccflags
samples: appdev: static_lib: added "make flash"
samples: demonstrate the use of KBUILD_ZEPHYR_APP
samples: move app developemnt samples samples/appdev
samples: move all MPU samples into one directory
samples: move quark se power samples to boards/
samples: move environment sensing to boards
samples: gpio: Add support for SensorTag board.
samples: net: socket_echo: Add sample.yaml
samples: mark samples that require usb_device support
samples: random: Add sentinel to check for buffer overflows
samples: net: zperf: Add net tag
samples: net: ieee802154: add KW40Z prj conf file
samples: net: ieee802154: kw41z: reduce log level
samples: wpanusb: add testcases


JIRA Related Items
******************

.. comment  List derived from Jira query: ...

* :jira:`ZEP-1843` - provide mechanism to filter test cases based on available hardware
* :jira:`ZEP-1892` - Fix issues with Fix Release
* :jira:`ZEP-1902` - Missing board documentation for arm/nucleo_f334r8
* :jira:`ZEP-1911` - Missing board documentation for arm/stm3210c_eval
* :jira:`ZEP-1917` - Missing board documentation for arm/stm32373c_eval
* :jira:`ZEP-2020` - tests/crypto/test_ecc_dsa intermittently fails on riscv32
* :jira:`ZEP-2054` - Convert all helper script to use python3
* :jira:`ZEP-2071` - samples: warning: (SPI_CS_GPIO && SPI_SS_CS_GPIO && I2C_NRF5) selects GPIO which has unmet direct dependencies
* :jira:`ZEP-2184` - Split data, bss, noinit sections into application and kernel areas
* :jira:`ZEP-2225` - Ability to unregister GATT services
* :jira:`ZEP-2267` - Create Release Notes
* :jira:`ZEP-2270` - Convert mpu_stack_guard_test from using k_thread_spawn to k_thread_create
* :jira:`ZEP-2274` - Build warnings [-Wpointer-sign] with LLVM/icx (tests/net/ipv6_fragment)
* :jira:`ZEP-2279` - echo_server TCP handler corrupt by SYN flood
* :jira:`ZEP-2280` - add test case for KBUILD_ZEPHYR_APP
* :jira:`ZEP-2285` - non-boards shows up in board list for docs
* :jira:`ZEP-2289` - [DoS] Memory leak from large TCP packets
* :jira:`ZEP-2303` - Concurrent incoming TCP connections
* :jira:`ZEP-2306` - echo server hangs from IPv6 hop-by-hop option anomaly
* :jira:`ZEP-2318` - some kernel objects sections are misaligned
* :jira:`ZEP-2319` - tests/net/ieee802154/l2 uses semaphore before initialization
* :jira:`ZEP-2328` - gen_mmu.py appears to generate incorrect tables in some situations
* :jira:`ZEP-2329` - bad memory access tests/net/route
* :jira:`ZEP-2330` - bad memory access tests/net/rpl
* :jira:`ZEP-2332` - bad memory access tests/net/ip-addr
* :jira:`ZEP-2334` - bluetooth shell build warning when CONFIG_DEBUG=y
* :jira:`ZEP-2343` - Coverity static scan issues seen
* :jira:`ZEP-2367` - NULL pointer read in udp, tcp, context net tests
* :jira:`ZEP-2368` - x86: QEMU: enable MMU at boot by default

