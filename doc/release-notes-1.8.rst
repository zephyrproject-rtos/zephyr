.. _zephyr_1.8:

Zephyr Kernel 1.8.0
####################

We are pleased to announce the release of Zephyr kernel version 1.8.0.

This release ... (overview paragraph of major changes goes here)...

Major enhancements with this release include:

* Tickless kernel
* IP Stack improvements
* Bluetooth 5.0 features
* Ecosystem: Tracing, debugging support through third-party tools
* Improved build support on Mac and Windows development environments
* Improved debug support
* Third-Party compilers support
* Xtensa GCC support
* Initial implementation of MMU/MPU support
* Expanded device support

The following sections provide detailed lists of changes by component.

Kernel
******

•	kernel: remove remaining microkernel references
•	kernel: remove all remaining references to nanokernel
•	kernel: error on inclusion of nanokernel.h and microkernel.h
•	kernel: rename nanoArchInit->kernel_arch_init
•	kernel: remove mentions of obsolete CONFIG_NANO_TIMERS
•	kernel: add k_panic() and k_oops() APIs
•	kernel: add k_thread_create() API
•	kernel: Add thread events to kernel event logger
•	kernel: Add k_queue API
•	kernel: tickless: Add function to check if list contains multiple nodes
•	kernel: tickless: Rename _Swap to allow creation of macro
•	kernel: Add stack_info to k_thread
•	kernel: tickless: Add tickless kernel support
•	kernel: thread: remove legacy support
•	kernel: remove legacy kernel support
•	kernel: remove legacy.h and MDEF support
•	kernel: remove legacy semaphore groups support


Architectures
*************

•	arm: soc: beetle: Add regions for mpu configuration
•	arm: core: Add MPU parameter to the arm core
•	arm: core: mpu: Add ARM MPU support
•	dts: mps2_an385: Add ARM CMSDK support
•	arm: soc: nxp k6x: Add Initial support for NXP MPU
•	arm: core: mpu: Add arm core MPU interface
•	arm: core: mpu: Add core support to ARM MPU
•	arm: core: mpu: Add core MPU implementation
•	arm: core: Integrate thread stack guard feature
•	arm: spi: spi master support for nrf52 family
•	arm: dts: Add DTS support for SAME70 SoC
•	arm: dts: ti_lm3s6965: Add Device Tree Support
•	arm: dts: ti_lm3s6965: Add device tree support for Stellaris UART
•	arm: nrf52: Introduce NRF52 SoC Specific config options
•	arm: curie_ble: Report curie_ble as a unique board
•	arm: dts: Add DTS support for NRF52832 SoC
•	arch: sam3x: update Kconfig options after move to SAM SoC family tree
•	arm: Convert Atmel SAM4S series MCU to use ASF
•	arm: Support for new STM32F4 socs (STM32F407 and STM32F429)
•	arm: IRQ number correction in the soc configuration
•	arm: UART driver modifications for MKL25Z soc support
•	arm: Support for MKL25Z soc
•	arm: GPIO driver modifications for MKL25Z soc support
•	arm: stm32f4: Do not enable USART1/USART2 by default
•	arm: stm32f4: Add basic support for STM32F413
•	quark_se: do not enable x86 SPI on ARC
•	x86: add a more informative page fault handler



Boards
******

•	arm: dts: nrf: Add Device Tree Support for nRF52840 SoC & boards
•	arm: dts: nrf: Add Device Tree Support for nRF51822 SoC & boards
•	arm: dts: nrf: Add Device Tree Support for nRF52832 SoC based boards
•	hci_usb: Add project configuration for tinyTile board.
•	x86: add a more informative page fault handler
•	Revert "boards: panther: Use 115200 baudrate for BLE UART"
•	hci_usb: Add project configuration for tinyTile board.
•	dts: provide stm32 soc dtsi files for stm32 base boards
•	dts: add dts for nucleo boards
•	dts: provide dts for stm32 eval boards
•	dts: provide dts files for non st stm32 based boards
•	board: Add support for board disco_l475_iot1
•	boards: disco_l475_iot1: Remove unexpected I2C address for HTS221
•	stm32f4: Add STM32F413 Nucleo board
•	boards: Add support for the CC3220SF_LAUNCHXL board
•	cc3200: Set warning to deprecate board in Zephyr v1.8
•	arm: Support for new ARM board FRDM-KL25Z
•	boards: Update arc em_starterkit support from 2.2 to 2.3
•	boards: mps2_an385: Enable I2C devices
•	boards: Add panther & panther_ss to sanity
•	arm: dts: st: Convert STM32F4 based boards to dts
•	arm: dts: st: Convert STM32F1 based boards to dts
•	arm: dts: st: Convert STM32F3 based boards to dts
•	arm: curie_ble: Report curie_ble as a unique board
•	boards: arm: add support for redbear ble nano 2
•	hexiwear_kw40z: Add hexiwear_kw40z board
•	scripts: Add support for 'make debug' using Segger JLink on NXP boards
•	scripts: Add flash and debug support using pyOCD on NXP boards
•	scripts:nrf: Add 'make flash' for nrf boards.
•	boards: arm: Add support for Nucleo L432KC
•	boards: arm: Add support for STM32L496G Discovery board
•	arm: Add support for STM32F4DISCOVERY Board
•	boards: arm: Add support for STM32F469I-DISCO
•	boards: 96b_carbon_nrf51: add support for 96Boards Carbon nRF51 chip
•	boards: 96b_carbon: Add MPU support
•	boards: nucleo_f401re: Add MPU support
•	boards: nucleo_f411re: Add MPU support
•	boards: v2m_beetle: Add OpenOCD for debugging
•	dts: arm: Add base DTS support for Olimexino STM32 board
•	dts: add dts for nucleo boards	
•	pinmux: stm32: Add support for Nucleo L432KC
•	pinmux: stm32l4x: Fix USART 2 pinmux for nucleo-l432kc
•	Xtensa port: Moved coporcessor context area inside struct _k_thread
•	xtensa port: Fixed crash on startup on CP enabled cores
•	xtensa port: Clear the CP descriptor of new created thread.
•	xtensa port: Added documentation for xt-sim board.
•	xtensa port: Fixed compilation error introduced by recent changes.
•	xtensa port: Removed XRC_D2PM SoC configuration
•	xtensa port: Fixed crash on interrupt handlers when logger is enabled



Drivers and Sensors
*******************

* UART interrupt-driver API is better defined
* Support for pull-style console API
•	drivers: console: Do not wait on the DTR signal from the host USB controller.
•	drivers: pwm: remove deprecated PWM API usage.
•	drivers: spi: add nRF5 slave driver
•	ext: mcux: add Makefiles for building the XCVR driver
•	net: ieee802154: add native IEEE 802.15.4 driver for KW41Z
•	ext: mcux: update XCVR driver to MCUX 2.2
•	Added sensor driver for ADXL362
•	drivers: i2c: stm32lx: Add support for I2C_2
•	pinmux: added support for the SiFive Freedom E310 pinmux driver
•	interrupt_controller: added support for the SiFive Freedom E310 PLIC driver
•	serial: added support for the SiFive Freedom E310 UART driver
•	gpio: added support for the SiFive Freedom E310 GPIO driver
•	drivers: Add support for BBC micro:bit LED display
•	gpio: Add gpio_mmio32 driver to access basic 32-bit i/o registers
•	sensors: BME280: added support for SPI communication
•	sensors: add lps22hb sensor driver
•	sensor: add lsm6dsl sensor driver
•	sensor: fxas21002: Add gyroscope driver
•	sensor: Introduce red and green light sensor channels
•	sensor: max30101: Add heart rate sensor driver
•	hexiwear_k64: Add support for max30101 heart rate sensor
•	sensor: lis2dh: Add support for lis2dh accelerometer
•	drivers: Add Atmel SAM family I2C (TWIHS) driver
•	drivers: Add Atmel SAM serial (UART) driver
•	drivers: Extend Atmel SAM GPIO driver
•	watchdog: Add WDT driver for Atmel SAM SoCs



Networking
**********

•	net: ieee802154: add native IEEE 802.15.4 driver for KW41Z
•	net/dns: Add the static keyword to the dns_find_null routine
•	net: Add network link technology type to linkaddr
•	net: nbuf: Set the link address type in nbuf
•	net: Add net_buf pool support to each context
•	net: nbuf: Create net_nbuf_frag_del() for tracking allocations
•	net: core: Fix IPv6 extension parsing
•	net: if: Create support to flush the TX queue
•	net: ipv6: Add router alert option to MLD msg
•	net: icmpv6: Add function to unregister ICMPv6 handler
•	net: ipv6: Add debug function to convert nbr state to string
•	net: icmp: Update ICMP statistics for every ICMP packet
•	net: shell: Add command for testing TCP connection
•	net: shell: Add DNS query support
•	net: http: Add HTTP server library support
•	net: tcp: Add TCP statistics support
•	net: connection: Move proto2str() to utils.c, rename to net_proto2str()
•	net: tcp: Add TCP sequence number comparison compliant with RFC793.
•	net: rpl: Add RPL header revert utility
•	net: icmpv6: Add support for HBH and RPL option handling
•	net: rpl: Add API to access default rpl instance
•	net: rpl: Add support for handling of DAO ACK
•	net: rpl: Add support for DAO retransmissions
•	net/mgmt: Add a network interface based synchronous event listener
•	net/icpmv4: Add dynamically registered ICMPv4 handlers
•	net/samples: Add static ipv4 addresses for arduino 101 echo apps config
•	net/ieee802154: Add Auxiliary Security Header definitions
•	net/ieee802154: Add generic support for link layer cipher operations
•	net/ieee802154: Provide the means to create secured data frames
•	net/ieee802154: Provide the means to decipher data frames
•	net/ieee802154: Integrate link-layer security relevantly
•	net/ieee802154: Add net mgmt requests to set/get security settings
•	net/ieee802154: Expose auxiliary security header validation function
•	net: Add 802.15.4 useful Kconfig application settings
•	net/samples: Add 802.15.4 link-layer security settings for the samples
•	net/nbuf: Add an attribute to store/get IEEE 802.15.4 RSSI on RX
•	net/ieee802154: Add a Kconfig option to set a default tx power
•	net/ieee802154/samples: Add a Kconfig option to tweak the TX power
•	net/ieee802154: Add ieee15_4 shell module functions to set/get tx power



Bluetooth
*********

•	Bluetooth: AVDTP: Add AVDTP Receive Function
•	Bluetooth: controller: Introduce BLUETOOTH_LL_SW
•	Bluetooth: test: Add "tiny" controller configuration
•	Bluetooth: HCI: Add Bluetooth 5.0 LE commands and events
•	Bluetooth: controller: Add flow control logging
•	Bluetooth: Add support for Bluetooth 5.0 version specifier
•	Bluetooth: monitor: Add support for logging packet drops
•	Bluetooth: HCI: Add define for success status
•	Bluetooth: Controller: Introduce rate-limiting on stack analysis
•	Bluetooth: Add support for tracking transmitted packets
•	Bluetooth: Introduce a timeout for synchronous HCI command sending
•	Bluetooth: Introduce flow control for outgoing ATT packets
•	Bluetooth: Introduce buffer type parameter to bt_buf_get_rx
•	Bluetooth: (Re)introduce ACL host flow control
•	Bluetooth: L2CAP: Add TX queueing for LE CoC
•	Bluetooth: ipsp: Add TX pools for TCP
•	Bluetooth: Controller: Use direct ISR for Radio IRQ only
•	Bluetooth: Controller: Add advertisement event indication feature
•	Bluetooth: Controller: Add Kconfig range check for public address
•	luetooth: shell: Add connection update command
•	Bluetooth: controller: Kconfig for advanced event preparation
•	Bluetooth: controller: Kconfig option for advanced scheduling
•	Bluetooth: controller: Hide advance features in Kconfig
•	Bluetooth: controller: Move comp id and subver to configuration
•	Bluetooth: controller: Add BT 5.0 PDU structs
•	Bluetooth: HCI: Add Bluetooth 5.0 FeatureSet field's bit mapping
•	Bluetooth: l2cap: Use global conn tx pool for segmentation
•	Bluetooth: controller: Add Kconfig options for states and roles
•	Bluetooth: Add LE Features test macro
•	Bluetooth: Add PHY auto-update to 2Mbps on connection
•	Bluetooth: kconfig: Use menu instead of menuconfig
•	Bluetooth: shell: Add L2CAP throughput measurement
•	Bluetooth: shell: Add GATT write cmd throughput measurement



Build and Infrastructure
************************

•	build: make sure we are calling python3 in all scripts
•	build: build host-tools when prebuilts are enabled
•	build: support building host tools
•	build: honor V=1 when flashing via DFU
•	build: Don't remove dts/ directory on clean
•	build: Add separate DTS target
•	build: Add support for MSYS2
•	build: only build gen_idt on x86
•	build: use -O2 instead of -Os for ARC with SDK 0.9


Libraries
*********

•	i2c: bitbang: Add library for software driven I2C
•	net: http: Add HTTP server library support
•	ext: lib: mbedtls: Upgrading mbedTLS library
•	lib: Add minimal JSON library
•	test: Add test for JSON library
•	ext/lib/crypto: Update TinyCrypt to version 0.2.6
•	libc: Add isalnum() to ctype.h
•	lib: Add minimal JSON library
•	lib: json: Parse nested objects and arrays
•	lib: json: Add encoding support
•	debug: Add SEGGER SystemView libraries

HALs
****

•	ext: cc3220sdk: Import HAL for TI CC3220S SoC
•	arch: arm: Convert Atmel SAM4S series MCU to use ASF
•	drivers: Update Atmel SAM family GMAC Ethernet driver
•	ext: Import Atmel SAM4S header files from ASF library
•	arch: Add Atmel SAM4S SoC support
•	ext: Import Nordic 802.15.4 radio driver
•	ext: Integrate Nordic's 802.15.4 radio driver into Zephyr
•	scripts: Add support for 'make debug' using Segger JLink on NXP boards
•	scripts: Add flash and debug support using pyOCD on NXP boards
•	arm: soc: nxp k6x: Add Initial support for NXP MPU
•	ext qmsi: Update QMSI to 1.4 RC3
•	ext qmsi: Update QMSI to 1.4 RC4
•	soc: stm32l4xx: add support for STM32L475XG
•	arm: stm32f4: Add basic support for STM32F413
•	stm32f4: Add STM32F413 Nucleo board
•	dma: stm32f4x: Add dump regs function to aid debugging
•	dma: stm32f4x: Add support for dev-to-mem and mem-to-dev transfers
•	arm: stm32l4: Add configuration and dts for STM32L432XX
•	pinmux: stm32: Add support for Nucleo L432KC
•	arm: stm32l4: Add configuration and dts for STM32L496
•	boards: arm: Add support for STM32L496G Discovery board
•	arm: Add dts for STM32F407
•	arm: Add support for STM32F4DISCOVERY Board
•	stm32f4: Add support for STM32F469XI
•	boards: arm: Add support for STM32F469I-DISCO
•	flash_stm32f4: RDERR is not present on STM32F407
•	dts: arm: Add base DTS support for Olimexino STM32 board
•	ext: cc3220sdk: Import HAL for TI CC3220S SoC
•	cc3220sf: Add support for the TI CC3220SF SoC
•	boards: Add support for the CC3220SF_LAUNCHXL board
•	cc3220sf: Update "baud-rate" dts property to "current-speed"
•	ext: simplelink: Restructure CC3220SDK as SimpleLink SDK



Documentation
*************

* Board documentation added for new board ports
* Continued migration of wiki.zephyrproject.org material to website and github wiki
* Improved CSS formatting and appearance of generated documents
* Added breadcrumb navigation header with kernel version number
* Updated getting started setup guides for Linux, Windows, and macOS
* Updates and additions to follow new and updated kernel features
* Broken link and spelling check scans
* Removed deprecated kernel documentation (pre 1.6 release) from website (still available in git repo if needed)



Tests and Samples
*****************

•	tests: add tests for SYS_DLIST/SLIST_ITERATE_FROM()
•	tests/kernel/common: add test to verify same tick timeout expiry order
•	tests/net/dns: Add the test case for the DNS low-level routines
•	tests: net: Add unit tests for mld
•	tests: add zephyr uart driver api test case
•	tests: add zephyr SPI driver api test case
•	tests: add zephyr pinmux driver api test case
•	tests: kernel: added test case k_is_preempt_thread
•	tests: kernel: added test case k_fifo_is_empty
•	tests: kernel: add test point k_delayed_work_remaining_get
•	tests: kernel: added testapp profiling_api
•	tests: kernel: added test cases k_pipe_block_put
•	tests: kernel: added clock_test
•	tests: kernel: add tickless test
•	tests:kernel: added tests for printk left justifier
•	tests/net/mgmt: Add 2 unit tests around synchronous event listeners.
•	tests/net/ieee802154: Add a simple CC2520 crypto dev test
•	tests/net/ieee802154: Add a unit test for secured data frame validation
•	tests: net: Add mqtt testcases
•	samples/net: Add the QEMU x86 prj file to the HTTP client sample app
•	samples/net: Add the HTTP API to the HTTP server sample application
•	samples/net: Add the HTTP API to the HTTP client sample application
•	samples/net/http: Add the HTTP Basic Authentication routine
•	Bluetooth: samples: Add combined observer & broadcaster app
•	net: samples: echo-client: Allow UDP and TCP run at the same time
•	net: samples: Add support to wait both IPv4 and IPv6
•	samples/dns: Add config file for Arduino-101 and FRDM-K64F
•	samples: tickless: Enables tickless kernel option in some apps


Deprecations
************

* Interesting change

JIRA Related Items
******************

.. comment  List derived from Jira query: ...

•	ZEP-248	        Add a BOARD/SOC porting guide
•	ZEP-339	        Tickless Kernel
•	ZEP-540	        add APIs for asynchronous transfer callbacks
•	ZEP-628	        Validate RPL Routing node support
•	ZEP-638	        feature to consider: flag missing functionality at build time when possible
•	ZEP-720	        Add MAX30101 heart rate sensor driver
•	ZEP-828	        IPv6 - Multicast Join/Leave Support
•	ZEP-843	        Unified assert/unrecoverable error infrastructure
•	ZEP-888	        802.15.4 - Security support
•	ZEP-932	        Adapt kernel sample & test projects
•	ZEP-948	        Revisit the timeslicing algorithm
•	ZEP-973	        Remove deprecated API related to device PM, DEVICE_ and SYS_* macros
•	ZEP-1028	shrink k_block struct size
•	ZEP-1032	IPSP router role support
•	ZEP-1169	Sample mbedDTLS DTLS client stability on ethernet driver
•	ZEP-1171	Event group kernel APIs
•	ZEP-1280	Provide Event Queues Object
•	ZEP-1313	porting and user guides must include a security section
•	ZEP-1326	Clean up _THREAD_xxx APIs
•	ZEP-1388	Add support for KW40 SoC
•	ZEP-1391	Add support for Hexiwear KW40
•	ZEP-1392	Add FXAS21002 gyroscope sensor driver
•	ZEP-1435	Improve Quark SE C1000 ARC Floating Point Performance
•	ZEP-1438	AIO: AIO Comparator is not stable on D2000 and Arduino101
•	ZEP-1463	Add Zephyr Support in segger SystemView
•	ZEP-1500	net/mqtt: Test case for the MQTT high-level API
•	ZEP-1528	Provide template for multi-core applications
•	ZEP-1529	Unable to exit menuconfig
•	ZEP-1530	Hotkeys for the menu at the bottom of menuconfig sometimes doesn't work
•	ZEP-1568	Replace arm cortex_m scs and scb functionality with direct CMSIS-core calls   
•	ZEP-1586	menuconfig: Backspace is broken
•	ZEP-1599	printk() support for the '-' indicator  in format string (left justifier)
•	ZEP-1607	Json encoding/decoding library
•	ZEP-1621	Stack Monitoring
•	ZEP-1631	Ability to use k_mem_pool_alloc (or similar API) from ISR
•	ZEP-1684	Add Atmel SAM family watchdog (WDT) driver
•	ZEP-1695	Support ADXL362 sensor
•	ZEP-1698	BME280 support for SPI communication
•	ZEP-1711	xtensa build defines Kconfigs with lowercase names
•	ZEP-1718	support for IPv6 fragmentation
•	ZEP-1719	TCP does not work with 6lo
•	ZEP-1721	many tinycrypt test cases only run on ARM and x86
•	ZEP-1722	xtensa: tinycrypt does not build
•	ZEP-1735	Controller to Host flow control
•	ZEP-1759	All python scripts needed for build should be moved to python 3 to minimize dependencies
•	ZEP-1761	K_MEM_POOL_DEFINE build error "invalid register name" when built with llvm/icx from ISSM toolchain
•	ZEP-1769	Implement  Set Event Mask and LE Set Event Mask commands
•	ZEP-1772	re-introduce controller to host flow control
•	ZEP-1776	sending LE COC data from RX thread can lead to deadlock
•	ZEP-1785	Tinytile: Flashing not supported with this board
•	ZEP-1788	[REG] bt_enable: No HCI driver registered
•	ZEP-1800	Update external mbed TLS library to latest version (2.4.2)
•	ZEP-1812	Add tickless kernel support in HPET timer
•	ZEP-1816	Add tickless kernel support in LOAPIC timer
•	ZEP-1817	Add tickless kernel support in ARCV2 timer
•	ZEP-1818	Add tickless kernel support in cortex_m_systick timer
•	ZEP-1821	Update PM apps to use mili/micro seconds instead of ticks
•	ZEP-1823	Improved Benchmarks
•	ZEP-1825	Context Switching KPI
•	ZEP-1836	Expose current ecb_encrypt() as bt_encrypt() so host can directly access it
•	ZEP-1856	remove legacy micro/nano kernel APIs
•	ZEP-1857	Build warnings [-Wpointer-sign] with LLVM/icx (bluetooth_handsfree)
•	ZEP-1866	Add Atmel SAM family I2C (TWIHS) driver
•	ZEP-1880	"samples/grove/temperature": warning raised when generating configure file
•	ZEP-1886	Build warnings [-Wpointer-sign] with LLVM/icx (tests/net/nbuf)
•	ZEP-1887	Build warnings [-Wpointer-sign] with LLVM/icx (tests/drivers/spi/spi_basic_api)
•	ZEP-1893	openocd: 'make flash' works with Zephyr SDK only and fails for all other toolchains
•	ZEP-1896	[PTS] L2CAP/LE/CFC/BV-06-C 
•	ZEP-1899	Missing board documentation for xtensa/xt-sim
•	ZEP-1908	Missing board documentation for arm/nucleo_96b_nitrogen
•	ZEP-1910	Missing board documentation for arm/96b_carbon
•	ZEP-1927	AIO: AIO_CMP_POL_FALL is triggered immediately after aio_cmp_configure
•	ZEP-1935	Packet loss make RPL mesh more vulnerable
•	ZEP-1936	tests/drivers/spi/spi_basic_api/testcase.ini#test_spi - Assertion Fail
•	ZEP-1946	Time to Next Event
•	ZEP-1955	Nested interrupts crash on Xtensa architecture
•	ZEP-1959	Add Atmel SAM family serial (UART) driver
•	ZEP-1965	net-tools HEAD is broken for QEMU/TAP
•	ZEP-1966	Doesn't seem to be able to both send and receive locally via local address
•	ZEP-1968	"make mrproper" removes top-level dts/ dir, makes ARM builds fail afterwards
•	ZEP-1980	Move app_kernel benchmark to unified kernel
•	ZEP-1984	net_nbuf_append(), net_nbuf_append_bytes() have data integrity problems
•	ZEP-1990	Basic support for the BBC micro:bit LED display
•	ZEP-1993	Flowcontrol Required for CDC_ACM
•	ZEP-1995	samples/subsys/console breaks xtensa build
•	ZEP-1997	Crash during startup if co-processors are present
•	ZEP-2008	Port tickless idle test to unified kernel and cleanup
•	ZEP-2009	Port test_sleep test to unified kernel and cleanup
•	ZEP-2011	Retrieve RPL node information through CoAP requests 
•	ZEP-2012	Fault in networking stack for cores that can't access unaligned memory
•	ZEP-2013	dead object monitor code
•	ZEP-2014	Defaul samples/subsys/shell/shell fails to build on QEMU RISCv32 / NIOS2
•	ZEP-2019	Xtensa port does not compile if CONFIG_TICKLESS_IDLE is enabled
•	ZEP-2027	Bluetooth Peripheral Sample won't pair with certain Android devices
•	ZEP-2029	xtensa: irq_offload() doesn't work on XRC_D2PM
•	ZEP-2033	Channel Selection Algorithm #2
•	ZEP-2034	High Duty Cycle Non-Connectable Advertising
•	ZEP-2037	Malformed echo response
•	ZEP-2048	Change UART "baud-rate" property to "current-speed"
•	ZEP-2051	Move away from C99 types to zephyr defined types
•	ZEP-2052	arm: unhandled exceptions in thread take down entire system
•	ZEP-2055	Add README.rst in the root of the project for github
•	ZEP-2057	crash in tests/net/rpl on qemu_x86 causing intermittent sanitycheck failure
•	ZEP-2061	samples/net/dns_resolve networking setup/README is confusing
•	ZEP-2064	RFC: Making net_shell command handlers reusable
•	ZEP-2065	struct dns_addrinfo has unused fields
•	ZEP-2066	nitpick: SOCK_STREAM/SOCK_DGRAM values swapped compared to most OSes
•	ZEP-2069	samples:net:dhcpv4_client: runs failed on frdm k64f board
•	ZEP-2070	net pkt doesn't full unref after send a data form bluetooth's ipsp
•	ZEP-2076	samples: net: coaps_server: build failed
•	ZEP-2077	Fix IID when using CONFIG_NET_L2_BLUETOOTH_ZEP1656
•	ZEP-2080	No reply from RPL node after 20-30 minutes.
•	ZEP-2092	[NRF][BT] Makefile:946: recipe for target 'include/generated/generated_dts_board.h' failed
•	ZEP-2114	tests/kernel/fatal : Fail for QC1000/arc
•	ZEP-2125	Compilation error when UART1 port is enabled via menuconfig
•	ZEP-2132	Build samples/bluetooth/hci_uart fail
•	ZEP-2138	Static code scan (coverity) issues seen
•	ZEP-2143	Compilation Error on Windows 10 with MSYS2
•	ZEP-2152	Xtensa crashes on startup for cores with coprocessors 
•	ZEP-2178	Static code scan (coverity) issues seen
•	ZEP-2181	HTTP 400 Bad request forever
•	ZEP-2200	BLE central error "ATT Timeout" when subscribing to NOTIFY attr  
•	ZEP-2208	Coverity static scan issues seen
•	ZEP-2211	Makefile.toolchain.gccarmemb sets DTC to /usr/bin/dtc which is not available on macOS
•	ZEP-2219	Infinite loop caused by NA with unkown ICMPv6 option type (0xff)
•	ZEP-2221	irq_offload() unimplemented for Cortex M0 boards
•	ZEP-2222	[nrf 51] Zephyr fails to compile if TICKLESS_KERNEL is enabled for nrf51
•	ZEP-2223	samples: net: http_client: Cannot send OPTIONS/HEAD request on FRDM K64F
•	ZEP-2224	kernel/mem_pool: test_mpool_alloc_free_isr hits memory crash
•	ZEP-2235	Coverity static scan issues seen
•	ZEP-2240	kernel/test_build/test_newlib / fails for stm32373c_eval with ARM cross-compiler
•	ZEP-2257	test_context fails on bbc_microbit



Known Issues
************

* :jira:`ZEP-0000` - Title
  - Workaround if available, or "No workaround, will address in a future release."
