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

* Use k_cycle_get_32 instead of sys_cycle_get_32 for Kernel
* Added k_panic() and k_oops() APIs for Kernel
* Added k_thread_create() API for Kernel
* Added k_queue API for Kernel
* Add tickless kernel support

Architectures
*************

* arm: Update core to use struct k_thread
* arm: Added ARM MPU support
* dts: Added ARM CMSDK support
* arm: Added Initial support for NXP MPU
* arm: Added Device Tree Support for nRF52832 SoC based boards
* arm: Fixed nRF52840-QIAA SoC support for device tree
* arm: Added Device Tree Support for nRF52840 SoC & boards
* arm: Added Device Tree Support for nRF51822 SoC & boards
* dts: Introduced st/mem.h for FLASH & SRAM sizes
* dts: Put IRQ priority into the interrupt property
* arm: Support for MKL25Z soc
* arm: Added FPU support
* x86: defined MMU data structures



Boards
******

* Added qemu_xtensa board definition
* Added a more informative page fault handler x86 board
* xtensa: build similar to other Zephyr arches
* Define MMU data structures for x86 board
* Added support for board disco_l475_iot1
* Added STM32F413 Nucleo board
* Added support for the CC3220SF_LAUNCHXL board
* Support for new ARM board FRDM-KL25Z
* arduino_101 board enable GPIO by default
* boards: convert to using newly introduced integer sized types
* arm: Added support for Nucleo L432KC board
* arm: Added support for STM32L496G Discovery board
* arm: Added support for STM32F469I-DISCO board



Drivers and Sensors
*******************

* UART interrupt-driver API is better defined
* Support for pull-style console API
* Added MCUX TRNG driver
* Added support for the SiFive Freedom E310 pinmux driver
* drivers/sensor: Convert formatter strings to use PRI defines
* Added lps22hb sensor driver
* Added lsm6dsl sensor driver
* Added heart rate sensor driver
* Added support for max30101 heart rate sensor
* Added support for lis2dh accelerometer



Networking
**********

* net: ieee802154: Added native IEEE 802.15.4 driver for KW41Z
* net/http: Added the HTTP/1.1 API
* net: Added net_buf pool support to each context
* net: ipv6: Added support to join/leave multicast group
* net: Synchronize TX and RX threads startup
* net: Support fragmented IPv6 packets
* net: Added HTTP server library support
* net: Added TCP statistics support



Bluetooth
*********

* Added Bluetooth 5.0 LE commands and events
* Added support for logging packet drops
* Introduced a timeout for synchronous HCI command sending
* Support for ServiceSearchAttributeRequest
* Support BT 5.0 feature set bit fields
* Added L2CAP throughput measurement


Build and Infrastructure
************************

* Support building host tools
* Added separate DTS target
* Added support for MSYS2
* Use -O2 instead of -Os for ARC with SDK 0.9


Libraries
*********

* Added library for software driven I2C
* Created a HTTP library
* Added HTTP server library support
* Added minimal JSON library
* Update TinyCrypt to version 0.2.6
* Added minimal JSON library


HALs
****


* Added Atmel SAM family I2C (TWIHS) driver
* Added Atmel SAM serial (UART) driver
* Added WDT driver for Atmel SAM SoCs
* Added Atmel SAM4S SoC support
* Imported Nordic 802.15.4 radio driver
* Added Initial support for NXP MPU
* Updated QMSI to 1.4 RC4
* Added FPU support
* Added basic support for STM32F413
* Introduced STM32F4x DMA driver
* pinmux: stm32: Added support for Nucleo L432KC
* Added support for STM32L496G Discovery board
* Added dts for STM32F407
* Added support for STM32F4DISCOVERY Board
* Added support for STM32F469XI
* Added support for STM32F469I-DISCO



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


* Added test to verify same tick timeout expiry order
* Added clock_test for kernel
* Added tickless tests
* Added a simple CC2520 crypto dev test
* Added combined observer & broadcaster app for Bluetooth samples
* Added support to wait both IPv4 and IPv6
* Enabled tickless kernel option in some apps


Deprecations
************

* Interesting change

JIRA Related Items
******************

.. comment  List derived from Jira query: ...

Zephyr Kernel 1.8.0
The following sections provide a detailed list of changes, by component, since kernel version 1.7.0.

* :jira:`ZEP-248` - Add a BOARD/SOC porting guide
* :jira:`ZEP-287` - CC2520 driver won't work with multiple devices on the same SPI bus
* :jira:`ZEP-339` - Tickless Kernel
* :jira:`ZEP-472` - Ethernet packets are getting missed if sent in quick succession.
* :jira:`ZEP-540` - add APIs for asynchronous transfer callbacks
* :jira:`ZEP-616` - OS X setup instructions not working on El Capitan
* :jira:`ZEP-628` - Validate RPL Routing node support
* :jira:`ZEP-634` - document IRQ handling internals
* :jira:`ZEP-638` - feature to consider: flag missing functionality at build time when possible
* :jira:`ZEP-720` - Add MAX30101 heart rate sensor driver
* :jira:`ZEP-828` - IPv6 - Multicast Join/Leave Support
* :jira:`ZEP-843` - Unified assert/unrecoverable error infrastructure
* :jira:`ZEP-852` - SPI API Update
* :jira:`ZEP-888` - 802.15.4 - Security support
* :jira:`ZEP-932` - Adapt kernel sample & test projects
* :jira:`ZEP-948` - Revisit the timeslicing algorithm
* :jira:`ZEP-973` - Remove deprecated API related to device PM, DEVICE and * SYS * macros
* :jira:`ZEP-1028` - shrink k_block struct size
* :jira:`ZEP-1032` - IPSP router role support
* :jira:`ZEP-1086` - new IP stack: Cryptic debug messages
* :jira:`ZEP-1169` - Sample mbedDTLS DTLS client stability on ethernet driver
* :jira:`ZEP-1171` - Event group kernel APIs
* :jira:`ZEP-1280` - Provide Event Queues Object
* :jira:`ZEP-1313` - porting and user guides must include a security section
* :jira:`ZEP-1326` - Clean up _THREAD_xxx APIs
* :jira:`ZEP-1388` - Add support for KW40 SoC
* :jira:`ZEP-1391` - Add support for Hexiwear KW40
* :jira:`ZEP-1392` - Add FXAS21002 gyroscope sensor driver
* :jira:`ZEP-1399` - Neighbor Solicitation not sent before echo request
* :jira:`ZEP-1435` - Improve Quark SE C1000 ARC Floating Point Performance
* :jira:`ZEP-1438` - AIO: AIO Comparator is not stable on D2000 and Arduino101
* :jira:`ZEP-1444` - Arduino_101 doesn't response ipv4 ping request after enable ipv4
* :jira:`ZEP-1463` - Add Zephyr Support in segger SystemView
* :jira:`ZEP-1464` - Review files in source tree and add missing licences
* :jira:`ZEP-1470` - Enabling CONFIG_FP_SHARING on ARM causes usage fault
* :jira:`ZEP-1500` - net/mqtt: Test case for the MQTT high-level API
* :jira:`ZEP-1528` - Provide template for multi-core applications
* :jira:`ZEP-1529` - Unable to exit menuconfig
* :jira:`ZEP-1530` - Hotkeys for the menu at the bottom of menuconfig sometimes doesn't work
* :jira:`ZEP-1544` - Arduino_101 doesn't respond to ipv6 ping request after enable echo_server ipv6
* :jira:`ZEP-1551` - Build error when STM32 GPIO Port F/G is selected
* :jira:`ZEP-1568` - Replace arm cortex_m scs and scb functionality with direct CMSIS-core calls
* :jira:`ZEP-1586` - menuconfig: Backspace is broken
* :jira:`ZEP-1599` - printk() support for the '-' indicator in format string (left justifier)
* :jira:`ZEP-1607` - Json encoding/decoding library
* :jira:`ZEP-1621` - Stack Monitoring
* :jira:`ZEP-1627` - Echo_server doesn't respond after client reconnects
* :jira:`ZEP-1631` - Ability to use k_mem_pool_alloc (or similar API) from ISR
* :jira:`ZEP-1652` - Port eth_dw (Ethernet device of Galileo boards) to the new IP stack
* :jira:`ZEP-1684` - Add Atmel SAM family watchdog (WDT) driver
* :jira:`ZEP-1695` - Support ADXL362 sensor
* :jira:`ZEP-1698` - BME280 support for SPI communication
* :jira:`ZEP-1711` - xtensa build defines Kconfigs with lowercase names
* :jira:`ZEP-1718` - support for IPv6 fragmentation
* :jira:`ZEP-1719` - TCP does not work with 6lo
* :jira:`ZEP-1721` - many tinycrypt test cases only run on ARM and x86
* :jira:`ZEP-1722` - xtensa: tinycrypt does not build
* :jira:`ZEP-1725` - SPI Bus Frequency Mislabelled
* :jira:`ZEP-1726` - Kernel Primer doc missing for k_poll
* :jira:`ZEP-1735` - Controller to Host flow control
* :jira:`ZEP-1737` - Network samples sometimes crash right after startup
* :jira:`ZEP-1759` - All python scripts needed for build should be moved to python 3 to minimize dependencies
* :jira:`ZEP-1761` - K_MEM_POOL_DEFINE build error "invalid register name" when built with llvm/icx from ISSM toolchain
* :jira:`ZEP-1769` - Implement Set Event Mask and LE Set Event Mask commands
* :jira:`ZEP-1772` - re-introduce controller to host flow control
* :jira:`ZEP-1773` - bt_conn_le_param_update API improvements
* :jira:`ZEP-1776` - sending LE COC data from RX thread can lead to deadlock
* :jira:`ZEP-1777` - l2cap-connect re-use CID of already connected PSM
* :jira:`ZEP-1785` - Tinytile: Flashing not supported with this board
* :jira:`ZEP-1788` - [REG] bt_enable: No HCI driver registered
* :jira:`ZEP-1791` - Add missing platforms to the "Reference Hardware / Platform" field's selection list
* :jira:`ZEP-1800` - Update external mbed TLS library to latest version (2.4.2)
* :jira:`ZEP-1812` - Add tickless kernel support in HPET timer
* :jira:`ZEP-1816` - Add tickless kernel support in LOAPIC timer
* :jira:`ZEP-1817` - Add tickless kernel support in ARCV2 timer
* :jira:`ZEP-1818` - Add tickless kernel support in cortex_m_systick timer
* :jira:`ZEP-1821` - Update PM apps to use mili/micro seconds instead of ticks
* :jira:`ZEP-1823` - Improved Benchmarks
* :jira:`ZEP-1825` - Context Switching KPI
* :jira:`ZEP-1829` - Approval and publication of the Zephyr Trademark Usage Guidelines
* :jira:`ZEP-1836` - Expose current ecb_encrypt() as bt_encrypt() so host can directly access it
* :jira:`ZEP-1841` - Implement missing functionality for handling of IPv6 next header (HBH hop-by-hop-option) which holds RPL related information.
* :jira:`ZEP-1856` - remove legacy micro/nano kernel APIs
* :jira:`ZEP-1857` - Build warnings [-Wpointer-sign] with LLVM/icx (bluetooth_handsfree)
* :jira:`ZEP-1862` - the mqtt_publisher sample app tested failed on arduino_101 board
* :jira:`ZEP-1863` - RPL based replies are dropped at L2 (802154) level
* :jira:`ZEP-1866` - Add Atmel SAM family I2C (TWIHS) driver
* :jira:`ZEP-1867` - Create SWQRC-specific filters
* :jira:`ZEP-1880` - "samples/grove/temperature": warning raised when generating configure file
* :jira:`ZEP-1882` - Enabling ADC on arduino101 for Zephyr OS
* :jira:`ZEP-1884` - Build warnings [-Wpointer-sign] with LLVM/icx (samples/net/zperf)
* :jira:`ZEP-1885` - Build warnings [-Wpointer-sign] with LLVM/icx (tests/net/ip-addr)
* :jira:`ZEP-1886` - Build warnings [-Wpointer-sign] with LLVM/icx (tests/net/nbuf)
* :jira:`ZEP-1887` - Build warnings [-Wpointer-sign] with LLVM/icx (tests/drivers/spi/spi_basic_api)
* :jira:`ZEP-1889` - samples/logger-hook: sample case won't run on d2000 in v1.7.0-rc4 branch
* :jira:`ZEP-1893` - openocd: 'make flash' works with Zephyr SDK only and fails for all other toolchains
* :jira:`ZEP-1894` - the http_client sample app issue on FRDM K64F
* :jira:`ZEP-1896` - [PTS] L2CAP/LE/CFC/BV-06-C
* :jira:`ZEP-1899` - Missing board documentation for xtensa/xt-sim
* :jira:`ZEP-1901` - Missing board documentation for arm/olimexino_stm32
* :jira:`ZEP-1908` - Missing board documentation for arm/nucleo_96b_nitrogen
* :jira:`ZEP-1910` - Missing board documentation for arm/96b_carbon
* :jira:`ZEP-1913` - Missing board documentation for x86/minnowboard
* :jira:`ZEP-1914` - Missing board documentation for x86/galileo
* :jira:`ZEP-1927` - AIO: AIO_CMP_POL_FALL is triggered immediately after aio_cmp_configure
* :jira:`ZEP-1935` - Packet loss make RPL mesh more vulnerable
* :jira:`ZEP-1936` - tests/drivers/spi/spi_basic_api/testcase.ini#test_spi - Assertion Fail
* :jira:`ZEP-1940` - Event_mask conflict in net_event.h
* :jira:`ZEP-1941` - Controller gets stuck when disabling advertising (no assert)
* :jira:`ZEP-1946` - Time to Next Event
* :jira:`ZEP-1953` - The sequence number of TCP Retransmission is Error.
* :jira:`ZEP-1955` - Nested interrupts crash on Xtensa architecture
* :jira:`ZEP-1956` - There is no TCP timeout retransmission mechanism.
* :jira:`ZEP-1957` - There is no upper limit on the number of tcp data retransmissions.
* :jira:`ZEP-1959` - Add Atmel SAM family serial (UART) driver
* :jira:`ZEP-1961` - After the connection is established, there are some errors during the 4-way Handshake.
* :jira:`ZEP-1965` - net-tools HEAD is broken for QEMU/TAP
* :jira:`ZEP-1966` - Doesn't seem to be able to both send and receive locally via local address
* :jira:`ZEP-1968` - "make mrproper" removes top-level dts/ dir, makes ARM builds fail afterwards
* :jira:`ZEP-1974` - There is no timeout retransmission mechanism in the Client side during the 3-way Handshake.
* :jira:`ZEP-1979` - Restoring "make outputexports" expected behavior after DTS merge
* :jira:`ZEP-1980` - Move app_kernel benchmark to unified kernel
* :jira:`ZEP-1984` - net_nbuf_append(), net_nbuf_append_bytes() have data integrity problems
* :jira:`ZEP-1990` - Basic support for the BBC micro:bit LED display
* :jira:`ZEP-1993` - Flowcontrol Required for CDC_ACM
* :jira:`ZEP-1994` - Linker error when building Arduino 101 on Mac
* :jira:`ZEP-1995` - samples/subsys/console breaks xtensa build
* :jira:`ZEP-1997` - Crash during startup if co-processors are present
* :jira:`ZEP-2006` - missing file reference in boards/nios2/altera_max10 doc
* :jira:`ZEP-2008` - Port tickless idle test to unified kernel and cleanup
* :jira:`ZEP-2009` - Port test_sleep test to unified kernel and cleanup
* :jira:`ZEP-2011` - Retrieve RPL node information through CoAP requests
* :jira:`ZEP-2012` - Fault in networking stack for cores that can't access unaligned memory
* :jira:`ZEP-2013` - dead object monitor code
* :jira:`ZEP-2014` - Defaul samples/subsys/shell/shell fails to build on QEMU RISCv32 / NIOS2
* :jira:`ZEP-2015` - the http_client sample app runs failed on FRDM K64F board
* :jira:`ZEP-2016` - Description on UART API (uart.h) is vague, semantics unclear, possibly questionable
* :jira:`ZEP-2019` - Xtensa port does not compile if CONFIG_TICKLESS_IDLE is enabled
* :jira:`ZEP-2021` - tests/crypto/test_ecc_dsa hangs on xtensa
* :jira:`ZEP-2024` - A101 can't use BMI160 sensor and GPIO input at the same time
* :jira:`ZEP-2026` - Add 802.15.4 driver for KW41Z
* :jira:`ZEP-2027` - Bluetooth Peripheral Sample won't pair with certain Android devices
* :jira:`ZEP-2029` - xtensa: irq_offload() doesn't work on XRC_D2PM
* :jira:`ZEP-2033` - Channel Selection Algorithm #2
* :jira:`ZEP-2034` - High Duty Cycle Non-Connectable Advertising
* :jira:`ZEP-2036` - the mqtt_publisher sample app tested failed on frdm_k64f board
* :jira:`ZEP-2037` - Malformed echo response
* :jira:`ZEP-2040` - samples: net: dns_resolve: got dns timeout and the Incomplete README.rst file
* :jira:`ZEP-2048` - Change UART "baud-rate" property to "current-speed"
* :jira:`ZEP-2050` - Enable LE events based on controller supported features
* :jira:`ZEP-2051` - Move away from C99 types to zephyr defined types
* :jira:`ZEP-2052` - arm: unhandled exceptions in thread take down entire system
* :jira:`ZEP-2053` - Update tinycrypt to v.0.2.6 in recent releases
* :jira:`ZEP-2055` - Add README.rst in the root of the project for github
* :jira:`ZEP-2057` - crash in tests/net/rpl on qemu_x86 causing intermittent sanitycheck failure
* :jira:`ZEP-2060` - Test Task
* :jira:`ZEP-2061` - samples/net/dns_resolve networking setup/README is confusing
* :jira:`ZEP-2064` - RFC: Making net_shell command handlers reusable
* :jira:`ZEP-2065` - struct dns_addrinfo has unused fields
* :jira:`ZEP-2066` - nitpick: SOCK_STREAM/SOCK_DGRAM values swapped compared to most OSes
* :jira:`ZEP-2067` - Atmel SAM3 SoC needs to be moved to the Atmel SAM directory.
* :jira:`ZEP-2069` - samples: net: dhcpv4_client: runs failed on frdm k64f board
* :jira:`ZEP-2070` - net pkt doesn't full unref after send a data form bluetooth's ipsp
* :jira:`ZEP-2072` - samples/net/mbedtls_dtlsclient: compile warnings
* :jira:`ZEP-2074` - cdc-acm printing error
* :jira:`ZEP-2076` - samples: net: coaps_server: build failed
* :jira:`ZEP-2077` - Fix IID when using CONFIG_NET_L2_BLUETOOTH_ZEP1656
* :jira:`ZEP-2079` - tests/crypto/test_ccm_mode - failing for quark_d2000
* :jira:`ZEP-2080` - No reply from RPL node after 20-30 minutes.
* :jira:`ZEP-2088` - [net/rpl] Insert a empty hop-by-hop option header when send UDP, result in receiver drop out this message
* :jira:`ZEP-2092` - [NRF][BT] Makefile:946: recipe for target 'include/generated/generated_dts_board.h' failed
* :jira:`ZEP-2100` - HTTP client API is missing
* :jira:`ZEP-2104` - IP stack: TCP FIN handling is incorrect for both local close and peer close
* :jira:`ZEP-2110` - static code scan (coverity) issues seen
* :jira:`ZEP-2112` - Legacy Application Porting Guide refers to deleted file: include/legacy.h
* :jira:`ZEP-2113` - Bluetooth peripheral examples don't print 6-digit passcodes correctly
* :jira:`ZEP-2114` - tests/kernel/fatal : Fail for QC1000/arc
* :jira:`ZEP-2119` - 'make flash' target fails for SoCs with DTS flow
* :jira:`ZEP-2120` - IP stack: Incorrect algorithm for TCP sequence number comparison
* :jira:`ZEP-2121` - IP stack: If Zephyr's ACK is lost, TCP connection is hosed
* :jira:`ZEP-2122` - BT controller occasionally generates imprecise data abort
* :jira:`ZEP-2125` - Compilation error when UART1 port is enabled via menuconfig
* :jira:`ZEP-2128` - Static code scan (coverity) issues seen
* :jira:`ZEP-2130` - Static code scan (coverity) issues seen
* :jira:`ZEP-2131` - Static code scan (coverity) issues seen
* :jira:`ZEP-2132` - Build samples/bluetooth/hci_uart fail
* :jira:`ZEP-2134` - Static code scan (coverity) issues seen
* :jira:`ZEP-2135` - Static code scan (coverity) issues seen
* :jira:`ZEP-2138` - Static code scan (coverity) issues seen
* :jira:`ZEP-2139` - galileo pinmux driver is using deprecated APIs
* :jira:`ZEP-2141` - Coverity CID 169303 in tests/net/ipv6/src/main.c
* :jira:`ZEP-2143` - Compilation Error on Windows 10 with MSYS2
* :jira:`ZEP-2146` - Failed to setup development environment on macOS
* :jira:`ZEP-2147` - device tree fixups are appended to header file, after the #endif
* :jira:`ZEP-2152` - Xtensa crashes on startup for cores with coprocessors
* :jira:`ZEP-2159` - Build warnings [-Wpointer-sign] with LLVM/icx (tests/lib/json)
* :jira:`ZEP-2166` - Memory leak from fragmented payload
* :jira:`ZEP-2170` - subsys/console/getchar.c: build warning with llvm
* :jira:`ZEP-2172` - re-add cpp samples that were removed by mistake
* :jira:`ZEP-2174` - Infinite loop caused by NS with unkown ICMPv6 option type (0xff)
* :jira:`ZEP-2177` - Ensure the Licensing page is up-to-date for the release
* :jira:`ZEP-2178` - Static code scan (coverity) issues seen
* :jira:`ZEP-2181` - HTTP 400 Bad request forever
* :jira:`ZEP-2200` - BLE central error "ATT Timeout" when subscribing to NOTIFY attr
* :jira:`ZEP-2208` - Coverity static scan issues seen
* :jira:`ZEP-2211` - Makefile.toolchain.gccarmemb sets DTC to /usr/bin/dtc which is not available on macOS
* :jira:`ZEP-2219` - Infinite loop caused by NA with unkown ICMPv6 option type (0xff)
* :jira:`ZEP-2221` - irq_offload() unimplemented for Cortex M0 boards
* :jira:`ZEP-2222` - [nrf 51] Zephyr fails to compile if TICKLESS_KERNEL is enabled for nrf51
* :jira:`ZEP-2223` - samples: net: http_client: Cannot send OPTIONS/HEAD request on FRDM K64F
* :jira:`ZEP-2224` - kernel/mem_pool: test_mpool_alloc_free_isr hits memory crash
* :jira:`ZEP-2229` - test_time_slicing_preemptible fails on bbc_microbit and other NRF boards
* :jira:`ZEP-2235` - Coverity static scan issues seen
* :jira:`ZEP-2240` - kernel/test_build/test_newlib / fails for stm32373c_eval with ARM cross-compiler
* :jira:`ZEP-2243` - irq_lock() does not disable SYSTICK on Arduino Due
* :jira:`ZEP-2244` - TCF: tests/kernel/fatal/testcase.ini#stack-sentinel multiple failures on ARM platforms
* :jira:`ZEP-2245` - TCF: tests/kernel/mem_slab/test_mslab_threadsafe/testcase.ini FAILURE @ sam-e70
* :jira:`ZEP-2257` - test_context fails on bbc_microbit
* :jira:`ZEP-2260` - Arduino Due doesn't print to console after v1.8.0-rc3-9-gbd290b4



Known Issues
************

* :jira:`ZEP-0000` - Title
  - Workaround if available, or "No workaround, will address in a future release."
