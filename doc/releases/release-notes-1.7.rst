.. _zephyr_1.7:

Zephyr Kernel 1.7.0
####################

We are pleased to announce the release of Zephyr kernel version 1.7.0. This
release continues refinement of the unified kernel introduced with the 1.6.0
kernel release, simplifying the overall Zephyr architecture and programming
interfaces. This is the last release that will support the deprecated legacy
nano- and micro-kernel APIs found in the 1.5.0 release and earlier.

This release introduces a new native IP stack, replacing the legacy uIP stack,
maintaining the legacy functionality, adding additional capabilities, and allowing
future improvements.

We have introduced support for the RISC V and Xtensa architectures and now
support 6 architectures in total.

Device tree support for ARM based boards added. The initial
device tree support includes flash/sram base address and UART devices.  Board
support includes NXP Kinetis based SoCs, ARM Beetle, TI CC3200 LaunchXL, and
STML32L476 based SoCs. Plan is to add support for other architectures and
expand device support in upcoming Zephyr releases.

The following sections provide a detailed list of changes, by component,  since
kernel version 1.6.0.

Kernel
******

* Introduction of k_poll API: k_poll() is similar to the POSIX poll() API in
  spirit in that it allows a single thread to monitor multiple events without
  actively polling them, but rather pending for one or more to become ready.
* Optimized memory use of some thread fields
* Remove usage of micro/nano kernel terminology from kernel code and introduced
  a legacy option to enable/disable legacy APIs. (using legacy.h)


Architectures
*************

* ARM: Added support for device tree
* ARM: Fixed exception priority access on Cortex M0(+)
* ARM: Refactored to use CMSIS

Boards
******

* Added ARM MPS2_AN385 board
* Added Atmel SAM E70 Xplained board
* Added Nordic pca10056 PDK board
* Added NXP FRDM-KW41Z board
* Added ST Nucleo-F334R8, Nucleo-L476G, STM3210C-EVAL, and STM32373C-EVAL boards
* Added Panther and tinyTILE boards, based on Quark SE C1000 and Intel Curie
* Added support for Zedboard Pulpino, a RISC V based board
* Added Qemu target for RISC V and a simulator target for the Xtensa architecture.

Drivers and Sensors
*******************

* Added Atmel SAM pmc, gpio, uart, and ethernet drivers
* Added STM32F3x clock, flash, gpio, pinmux drivers
* Added stm32cube pwm and clock drivers
* Added cc3200 gpio driver
* Added mcr20a ieee802154 driver
* Added mcux pinmux, gpio, uart, and spi drivers
* Added Beetle clock control and watchdog drivers

Networking
**********

This version removes the legacy uIP stack and introduces a new native IP stack.
Because of this there is lot of changes in the code base. The native IP stack
will support the same functionality as the legacy IP stack found in 1.6, and
add new networking features which are described below.

* IP stack code is moved to subsys/net/ip directory.
* IP stack supports both IPv6 and IPv4, and they can be enabled simultaneously.
* Multiple network technologies like Bluetooth IPSP and IEEE 802.15.4 can be
  enabled simultaneously. No routing functionality is provided by IP stack
  between enabled network technologies, applications need to decide where to
  send the network packets.
* Network technologies are abstracted in IP layer 2 (L2) and presented to
  rest of the system as network interfaces. There exists L2 driver for
  Ethernet, Bluetooth and IEEE 802.15.4.
* Created Bluetooth Internet Protocol Support Profile (IPSP) support. It will
  provide IPv6 connection over Bluetooth connection oriented channel (L2CAP).
* Created DHCPv4 support.
* Created CoAP implementation called ZoAP which replaces uIP based one.
* Updated 6Lo implementation to support both Bluetooth and IEEE 802.15.4
* Created application API (net_context) for creating connections and
  transferring data to external systems.
* Added sample application (wpanusb) for exporting IEEE 802.15.4 radio over
  USB to external operating systems like Linux.
* Added DNS client library.
* Updated TCP implementation.
* Created MQTT publisher support.
* Created network test generator (zperf).
* Created telnet console support.
* Created IRC client sample application.
* Created HTTP server and client sample applications.
* Created net-shell module for interacting with network sub-system.
* Created ieee15_4 shell module for dedicated interaction with
  IEEE 802.15.4 Soft MAC.
* Created network management API for generic network settings request as well
  as a network event notification system (sender/listener).
* Redesigned buffer & pool allocation API.

Bluetooth
*********

* Redesigned buffer pools for smaller memory consumption
* Redesigned thread model for smaller memory consumption
* Utilized new k_poll API to consolidate all TX threads into a single one
* Added more SDP functionality
* Improved RFCOMM support
* Reduced latencies in the Controller
* Added SPI HCI driver

Libraries
*********

* Updated mbedTLS library
* Updated TinyCrypt to version 0.2.5

HALs
****

* Updated FAT FS to rev 0.12b
* Updated Nordic MDK header files
* Updated QMSI to 1.4 RC3
* Imported Atmel SDK (ASF) for SAM E70 and SAM3X
* Imported Nordic SDK HAL and 802.15.4 radio driver
* Renamed NXP KSDK to MCUX
* Imported NXP MCUX for KW41Z
* Imported Segger J-Link RTT library
* Imported stm32cube for F4 and L4

Documentation
*************

* General improvements and additions to kernel component docs
* Moved supported board information back to the website site.
* New website documentation theme to go with the new zephyrproject.org site.
* New local-content generation theme (read-the-docs)
* General spelling checks and organizational improvements.
* Site-wide glossary added.
* Porting guides added.
* Sample README files converted to documents included in the website.
* Improved consistency of :ref:`boards` and :zephyr:code-sample-category:`samples`.


JIRA Related Items
******************


.. comment  List derived from https://jira.zephyrproject.org/issues/?filter=10345

* ``ZEP-19`` - IPSP node support
* ``ZEP-145`` - no 'make flash' for Arduino Due
* ``ZEP-328`` - HW Encryption Abstraction
* ``ZEP-359`` - Move QEMU handling to a central location
* ``ZEP-365`` - Zephyr's MQTT library
* ``ZEP-437`` - TCP/IP API
* ``ZEP-513`` - extern declarations of  small microkernel objects in designated sections require __attribute__((section)) in gp-enabled systems
* ``ZEP-591`` - MQTT Port to New IP Stack
* ``ZEP-604`` - In coap_server sample app, CoAP resource separate is not able to send separate response
* ``ZEP-613`` - TCP/UDP client and server mode functionality
* ``ZEP-641`` - Bluetooth Eddystone sample does not correctly implement Eddystone beacon
* ``ZEP-648`` - New CoAP Implementation
* ``ZEP-664`` - Extend spi_qmsi_ss driver to support save/restore peripheral context
* ``ZEP-665`` - Extend gpio_qmsi_ss driver to support save/restore peripheral context
* ``ZEP-666`` - Extend i2c_qmsi_ss driver to support save/restore peripheral context
* ``ZEP-667`` - Extend adc_qmsi_ss driver to support save/restore peripheral context
* ``ZEP-686`` - docs: Info in Application Development Primer and Developing an Application and the Build System is largely duplicated
* ``ZEP-706`` - cannot set debug breakpoints on ARC side of Arduino 101
* ``ZEP-719`` - Add ksdk uart shim driver
* ``ZEP-734`` - Port AES-CMAC-PRF-128 [RFC 4615] encryption library for Thread support
* ``ZEP-742`` - nRF5x Series: System Clock driver using NRF_RTC
* ``ZEP-744`` - USB WebUSB
* ``ZEP-748`` - Enable mbedtls_sslclient sample to run on quark se board
* ``ZEP-759`` - Add preliminary support for Atmel SAM E70 (Cortex-M7) chipset family and SAM E70 Xplained board
* ``ZEP-788`` - UDP
* ``ZEP-789`` - IPv4
* ``ZEP-790`` - ICMPv4
* ``ZEP-791`` - TCP
* ``ZEP-792`` - ARP
* ``ZEP-793`` - DNS Resolver
* ``ZEP-794`` - Requirements for Internet Hosts - Communication Layers
* ``ZEP-796`` - DHCPv4
* ``ZEP-798`` - IPv6
* ``ZEP-799`` - HTTP over TLS
* ``ZEP-801`` - DNS Extensions to support IPv6
* ``ZEP-804`` - IPv6 Addressing Architecture
* ``ZEP-805`` - Internet Control Message Protocol (ICMP) v6
* ``ZEP-807`` - Neighbor Discovery for IPv6
* ``ZEP-808`` - IPv6 Stateless Autoconfiguration (SLAAC)
* ``ZEP-809`` - IPv6 over 802.15.4
* ``ZEP-811`` - The Trickle Algorithm
* ``ZEP-812`` - Compression Format for IPv6 over 802.15.4
* ``ZEP-813`` - RPL:  IPv6 Routing Protocol
* ``ZEP-814`` - Routing Metrics used in Path Selection
* ``ZEP-815`` - Objective Function Zero for RPL
* ``ZEP-816`` - Minimum Rank with Hysteresis (RPL)
* ``ZEP-818`` - CoAP working over the new IP stack
* ``ZEP-820`` - HTTP v1.1 Server Sample
* ``ZEP-823`` - New IP Stack - Documentation
* ``ZEP-824`` - Network Device Driver Porting Guide
* ``ZEP-825`` - Porting guide for old-to-new IP Stack APIs
* ``ZEP-827`` - HTTP Client sample application
* ``ZEP-830`` - ICMPv6 Parameter Problem Support
* ``ZEP-832`` - Hop-by-Hop option handling
* ``ZEP-847`` - Network protocols must be moved to subsys/net/lib
* ``ZEP-854`` - CoAP with DTLS sample
* ``ZEP-859`` - Migrate ENC28J60 driver to YAIP IP stack
* ``ZEP-865`` - convert filesystem sample to a runnable test
* ``ZEP-872`` - Unable to flash Zephyr on Arduino 101 using Ubuntu and following wiki instructions
* ``ZEP-873`` - DMA API Update
* ``ZEP-875`` - 6LoWPAN - Context based compression support
* ``ZEP-876`` - 6LoWPAN - Offset based Reassembly of 802.15.4 packets
* ``ZEP-879`` - 6LoWPAN - Stateless Address Autoconfiguration
* ``ZEP-882`` - 6LoWPAN - IPv6 Next Header Compression
* ``ZEP-883`` - IP Stack L2 Interface Management API
* ``ZEP-884`` - 802.15.4 - CSMA-CA Radio protocol support
* ``ZEP-885`` - 802.15.4 - Beacon frame support
* ``ZEP-886`` - 802.15.4 - MAC command frame support
* ``ZEP-887`` - 802.15.4 - Management service: RFD level support
* ``ZEP-911`` - Refine thread priorities & locking
* ``ZEP-919`` - Purge obsolete microkernel & nanokernel code
* ``ZEP-929`` - Verify the preempt-thread-only and coop-thread-only configurations
* ``ZEP-931`` - Finalize kernel file naming & locations
* ``ZEP-936`` - Adapt drivers to unified kernel
* ``ZEP-937`` - Adapt networking to unified kernel
* ``ZEP-946`` - Galileo Gen1 board support dropped?
* ``ZEP-951`` - CONFIG_GDB_INFO build not working on ARM
* ``ZEP-953`` - CONFIG_HPET_TIMER_DEBUG build warning
* ``ZEP-958`` - simplify pinmux interface and merge the pinmux_dev into one single API
* ``ZEP-964`` - Add a (hidden?) Kconfig option for disabling legacy API
* ``ZEP-975`` - DNS client port to new IP stack
* ``ZEP-1012`` - NATS client port to new IP stack
* ``ZEP-1038`` - Hard real-time interrupt support
* ``ZEP-1060`` - Contributor guide for documentation missing
* ``ZEP-1103`` - Propose and implement synchronization flow for multicore power management
* ``ZEP-1165`` - support enums as IRQ line argument in IRQ_CONNECT()
* ``ZEP-1172`` - Update logger Api to allow using a hook for SYS_LOG_BACKEND_FN function
* ``ZEP-1177`` - Reduce Zephyr's Dependency on Host Tools
* ``ZEP-1179`` - Build issues when compiling with LLVM from ISSM (icx)
* ``ZEP-1189`` - SoC I2C peripheral of the Quark SE cannot be used from the ARC core
* ``ZEP-1190`` - SoC SPI peripheral of the Quark SE cannot be used from the ARC core
* ``ZEP-1222`` - Add save/restore support to ARC core
* ``ZEP-1223`` - Add save/restore support to arcv2_irq_unit
* ``ZEP-1224`` - Add save/restore support to arcv2_timer_0/sys_clock
* ``ZEP-1230`` - Optimize interrupt return code on ARC.
* ``ZEP-1233`` - mbedDTLS DTLS client stability does not work on top of the tree for the net branch
* ``ZEP-1251`` - Abstract driver re-entrancy code
* ``ZEP-1267`` - Echo server crashes upon reception of router advertisement
* ``ZEP-1276`` - Move disk_access_* out of file system subsystem
* ``ZEP-1283`` - compile option to skip gpio toggle in samples/power/power_mgr
* ``ZEP-1284`` - Remove arch/arm/core/gdb_stub.S and all the abstractions it introduced
* ``ZEP-1288`` - Define _arc_v2_irq_unit device
* ``ZEP-1292`` - Update external mbed TLS library to latest version (2.4.0)
* ``ZEP-1300`` - ARM LTD V2M Beetle Support [Phase 2]
* ``ZEP-1304`` - Define device tree bindings for NXP Kinetis K64F
* ``ZEP-1305`` - Add DTS/DTB targets to build infrastructure
* ``ZEP-1306`` - Create DTS/DTB parser
* ``ZEP-1307`` - Plumbing the DTS configuration
* ``ZEP-1308`` - zephyr thread function k_sleep doesn't work with nrf51822
* ``ZEP-1320`` - Update Architecture Porting Guide
* ``ZEP-1321`` - Glossary of Terms needs updating
* ``ZEP-1323`` - Eliminate references to fiber, task, and nanokernel under ./include
* ``ZEP-1324`` - Get rid of references to CONFIG_NANOKERNEL
* ``ZEP-1325`` - Eliminate TICKLESS_IDLE_SUPPORTED option
* ``ZEP-1327`` - Eliminate obsolete kernel directories
* ``ZEP-1329`` - Rename kernel APIs that have nano\_ prefixes
* ``ZEP-1334`` - Add make debug support for QEMU-based boards
* ``ZEP-1337`` - Relocate event logger files
* ``ZEP-1338`` - Update external fs with new FATFS revision 0.12b
* ``ZEP-1342`` - legacy/kernel/test_early_sleep/ fails on EMSK
* ``ZEP-1347`` - sys_bitfield_*() take unsigned long* vs memaddr_t
* ``ZEP-1351`` - FDRM k64f SPI does not work
* ``ZEP-1355`` - Connection Failed to be Established
* ``ZEP-1357`` - iot/dns: Client is broken
* ``ZEP-1358`` - BMI160 accelerometer gives 0 on all axes
* ``ZEP-1361`` - IP stack is broken
* ``ZEP-1363`` - Missing wiki board support page for arm/arduino_101_ble
* ``ZEP-1365`` - Missing wiki board support page for arm/c3200_launchxl
* ``ZEP-1370`` - There's a wiki page for arduino_due but no zephyr/boards support folder
* ``ZEP-1374`` - Add ksdk spi shim driver
* ``ZEP-1387`` - Add a driver for Atmel ataes132a  HW Crypto module
* ``ZEP-1389`` - Add support for KW41 SoC
* ``ZEP-1390`` - Add support for FRDM-KW41Z
* ``ZEP-1393`` - Add ksdk pinmux driver
* ``ZEP-1394`` - Add ksdk gpio driver
* ``ZEP-1395`` - Add data ready trigger to FXOS8700 driver
* ``ZEP-1401`` - Enhance ready queue cache and interrupt exit code to reduce interrupt latency.
* ``ZEP-1403`` - remove CONFIG_OMIT_FRAME_POINTER from ARC boards
* ``ZEP-1405`` - function l2cap_br_conn_req in /subsys/bluetooth/host/l2cap_br.c references uninitialized pointer
* ``ZEP-1406`` - Update sensor driver paths in wiki
* ``ZEP-1408`` - quark_se_c1000_ss enter_arc_state() might need cc and memory clobber
* ``ZEP-1411`` - Deprecate device_sync_call API and use semaphore directly
* ``ZEP-1413`` - [ARC] test/legacy/kernel/test_tickless/microkernel fails to build
* ``ZEP-1415`` - drivers/timer/* code comments still refer to micro/nano kernel
* ``ZEP-1418`` - Add support for Nordic nRF52840 and its DK
* ``ZEP-1419`` - SYS_LOG macros cause potentially bad behavior due to printk/printf selection
* ``ZEP-1420`` - Make the time spent with interrupts disabled deterministic
* ``ZEP-1421`` - BMI160 gyroscope driver stops reporting after 1-5 minutes
* ``ZEP-1422`` - Arduino_101 doesn't response ipv6 ping request after enable echo_client ipv6
* ``ZEP-1427`` - wpanusb dongle / 15.4 communication instability
* ``ZEP-1429`` - NXP MCR20A Driver
* ``ZEP-1432`` - ksdk pinmux driver should expose the public pinmux API
* ``ZEP-1434`` - menuconfig screen shots show nanokernel options
* ``ZEP-1437`` - AIO: Fail to retrieve pending interrupt in ISR
* ``ZEP-1440`` - Kconfig choice for MINIMAL_LIBC vs NEWLIB_LIBC is not selectable
* ``ZEP-1442`` - Samples/net/dhcpv4_client: Build fail as No rule to make target prj\_.conf
* ``ZEP-1443`` - Samples/net/zperf: Build fail as net_private.h can not be found
* ``ZEP-1448`` - Samples/net/mbedtls_sslclient:Build fail as net/ip_buf.h can not be found
* ``ZEP-1449`` - samples: logger_hook
* ``ZEP-1456`` - Asserts on nrf51 running Bluetooth hci_uart sample
* ``ZEP-1457`` - Add SPDX Tags to Zephyr license boilerplate
* ``ZEP-1460`` - Sanity check reports some qemu step failures as 'build_error'
* ``ZEP-1461`` - Add zephyr support to openocd upstream
* ``ZEP-1467`` - Cleanup misc/ and move features to subsystems in subsys/
* ``ZEP-1473`` - ARP cache confused by use of gateway.
* ``ZEP-1474`` - BLE Connection Parameter Request/Response Processing
* ``ZEP-1475`` - k_free documentation should specify that NULL is valid
* ``ZEP-1476`` - echo_client display port unreachable
* ``ZEP-1480`` - Update supported distros in getting started guide
* ``ZEP-1481`` - Bluetooth fails to init
* ``ZEP-1483`` - H:4 HCI driver (h4.c) should rely on UART flow control to avoid dropping packets
* ``ZEP-1487`` - I2C_SS: I2C doesn't set device busy before starting data transfer
* ``ZEP-1488`` - SPI_SS: SPI doesn't set device busy before starting data transfer
* ``ZEP-1489`` - [GATT] Nested Long Characteristic Value Reliable Writes
* ``ZEP-1490`` - [PTS] TC_CONN_CPUP_BV_04_C test case is failing
* ``ZEP-1492`` - Add Atmel SAM family GMAC Ethernet driver
* ``ZEP-1493`` - Zephyr project documentation copyright
* ``ZEP-1495`` - Networking API details documentation is missing
* ``ZEP-1496`` - gpio_pin_enable_callback error
* ``ZEP-1497`` - Cortex-M0 port exception and interrupt priority setting and getting is broken
* ``ZEP-1507`` - fxos8700 broken gpio_callback implementation
* ``ZEP-1512`` - doc-theme has its own conf.py
* ``ZEP-1514`` - samples/bluetooth/ipsp build fail: net/ip_buf.h No such file or directory
* ``ZEP-1525`` - driver: gpio: GPIO driver still uses  nano_timer
* ``ZEP-1532`` - Wrong accelerometer readings
* ``ZEP-1536`` - Convert documentation of PWM samples to RST
* ``ZEP-1537`` - Convert documentation of power management samples to RST
* ``ZEP-1538`` - Convert documentation of zoap samples to RST
* ``ZEP-1539`` - Create documentation in RST for all networking samples
* ``ZEP-1540`` - Convert Bluetooth samples to RST
* ``ZEP-1542`` - Multi Sessions HTTP Server sample
* ``ZEP-1543`` - HTTP Server sample with basic authentication
* ``ZEP-1544`` - Arduino_101 doesn't respond to ipv6 ping request after enable echo_server ipv6
* ``ZEP-1545`` - AON Counter : ISR triggered twice on ARC
* ``ZEP-1546`` - Bug in Zephyr OS high-precision timings sub-system (function sys_cycle_get_32())
* ``ZEP-1547`` - Add support for H7 crypto function and CT2 SMP auth flag
* ``ZEP-1548`` - Python script invocation is inconsistent
* ``ZEP-1549`` - k_cpu_sleep_mode unaligned byte address
* ``ZEP-1554`` - Xtensa integration
* ``ZEP-1557`` - RISC V Port
* ``ZEP-1558`` - Support of user private data pointer in Timer expiry function
* ``ZEP-1559`` - Implement _tsc_read  for ARC architecture
* ``ZEP-1562`` - echo_server/echo_client examples hang randomly after some time of operation
* ``ZEP-1563`` - move board documentation for NRF51/NRF52 back to git tree
* ``ZEP-1564`` - 6lo uncompress_IPHC_header overwrites IPHC fields
* ``ZEP-1566`` - WDT: Interrupt is triggered multiple times
* ``ZEP-1569`` - net/tcp: TCP in server mode doesn't support multiple concurrent connections
* ``ZEP-1570`` - net/tcp: TCP in server mode is unable to close client connections
* ``ZEP-1571`` - Update "Changes from Version 1 Kernel" to include a "How-To Port Apps" section
* ``ZEP-1572`` - Update QMSI to 1.4
* ``ZEP-1573`` - net/tcp: User provided data in net_context_recv is not passed to callback
* ``ZEP-1574`` - Samples/net/dhcpv4_client: Build fail as undefined reference to net_mgmt_add_event_callback
* ``ZEP-1579`` - external links to zephyr technical docs are broken
* ``ZEP-1581`` - [nRF52832] Blinky hangs after some minutes
* ``ZEP-1583`` - ARC: warning: unmet direct dependencies (SOC_RISCV32_PULPINO || SOC_RISCV32_QEMU)
* ``ZEP-1585`` - legacy.h should be disabled in kernel.h with CONFIG_LEGACY_KERNEL=n
* ``ZEP-1587`` - sensor.h still uses legacy APIs and structs
* ``ZEP-1588`` - I2C doesn't work on Arduino 101
* ``ZEP-1589`` - Define yaml descriptions for UART devices
* ``ZEP-1590`` - echo_server run on FRDM-K64F displays BUS FAULT
* ``ZEP-1591`` - wiki: add Networking section and point https://wiki.zephyrproject.org/view/Network_Interfaces
* ``ZEP-1592`` - echo-server does not build with newlib
* ``ZEP-1593`` - /scripts/sysgen should create output using SPDX licensing tag
* ``ZEP-1598`` - samples/philosophers build failed unexpectedly @quark_d2000  section noinit will not fit in region RAM
* ``ZEP-1601`` - Console over Telnet
* ``ZEP-1602`` - IPv6 ping fails using sample application echo_server on FRDM-K64F
* ``ZEP-1611`` - Hardfault after a few echo requests (IPv6 over BLE)
* ``ZEP-1614`` - Use correct i2c device driver name
* ``ZEP-1616`` - Mix up between "network address" and "socket address" concepts in declaration of net_addr_pton()
* ``ZEP-1617`` - mbedTLS server/client failing to run on qemu
* ``ZEP-1619`` - Default value of NET_NBUF_RX_COUNT is too low, causes lock up on startup
* ``ZEP-1623`` - (Parts) of Networking docs still refer to 1.5 world model (with fibers and tasks) and otherwise not up to date
* ``ZEP-1626`` - SPI: spi cannot work in CPHA mode @ ARC
* ``ZEP-1632`` - TCP ACK packet should not be forwarded to application recv cb.
* ``ZEP-1635`` - MCR20A driver unstable
* ``ZEP-1638`` - No (public) analog of inet_ntop()
* ``ZEP-1644`` - Incoming connection handling for UDP is not exactly correct
* ``ZEP-1645`` - API to wait on multiple kernel objects
* ``ZEP-1648`` - Update links to wiki pages for board info back into the web docs
* ``ZEP-1650`` - make clean (or pristine) is not removing all artifacts of document generation
* ``ZEP-1651`` - i2c_dw malfunctioning due to various changes.
* ``ZEP-1653`` - build issue when compiling with LLVM in ISSM (altmacro)
* ``ZEP-1654`` - Build issues when compiling with LLVM(unknown attribute '_alloc_align_)
* ``ZEP-1655`` - Build issues when compiling with LLVM(memory pool)
* ``ZEP-1656`` - IPv6 over BLE no longer works after commit 2e9fd88
* ``ZEP-1657`` - Zoap doxygen documentation needs to be perfected
* ``ZEP-1658`` - IPv6 TCP low on buffers, stops responding after about 5 requests
* ``ZEP-1662`` - zoap_packet_get_payload() should return the payload length
* ``ZEP-1663`` - sanitycheck overrides user's environment for CCACHE
* ``ZEP-1665`` - pinmux: missing default pinmux driver config for quark_se_ss
* ``ZEP-1669`` - API documentation does not follow in-code documentation style
* ``ZEP-1672`` - flash: Flash device binding failed on Arduino_101_sss
* ``ZEP-1674`` - frdm_k64f: With Ethernet driver enabled, application can't start up without connected network cable
* ``ZEP-1677`` - SDK: BLE cannot be initialized/advertised with CONFIG_ARC_INIT=y on Arduino 101
* ``ZEP-1681`` - Save/restore debug registers during soc_sleep/soc_deep_sleep in c1000
* ``ZEP-1692`` - [PTS] GATT/SR/GPA/BV-11-C fails
* ``ZEP-1701`` - Provide an HTTP API
* ``ZEP-1704`` - BMI160 samples fails to run
* ``ZEP-1706`` - Barebone Panther board support
* ``ZEP-1707`` - [PTS] 7 SM/MAS cases fail
* ``ZEP-1708`` - [PTS] SM/MAS/PKE/BI-01-C fails
* ``ZEP-1709`` - [PTS] SM/MAS/PKE/BI-02-C fails
* ``ZEP-1710`` - Add TinyTILE board support
* ``ZEP-1713`` - xtensa: correct all checkpatch issues
* ``ZEP-1716`` - HTTP server sample that does not support up to 10 concurrent sessions.
* ``ZEP-1717`` - GPIO: GPIO LEVEL interrupt cannot work well in deep sleep mode
* ``ZEP-1723`` - Warnings in Network code/ MACROS, when built with ISSM's  llvm/icx compiler
* ``ZEP-1732`` - sample of zoap_server runs error.
* ``ZEP-1733`` - Work on ZEP-686 led to regressions in docs on integration with 3rd-party code
* ``ZEP-1745`` - Bluetooth samples build failure
* ``ZEP-1753`` - sample of dhcpv4_client runs error on Arduino 101
* ``ZEP-1754`` - sample of coaps_server was tested failed on qemu
* ``ZEP-1756`` - net apps: [-Wpointer-sign] build warning raised when built with ISSM's  llvm/icx compiler
* ``ZEP-1758`` - PLL2 is not correctly enabled in STM32F10x connectivity line SoC
* ``ZEP-1763`` - Nordic RTC timer driver not correct with tickless idle
* ``ZEP-1764`` - samples: sample cases use hard code device name, such as "GPIOB" "I2C_0"
* ``ZEP-1768`` - samples: cases miss testcase.ini
* ``ZEP-1774`` - Malformed packet included with IPv6 over 802.15.4
* ``ZEP-1778`` - tests/power: multicore case won't work as expected
* ``ZEP-1786`` - TCP does not work on Arduino 101 board.
* ``ZEP-1787`` - kernel event logger build failed with "CONFIG_LEGACY_KERNEL=n"
* ``ZEP-1789`` - ARC: "samples/logger-hook" crashed __memory_error from sys_ring_buf_get
* ``ZEP-1799`` - timeout_order_test _ASSERT_VALID_PRIO failed
* ``ZEP-1803`` - Error occurs when exercising dma_transfer_stop
* ``ZEP-1806`` - Build warnings with LLVM/icx (gdb_server)
* ``ZEP-1809`` - Build error in net/ip with LLVM/icx
* ``ZEP-1810`` - Build failure in net/lib/zoap with LLVM/icx
* ``ZEP-1811`` - Build error in net/ip/net_mgmt.c with LLVM/icx
* ``ZEP-1839`` - LL_ASSERT in event_common_prepareA
* ``ZEP-1851`` - Build warnings with obj_tracing
* ``ZEP-1852`` - LL_ASSERT in isr_radio_state_close()
* ``ZEP-1855`` - IP stack buffer allocation fails over time
* ``ZEP-1858`` - Zephyr NATS client fails to respond to  server MSG
* ``ZEP-1864`` - llvm icx build warning in tests/drivers/uart/*
* ``ZEP-1872`` - samples/net: the HTTP client sample app must run on QEMU x86
* ``ZEP-1877`` - samples/net: the coaps_server sample app runs failed on Arduino 101
* ``ZEP-1883`` - Enabling Console on ARC Genuino 101
* ``ZEP-1890`` - Bluetooth IPSP sample: Too small user data size
