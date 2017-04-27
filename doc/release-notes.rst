Release Notes
#############

.. _zephyr_1.7.1:

Zephyr Kernel 1.7.1
********************

* [ZEP-1800] Updated TinyCrypt to latest version: 0.2.6
* [ZEP-749] Updated mbedTLS to 2.4.2
* [ZEP-1470] ARM: Increase idle stack size to fix corruption by FP_SHARING

.. _zephyr_1.7:

Zephyr Kernel 1.7.0
********************

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
======

* Introduction of k_poll API: k_poll() is similar to the POSIX poll() API in
  spirit in that it allows a single thread to monitor multiple events without
  actively polling them, but rather pending for one or more to become ready.
* Optimized memory use of some thread fields
* Remove usage of micro/nano kernel terminology from kernel code and introduced
  a legacy option to enable/disable legacy APIs. (using legacy.h)


Architectures
=============

* ARM: Added support for device tree
* ARM: Fixed exception priority access on Cortex M0(+)
* ARM: Refactored to use CMSIS

Boards
======

* Added ARM MPS2_AN385 board
* Added Atmel SAM E70 Xplained board
* Added Nordic pca10056 PDK board
* Added NXP FRDM-KW41Z board
* Added ST Nucleo-F334R8, Nucleo-L476G, STM3210C-EVAL, and STM32373C-EVAL boards
* Added Panther and tinyTILE boards, based on Quark SE C1000 and Intel Curie
* Added support for Zedboard Pulpino, a RISC V based board
* Added Qemu target for RISC V and a simulator target for the Xtensa architecture.

Drivers and Sensors
===================

* Added Atmel SAM pmc, gpio, uart, and ethernet drivers
* Added STM32F3x clock, flash, gpio, pinmux drivers
* Added stm32cube pwm and clock drivers
* Added cc3200 gpio driver
* Added mcr20a ieee802154 driver
* Added mcux pinmux, gpio, uart, and spi drivers
* Added Beetle clock control and watchdog drivers

Networking
==========

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
=========

* Redesigned buffer pools for smaller memory consumption
* Redesigned thread model for smaller memory consumption
* Utilized new k_poll API to consolidate all TX threads into a single one
* Added more SDP functionality
* Improved RFCOMM support
* Reduced latencies in the Controller
* Added SPI HCI driver

Libraries
=========

* Updated mbedTLS library
* Updated TinyCrypt to version 0.2.5

HALs
====

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
=============

* General improvements and additions to kernel component docs
* Moved supported board information back to the website site.
* New website documentation theme to go with the new zephyrproject.org site.
* New local-content generation theme (read-the-docs)
* General spelling checks and organizational improvements.
* Site-wide glossary added.
* Porting guides added.
* Sample README files converted to documents included in the website.
* Improved consistency of :ref:`boards` and `samples documentation`_.

.. _samples documentation: https:/zephyrproject.org/doc/samples/samples.html


JIRA Related Items
==================


.. comment  List derived from https://jira.zephyrproject.org/issues/?filter=10345

* :jira:`ZEP-19` - IPSP node support
* :jira:`ZEP-145` - no 'make flash' for Arduino Due
* :jira:`ZEP-328` - HW Encryption Abstraction
* :jira:`ZEP-359` - Move QEMU handling to a central location
* :jira:`ZEP-365` - Zephyr's MQTT library
* :jira:`ZEP-437` - TCP/IP API
* :jira:`ZEP-513` - extern declarations of  small microkernel objects in designated sections require __attribute__((section)) in gp-enabled systems
* :jira:`ZEP-591` - MQTT Port to New IP Stack
* :jira:`ZEP-604` - In coap_server sample app, CoAP resource separate is not able to send separate response
* :jira:`ZEP-613` - TCP/UDP client and server mode functionality
* :jira:`ZEP-641` - Bluetooth Eddystone sample does not correctly implement Eddystone beacon
* :jira:`ZEP-648` - New CoAP Implementation
* :jira:`ZEP-664` - Extend spi_qmsi_ss driver to support save/restore peripheral context
* :jira:`ZEP-665` - Extend gpio_qmsi_ss driver to support save/restore peripheral context
* :jira:`ZEP-666` - Extend i2c_qmsi_ss driver to support save/restore peripheral context
* :jira:`ZEP-667` - Extend adc_qmsi_ss driver to support save/restore peripheral context
* :jira:`ZEP-686` - docs: Info in Application Development Primer and Developing an Application and the Build System is largely duplicated
* :jira:`ZEP-706` - cannot set debug breakpoints on ARC side of Arduino 101
* :jira:`ZEP-719` - Add ksdk uart shim driver
* :jira:`ZEP-734` - Port AES-CMAC-PRF-128 [RFC 4615] encryption library for Thread support
* :jira:`ZEP-742` - nRF5x Series: System Clock driver using NRF_RTC
* :jira:`ZEP-744` - USB WebUSB
* :jira:`ZEP-748` - Enable mbedtls_sslclient sample to run on quark se board
* :jira:`ZEP-759` - Add preliminary support for Atmel SAM E70 (Cortex-M7) chipset family and SAM E70 Xplained board
* :jira:`ZEP-788` - UDP
* :jira:`ZEP-789` - IPv4
* :jira:`ZEP-790` - ICMPv4
* :jira:`ZEP-791` - TCP
* :jira:`ZEP-792` - ARP
* :jira:`ZEP-793` - DNS Resolver
* :jira:`ZEP-794` - Requirements for Internet Hosts - Communication Layers
* :jira:`ZEP-796` - DHCPv4
* :jira:`ZEP-798` - IPv6
* :jira:`ZEP-799` - HTTP over TLS
* :jira:`ZEP-801` - DNS Extensions to support IPv6
* :jira:`ZEP-804` - IPv6 Addressing Architecture
* :jira:`ZEP-805` - Internet Control Message Protocol (ICMP) v6
* :jira:`ZEP-807` - Neighbor Discovery for IPv6
* :jira:`ZEP-808` - IPv6 Stateless Autoconfiguration (SLAAC)
* :jira:`ZEP-809` - IPv6 over 802.15.4
* :jira:`ZEP-811` - The Trickle Algorithm
* :jira:`ZEP-812` - Compression Format for IPv6 over 802.15.4
* :jira:`ZEP-813` - RPL:  IPv6 Routing Protocol
* :jira:`ZEP-814` - Routing Metrics used in Path Selection
* :jira:`ZEP-815` - Objective Function Zero for RPL
* :jira:`ZEP-816` - Minimum Rank with Hysteresis (RPL)
* :jira:`ZEP-818` - CoAP working over the new IP stack
* :jira:`ZEP-820` - HTTP v1.1 Server Sample
* :jira:`ZEP-823` - New IP Stack - Documentation
* :jira:`ZEP-824` - Network Device Driver Porting Guide
* :jira:`ZEP-825` - Porting guide for old-to-new IP Stack APIs
* :jira:`ZEP-827` - HTTP Client sample application
* :jira:`ZEP-830` - ICMPv6 Parameter Problem Support
* :jira:`ZEP-832` - Hop-by-Hop option handling
* :jira:`ZEP-847` - Network protocols must be moved to subsys/net/lib
* :jira:`ZEP-854` - CoAP with DTLS sample
* :jira:`ZEP-859` - Migrate ENC28J60 driver to YAIP IP stack
* :jira:`ZEP-865` - convert filesystem sample to a runnable test
* :jira:`ZEP-872` - Unable to flash Zephyr on Arduino 101 using Ubuntu and following wiki instructions
* :jira:`ZEP-873` - DMA API Update
* :jira:`ZEP-875` - 6LoWPAN - Context based compression support
* :jira:`ZEP-876` - 6LoWPAN - Offset based Reassembly of 802.15.4 packets
* :jira:`ZEP-879` - 6LoWPAN - Stateless Address Autoconfiguration
* :jira:`ZEP-882` - 6LoWPAN - IPv6 Next Header Compression
* :jira:`ZEP-883` - IP Stack L2 Interface Management API
* :jira:`ZEP-884` - 802.15.4 - CSMA-CA Radio protocol support
* :jira:`ZEP-885` - 802.15.4 - Beacon frame support
* :jira:`ZEP-886` - 802.15.4 - MAC command frame support
* :jira:`ZEP-887` - 802.15.4 - Management service: RFD level support
* :jira:`ZEP-911` - Refine thread priorities & locking
* :jira:`ZEP-919` - Purge obsolete microkernel & nanokernel code
* :jira:`ZEP-929` - Verify the preempt-thread-only and coop-thread-only configurations
* :jira:`ZEP-931` - Finalize kernel file naming & locations
* :jira:`ZEP-936` - Adapt drivers to unified kernel
* :jira:`ZEP-937` - Adapt networking to unified kernel
* :jira:`ZEP-946` - Galileo Gen1 board support dropped?
* :jira:`ZEP-951` - CONFIG_GDB_INFO build not working on ARM
* :jira:`ZEP-953` - CONFIG_HPET_TIMER_DEBUG build warning
* :jira:`ZEP-958` - simplify pinmux interface and merge the pinmux_dev into one single API
* :jira:`ZEP-964` - Add a (hidden?) Kconfig option for disabling legacy API
* :jira:`ZEP-975` - DNS client port to new IP stack
* :jira:`ZEP-1012` - NATS client port to new IP stack
* :jira:`ZEP-1038` - Hard real-time interrupt support
* :jira:`ZEP-1060` - Contributor guide for documentation missing
* :jira:`ZEP-1103` - Propose and implement synchronization flow for multicore power management
* :jira:`ZEP-1165` - support enums as IRQ line argument in IRQ_CONNECT()
* :jira:`ZEP-1172` - Update logger Api to allow using a hook for SYS_LOG_BACKEND_FN function
* :jira:`ZEP-1177` - Reduce Zephyr's Dependency on Host Tools
* :jira:`ZEP-1179` - Build issues when compiling with LLVM from ISSM (icx)
* :jira:`ZEP-1189` - SoC I2C peripheral of the Quark SE cannot be used from the ARC core
* :jira:`ZEP-1190` - SoC SPI peripheral of the Quark SE cannot be used from the ARC core
* :jira:`ZEP-1222` - Add save/restore support to ARC core
* :jira:`ZEP-1223` - Add save/restore support to arcv2_irq_unit
* :jira:`ZEP-1224` - Add save/restore support to arcv2_timer_0/sys_clock
* :jira:`ZEP-1230` - Optimize interrupt return code on ARC.
* :jira:`ZEP-1233` - mbedDTLS DTLS client stability does not work on top of the tree for the net branch
* :jira:`ZEP-1251` - Abstract driver re-entrancy code
* :jira:`ZEP-1267` - Echo server crashes upon reception of router advertisement
* :jira:`ZEP-1276` - Move disk_access_* out of file system subsystem
* :jira:`ZEP-1283` - compile option to skip gpio toggle in samples/power/power_mgr
* :jira:`ZEP-1284` - Remove arch/arm/core/gdb_stub.S and all the abstractions it introduced
* :jira:`ZEP-1288` - Define _arc_v2_irq_unit device
* :jira:`ZEP-1292` - Update external mbed TLS library to latest version (2.4.0)
* :jira:`ZEP-1300` - ARM LTD V2M Beetle Support [Phase 2]
* :jira:`ZEP-1304` - Define device tree bindings for NXP Kinetis K64F
* :jira:`ZEP-1305` - Add DTS/DTB targets to build infrastructure
* :jira:`ZEP-1306` - Create DTS/DTB parser
* :jira:`ZEP-1307` - Plumbing the DTS configuration
* :jira:`ZEP-1308` - zephyr thread function k_sleep doesn't work with nrf51822
* :jira:`ZEP-1320` - Update Architecture Porting Guide
* :jira:`ZEP-1321` - Glossary of Terms needs updating
* :jira:`ZEP-1323` - Eliminate references to fiber, task, and nanokernel under ./include
* :jira:`ZEP-1324` - Get rid of references to CONFIG_NANOKERNEL
* :jira:`ZEP-1325` - Eliminate TICKLESS_IDLE_SUPPORTED option
* :jira:`ZEP-1327` - Eliminate obsolete kernel directories
* :jira:`ZEP-1329` - Rename kernel APIs that have nano\_ prefixes
* :jira:`ZEP-1334` - Add make debug support for QEMU-based boards
* :jira:`ZEP-1337` - Relocate event logger files
* :jira:`ZEP-1338` - Update external fs with new FATFS revision 0.12b
* :jira:`ZEP-1342` - legacy/kernel/test_early_sleep/ fails on EMSK
* :jira:`ZEP-1347` - sys_bitfield_*() take unsigned long* vs memaddr_t
* :jira:`ZEP-1351` - FDRM k64f SPI does not work
* :jira:`ZEP-1355` - Connection Failed to be Established
* :jira:`ZEP-1357` - iot/dns: Client is broken
* :jira:`ZEP-1358` - BMI160 accelerometer gives 0 on all axes
* :jira:`ZEP-1361` - IP stack is broken
* :jira:`ZEP-1363` - Missing wiki board support page for arm/arduino_101_ble
* :jira:`ZEP-1365` - Missing wiki board support page for arm/c3200_launchxl
* :jira:`ZEP-1370` - There's a wiki page for arduino_due but no zephyr/boards support folder
* :jira:`ZEP-1374` - Add ksdk spi shim driver
* :jira:`ZEP-1387` - Add a driver for Atmel ataes132a  HW Crypto module
* :jira:`ZEP-1389` - Add support for KW41 SoC
* :jira:`ZEP-1390` - Add support for FRDM-KW41Z
* :jira:`ZEP-1393` - Add ksdk pinmux driver
* :jira:`ZEP-1394` - Add ksdk gpio driver
* :jira:`ZEP-1395` - Add data ready trigger to FXOS8700 driver
* :jira:`ZEP-1401` - Enhance ready queue cache and interrupt exit code to reduce interrupt latency.
* :jira:`ZEP-1403` - remove CONFIG_OMIT_FRAME_POINTER from ARC boards
* :jira:`ZEP-1405` - function l2cap_br_conn_req in /subsys/bluetooth/host/l2cap_br.c references uninitialized pointer
* :jira:`ZEP-1406` - Update sensor driver paths in wiki
* :jira:`ZEP-1408` - quark_se_c1000_ss enter_arc_state() might need cc and memory clobber
* :jira:`ZEP-1411` - Deprecate device_sync_call API and use semaphore directly
* :jira:`ZEP-1413` - [ARC] test/legacy/kernel/test_tickless/microkernel fails to build
* :jira:`ZEP-1415` - drivers/timer/* code comments still refer to micro/nano kernel
* :jira:`ZEP-1418` - Add support for Nordic nRF52840 and its DK
* :jira:`ZEP-1419` - SYS_LOG macros cause potentially bad behavior due to printk/printf selection
* :jira:`ZEP-1420` - Make the time spent with interrupts disabled deterministic
* :jira:`ZEP-1421` - BMI160 gyroscope driver stops reporting after 1-5 minutes
* :jira:`ZEP-1422` - Arduino_101 doesn't response ipv6 ping request affer enable echo_client ipv6
* :jira:`ZEP-1427` - wpanusb dongle / 15.4 communication instability
* :jira:`ZEP-1429` - NXP MCR20A Driver
* :jira:`ZEP-1432` - ksdk pinmux driver should expose the public pinmux API
* :jira:`ZEP-1434` - menuconfig screen shots show nanokernel options
* :jira:`ZEP-1437` - AIO: Fail to retrieve pending interrupt in ISR
* :jira:`ZEP-1440` - Kconfig choice for MINIMAL_LIBC vs NEWLIB_LIBC is not selectable
* :jira:`ZEP-1442` - Samples/net/dhcpv4_client: Build fail as No rule to make target prj\_.conf
* :jira:`ZEP-1443` - Samples/net/zperf: Build fail as net_private.h can not be found
* :jira:`ZEP-1448` - Samples/net/mbedtls_sslclient:Build fail as net/ip_buf.h can not be found
* :jira:`ZEP-1449` - samples: logger_hook
* :jira:`ZEP-1456` - Asserts on nrf51 running Bluetooth hci_uart sample
* :jira:`ZEP-1457` - Add SPDX Tags to Zephyr licence boilerplate
* :jira:`ZEP-1460` - Sanity check reports some qemu step failures as 'build_error'
* :jira:`ZEP-1461` - Add zephyr support to openocd upstream
* :jira:`ZEP-1467` - Cleanup misc/ and move features to subsystems in subsys/
* :jira:`ZEP-1473` - ARP cache confused by use of gateway.
* :jira:`ZEP-1474` - BLE Connection Parameter Request/Response Processing
* :jira:`ZEP-1475` - k_free documentation should specify that NULL is valid
* :jira:`ZEP-1476` - echo_client display port unreachable
* :jira:`ZEP-1480` - Update supported distros in getting started guide
* :jira:`ZEP-1481` - Bluetooth fails to init
* :jira:`ZEP-1483` - H:4 HCI driver (h4.c) should rely on UART flow control to avoid dropping packets
* :jira:`ZEP-1487` - I2C_SS: I2C doesn't set device busy before starting data transfer
* :jira:`ZEP-1488` - SPI_SS: SPI doesn't set device busy before starting data transfer
* :jira:`ZEP-1489` - [GATT] Nested Long Characteristic Value Reliable Writes
* :jira:`ZEP-1490` - [PTS] TC_CONN_CPUP_BV_04_C test case is failing
* :jira:`ZEP-1492` - Add Atmel SAM family GMAC Ethernet driver
* :jira:`ZEP-1493` - Zephyr project documentation copyright
* :jira:`ZEP-1495` - Networking API details documentation is missing
* :jira:`ZEP-1496` - gpio_pin_enable_callback error
* :jira:`ZEP-1497` - Cortex-M0 port exception and interrupt priority setting and getting is broken
* :jira:`ZEP-1507` - fxos8700 broken gpio_callback implementation
* :jira:`ZEP-1512` - doc-theme has its own conf.py
* :jira:`ZEP-1514` - samples/bluetooth/ipsp build fail: net/ip_buf.h No such file or directory
* :jira:`ZEP-1525` - driver: gpio: GPIO driver still uses  nano_timer
* :jira:`ZEP-1532` - Wrong accelerometer readings
* :jira:`ZEP-1536` - Convert documentation of PWM samples to RST
* :jira:`ZEP-1537` - Convert documentation of power management samples to RST
* :jira:`ZEP-1538` - Convert documentation of zoap samples to RST
* :jira:`ZEP-1539` - Create documentation in RST for all networking samples
* :jira:`ZEP-1540` - Convert Bluetooth samples to RST
* :jira:`ZEP-1542` - Multi Sessions HTTP Server sample
* :jira:`ZEP-1543` - HTTP Server sample with basic authentication
* :jira:`ZEP-1544` - Arduino_101 doesn't respond to ipv6 ping request after enable echo_server ipv6
* :jira:`ZEP-1545` - AON Counter : ISR triggered twice on ARC
* :jira:`ZEP-1546` - Bug in Zephyr OS high-precision timings sub-system (function sys_cycle_get_32())
* :jira:`ZEP-1547` - Add support for H7 crypto function and CT2 SMP auth flag
* :jira:`ZEP-1548` - Python script invocation is inconsistent
* :jira:`ZEP-1549` - k_cpu_sleep_mode unaligned byte address
* :jira:`ZEP-1554` - Xtensa integration
* :jira:`ZEP-1557` - RISC V Port
* :jira:`ZEP-1558` - Support of user private data pointer in Timer expiry function
* :jira:`ZEP-1559` - Implement _tsc_read  for ARC architecture
* :jira:`ZEP-1562` - echo_server/echo_client examples hang randomly after some time of operation
* :jira:`ZEP-1563` - move board documentation for NRF51/NRF52 back to git tree
* :jira:`ZEP-1564` - 6lo uncompress_IPHC_header overwrites IPHC fields
* :jira:`ZEP-1566` - WDT: Interrupt is triggered multiple times
* :jira:`ZEP-1569` - net/tcp: TCP in server mode doesn't support multiple concurrent connections
* :jira:`ZEP-1570` - net/tcp: TCP in server mode is unable to close client connections
* :jira:`ZEP-1571` - Update "Changes from Version 1 Kernel" to include a "How-To Port Apps" section
* :jira:`ZEP-1572` - Update QMSI to 1.4
* :jira:`ZEP-1573` - net/tcp: User provided data in net_context_recv is not passed to callback
* :jira:`ZEP-1574` - Samples/net/dhcpv4_client: Build fail as undefined reference to net_mgmt_add_event_callback
* :jira:`ZEP-1579` - external links to zephyr technical docs are broken
* :jira:`ZEP-1581` - [nRF52832] Blinky hangs after some minutes
* :jira:`ZEP-1583` - ARC: warning: unmet direct dependencies (SOC_RISCV32_PULPINO || SOC_RISCV32_QEMU)
* :jira:`ZEP-1585` - legacy.h should be disabled in kernel.h with CONFIG_LEGACY_KERNEL=n
* :jira:`ZEP-1587` - sensor.h still uses legacy APIs and structs
* :jira:`ZEP-1588` - I2C doesn't work on Arduino 101
* :jira:`ZEP-1589` - Define yaml descriptions for UART devices
* :jira:`ZEP-1590` - echo_server run on FRDM-K64F displays BUS FAULT
* :jira:`ZEP-1591` - wiki: add Networking section and point https://wiki.zephyrproject.org/view/Network_Interfaces
* :jira:`ZEP-1592` - echo-server does not build with newlib
* :jira:`ZEP-1593` - /scripts/sysgen should create output using SPDX licensing tag
* :jira:`ZEP-1598` - samples/philosophers build failed unexpectedly @quark_d2000  section noinit will not fit in region RAM
* :jira:`ZEP-1601` - Console over Telnet
* :jira:`ZEP-1602` - IPv6 ping fails using sample application echo_server on FRDM-K64F
* :jira:`ZEP-1611` - Hardfault after a few echo requests (IPv6 over BLE)
* :jira:`ZEP-1614` - Use correct i2c device driver name
* :jira:`ZEP-1616` - Mix up between "network address" and "socket address" concepts in declaration of net_addr_pton()
* :jira:`ZEP-1617` - mbedTLS server/client failing to run on qemu
* :jira:`ZEP-1619` - Default value of NET_NBUF_RX_COUNT is too low, causes lock up on startup
* :jira:`ZEP-1623` - (Parts) of Networking docs still refer to 1.5 world model (with fibers and tasks) and otherwise not up to date
* :jira:`ZEP-1626` - SPI: spi cannot work in CPHA mode @ ARC
* :jira:`ZEP-1632` - TCP ACK packet should not be forwarded to application recv cb.
* :jira:`ZEP-1635` - MCR20A driver unstable
* :jira:`ZEP-1638` - No (public) analog of inet_ntop()
* :jira:`ZEP-1644` - Incoming connection handling for UDP is not exactly correct
* :jira:`ZEP-1645` - API to wait on multiple kernel objects
* :jira:`ZEP-1648` - Update links to wiki pages for board info back into the web docs
* :jira:`ZEP-1650` - make clean (or pristine) is not removing all artifacts of document generation
* :jira:`ZEP-1651` - i2c_dw malfunctioning due to various changes.
* :jira:`ZEP-1653` - build issue when compiling with LLVM in ISSM (altmacro)
* :jira:`ZEP-1654` - Build issues when compiling with LLVM(unknown attribute '_alloc_align_)
* :jira:`ZEP-1655` - Build issues when compiling with LLVM(memory pool)
* :jira:`ZEP-1656` - IPv6 over BLE no longer works after commit 2e9fd88
* :jira:`ZEP-1657` - Zoap doxygen documentation needs to be perfected
* :jira:`ZEP-1658` - IPv6 TCP low on buffers, stops responding after about 5 requests
* :jira:`ZEP-1662` - zoap_packet_get_payload() should return the payload length
* :jira:`ZEP-1663` - sanitycheck overrides user's environment for CCACHE
* :jira:`ZEP-1665` - pinmux: missing default pinmux driver config for quark_se_ss
* :jira:`ZEP-1669` - API documentation does not follow in-code documentation style
* :jira:`ZEP-1672` - flash: Flash device binding failed on Arduino_101_sss
* :jira:`ZEP-1674` - frdm_k64f: With Ethernet driver enabled, application can't start up without connected network cable
* :jira:`ZEP-1677` - SDK: BLE cannot be initialized/advertised with CONFIG_ARC_INIT=y on Arduino 101
* :jira:`ZEP-1681` - Save/restore debug registers during soc_sleep/soc_deep_sleep in c1000
* :jira:`ZEP-1692` - [PTS] GATT/SR/GPA/BV-11-C fails
* :jira:`ZEP-1701` - Provide an HTTP API
* :jira:`ZEP-1704` - BMI160 samples fails to run
* :jira:`ZEP-1706` - Barebone Panther board support
* :jira:`ZEP-1707` - [PTS] 7 SM/MAS cases fail
* :jira:`ZEP-1708` - [PTS] SM/MAS/PKE/BI-01-C fails
* :jira:`ZEP-1709` - [PTS] SM/MAS/PKE/BI-02-C fails
* :jira:`ZEP-1710` - Add TinyTILE board support
* :jira:`ZEP-1713` - xtensa: correct all checkpatch issues
* :jira:`ZEP-1716` - HTTP server sample that does not support up to 10 concurrent sessions.
* :jira:`ZEP-1717` - GPIO: GPIO LEVEL interrupt cannot work well in deep sleep mode
* :jira:`ZEP-1723` - Warnings in Network code/ MACROS, when built with ISSM's  llvm/icx compiler
* :jira:`ZEP-1732` - sample of zoap_server runs error.
* :jira:`ZEP-1733` - Work on ZEP-686 led to regressions in docs on integration with 3rd-party code
* :jira:`ZEP-1745` - Bluetooth samples build failure
* :jira:`ZEP-1753` - sample of dhcpv4_client runs error on Arduino 101
* :jira:`ZEP-1754` - sample of coaps_server was tested failed on qemu
* :jira:`ZEP-1756` - net apps: [-Wpointer-sign] build warning raised when built with ISSM's  llvm/icx compiler
* :jira:`ZEP-1758` - PLL2 is not correctly enabled in STM32F10x connectivity line SoC
* :jira:`ZEP-1763` - Nordic RTC timer driver not correct with tickless idle
* :jira:`ZEP-1764` - samples: sample cases use hard code device name, such as "GPIOB" "I2C_0"
* :jira:`ZEP-1768` - samples: cases miss testcase.ini
* :jira:`ZEP-1774` - Malformed packet included with IPv6 over 802.15.4
* :jira:`ZEP-1778` - tests/power: multicore case won't work as expected
* :jira:`ZEP-1786` - TCP does not work on Arduino 101 board.
* :jira:`ZEP-1787` - kernel event logger build failed with "CONFIG_LEGACY_KERNEL=n"
* :jira:`ZEP-1789` - ARC: "samples/logger-hook" crashed __memory_error from sys_ring_buf_get
* :jira:`ZEP-1799` - timeout_order_test _ASSERT_VALID_PRIO failed
* :jira:`ZEP-1803` - Error occurs when exercising dma_transfer_stop
* :jira:`ZEP-1806` - Build warnings with LLVM/icx (gdb_server)
* :jira:`ZEP-1809` - Build error in net/ip with LLVM/icx
* :jira:`ZEP-1810` - Build failure in net/lib/zoap with LLVM/icx
* :jira:`ZEP-1811` - Build error in net/ip/net_mgmt.c with LLVM/icx
* :jira:`ZEP-1839` - LL_ASSERT in event_common_prepareA
* :jira:`ZEP-1851` - Build warnings with obj_tracing
* :jira:`ZEP-1852` - LL_ASSERT in isr_radio_state_close()
* :jira:`ZEP-1855` - IP stack buffer allocation fails over time
* :jira:`ZEP-1858` - Zephyr NATS client fails to respond to  server MSG
* :jira:`ZEP-1864` - llvm icx build warning in tests/drivers/uart/*
* :jira:`ZEP-1872` - samples/net: the HTTP client sample app must run on QEMU x86
* :jira:`ZEP-1877` - samples/net: the coaps_server sample app runs failed on Arduino 101
* :jira:`ZEP-1883` - Enabling Console on ARC Genuino 101
* :jira:`ZEP-1890` - Bluetooth IPSP sample: Too small user data size


.. _zephyr_1.6:

Zephyr Kernel 1.6.0
********************

We are pleased to announce the release of Zephyr kernel version 1.6.0. This
release introduces the unified Kernel replacing the separate nano- and
micro-kernels and simplifying the overall Zephyr architecture and programming
interfaces.
Support for the ARM Cortex-M0/M0+ family was added and board support for
Cortex-M was expanded.
Additionally, this release adds many improvements for documentation, build
infrastructure, and testing.

Major enhancements included with the release:

* Introduced the Unified Kernel; the nano and micro kernel were removed.
* The legacy API is still supported but deprecated.
* Legacy tests and samples were moved to tests/legacy and samples/legacy.
* Unified kernel documentation was added and legacy nanokernel/microkernel
  documentation was removed.
* Added support for several ARM Cortex-M boards
* Added support for USB mass storage and access to the filesystem.
* Added native Bluetooth Controller support. Currently nRF51 & nRF52 are supported.

A detailed list of changes since v1.5.0 by component follows:

Kernel
======

* Introduced the unified kernel.
* Removed deprecated Tasks IRQs.
* Removed deprecated dynamic interrupt API.
* Added DLIST to operate in all elements of a doubly-linked list.
* SLIST: Added sys_slist_get() to fetch and remove the head, also Added
  append_list and merge_slist.
* Added nano_work_pending to check if it is pending execution.
* Unified: Added support for k_malloc and k_free.
* Renamed kernel objects event to alert and memory map to memory slab.
* Changed memory pool, memory maps, message queues and event handling APIs.

Architectures
=============

* ARC: Removed CONFIG_TIMER0_CLOCK_FREQ.
* ARC: Unified linker scripts.
* ARC: Removed dynamic interrupts.
* ARM: Added choice to use floating point ABI.
* ARM: Added NXP Kinetis kconfig options to configure clocks.
* ARM: Removed dynamic interrupts and exceptions.
* ARM: Atmel: Added constants and structures for watchdog registers.
* ARM: Added support for ARM Cortex-M0/M0+.
* x86: Removed dynamic interrupts and exceptions.
* x86: Declared internal API for interrupt controllers.
* x86: Changed IRQ controller to return -1 if cannot determine source vector.
* x86: Grouped Quark SoC's under intel_quark family.
* x86: Optimized and simplified IRQ and exception stubs.

Boards
======

* Renamed board Quark SE devboard to Quark SE C1000 devboard.
* Renamed board Quark SE SSS devboard to Quark SE C1000 SS devboard.
* Quark SE C1000: Disabled IPM and enabled UART0 on the Sensor Subsystem.
* Removed basic_cortex_m3 and basic_minuteia boards.
* Arduino 101: Removed backup/restore scripts. To restore original bootloader
  use flashpack utility instead.
* Renamed nRF52 Nitrogen to 96Boards Nitrogen.
* Added ARM LTD Beetle SoC and V2M Beetle board.
* Added Texas Instruments CC3200 LaunchXL support.
* Added support for Nordic Semiconductor nRF51822.
* Added support for NXP Hexiwear board.

Drivers and Sensors
===================

* SPI: Fixed typos in SPI port numbers.
* Pinmux: Removed Quark dev unused file.
* I2C: Added KSDK shim driver.
* Ethernet: Added KSDK shim driver.
* Flash: Added KSDK shim driver
* I2C: Changed config parameters to SoC specific.
* QMSI: Implemented suspend and resume functions QMSI shim drivers
* Added HP206C sensor.
* Changed config_info pointers to const.
* Added support for SoCWatch driver.
* Added FXOS8700 accelerometer / magnetometer sensor driver.

Networking
==========

* Minor fixes to uIP networking stack (This will be deprecated in 1.7)

Bluetooth
=========

* Added native Bluetooth Controller support. Currently nRF51 & nRF52 are supported.
* New location for Controller & Host implementations: subsys/bluetooth/
* Added raw HCI API to enable physical HCI transport for a Controller-only build.
* Added sample raw HCI apps for USB and UART.
* Added cross-transport pairing support for the Security Manager Protocol.
* Added RFCOMM support (for Bluetooth Classic)
* Added basic persistent storage support (filesystem-backed)
* Renamed bt_driver API to bt_hci_driver, in anticipation of Bluetooth radio drivers.

Build Infrastructure
====================

* Makefile: Changed outdir into board-specific directory to avoid build collisions.
* Makefile: Changed to use HOST_OS environment variable.
* Makefile: Added support for third party build systems.
* Sanity: Added support to filter using environment variables.
* Sanity: Added support for multiple toolchains.
* Sanity: Added ISSM and ARM GCC embedded toolchains to the supported toolchains.
* Sanity: Added extra arguments to be passed to the build.
* Sanity: Removed linker VMA/LMA offset check.
* Sysgen: Added --kernel_type argument.
* Modified build infrastructure to support unified kernel.
* SDK: Zephyr: Added check for minimum required version.
* Imported get_maintainer.pl from Linux kernel.

Libraries
=========

* libc: Added subset of standard types in inttypes.h.
* libc: Added support for 'z' length specifier.
* libc: Removed stddef.h which is provided by the compiler.
* libc: printf: Improved code for printing.
* printk: Added support for modifiers.
* Added CoAP implementation for Zephyr.
* File system: Added API to grow or shrink a file.
* File system: Added API to get volume statistics.
* File system: Added API to flush cache of an opened file.

HALs
====

* QMSI: Updated to version 1.3.1.
* HAL: Imported CC3200 SDK.
* Imported Nordic MDK nRF51 files.
* Imported Kinetis SDK Ethernet phy driver.
* Imported SDK RNGA driver.

Documentation
=============

* Drivers: Improved Zephyr Driver model.
* Updated device power management API.
* Unified Kernel primer.
* Moved supported board information to the wiki.zephyrproject.org site.
* Revised documentation for Kernel Event logger and Timing.

Test and Samples
================

* Fixed incorrect printk usage.
* Removed test for dynamic exceptions.
* Added USB sample.
* Added tests and samples for CoAP client and server.
* Added philosophers unified sample.
* Removed printf/printk wrappers.
* Added Unified kernel API samples.
* Imported tinycrypt test cases for CTR, ECC DSA and ECC DH algorithm.

Deprecations
============

* Deprecated microkernel and nanokernel APIs.
* Removed dynamic IRQs and exceptions.
* Removed Tasks IRQs.

JIRA Related Items
==================

* :jira:`ZEP-308` - Build System cleanup and Kernel / Application build separation
* :jira:`ZEP-334` - Unified Kernel
* :jira:`ZEP-766` - USB Mass Storage access to internal filesystem
* :jira:`ZEP-1090` - CPU x86 save/restore using new QMSI bootloader flow
* :jira:`ZEP-1173` - Add support for bonding remove
* :jira:`ZEP-48` - define API for interrupt controllers
* :jira:`ZEP-181` - Persistent storage APIs
* :jira:`ZEP-233` - Support USB mass storage device class
* :jira:`ZEP-237` - Support pre-built host tools
* :jira:`ZEP-240` - printk/printf usage in samples
* :jira:`ZEP-248` - Add a BOARD/SOC porting guide
* :jira:`ZEP-342` - USB DFU
* :jira:`ZEP-451` - Quark SE output by default redirected to IPM
* :jira:`ZEP-521` - ARM - add choice to floating point ABI selection
* :jira:`ZEP-546` - UART interrupts not triggered on ARC
* :jira:`ZEP-584` - warn user if SDK is out of date
* :jira:`ZEP-592` - Sanitycheck support for multiple toolchains
* :jira:`ZEP-605` - SMP over BR/EDR
* :jira:`ZEP-614` - Port tinycrypt 2.0 test cases to Zephyr
* :jira:`ZEP-622` - Add FS API to truncate/shrink a file
* :jira:`ZEP-627` - Port Trickle support from Contiki into current stack
* :jira:`ZEP-635` - Add FS API to grow a file
* :jira:`ZEP-636` - Add FS API to get volume total and free space
* :jira:`ZEP-640` - Remove dynamic IRQs/exceptions from Zephyr
* :jira:`ZEP-653` - QMSI shim driver: Watchdog: Implement suspend and resume callbacks
* :jira:`ZEP-654` - QMSI shim driver: I2C: Implement suspend and resume callbacks
* :jira:`ZEP-657` - QMSI shim driver: AONPT: Implement suspend and resume callbacks
* :jira:`ZEP-661` - QMSI shim driver: SPI: Implement suspend and resume callbacks
* :jira:`ZEP-688` - unify duplicated sections of arch linker scripts
* :jira:`ZEP-715` - Add K64F clock configurations
* :jira:`ZEP-716` - Add Hexiwear board support
* :jira:`ZEP-717` - Add ksdk I2C shim driver
* :jira:`ZEP-718` - Add ksdk ethernet shim driver
* :jira:`ZEP-721` - Add FXOS8700 accelerometer/magnetometer sensor driver
* :jira:`ZEP-737` - Update host tools from upstream: fixdep.c
* :jira:`ZEP-740` - PWM API: Check if 'flags' argument is really required
* :jira:`ZEP-745` - Revisit design of PWM Driver API
* :jira:`ZEP-750` - Arduino 101 board should support one configuration using original bootloader
* :jira:`ZEP-758` - Rename Quark SE Devboard to its official name: Quark SE C1000
* :jira:`ZEP-767` - Add FS API to flush cache of an open file
* :jira:`ZEP-775` - Enable USB CDC by default on Arduino 101 and redirect serial to USB
* :jira:`ZEP-783` - ARM Cortex-M0/M0+ support
* :jira:`ZEP-784` - Add support for Nordic Semiconductor nRF51822 SoC
* :jira:`ZEP-850` - remove obsolete boards basic_minuteia and basic_cortex_m3
* :jira:`ZEP-906` - [unified] Add scheduler time slicing support
* :jira:`ZEP-907` - Test memory pool support (with mailboxes)
* :jira:`ZEP-908` - Add task offload to fiber support
* :jira:`ZEP-909` - Adapt tickless idle + power management for ARM
* :jira:`ZEP-910` - Adapt tickless idle for x86
* :jira:`ZEP-912` - Finish renaming kernel object types
* :jira:`ZEP-916` - Eliminate kernel object API anomalies
* :jira:`ZEP-920` - Investigate malloc/free support
* :jira:`ZEP-921` - Miscellaneous documentation work
* :jira:`ZEP-922` - Revise documentation for Kernel Event Logger
* :jira:`ZEP-923` - Revise documentation for Timing
* :jira:`ZEP-924` - Revise documentation for Interrupts
* :jira:`ZEP-925` - API changes to message queues
* :jira:`ZEP-926` - API changes to memory pools
* :jira:`ZEP-927` - API changes to memory maps
* :jira:`ZEP-928` - API changes to event handling
* :jira:`ZEP-930` - Cut over to unified kernel
* :jira:`ZEP-933` - Unified kernel ARC port
* :jira:`ZEP-934` - NIOS_II port
* :jira:`ZEP-935` - Kernel logger support (validation)
* :jira:`ZEP-954` - Update device PM API to allow setting additional power states
* :jira:`ZEP-957` - Create example sample for new unified kernel API usage
* :jira:`ZEP-959` - sync checkpatch.pl with upstream Linux
* :jira:`ZEP-966` - need support for EM7D SOC on em_starterkit
* :jira:`ZEP-975` - DNS client port to new IP stack
* :jira:`ZEP-981` - Add doxygen documentation to both include/kernel.h and include/legacy.h
* :jira:`ZEP-989` - Cache next ready thread instead of finding out the long way
* :jira:`ZEP-993` - Quark SE (x86): Refactor save/restore execution context feature
* :jira:`ZEP-994` - Quark SE (ARC): Add PMA sample
* :jira:`ZEP-996` - Refactor save/restore feature from i2c_qmsi driver
* :jira:`ZEP-997` - Refactor save/restore feature from spi_qmsi driver
* :jira:`ZEP-998` - Refactor save/restore feature from uart_qmsi driver
* :jira:`ZEP-999` - Refactor save/restore feature from gpio_qmsi driver
* :jira:`ZEP-1000` - Refactor save/restore feature from rtc_qmsi driver
* :jira:`ZEP-1001` - Refactor save/restore feature from wdt_qmsi driver
* :jira:`ZEP-1002` - Refactor save/restore feature from counter_qmsi_aonpt driver
* :jira:`ZEP-1004` - Extend counter_qmsi_aon driver to support save/restore peripheral context
* :jira:`ZEP-1005` - Extend dma_qmsi driver to support save/restore peripheral context
* :jira:`ZEP-1006` - Extend soc_flash_qmsi driver to support save/restore peripheral context
* :jira:`ZEP-1008` - Extend pwm_qmsi driver to support save/restore peripheral context
* :jira:`ZEP-1023` - workq in Kernel primer for unified kernel
* :jira:`ZEP-1030` - Enable QMSI shim drivers of SoC peripherals on the sensor subsystem
* :jira:`ZEP-1043` - Update QMSI to 1.2
* :jira:`ZEP-1045` - Add/Enhance shim layer to wrap SOC specific PM implementations
* :jira:`ZEP-1046` - Implement RAM sharing between bootloader and Zephyr
* :jira:`ZEP-1047` - Adapt to new PM related boot flow changes in QMSI boot loader
* :jira:`ZEP-1106` - Fix all test failures from TCF
* :jira:`ZEP-1107` - Update QMSI to 1.3
* :jira:`ZEP-1109` - Texas Instruments CC3200 LaunchXL Support
* :jira:`ZEP-1119` - move top level usb/ to sys/usb
* :jira:`ZEP-1120` - move top level fs/ to sys/fs
* :jira:`ZEP-1121` - Add config support for enabling SoCWatch in Zephyr
* :jira:`ZEP-1140` - Add a unified kernel version of power_mgr sample app for testing PM code with the new kernel
* :jira:`ZEP-1188` - Add an API to retrieve pending interrupts for wake events
* :jira:`ZEP-1191` - Create wiki page for Hexiwear board
* :jira:`ZEP-1235` - Basic shell support for file system browsing
* :jira:`ZEP-1245` - ARM LTD V2M Beetle Support
* :jira:`ZEP-1313` - porting and user guides must include a security section
* :jira:`ZEP-1386` - Revise power management document to reflect latest changes
* :jira:`ZEP-199` - Zephyr driver model is undocumented
* :jira:`ZEP-436` - Test case tests/kernel/test_mem_safe fails on ARM hardware
* :jira:`ZEP-471` - Ethernet packet with multicast address is not working
* :jira:`ZEP-472` - Ethernet packets are getting missed if sent in quick succession.
* :jira:`ZEP-517` - build on windows failed "zephyr/Makefile:869: \*\*\* multiple target patterns"
* :jira:`ZEP-528` - ARC has 2 almost identical copies of the linker script
* :jira:`ZEP-577` - Sample application source does not compile on Windows
* :jira:`ZEP-601` - enable CONFIG_DEBUG_INFO
* :jira:`ZEP-602` - unhandled CPU exceptions/interrupts report wrong faulting vector if triggered by CPU
* :jira:`ZEP-615` - Un-supported flash erase size listed in SPI flash w25qxxdv driver header file
* :jira:`ZEP-639` - device_pm_ops structure should be defined as static
* :jira:`ZEP-686` - docs: Info in "Application Development Primer" and "Developing an Application and the Build System" is largely duplicated
* :jira:`ZEP-698` - samples/task_profiler issues
* :jira:`ZEP-707` - mem_safe test stomps on top of .data and bottom of .noinit
* :jira:`ZEP-724` - build on windows failed: 'make: execvp: uname: File or path name too long'
* :jira:`ZEP-733` - Minimal libc shouldn't be providing stddef.h
* :jira:`ZEP-762` - unexpected "abspath" and "notdir" from mingw make system
* :jira:`ZEP-777` - samples/driver/i2c_stts751: kconfig build warning from "select DMA_QMSI"
* :jira:`ZEP-778` - Samples/drivers/i2c_lsm9ds0: kconfig build warning from "select DMA_QMSI"
* :jira:`ZEP-779` - Using current MinGW gcc version 5.3.0 breaks Zephyr build on Windows
* :jira:`ZEP-845` - UART for ARC on Arduino 101 behaves unexpectedly
* :jira:`ZEP-905` - hello_world compilation for arduino_due target fails when using CROSS_COMPILE
* :jira:`ZEP-940` - Fail to get ATT response
* :jira:`ZEP-950` - USB: Device is not listed by USB20CV test suite
* :jira:`ZEP-961` - samples: other cases cannot execute after run aon_counter case
* :jira:`ZEP-967` - Sanity doesn't build 'samples/usb/dfu' with assertions (-R)
* :jira:`ZEP-970` - Sanity doesn't build 'tests/kernel/test_build' with assertions (-R)
* :jira:`ZEP-982` - Minimal libc has EWOULDBLOCK != EAGAIN
* :jira:`ZEP-1014` - [TCF] tests/bluetooth/init build fail
* :jira:`ZEP-1025` - Unified kernel build sometimes breaks on a missing .d dependency file.
* :jira:`ZEP-1027` - Documentation for GCC ARM is not accurate
* :jira:`ZEP-1031` - qmsi: dma: driver test fails with LLVM
* :jira:`ZEP-1048` - grove_lcd sample: sample does not work if you disable serial
* :jira:`ZEP-1051` - mpool allocation failed after defrag twice...
* :jira:`ZEP-1062` - Unified kernel isn't compatible with CONFIG_NEWLIB_LIBC
* :jira:`ZEP-1074` - ATT retrying misbehaves when ATT insufficient Authentication is received
* :jira:`ZEP-1076` - "samples/philosophers/unified" build failed with dynamic stack
* :jira:`ZEP-1077` - "samples/philosophers/unified" build warnings with NUM_PHIL<6
* :jira:`ZEP-1079` - Licensing not clear for imported components
* :jira:`ZEP-1097` - ENC28J60 driver fails on concurrent tx and rx
* :jira:`ZEP-1098` - ENC28J60 fails to receive big data frames
* :jira:`ZEP-1100` - Current master still identifies itself as 1.5.0
* :jira:`ZEP-1101` - SYS_KERNEL_VER_PATCHLEVEL() and friends artificially limit version numbers to 4 bits
* :jira:`ZEP-1124` - tests/kernel/test_sprintf/microkernel/testcase.ini#test failure on frdm_k64f
* :jira:`ZEP-1130` - region 'RAM' overflowed occurs while building test_hmac_prng
* :jira:`ZEP-1138` - Received packets not being passed to upper layer from IP stack when using ENC28J60 driver
* :jira:`ZEP-1139` - Fix build error when power management is built with unified kernel
* :jira:`ZEP-1141` - Tinycrypt SHA256 test fails with system crash using unified kernel type
* :jira:`ZEP-1144` - Tinycrypt AES128 fixed-key with variable-text test fails using unified kernel type
* :jira:`ZEP-1145` - system hang after tinycrypt HMAC test
* :jira:`ZEP-1146` - zephyrproject.org home page needs technical scrub for 1.6 release
* :jira:`ZEP-1149` - port ztest framework to unified kernel
* :jira:`ZEP-1154` - tests/samples failing with unified kernel
* :jira:`ZEP-1155` - Fix filesystem API namespace
* :jira:`ZEP-1163` - LIB_INCLUDE_DIR is clobbered in Makefile second pass
* :jira:`ZEP-1164` - ztest skip waiting the test case to finish its execution
* :jira:`ZEP-1179` - Build issues when compiling with LLVM from ISSM (icx)
* :jira:`ZEP-1182` - kernel.h doxygen show unexpected "asm" blocks
* :jira:`ZEP-1183` - btshell return "panic: errcode -1" when init bt
* :jira:`ZEP-1195` - Wrong ATT error code passed to the application
* :jira:`ZEP-1199` - [L2CAP] No credits to receive packet
* :jira:`ZEP-1219` - [L2CAP] Data sent exceeds maximum PDU size
* :jira:`ZEP-1221` - Connection Timeout during pairing
* :jira:`ZEP-1226` - cortex M7 port assembler error
* :jira:`ZEP-1227` - ztest native testing not working in unified kernel
* :jira:`ZEP-1232` - Daily build is failing asserts
* :jira:`ZEP-1234` - Removal of fiber* APIs due to unified migration breaks USB mass storage patchset
* :jira:`ZEP-1247` - Test tests/legacy/benchmark/latency_measure is broken for daily sanitycheck
* :jira:`ZEP-1252` - Test test_chan_blen_transfer does not build for quark_d2000_crb
* :jira:`ZEP-1277` - Flash driver (w25qxxdv) erase function is not checking for offset alignment
* :jira:`ZEP-1278` - Incorrect boundary check in flash driver (w25qxxdv) for erase offset
* :jira:`ZEP-1287` - ARC SPI 1 Port is not working
* :jira:`ZEP-1289` - Race condition with k_sem_take
* :jira:`ZEP-1291` - libzephyr.a dependency on phony "gcc" target
* :jira:`ZEP-1293` - ENC28J60 driver doesn't work on Arduino 101
* :jira:`ZEP-1295` - incorrect doxygen comment in kernel.h:k_work_pending()
* :jira:`ZEP-1297` - test/legacy/kernel/test_mail: failure on ARC platforms
* :jira:`ZEP-1299` - System can't resume completely with DMA suspend and resume operation
* :jira:`ZEP-1302` - ENC28J60 fails with rx/tx of long frames
* :jira:`ZEP-1303` - Configuration talks about >32 thread prios, but the kernel does not support it
* :jira:`ZEP-1309` - ARM uses the end of memory for its init stack
* :jira:`ZEP-1310` - ARC uses the end of memory for its init stack
* :jira:`ZEP-1312` - ARC: software crashed at k_mbox_get() with async sending a message
* :jira:`ZEP-1319` - Zephyr is unable to compile when CONFIG_RUNTIME_NMI is enabled on ARM platforms
* :jira:`ZEP-1341` - power_states test app passes wrong value as power state to post_ops functions
* :jira:`ZEP-1343` - tests/drivers/pci_enum: failing on QEMU ARM and X86 due to missing commit
* :jira:`ZEP-1345` - cpu context save and restore could corrupt stack
* :jira:`ZEP-1349` - ARC sleep needs to pass interrupt priority threshold when interrupts are enabled
* :jira:`ZEP-1353` - FDRM k64f Console output broken on normal flash mode

Known Issues
============

* :jira:`ZEP-1405` - function l2cap_br_conn_req in /subsys/bluetooth/host/l2cap_br.c
  references uninitialized pointer
