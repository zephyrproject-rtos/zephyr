:orphan:

.. _zephyr_1.14:
.. _zephyr_1.14.1:

Zephyr 1.14.3
####################

This is an LTS maintenance release with fixes.

Security Vulnerability Related
******************************

The following security vulnerabilities (CVEs) were addressed in this
release:

* CVE-2020-10066
* CVE-2020-10069
* CVE-2020-13601
* CVE-2020-13602

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Issues Fixed
************

These GitHub issues were addressed since the previous 1.14.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`18334` - DNS resolution is broken for some addresses in master/2.0-pre
* :github:`19917` - Bluetooth: Controller: Missing LL_ENC_RSP after HCI LTK Negative Reply
* :github:`21107` - LL_ASSERT and 'Imprecise data bus error' in LL Controller
* :github:`21257` - tests/net/net_pkt failed on mimxrt1050_evk board.
* :github:`21299` - bluetooth: Controller does not release buffer on central side after peripheral reset
* :github:`21601` - '!radio_is_ready()' failed
* :github:`21756` - tests/kernel/obj_tracing failed on mec15xxevb_assy6853 board.
* :github:`22968` - Bluetooth: controller: LEGACY: ASSERTION failure on invalid packet sequence
* :github:`23069` - Bluetooth: controller: Assert in data length update procedure
* :github:`23109` - LL.TS Test LL/CON/SLA/BV-129-C fails (split)
* :github:`23805` - Bluetooth: controller: Switching to non conn adv fails for Mesh LPN
* :github:`24601` - Bluetooth: Mesh: Config Client's net_key_status pulls two key indexes, should pull one.
* :github:`25518` - settings_fcb: Fix storing the data
* :github:`25519` - wrong debug function cause kinds of building error
* :github:`26080` - gPTP time sync fails if having more than one port
* :github:`28151` - gPTP should allow user setting of priority1 and priority2 fields used in BMCA
* :github:`28177` - gPTP gptp_priority_vector struct field ordering is wrong
* :github:`29386` - unexpected behavior when doing syscall with 7 or more arguments
* :github:`29858` - Bluetooth: Mesh: RPL cleared on LPN disconnect
* :github:`32430` - Bluetooth: thread crashes when configuring a non 0 Slave Latency
* :github:`32898` - Bluetooth: controller: Control PDU buffer leak into Data PDU buffer pool

Zephyr 1.14.2
####################

This is an LTS maintenance release with fixes.

Security Vulnerability Related
******************************

The following security vulnerabilities (CVEs) were addressed in this
release:

* CVE-2020-10019
* CVE-2020-10021
* CVE-2020-10022
* CVE-2020-10023
* CVE-2020-10024
* CVE-2020-10027
* CVE-2020-10028

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Issues Fixed
************

These GitHub issues were addressed since the previous 1.14.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`11617` - net: ipv4: udp: broadcast delivery not supported
* :github:`11743` - logging: add user mode access
* :github:`14459` - usb: samples: mass: doesn't build with FLASH overlay
* :github:`15119` - GPIO callback not disabled from an interrupt
* :github:`15339` - RISC-V: RV32M1: Load access fault when accessing GPIO port E
* :github:`15354` - counter: stm32: Issue with LSE clock source selection
* :github:`15373` - IPv4 link local packets are not sent with ARP ethernet type
* :github:`15443` - usb_dc_stm32: Missing semaphore initialization and missing pin remapping configuration
* :github:`15444` - Error initiating sdhc disk
* :github:`15497` - USB DFU: STM32: usb dfu mode doesn't work
* :github:`15507` - NRF52840: usb composite MSC + HID (with CONFIG_ENABLE_HID_INT_OUT_EP)
* :github:`15526` - Unhandled identity in bt_conn_create_slave_le
* :github:`15558` - support for power-of-two MPUs on non-XIP systems
* :github:`15601` - pwm: nRF default prescalar value is wrong
* :github:`15603` - Unable to use C++ Standard Library
* :github:`15605` - Unaligned memory access by ldrd
* :github:`15678` - Watchdog peripheral api docs aren't generated correctly.
* :github:`15698` - bluetooth: bt_conn: No proper ID handling
* :github:`15733` - Bluetooth: controller: Central Encryption setup overlaps Length Request procedure
* :github:`15794` - mps2_an385 crashes if CONFIG_INIT_STACKS=y and CONFIG_COVERAGE=y
* :github:`15817` - nrf52: HFXO is not turned off as expected
* :github:`15904` - concerns with use of CONFIG_BT_MESH_RPL_STORE_TIMEOUT in examples
* :github:`15911` - Stack size is smaller than it should be
* :github:`15975` - Openthread - fault with dual network interfaces
* :github:`16001` - ARC iotdk supports MPU and fpu in hardware but not enabled in kconfig
* :github:`16002` - the spi base reg address in arc_iot.dtsi has an error
* :github:`16010` - Coverage reporting fails on many tests
* :github:`16012` - Source IP address for DHCP renewal messages is unset
* :github:`16046` - modules are being processed too late.
* :github:`16080` - Zephyr UART shell crashes on start if main() is blocked
* :github:`16089` - Mcux Ethernet driver does not detect carrier anymore (it's alway on)
* :github:`16090` - mpu align support for code relocation on non-XIP system
* :github:`16143` - posix: clock_settime calculates the base time incorrectly
* :github:`16155` - drivers: can: wrong value used for filter mode set
* :github:`16257` - net: icmpv4: Zephyr sends echo reply with multicast source address
* :github:`16307` - cannot move location counter backwards error happen
* :github:`16323` - net: ipv6: tcp: unexpected reply to malformed HBH in TCP/IPv6 SYN
* :github:`16339` - openthread: off-by-one error when calculating ot_flash_offset for settings
* :github:`16354` - net: ipv6: Zephyr does not reply to fragmented packet
* :github:`16375` - net: ipv4: udp: Zephyr does not reply to a valid datagram with checksum zero
* :github:`16379` - net: ipv6: udp: Zephyr replies with illegal UDP checksum zero
* :github:`16411` - bad regex for west version check in host-tools.cmake
* :github:`16412` - on reel_board the consumption increases because TX pin is floating
* :github:`16413` - Missing dependency in cmake
* :github:`16414` - Backport west build --pristine
* :github:`16415` - Build errors with C++
* :github:`16416` - sram size for RT1015 and RT1020 needs to be update.
* :github:`16417` - issues with can filter mode set
* :github:`16418` - drivers: watchdog: sam0: check if timeout is valid
* :github:`16419` - Bluetooth: XTAL feature regression
* :github:`16478` - Bluetooth: Improper bonded peers handling
* :github:`16570` - [Coverity CID :198877]Null pointer dereferences in /subsys/net/ip/net_if.c
* :github:`16577` - [Coverity CID :198870]Error handling issues in /subsys/net/lib/lwm2m/lwm2m_obj_firmware_pull.c
* :github:`16581` - [Coverity CID :198866]Null pointer dereferences in /subsys/net/lib/dns/llmnr_responder.c
* :github:`16584` - [Coverity CID :198863]Error handling issues in /subsys/net/lib/sntp/sntp.c
* :github:`16600` - Bluetooth: Mesh: Proxy SAR timeout is not implemented
* :github:`16602` - Bluetooth: GATT Discovery: Descriptor Discovery by range Seg Fault
* :github:`16639` - eth: pinging frdm k64f eventually leads to unresponsive ethernet device
* :github:`16678` - LPN establishment of Friendship never completes if there is no response to the initial Friend Poll
* :github:`16711` - Settings reworked to const char processing
* :github:`16734` - Bluetooth: GATT: Writing 1 byte to a CCC access invalid memory
* :github:`16745` - PTHREAD_MUTEX_DEFINE(): don't store into the _k_mutex section
* :github:`16746` - boards: nrf52840_pca10059: Configure NFC pins as GPIOs by default
* :github:`16749` - IRQ_CONNECT and irq_enable calls in the SiFive UART driver is misconfigured
* :github:`16750` - counter:  lack of interrupt when CC=0
* :github:`16760` - K_THREAD_STACK_EXTERN() confuses gen_kobject_list.py
* :github:`16779` - [Zephyr v1.14] ARM: fix the start address of MPU guard in stack-fail checking (when building with no user mode)
* :github:`16799` - Bluetooth: L2CAP: Interpretation of SCID and DCID in Disconnect is wrong
* :github:`16861` - nRF52: UARTE: Data corruption right after resuming device
* :github:`16864` - Bluetooth: Mesh: Rx buffer exhaustion causes deadlock
* :github:`16893` - Bluetooth: Multiple local IDs, privacy problem
* :github:`16943` - Missing test coverage for lib/os/crc*.c
* :github:`16944` - Insufficient test coverage for lib/os/json.c
* :github:`17031` - Compiler warnings in settings module in Zephyr 1.14
* :github:`17038` - code relocation generating different memory layout cause user mode not working
* :github:`17041` - [1.14] Bluetooth: Mesh: RPL handling is not in line with the spec
* :github:`17055` - net: Incorrect data length after the connection is established
* :github:`17057` - Bluetooth: Mesh: Implementation doesn't conform to latest errata and 1.0.1 version
* :github:`17092` - Bluetooth: GAP/IDLE/NAMP/BV-01-C requires Read by UUID
* :github:`17170` - x86_64 crash with spinning child thread
* :github:`17171` - Insufficient code coverage for lib/os/fdtable.c
* :github:`17177` - ARM: userspace/test_bad_syscall fails on ARMv8-M
* :github:`17190` - net-mgmt should pass info element size to callback
* :github:`17250` - After first GC operation the 1st sector had become scratch and the 2nd sector had became write sector.
* :github:`17251` - w25q: erase operations must be erase-size aligned
* :github:`17262` - insufficient code coverage for lib/os/base64.c
* :github:`17288` - Bluetooth: controller: Fix handling of L2CAP start frame with zero PDU length
* :github:`17294` - DB corruption when adding/removing service
* :github:`17337` - ArmV7-M mpu sub region alignment
* :github:`17338` - kernel objects address check in elf_helper.py
* :github:`17368` - Time Slicing cause system sleep short time
* :github:`17399` - LwM2M: Can't use an alternate mbedtls implementation
* :github:`17401` - LwM2M: requires that CONFIG_NET_IPV* be enabled (can't use 100% offloaded IP stack)
* :github:`17415` - Settings Module - settings_line_val_read() returning -EINVAL instead of 0 for deleted setting entries
* :github:`17427` - net: IPv4/UDP datagram with zero src addr and TTL causes Zephyr to segfault
* :github:`17450` - net: IPv6/UDP datagram with unspecified addr and zero hop limit causes Zephyr to quit
* :github:`17463` - Bluetooth: API limits usage of MITM flags in Pairing Request
* :github:`17534` - Race condition in GATT API.
* :github:`17595` - two userspace tests fail if stack canaries are enabled in board configuration
* :github:`17600` - Enable Mesh Friend support in Bluetooth tester application
* :github:`17613` - POSIX arch: occasional failures of tests/kernel/sched/schedule_api on CI
* :github:`17630` - efr32mg_sltb004a tick clock error
* :github:`17723` - Advertiser never clears state flags
* :github:`17732` - cannot use bt_conn_security in connected callback
* :github:`17764` - Broken link to latest development version of docs
* :github:`17802` - [zephyr 1.14] Address type 0x02 is used by LE Create Connection in device privacy mode
* :github:`17820` - Mesh  bug report In access.c
* :github:`17838` - state DEVICE_PM_LOW_POWER_STATE of Device Power Management
* :github:`17843` - Bluetooth: controller: v1.14.x release conformance test failures
* :github:`17857` - GATT: Incorrect byte order for GATT database hash
* :github:`17861` - Tester application lacks BTP Discover All Primary Services handler
* :github:`17880` - Unable to re-connect to privacy enabled peer when using stack generated Identity
* :github:`17944` - [zephyr 1.14]  LE Enhanced Connection Complete indicates Resolved Public once connected to Public peer address
* :github:`17948` - Bluetooth: privacy: Reconnection issue
* :github:`17967` - drivers/pwm/pwm_api test failed on frdm_k64f board.
* :github:`17971` - [zephyr 1.14] Unable to register GATT service that was unregistered before
* :github:`17979` - Security level cannot be elevated after re-connection with privacy
* :github:`18021` - Socket vtable can access null pointer callback function
* :github:`18090` - [zephyr 1.14][MESH/NODE/FRND/FN/BV-08-C] Mesh Friend queues more messages than indicates it's Friend Cache
* :github:`18178` - BLE Mesh When Provisioning Use Input OOB Method
* :github:`18183` - [zephyr 1.14][GATT/SR/GAS/BV-07-C] GATT Server does not inform change-unaware client about DB changes
* :github:`18297` - Bluetooth: SMP: Pairing issues
* :github:`18306` - Unable to reconnect paired devices with controller privacy disabled (host privacy enabled)
* :github:`18308` - net: TCP/IPv6 set of fragmented packets causes Zephyr to quit
* :github:`18394` - [Coverity CID :203464]Memory - corruptions in /subsys/net/l2/ethernet/gptp/gptp_mi.c
* :github:`18462` - potential buffer overrun in logging infrastructure
* :github:`18580` - Bluetooth: Security fail on initial pairing
* :github:`18658` - Bluetooth BR/EDR encryption key negotiation vulnerability
* :github:`18739` - k_uptime_get_32() does not behave as documented
* :github:`18935` - [Zephyr 1.14] drivers: flash: spi_nor: Problematic write with page boundaries
* :github:`18961` - [Coverity CID :203912]Error handling issues in /samples/net/sockets/coap_client/src/coap-client.c
* :github:`19015` - Bluetooth: Mesh: Node doesn't respond to "All Proxies" address
* :github:`19038` - [zephyr branch 1.14 and master -stm32-netusb]:errors when i view RNDIS Device‘s properties on Windows 10
* :github:`19059` - i2c_ll_stm32_v2: nack on write is not handled correctly
* :github:`19103` - zsock_accept_ctx blocks even when O_NONBLOCK is specified
* :github:`19165` - zephyr_file generates bad links on branches
* :github:`19263` - Bluetooth: Mesh: Friend Clear Procedure Timeout
* :github:`19515` - Bluetooth: Controller: assertion failed
* :github:`19612` - ICMPv6 packet is routed to wrong interface when peer is not found in neighbor cache
* :github:`19678` - Noticeable delay between processing multiple client connection requests (200ms+)
* :github:`19889` - Buffer leak in GATT for Write Without Response and Notifications
* :github:`19982` - Periodically wake up log process thread consume more power
* :github:`20042` - Telnet can connect only once
* :github:`20100` - Slave PTP clock time is updated with large value when Master PTP Clock time has changed
* :github:`20229` - cmake: add --divide to GNU assembler options for x86
* :github:`20299` - bluetooth: host: Connection not being unreferenced when using CCC match callback
* :github:`20313` - Zperf documentation points to wrong iPerf varsion
* :github:`20811` - spi driver
* :github:`20970` - Bluetooth: Mesh: seg_tx_reset in the transport layer
* :github:`21131` - Bluetooth: host: Subscriptions not removed upon unpair
* :github:`21306` - ARC: syscall register save/restore needs backport to 1.14
* :github:`21431` - missing async uart.h system calls
* :github:`21432` - watchdog subsystem has no system calls
* :github:`22275` - arm: cortex-R & M: CONFIG_USERSPACE: intermittent Memory region write access failures
* :github:`22280` - incorrect linker routing
* :github:`23153` - Binding AF_PACKET socket second time will fail with multiple network interfaces
* :github:`23339` - tests/kernel/sched/schedule_api failed on mps2_an385 with v1.14 branch.
* :github:`23346` - bl65x_dvk boards do not reset after flashing

Zephyr 1.14.1
#############

This is an LTS maintenance release with fixes, as well as Bluetooth
qualification listings for the Bluetooth protocol stack included in Zephyr.

See :ref:`zephyr_1.14.0` for the previous version release notes.

Security Vulnerability Related
******************************

The following security vulnerability (CVE) was addressed in this
release:

* Fixes CVE-2019-9506: The Bluetooth BR/EDR specification up to and
  including version 5.1 permits sufficiently low encryption key length
  and does not prevent an attacker from influencing the key length
  negotiation. This allows practical brute-force attacks (aka "KNOB")
  that can decrypt traffic and inject arbitrary ciphertext without the
  victim noticing.

Bluetooth
*********

* Qualification:

  * 1.14.x Host subsystem qualified with QDID 139258
  * 1.14.x Mesh subsystem qualified with QDID 139259
  * 1.14.x Controller component qualified on Nordic nRF52 with QDID 135679

Issues Fixed
************

These GitHub issues were addressed since the previous 1.14.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`11617` - net: ipv4: udp: broadcast delivery not supported
* :github:`11743` - logging: add user mode access
* :github:`14459` - usb: samples: mass: doesn't build with FLASH overlay
* :github:`15279` - mempool alignment might cause a memory block allocated twice
* :github:`15339` - RISC-V: RV32M1: Load access fault when accessing GPIO port E
* :github:`15354` - counter: stm32: Issue with LSE clock source selection
* :github:`15373` - IPv4 link local packets are not sent with ARP ethernet type
* :github:`15443` - usb_dc_stm32: Missing semaphore initialization and missing pin remapping configuration
* :github:`15444` - Error initiating sdhc disk
* :github:`15497` - USB DFU: STM32: usb dfu mode doesn't work
* :github:`15507` - NRF52840: usb composite MSC + HID (with CONFIG_ENABLE_HID_INT_OUT_EP)
* :github:`15526` - Unhandled identity in bt_conn_create_slave_le
* :github:`15558` - support for power-of-two MPUs on non-XIP systems
* :github:`15601` - pwm: nRF default prescalar value is wrong
* :github:`15603` - Unable to use C++ Standard Library
* :github:`15605` - Unaligned memory access by ldrd
* :github:`15606` - trickle.c can't work for multiple triggerings
* :github:`15678` - Watchdog peripheral api docs aren't generated correctly.
* :github:`15698` - bluetooth: bt_conn: No proper ID handling
* :github:`15733` - Bluetooth: controller: Central Encryption setup overlaps Length Request procedure
* :github:`15794` - mps2_an385 crashes if CONFIG_INIT_STACKS=y and CONFIG_COVERAGE=y
* :github:`15817` - nrf52: HFXO is not turned off as expected
* :github:`15904` - concerns with use of CONFIG_BT_MESH_RPL_STORE_TIMEOUT in examples
* :github:`15911` - Stack size is smaller than it should be
* :github:`15975` - Openthread - fault with dual network interfaces
* :github:`16001` - ARC iotdk supports MPU and fpu in hardware but not enabled in kconfig
* :github:`16002` - the spi base reg address in arc_iot.dtsi has an error
* :github:`16010` - Coverage reporting fails on many tests
* :github:`16012` - Source IP address for DHCP renewal messages is unset
* :github:`16027` - support for no-flash systems
* :github:`16046` - modules are being processed too late.
* :github:`16090` - mpu align support for code relocation on non-XIP system
* :github:`16107` - Using bt_gatt_read() with 'by_uuid' method returns 3 extra bytes
* :github:`16143` - posix: clock_settime calculates the base time incorrectly
* :github:`16155` - drivers: can: wrong value used for filter mode set
* :github:`16257` - net: icmpv4: Zephyr sends echo reply with multicast source address
* :github:`16307` - cannot move location counter backwards error happen
* :github:`16323` - net: ipv6: tcp: unexpected reply to malformed HBH in TCP/IPv6 SYN
* :github:`16339` - openthread: off-by-one error when calculating ot_flash_offset for settings
* :github:`16354` - net: ipv6: Zephyr does not reply to fragmented packet
* :github:`16375` - net: ipv4: udp: Zephyr does not reply to a valid datagram with checksum zero
* :github:`16379` - net: ipv6: udp: Zephyr replies with illegal UDP checksum zero
* :github:`16411` - bad regex for west version check in host-tools.cmake
* :github:`16412` - on reel_board the consumption increases because TX pin is floating
* :github:`16413` - Missing dependency in cmake
* :github:`16414` - Backport west build --pristine
* :github:`16415` - Build errors with C++
* :github:`16416` - sram size for RT1015 and RT1020 needs to be update.
* :github:`16417` - issues with can filter mode set
* :github:`16418` - drivers: watchdog: sam0: check if timeout is valid
* :github:`16419` - Bluetooth: XTAL feature regression
* :github:`16478` - Bluetooth: Improper bonded peers handling
* :github:`16570` - [Coverity CID :198877]Null pointer dereferences in /subsys/net/ip/net_if.c
* :github:`16577` - [Coverity CID :198870]Error handling issues in /subsys/net/lib/lwm2m/lwm2m_obj_firmware_pull.c
* :github:`16581` - [Coverity CID :198866]Null pointer dereferences in /subsys/net/lib/dns/llmnr_responder.c
* :github:`16584` - [Coverity CID :198863]Error handling issues in /subsys/net/lib/sntp/sntp.c
* :github:`16594` - net: dns: Zephyr is unable to unpack mDNS answers produced by another Zephyr node
* :github:`16600` - Bluetooth: Mesh: Proxy SAR timeout is not implemented
* :github:`16602` - Bluetooth: GATT Discovery: Descriptor Discovery by range Seg Fault
* :github:`16639` - eth: pinging frdm k64f eventually leads to unresponsive ethernet device
* :github:`16678` - LPN establishment of Friendship never completes if there is no response to the initial Friend Poll
* :github:`16711` - Settings reworked to const char processing
* :github:`16734` - Bluetooth: GATT: Writing 1 byte to a CCC access invalid memory
* :github:`16745` - PTHREAD_MUTEX_DEFINE(): don't store into the _k_mutex section
* :github:`16746` - boards: nrf52840_pca10059: Configure NFC pins as GPIOs by default
* :github:`16749` - IRQ_CONNECT and irq_enable calls in the SiFive UART driver is misconfigured
* :github:`16750` - counter:  lack of interrupt when CC=0
* :github:`16760` - K_THREAD_STACK_EXTERN() confuses gen_kobject_list.py
* :github:`16779` - [Zephyr v1.14] ARM: fix the start address of MPU guard in stack-fail checking (when building with no user mode)
* :github:`16799` - Bluetooth: L2CAP: Interpretation of SCID and DCID in Disconnect is wrong
* :github:`16864` - Bluetooth: Mesh: Rx buffer exhaustion causes deadlock
* :github:`16893` - Bluetooth: Multiple local IDs, privacy problem
* :github:`16943` - Missing test coverage for lib/os/crc\*.c
* :github:`16944` - Insufficient test coverage for lib/os/json.c
* :github:`17031` - Compiler warnings in settings module in Zephyr 1.14
* :github:`17038` - code relocation generating different memory layout cause user mode not working
* :github:`17041` - [1.14] Bluetooth: Mesh: RPL handling is not in line with the spec
* :github:`17055` - net: Incorrect data length after the connection is established
* :github:`17057` - Bluetooth: Mesh: Implementation doesn't conform to latest errata and 1.0.1 version
* :github:`17092` - Bluetooth: GAP/IDLE/NAMP/BV-01-C requires Read by UUID
* :github:`17170` - x86_64 crash with spinning child thread
* :github:`17177` - ARM: userspace/test_bad_syscall fails on ARMv8-M
* :github:`17190` - net-mgmt should pass info element size to callback
* :github:`17250` - After first GC operation the 1st sector had become scratch and the 2nd sector had became write sector.
* :github:`17251` - w25q: erase operations must be erase-size aligned
* :github:`17262` - insufficient code coverage for lib/os/base64.c
* :github:`17288` - Bluetooth: controller: Fix handling of L2CAP start frame with zero PDU length
* :github:`17294` - DB corruption when adding/removing service
* :github:`17337` - ArmV7-M mpu sub region alignment
* :github:`17338` - kernel objects address check in elf_helper.py
* :github:`17368` - Time Slicing cause system sleep short time
* :github:`17399` - LwM2M: Can't use an alternate mbedtls implementation
* :github:`17401` - LwM2M: requires that CONFIG_NET_IPV\* be enabled (can't use 100% offloaded IP stack)
* :github:`17415` - Settings Module - settings_line_val_read() returning -EINVAL instead of 0 for deleted setting entries
* :github:`17427` - net: IPv4/UDP datagram with zero src addr and TTL causes Zephyr to segfault
* :github:`17450` - net: IPv6/UDP datagram with unspecified addr and zero hop limit causes Zephyr to quit
* :github:`17463` - Bluetooth: API limits usage of MITM flags in Pairing Request
* :github:`17534` - Race condition in GATT API.
* :github:`17564` - Missing stdlib.h include when C++ standard library is used.
* :github:`17595` - two userspace tests fail if stack canaries are enabled in board configuration
* :github:`17600` - Enable Mesh Friend support in Bluetooth tester application
* :github:`17613` - POSIX arch: occasional failures of tests/kernel/sched/schedule_api on CI
* :github:`17723` - Advertiser never clears state flags
* :github:`17732` - cannot use bt_conn_security in connected callback
* :github:`17764` - Broken link to latest development version of docs
* :github:`17789` - Bluetooth: host: conn.c missing parameter copy
* :github:`17802` - [zephyr 1.14] Address type 0x02 is used by LE Create Connection in device privacy mode
* :github:`17809` - Bluetooth Mesh message cached too early when LPN
* :github:`17820` - Mesh  bug report In access.c
* :github:`17821` - Mesh Bug on access.c
* :github:`17843` - Bluetooth: controller: v1.14.x release conformance test failures
* :github:`17857` - GATT: Incorrect byte order for GATT database hash
* :github:`17861` - Tester application lacks BTP Discover All Primary Services handler
* :github:`17880` - Unable to re-connect to privacy enabled peer when using stack generated Identity
* :github:`17882` - [zephyr 1.14]  Database Out of Sync error is not returned as expected
* :github:`17907` - BLE Mesh when resend use GATT bearer
* :github:`17932` - BLE Mesh When Friend Send Seg Message To LPN
* :github:`17936` - Bluetooth: Mesh: The canceled buffer is not free, causing a memory leak
* :github:`17944` - [zephyr 1.14]  LE Enhanced Connection Complete indicates Resolved Public once connected to Public peer address
* :github:`17948` - Bluetooth: privacy: Reconnection issue
* :github:`17971` - [zephyr 1.14] Unable to register GATT service that was unregistered before
* :github:`17977` - BLE Mesh When IV Update Procedure
* :github:`17979` - Security level cannot be elevated after re-connection with privacy
* :github:`18013` - BLE Mesh On Net Buffer free issue
* :github:`18021` - Socket vtable can access null pointer callback function
* :github:`18090` - [zephyr 1.14][MESH/NODE/FRND/FN/BV-08-C] Mesh Friend queues more messages than indicates it's Friend Cache
* :github:`18150` - [zephyr 1.14] Host does not change the RPA
* :github:`18178` - BLE Mesh When Provisioning Use Input OOB Method
* :github:`18183` - [zephyr 1.14][GATT/SR/GAS/BV-07-C] GATT Server does not inform change-unaware client about DB changes
* :github:`18194` - [zephyr 1.14][MESH/NODE/CFG/HBP/BV-05-C] Zephyr does not send Heartbeat message on friendship termination
* :github:`18297` - Bluetooth: SMP: Pairing issues
* :github:`18306` - Unable to reconnect paired devices with controller privacy disabled (host privacy enabled)
* :github:`18308` - net: TCP/IPv6 set of fragmented packets causes Zephyr to quit
* :github:`18394` - [Coverity CID :203464]Memory - corruptions in /subsys/net/l2/ethernet/gptp/gptp_mi.c
* :github:`18462` - potential buffer overrun in logging infrastructure
* :github:`18522` - BLE: Mesh: When transport send seg_msg to LPN
* :github:`18580` - Bluetooth: Security fail on initial pairing
* :github:`18658` - Bluetooth BR/EDR encryption key negotiation vulnerability
* :github:`18739` - k_uptime_get_32() does not behave as documented
* :github:`18813` - fs: nvs: Cannot delete entries
* :github:`18873` - zsock_socket() should support proto==0
* :github:`18935` - [Zephyr 1.14] drivers: flash: spi_nor: Problematic write with page boundaries
* :github:`18961` - [Coverity CID :203912]Error handling issues in /samples/net/sockets/coap_client/src/coap-client.c
* :github:`19015` - Bluetooth: Mesh: Node doesn't respond to "All Proxies" address
* :github:`19165` - zephyr_file generates bad links on branches
* :github:`19181` - sock_set_flag implementation in sock_internal.h does not work for 64 bit pointers
* :github:`19191` - problem with implementation of sock_set_flag

.. _zephyr_1.14.0:

Zephyr Kernel 1.14.0
####################

We are pleased to announce the release of Zephyr kernel version 1.14.0.

Major enhancements with this release include:

* The Zephyr project now supports over 160 different board configurations
  spanning 8 architectures. All architectures are rigorously tested and
  validated using one of the many simulation platforms supported by the
  project: QEMU, Renode, ARC Simulator, and the native POSIX configuration.

* The timing subsystem has been reworked and reimplemented, greatly
  simplifying the resulting drivers, removing thousands of lines
  of code, and reducing a typical kernel build size by hundreds of bytes.
  TICKLESS_KERNEL mode is now the default on all architectures.

* The Symmetric Multi-Processing (SMP) subsystem continues to evolve
  with the addition of a new CPU affinity API that can "pin" threads to
  specific cores or sets of cores. The core kernel no longer uses the
  global irq_lock on SMP systems, and exclusively uses the spinlock API
  (which on uniprocessor systems reduces to the same code).

* Zephyr now has support for the x86_64 architecture. It is currently
  implemented only for QEMU targets, supports arbitrary numbers of CPUs,
  and runs in SMP mode by default, our first platform to do so.

* We've overhauled the Network packet (:ref:`net-pkt <net_pkt_interface>`)
  API and moved the majority of components and protocols to use the
  :ref:`BSD socket API <bsd_sockets_interface>`, including MQTT, CoAP,
  LWM2M, and SNTP.

* We enhanced the native POSIX port by adding UART, USB, and display
  drivers. Based on this port, we added a simulated NRF52832 SoC which enables
  running full system, multi-node simulations, without the need of real
  hardware.

* We added an experimental BLE split software Controller with Upper Link Layer
  and Lower Link Layer for supporting multiple BLE radio hardware
  architectures.

* The power management subsystem has been overhauled to support device idle
  power management and move most of the power management logic from the
  application back to the BSP.

* We introduced major updates and an overhaul to both the logging and
  shell subsystems, supporting multiple back-ends, integration
  of logging into the shell, and delayed log processing.

* Introduced the ``west`` tool for management of multiple repositories and
  enhanced support for flashing and debugging.

* Added support for application user mode, application memory
  partitions, and hardware stack protection in ARMv8m

* Applied MISRA-C code guideline on the kernel and core components of Zephyr.
  MISRA-C is a well established code guideline focused on embedded systems and
  aims to improve code safety, security and portability.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following security vulnerabilities (CVEs) were addressed in this release:

* Tinycrypt HMAC-PRNG implementation doesn't take the HMAC state
  clearing into account as it performs the HMAC operations, thereby using a
  incorrect HMAC key for some of the HMAC operations.
  (CVE-2017-14200)

* The shell DNS command can cause unpredictable results due to misuse of stack
  variables.
  (CVE-2017-14201)

* The shell implementation does not protect against buffer overruns resulting
  in unpredictable behavior.
  (CVE-2017-14202)

* We introduced Kernel Page Table Isolation, a technique for
  mitigating the Meltdown security vulnerability on x86 systems. This
  technique helps isolate user and kernel space memory by ensuring
  non-essential kernel pages are unmapped in the page tables when the CPU
  is running in the least privileged user mode, Ring 3. This is the
  fix for Rogue Data Cache Load. (CVE-2017-5754)

* We also addressed these CVEs for the x86 port:

  - Bounds Check Bypass (CVE-2017-5753)
  - Branch Target Injection (CVE-2017-5715)
  - Speculative Store Bypass (CVE-2018-3639)
  - L1 Terminal Fault (CVE-2018-3620)
  - Lazy FP State Restore (CVE-2018-3665)

Kernel
******

* The timing subsystem has been reworked and mostly replaced:

   - The timer driver API has been extensively reworked, greatly
     simplifying the resulting drivers. By removing thousands of lines
     of code, we reduced the size of a typical kernel build by hundreds
     of bytes.

   - TICKLESS_KERNEL mode is now the default on all architectures.  Many
     bugs were fixed in this support.

* Lots of work on the rapidly-evolving SMP subsystem:

  - There is a new CPU affinity API available to "pin" threads to
    specific cores or sets of cores.

  - The core kernel is now 100% free of use of the global irq_lock on
    SMP systems, and exclusively uses the spinlock API (which on
    uniprocessor systems reduces to the same code).

  - Zephyr now has a simple interprocessor interrupt framework for
    applications, such as the scheduler, to use for synchronously
    notifying other processors of state changes.  It's currently implemented
    only on x86_64 and used only for thread abort.

* Zephyr now has support for the x86_64 architecture.  It is
  currently implemented only for QEMU targets.

  - It supports arbitrary numbers of CPUs in SMP, and runs in SMP mode
    by default, our first platform to do so.

  - It currently runs code built for the "x32" ABI, which is a native
    64-bit hardware state, where pointers are 32 bit in memory.
    Zephyr still has some lurking word size bugs that will need to be
    fixed to turn on native 64 bit code generation.

* K_THREAD_STACK_BUFFER() has been demoted to a private API and will be removed
  in a future Zephyr release.
* A new API sys_mutex has been introduced. It has the same semantics
  as a k_mutex, but the memory for it can reside in user memory and so
  no explicit permission management is required.
* sys_mem_pool() now uses a sys_mutex() for concurrency control.
* Memory protection changes:

  - CONFIG_APPLICATION_MEMORY option has been removed from Zephyr. All test
    cases have been appropriately converted to use memory domains.
  - The build time memory domain partition generation mechanism, formerly
    an optional feature under CONFIG_APP_SHARED_MEM, has been overhauled
    and is now a core part of memory protection.
  - Userspace is no longer enabled by default for tests. Tests that are
    written to execute wholly or in part in user mode will need to enable
    CONFIG_TEST_USERSPACE in the test's project configuration. There are
    assertions in place to enforce that this is done.
  - The default stack size for handling system calls has been increased to
    1024 bytes.

* We started applying MISRA-C (https://www.misra.org.uk/) code guideline on
  the Zephyr kernel. MISRA-C is a well established code guideline focused on
  embedded systems and aims to improve code safety, security, and portability.
  This initial effort was narrowed to the Zephyr kernel and architecture
  code, and focused only on mandatory and required rules. The following rules
  were addressed:

  - Namespace changes
  - Normalize switch() operators
  - Avoid implicit conversion to boolean types
  - Fix and normalize headers guard
  - Make if() evaluate boolean operands
  - Remove all VLAs (variable length array)
  - Avoid undefined and implementation defined behavior with shift operator
  - Remove recursions

Architectures
*************

* Introduced X86_64 (64 bit) architecture support with SMP features
* High-level Kconfig symbol structure for Trusted Execution

* ARM:

  * Re-architect Memory Protection code for ARM and NXP
  * Fully support application user mode, memory partitions, and
    stack protection in ARMv8m
  * Support built-in stack overflow protection in user mode in ARMv8m
  * Fix stack overflow error reporting
  * Support executing from SRAM in XIP builds
  * Support non-cacheable memory sections
  * Remove power-of-two align and size requirement for ARMv8-m
  * Introduce sync barriers in ARM-specific IRQ lock/unlock functions
  * Enforce double-word stack alignment on exception entry
  * API to allow Non-Secure FPU Access (ARMv8-M)
  * Various enhancements in ARM system boot code
  * Indicate Secure domain fault in Non-Secure fault exception
  * Update ARM CMSIS headers to version 5.4.0

* ARC:

  * Userspace and MPU driver improvements
  * Optimization of the thread stack definition macros
  * Bug fixes: handling of lp_xxx registers in _rirq_return_from_coop, nested
    interrupt handling, hardware stack bounds checking, execution benchmarking
  * Atomic operations are now usable from user mode on all ARC CPUs

* x86:

  - Support for non-PAE page tables has been dropped.
  - Fixed various security CVEs related to micro-architecture side-effects of
    speculative execution, as detailed in the security notes.
  - Added robustness when reporting exceptions generated due to stack
    overflows or induced in user mode
  - Pages containing read-only data no longer have the execute disable (XD)
    bit un-set.
  - Fix potential IRQ stack corruption when handling double faults


Boards & SoC Support
********************

* Added the all new :ref:`NRF52 simulated board <nrf52_bsim>`:
  This simulator models some of the hardware in an NRF52832 SOC, to enable
  running full system, multi-node simulations, without the need of real
  hardware.  It enables fast, reproducible testing, development, and debugging
  of an application, BlueTooth (BT) stack, and kernel. It relies on `BabbleSim`_
  to simulate the radio physical layer.

* Added SoC configuration for nRF9160 and Musca ARM Cortex-M33 CPU

* Added support for the following ARM boards:

  * 96b_stm32_sensor_mez
  * b_l072z_lrwan1
  * bl652_dvk
  * bl654_dvk
  * cy8ckit_062_wifi_bt_m0
  * cy8ckit_062_wifi_bt_m4
  * efm32hg_slstk3400a
  * efm32pg_stk3402a
  * efr32mg_sltb004a
  * mimxrt1020_evk
  * mimxrt1060_evk
  * mimxrt1064_evk
  * nrf52832_mdk
  * nrf52840_blip
  * nrf52840_mdk
  * nrf52840_papyr
  * nrf52840_pca10090
  * nrf9160_pca10090
  * nucleo_f302r8
  * nucleo_f746zg
  * nucleo_f756zg
  * nucleo_l496zg
  * nucleo_l4r5zi
  * particle_argon
  * particle_xenon
  * v2m_musca

* Added support for the following RISC-V boards:

  * rv32m1_vega

* Added support for the following ARC boards:
  * Synopsys ARC IoT DevKit
  * Several ARC simulation targets (ARC nSIM EM/SEM; with and without MPU stack guards)

* Added support for the following shield boards:

  * frdm_kw41z
  * x_nucleo_iks01a1
  * x_nucleo_iks01a2

.. _BabbleSim:
   https://BabbleSim.github.io

Drivers and Sensors
*******************

* Added new drivers and backends for :ref:`native_posix <native_posix>`:

  * A UART driver that maps the Zephyr UART to a new host PTY
  * A USB driver that can expose a host connected USB device
  * A display driver that will render to a dedicated window using the SDL
    library
  * A dedicated backend for the new logger subsystem

* Counter

  * Refactored API
  * Ported existing counter and RTC drivers to the new API
  * Deprecated legacy API

* RTC

  - Deprecated the RTC API. The Counter API should be used instead

* UART

  * Added asynchronous API.
  * Added implementation of the new asynchronous API for nRF series (UART and
    UARTE).

* ADC

  * ADC driver APIs are now available to threads running in user mode.
  * Overhauled adc_dw and renamed it to adc_intel_quark_se_c1000_ss
  * Fixed handling of invalid sampling requests

* Display

  * Introduced mcux elcdif shim driver
  * Added support for ssd16xx monochrome controllers
  * Added support for ssd1608, gde029a1, and hink e0154a05
  * Added SDL based display emulation driver
  * Added SSD1673 EPD controller driver
  * Added SSD1306 display controller driver


* Flash:

  * nRF5 flash driver support UICR operations
  * Added driver for STM32F7x series
  * Added flash driver support for Atmel SAM E70
  * Added a generic spi nor flash driver
  * Added flash driver for SiLabs Gecko SoCs

* Ethernet:

  * Extended mcux driver for i.mx rt socs
  * Added driver for Intel PRO/1000 Ethernet controller

* I2C

  * Added mcux lpi2c shim driver
  * Removed deprecated i2c_atmel_sam3 driver
  * Introduced Silabs i2c shim driver
  * Added support for I2S stm32

* Pinmux

  * Added RV32M1 driver
  * Added pinmux driver for Intel S1000
  * Added support for STM32F302x8

* PWM

  * Added SiFive PWM driver
  * Added Atmel SAM PWM driver
  * Converted nRF drivers to use device tree

* Sensor

  * Added lis2ds12, lis2dw12, lis2mdl, and lsm303dlhc drivers
  * Added ms5837 driver
  * Added support for Nordic QDEC
  * Converted drivers to use device tree

* Serial

  * Added RV32M1 driver
  * Added new asynchronous UART API
  * Added support for ARM PL011 UART
  * Introduced Silabs leuart shim serial driver
  * Adapted gecko uart driver for Silabs EFM32HG

* USB

  * Added native_posix USB driver
  * Added usb device driver for Atmel SAM E70 family
  * Added nRF52840 USBD driver


* Other Drivers

  * clock_control: Added RV32M1 driver
  * console: Removed telnet driver
  * entropy: Added Atmel SAM entropy generator driver
  * spi: Converted nRF drivers to use device tree
  * watchdog: Converted drivers to new API
  * wifi: simplelink: Implemented setsockopt() for TLS offload
  * wifi: Added inventek es-WiFi driver
  * timer: Refactored and accuracy improvements of the arcv2 timer driver (boot
    time measurements)
  * timer: Added/reworked Xtensa, RISV-V, NRF, HPET, and ARM systick drivers
  * gpio: Added RV32M1 driver
  * hwinfo: Added new hwinfo API and drivers
  * ipm: Added IMX IPM driver for i.MX socs
  * interrupt_controller: Added RV32M1 driver
  * interrupt_controller: Added support for STM32F302x8 EXTI_LINES
  * neural_net: Added Intel GNA driver
  * can: Added socket CAN support


Networking
**********

* The :ref:`BSD socket API <bsd_sockets_interface>` should be used by
  applications for any network connectivity needs.
* Majority of the network sample applications were converted to use
  the BSD socket API.
* New BSD socket based APIs were created for these components and protocols:

  - :ref:`MQTT <mqtt_socket_interface>`
  - :ref:`CoAP <coap_sock_interface>`
  - :ref:`LWM2M <lwm2m_interface>`
  - :ref:`SNTP <sntp_interface>`
* net-app client and server APIs were removed. This also required removal of
  the following net-app based legacy APIs:

  - MQTT
  - CoAP
  - SNTP
  - LWM2M
  - HTTP client and server
  - Websocket
* Network packet (:ref:`net-pkt <net_pkt_interface>`) API overhaul. The new
  net-pkt API uses less memory and is more streamlined than the old one.
* Implement following BSD socket APIs: ``freeaddrinfo()``, ``gethostname()``,
  ``getnameinfo()``, ``getsockopt()``, ``select()``, ``setsockopt()``,
  ``shutdown()``
* Converted BSD socket code to use global file descriptor numbers.
* Network subsystem converted to use new :ref:`logging system <logging_api>`.
* Added support for disabling IPv4, IPv6, UDP, and TCP simultaneously.
* Added support for :ref:`BSD socket offloading <net_socket_offloading>`.
* Added support for long lifetime IPv6 prefixes.
* Added enhancements to IPv6 multicast address checking.
* Added support for IPv6 Destination Options Header extension.
* Added support for packet socket (AF_PACKET).
* Added support for socket CAN (AF_CAN).
* Added support for SOCKS5 proxy in MQTT client.
* Added support for IPSO Timer object in LWM2M.
* Added support for receiving gratuitous ARP request.
* Added sample application for Google IoT Cloud.
* :ref:`Network interface <net_if_interface>` numbering starts now from 1 for
  POSIX compatibility.
* :ref:`OpenThread <thread_protocol_interface>` enhancements.
* :zephyr:code-sample:`zperf <zperf>` sample application fixes.
* :ref:`LLDP <lldp_interface>` (Link Layer Discovery Protocol) enhancements.
* ARP cache update fix.
* gPTP link delay calculation fixes.
* Changed how network data is passed from
  :ref:`L2 to network device driver <network_stack_architecture>`.
* Removed RPL (Ripple) IPv6 mesh routing support.
* MQTT is now available to threads running in user mode.
* Network device driver additions and enhancements:

  - Added Intel PRO/1000 Ethernet driver (e1000).
  - Added SMSC9118/LAN9118 Ethernet driver (smsc911x).
  - Added Inventek es-WiFi driver for disco_l475_iot1 board.
  - Added support for automatically enabling QEMU based Ethernet drivers.
  - SAM-E70 gmac Ethernet driver Qav fixes.
  - enc28j60 Ethernet driver fixes and enhancements.

Bluetooth
*********

* Host:

  * GATT: Added support for Robust Caching
  * GATT: L2CAP: User driven flow control
  * Many fixes to Mesh
  * Fixed and improved persistent storage handling
  * Fixed direct advertising support
  * Fixed security level 4 handling
  * Add option to configure peripheral connection parameters
  * Added support for updating advertising data without having to restart advertising
  * Added API to iterate through existing bonds
  * Added support for setting channel map
  * Converted SPI HCI driver to use device tree

* New BLE split software Controller (experimental):

  - Split design with Upper Link Layer and Lower Link Layer
  - Enabled with :kconfig:option:`CONFIG_BT_LL_SW_SPLIT` (disabled by default)
  - Support for multiple BLE radio hardware architectures
  - Asynchronous handling of procedures in the ULL
  - Enhanced radio utilization (99% on continuous 100ms scan)
  - Latency resilience: Approx 100uS vs 10uS, 10x improvement
  - CPU and power usage: About 20% improvement
  - Multiple advertiser and scanner instances
  - Support for both Big and Little-Endian architectures

* Controller:

  * Added support for setting the public address
  * Multiple control procedures fixes and improvements
  * Advertising random delay fixes
  * Fixed a serious memory corruption issue during scanning
  * Fixes to RSSI measurement
  * Fixes to Connection Failed to be Established sequence
  * Transitioned to the new logging subsystem from syslog
  * Switched from ``-Ofast`` to ``-O2`` in time-critical sections
  * Reworked the RNG/entropy driver to make it available to apps
  * Multiple size optimizations to make it fit in smaller devices
  * nRF: Rework the PPI channel assignment to use pre-assigned ones
  * Add extensive documentation to the shared primitives

* Several fixes for big-endian architectures

Build and Infrastructure
************************

* Added support for out-of-tree architectures.
* Added support for out-of-tree implementations of in-tree drivers.
* `BabbleSim`_ has been integrated in Zephyr's CI system.
* Introduced ``DT_`` prefix for all labels generated for information extracted
  from device tree (with a few exceptions, such as labels for LEDs and buttons,
  kept for backward compatibility with existing applications).  Deprecated all
  other defines that are generated.
* Introduce CMake variables for DT symbols, just as we have for CONFIG symbols.
* Move DeviceTree processing before Kconfig. Thereby allowing software
  to be configured based on DeviceTree information.
* Automatically change the KCONFIG_ROOT when the application directory
  has a Kconfig file.
* Added :ref:`west <west>` tool for multiple repository management
* Added support for :ref:`Zephyr modules <modules>`
* Build system ``flash`` and ``debug`` targets now require west
* Added generation of DT_<COMPAT>_<INSTANCE>_<PROP> defines which allowed
  sensor or other drivers on buses like I2C or SPI to not require dts fixup.
* Added proper support for device tree boolean properties

Libraries / Subsystems
***********************

* Added a new display API and subsystem
* Added support for CTF Tracing
* Added support for JWT (JSON Web Tokens)
* Flash Maps:

  - API extension
  - Automatic generation of the list of flash areas

* Settings:

  - Enabled logging instead of ASSERTs
  - Always use the storage partition for FCB
  - Fixed FCB backend and common bugs

* Logging:

  - Removed sys_log, which has been replaced by the new logging subsystem
    introduced in v1.13
  - Refactored log modules registration macros
  - Improved synchronous operation (see ``CONFIG_LOG_IMMEDIATE``)
  - Added commands to control the logger using shell
  - Added :c:macro:`LOG_PANIC()` call to the fault handlers to ensure that
    logs are output on fault
  - Added mechanism for handling logging of transient strings. See
    :c:func:`log_strdup`
  - Added support for up to 15 arguments in the log message
  - Added optional function name prefix in the log message
  - Changed logging thread priority to the lowest application priority
  - Added notification about dropped log messages due to insufficient logger
    buffer size
  - Added log backends:

    - RTT
    - native_posix
    - net
    - SWO
    - Xtensa Sim
  - Changed default timestamp source function to :c:func:`k_uptime_get_32`

* Shell:

  - Added new implementation of the shell sub-system. See :ref:`shell_api`
  - Added shell backends:

    - UART
    - RTT
    - telnet

* Ring buffer:

  - Added byte mode
  - Added API to work directly on ring buffer memory to reduce memory copying
  - Removed ``sys_`` prefix from API functions

* MBEDTLS APIs may now be used from user mode.


HALs
****

* Updated Nordic nrfx to version 1.6.2
* Updated Nordic nrf ieee802154 radio driver to version 1.2.3
* Updated SimpleLink to TI CC32XX SDK 2.40.01.01
* Added Microchip MEC1701 Support
* Added Cypress PDL for PSoC6 SoC Support
* Updates to stm32cube, Silabs Gecko SDK, Atmel.
* Update ARM CMSIS headers to version 5.4.0


Documentation
*************

* Reorganized subsystem documentation into more meaningful collections
  and added or improved introductory material for each subsystem.
* Overhauled  Bluetooth documentation to split it into
  manageable units and included additional information, such as
  architecture and tooling.
* Added to and improved documentation on many subsystems and APIs
  including socket offloading, Ethernet management, LLDP networking,
  network architecture and overview, net shell, CoAP, network interface,
  network configuration library, DNS resolver, DHCPv4, DTS, flash_area,
  flash_mpa, NVS, settings, and more.
* Introduced a new debugging guide (see :ref:`debug-probes`) that documents
  the supported debug probes and host tools in
  one place, including which combinations are valid.
* Clarified and improved information about the west tool and its use.
* Improved :ref:`development process <development_model>` documentation
  including how new features
  are proposed and tracked, and clarifying API lifecycle, issue and PR
  tagging requirements, contributing guidelines, doc guidelines,
  release process, and PR review process.
* Introduced a developer "fast" doc build option to eliminate
  the time needed to create the full kconfig option docs from a local
  doc build, saving potentially five minutes for a full doc build. (Doc
  building time depends on your development hardware performance.)
* Made dramatic improvements to the doc build processing, bringing
  iterative local doc generation down from over two minutes to only a
  few seconds. This makes it much faster for doc developers to iteratively
  edit and test doc changes locally before submitting a PR.
* Added a new ``zephyr-file`` directive to link directly to files in the
  Git tree.
* Introduced simplified linking to doxygen-generated API reference
  material.
* Made board documentation consistent, enabling a board-image carousel
  on the zephyrproject.org home page.
* Reduced unnecessarily large images to improve page load times.
* Added CSS changes to improve API docs appearance and usability
* Made doc version selector more obvious, making it easier to select
  documentation for a specific release
* Added a friendlier and more graphic home page.

Tests and Samples
*****************

* A new set of, multinode, full system tests of the BT stack,
  based on `BabbleSim`_ have been added.
* Added unique identifiers to all tests and samples.
* Removed old footprint benchmarks
* Added tests for CMSIS RTOS API v2, BSD Sockets, CANBus, Settings, USB,
  and miscellaneous drivers.
* Added benchmark applications for the scheduler and mbedTLS
* Added samples for the display subsystem, LVGL, Google IOT, Sockets, CMSIS RTOS
  API v2, Wifi, Shields, IPC subsystem, USB CDC ACM, and USB HID.
* Add support for using sanitycheck testing with Renode


Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.13.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`15407` - [Coverity CID :197597]Incorrect expression in /tests/kernel/static_idt/src/main.c
* :github:`15406` - [Coverity CID :197598]Incorrect expression in /tests/drivers/uart/uart_async_api/src/test_uart_async.c
* :github:`15405` - [Coverity CID :197599]Incorrect expression in /tests/kernel/fatal/src/main.c
* :github:`15404` - [Coverity CID :197600]Incorrect expression in /tests/lib/c_lib/src/main.c
* :github:`15403` - [Coverity CID :197601]Incorrect expression in /tests/kernel/common/src/intmath.c
* :github:`15402` - [Coverity CID :197602]Incorrect expression in /tests/kernel/common/src/intmath.c
* :github:`15401` - [Coverity CID :197603]Incorrect expression in /tests/kernel/fatal/src/main.c
* :github:`15400` - [Coverity CID :197604]Memory - corruptions in /tests/kernel/mem_protect/userspace/src/main.c
* :github:`15399` - [Coverity CID :197605]Null pointer dereferences in /subsys/testsuite/ztest/src/ztest_mock.c
* :github:`15398` - [Coverity CID :197606]Incorrect expression in /tests/kernel/common/src/irq_offload.c
* :github:`15397` - [Coverity CID :197607]Incorrect expression in /tests/drivers/uart/uart_async_api/src/test_uart_async.c
* :github:`15396` - [Coverity CID :197608]Incorrect expression in /tests/lib/c_lib/src/main.c
* :github:`15395` - [Coverity CID :197609]Incorrect expression in /tests/kernel/interrupt/src/nested_irq.c
* :github:`15394` - [Coverity CID :197610]Incorrect expression in /tests/kernel/fatal/src/main.c
* :github:`15393` - [Coverity CID :197611]Integer handling issues in /lib/os/printk.c
* :github:`15392` - [Coverity CID :197612]Integer handling issues in /lib/os/printk.c
* :github:`15390` - [Coverity CID :197614]Incorrect expression in /tests/lib/c_lib/src/main.c
* :github:`15389` - [Coverity CID :197615]Incorrect expression in /tests/kernel/fatal/src/main.c
* :github:`15388` - [Coverity CID :197616]Null pointer dereferences in /subsys/testsuite/ztest/src/ztest_mock.c
* :github:`15387` - [Coverity CID :197617]Incorrect expression in /tests/kernel/common/src/multilib.c
* :github:`15386` - [Coverity CID :197618]Error handling issues in /subsys/shell/shell_telnet.c
* :github:`15385` - [Coverity CID :197619]Incorrect expression in /tests/kernel/mem_pool/mem_pool/src/main.c
* :github:`15384` - [Coverity CID :197620]Incorrect expression in /tests/kernel/static_idt/src/main.c
* :github:`15383` - [Coverity CID :197621]Incorrect expression in /tests/kernel/static_idt/src/main.c
* :github:`15382` - [Coverity CID :197622]Incorrect expression in /tests/kernel/tickless/tickless_concept/src/main.c
* :github:`15381` - [Coverity CID :197623]Incorrect expression in /tests/kernel/interrupt/src/nested_irq.c
* :github:`15380` - USAGE FAULT on tests/crypto/rand32/ on sam_e70_xplained
* :github:`15379` - foundries.io CI: tests/kernel/mem_protect/stackprot fails on nrf52
* :github:`15370` - log_strdup() leaks memory if log message is filtered
* :github:`15365` - Bluetooth qualification test MESH/SR/HM/CFS/BV-02-C is failing
* :github:`15361` - nRF timer: investigate race condition when setting clock timeout in TICKLESS mode
* :github:`15348` - ARM Cortex-M: SysTick: unhandled race condition
* :github:`15346` - VLAN support is broken
* :github:`15336` - Unable to transmit data using interrupt driven API with nrf UARTE peripheral
* :github:`15333` - hci_uart controller driver loses sync after host driver is reset
* :github:`15329` - Bluetooth: GATT Client Configuration is not cleared when device is unpaired
* :github:`15325` - conn->le.keys pointer is not cleared even after the keys struct is invalidated after unpair
* :github:`15324` - Error undefined reference to '__aeabi_uldivmod' when build Zephyr for nrf52_pca10040 board
* :github:`15309` - ARM Cortex-M SysTick Load value setting off-by-one
* :github:`15303` - net: Stackoverflow in net mgmt thread
* :github:`15300` - Bluetooth: Mesh: bt_mesh_fault_update() doesn't update publication message
* :github:`15299` - west init fails in powershell
* :github:`15289` - Zephyr module uses '\' in path on windows when creating Kconfig files
* :github:`15285` - arc: it's not reliable to use exc_nest_count to check nest interrupt
* :github:`15280` - tests/kernel/mem_protect/stackprot fails on platform qemu_riscv32
* :github:`15266` - doc: Contribution guidelines still link to IRC
* :github:`15260` - Shell doesn't always process input data when it arrives
* :github:`15259` - CAN sample does not work
* :github:`15251` - nRF Watchdog not triggering on kernel panic
* :github:`15246` - doc: confusion about dtc version
* :github:`15240` - esp32 build broken
* :github:`15236` - add external spi-nor flash will build fail
* :github:`15235` - Missing license references in DTS files
* :github:`15234` - Missing SPDX license references in drivers source files.
* :github:`15228` - tests: getnameinfo runs with user mode disabled
* :github:`15227` - sockets: no syscall for gethostname()
* :github:`15221` - ARC: incorrect value checked for MPU violation
* :github:`15216` - k_sleep() expires sooner than expected on STM32F4 (Cortex M4)
* :github:`15213` - cmake infrastructure in code missing file level license identifiers
* :github:`15206` - sanitycheck --coverage: stack overflows on qemu_x86, mps2_an385 and qemu_x86_nommu
* :github:`15205` - hci_usb not working on v1.14.0rc3 with SDK 0.10.0
* :github:`15204` - lwm2m engine hangs on native_posix
* :github:`15198` - tests/booting: Considering remove it
* :github:`15197` - Socket-based DNS API will hang device if DNS query is not answered
* :github:`15184` - Fix build issue with z_sys_trace_thread_switched_in
* :github:`15183` - BLE HID sample often asserts on Windows 10 reconnection
* :github:`15178` - samples/mpu/mem_domain_apis_test:  Did not get to "destroy app0 domain", went into indefinite loop
* :github:`15177` - samples/drivers/crypto:  CBC and CTR mode not supported
* :github:`15170` - undefined symbol TINYCBOR during doc build
* :github:`15169` - [Coverity CID :197534]Memory - corruptions in /subsys/logging/log_backend_rtt.c
* :github:`15168` - [Coverity CID :197535]Incorrect expression in /tests/drivers/uart/uart_async_api/src/test_uart_async.c
* :github:`15167` - [Coverity CID :197536]Parse warnings in /include/mgmt/buf.h
* :github:`15166` - [Coverity CID :197537]Control flow issues in /subsys/power/power.c
* :github:`15163` - nsim_*_mpu_stack_guard fails if CONFIG_USERSPACE=n but CONFIG_HW_STACK_PROTECTION=y
* :github:`15161` - stack overflow in tests/posix/common on nsim_em_mpu_stack_guard
* :github:`15157` - mps2_an385 and GNU Arm Embedded gcc-arm-none-eabi-7-2018-q2-update failed tests
* :github:`15154` - mempool can result in OOM while memory is available
* :github:`15153` - Some empty qemu_x86 output when running code coverage using sanity_check
* :github:`15152` - tests/kernel/pipe/pipe: "Kernel Oops" and "CPU Page Fault" when running coverage for qemu_x86
* :github:`15151` - tests/tickless/tickless_concept: Assertions when running code coverage on qemu_x86
* :github:`15150` - tests/kernel/threads/thread_api: "Double faults" when running code coverage in qemu_x86
* :github:`15149` - mps2_an385: fatal lockup when running code coverage
* :github:`15148` - tests/kernel/mem_pool/mem_pool_concept/: Assertion failures for mpns2_an385
* :github:`15146` - mps2_an385: Multiple "MPU Fault"s,  "Hardware Fault"s "Stack Check Fail!" and "Bus Fault" when running code coverage
* :github:`15145` - USB HF clock stop fail
* :github:`15131` - ARC: off-by-one in MPU V2 _is_in_region()
* :github:`15130` - ARC: Z_ARCH_THREAD_STACK_MEMBER defined incorrectly
* :github:`15129` - ARC: tests/kernel/critical times out on nsim_sem
* :github:`15126` - multiple intermittent test failure on ARC
* :github:`15124` - DNS not working with NET_OFFLOAD
* :github:`15109` - ATSAME70 MCU(SAM E70 Xplained) RAM random after a watchdog reset.
* :github:`15107` - samples/application_development/code_relocation fails to build with coverage on mps2_an385
* :github:`15103` - nrf52810_pca10040 SRAM space not enough
* :github:`15100` - Bluetooth: GATT (un)subscribe can silently fail
* :github:`15099` - Bluetooth: GATT Subscribe does not detect duplicate if new parameters are used.
* :github:`15096` - Cannot build samples/net/ipv4_autoconf
* :github:`15093` - zephyr_library_compile_options() lost support for duplicates
* :github:`15090` - FIFO: Clarify doc for k_fifo_alloc_put
* :github:`15085` - Sanitycheck when running on devices is not counting samples in the final report
* :github:`15083` - MCUBoot is linked to slot0 because overlay is dropped in boilerplate.cmake
* :github:`15077` - Cannot boot application flashed to nrf52840_pca10059
* :github:`15073` - Device crashes when starting with USB connected
* :github:`15070` - ieee802154: Configuration for CC2520 is not working
* :github:`15069` - arch: arm: thread arch.mode not always inline with thread's privilege mode (e.g. system calls)
* :github:`15067` - bluetooth: bt_set_name rejects names of size CONFIG_BT_DEVICE_NAME_MAX
* :github:`15064` - tests/kernel/fp_sharing: undefined reference k_float_disable()
* :github:`15063` - tests/subsys/settings/fcb/src/settings_test_save_unaligned.c fail with assertion failure on nrf52_pca10040
* :github:`15061` - Builds on Windows are broken due to invalid zephyr_modules.txt parsing
* :github:`15059` - Fix builds w/o modules
* :github:`15056` - arch: arm: arch.mode variable _not_ initialized to nPRIV in user space enter
* :github:`15050` - Using TCP in zperf causes free memory access
* :github:`15044` - ARC: test failure in tests/kernel/threads/thread_apis
* :github:`15039` - ADC drivers adc_read_async() keep pointers to sequence
* :github:`15037` - xtensa: context returns to thread after kernel oops
* :github:`15035` - build breakage on two ARC targets: missing arc_exc_saved_sp
* :github:`15031` - net: 9cd547f53b "Fix ref counting for the net_pkt" allegedly broke reference counting
* :github:`15019` - tests/kernel/common: test_bitfield: test_bitfield: (b1 not equal to (1 << bit))
* :github:`15018` - tests/kernel/threads/no-multithreading: Not booting
* :github:`15017` - Not able to set "0xFFFF No specific value" for GAP PPCP structured data
* :github:`15013` - tests/kernel/fatal: check_stack_overflow: (rv equal to TC_FAIL)
* :github:`15012` - Unable to establish security after reconnect to dongle
* :github:`15009` - sanitycheck --coverage on qemu_x86:  cannot move location counter backwards
* :github:`15008` - SWO logger backend produces no output in 'in place' mode
* :github:`14992` - West documentation is largely missing
* :github:`14989` - Doc build does not include the zephyr modules Kconfig files
* :github:`14988` - USB device not recognized on PCA10056 preview-DK
* :github:`14985` - Clarify in release docs NOT to use github tagging.
* :github:`14974` - Kconfig.modules needs to be at the top level build folder
* :github:`14958` - [Coverity CID :197457]Control flow issues in /subsys/bluetooth/host/gatt.c
* :github:`14957` - [Coverity CID :197458]Insecure data handling in /subsys/usb/usb_device.c
* :github:`14956` - [Coverity CID :197459]Memory - corruptions in /subsys/bluetooth/shell/gatt.c
* :github:`14955` - [Coverity CID :197460]Integer handling issues in /samples/bluetooth/ipsp/src/main.c
* :github:`14954` - [Coverity CID :197461]Insecure data handling in /subsys/usb/usb_device.c
* :github:`14953` - [Coverity CID :197462]Memory - corruptions in /subsys/bluetooth/host/gatt.c
* :github:`14952` - [Coverity CID :197463]Memory - corruptions in /samples/bluetooth/central_hr/src/main.c
* :github:`14951` - [Coverity CID :197464]Memory - corruptions in /subsys/bluetooth/host/gatt.c
* :github:`14950` - [Coverity CID :197465]Integer handling issues in /samples/bluetooth/ipsp/src/main.c
* :github:`14947` - no user mode access to MQTT subsystem
* :github:`14946` - cdc_acm example doesn't work on nrf52840_pca10059
* :github:`14945` - nrf52840_pca10059 executables do not work without mcuboot
* :github:`14943` - config BOARD_HAS_NRF5_BOOTLOADER not honored for nrf52840_pca10059
* :github:`14942` - tests/posix/fs don't build on em_starterkit_em11d
* :github:`14934` - tinycbor is failing in nightly CI
* :github:`14928` - Bluetooth: Mesh: Provisioning state doesn't always get properly re-initialized when doing reset
* :github:`14903` - tests/posix/fs test show messages dropped in the logs
* :github:`14902` - logger: Enabling USB CDC ACM disables logging
* :github:`14899` - Bluetooth controller ACL data packets stall
* :github:`14882` - USB DFU never enters DFU mode, when composite device is enabled and mcuboot is used
* :github:`14871` - tests/posix/fs : Dropped console output
* :github:`14870` - samples/mpu/mpu_stack_guard_test: Found "Test not passed"
* :github:`14869` - tests/lib/ringbuffer: Assertion failure at test_ring_buffer_main()
* :github:`14840` - settings: settings_save_one() doesn't always seem to store data, even if it returns success
* :github:`14837` - Bluetooth shell scan command parameter mandatory/optional evaluation is broken
* :github:`14833` - Bluetooth init procedure with BT_SETTINGS is not reliable
* :github:`14827` - cmake error
* :github:`14821` - [Coverity CID :196635]Error handling issues in /tests/net/mld/src/main.c
* :github:`14820` - [Coverity CID :196636]Integer handling issues in /kernel/sched.c
* :github:`14819` - [Coverity CID :196637]Uninitialized variables in /samples/net/sockets/can/src/main.c
* :github:`14818` - [Coverity CID :196638]Null pointer dereferences in /subsys/bluetooth/host/hci_core.c
* :github:`14817` - [Coverity CID :196639]Error handling issues in /samples/bluetooth/ipsp/src/main.c
* :github:`14816` - [Coverity CID :196640]Integer handling issues in /arch/x86/core/thread.c
* :github:`14815` - [Coverity CID :196641]Null pointer dereferences in /samples/net/nats/src/nats.c
* :github:`14814` - [Coverity CID :196642]Error handling issues in /subsys/shell/shell_uart.c
* :github:`14813` - [Coverity CID :196643]Null pointer dereferences in /subsys/net/ip/net_context.c
* :github:`14807` - disable SPIN_VALIDATE when SMP enabled
* :github:`14789` - doc: flash_map and flash_area
* :github:`14786` - Information about old sdk version provides wrong download link
* :github:`14782` - Build process produces hex files which will not install on BBC micro:bit
* :github:`14780` - USB: netusb: Unable to send maximum Ethernet packet
* :github:`14779` - stm32: If the memory usage is high, the flash is abnormal.
* :github:`14770` - samples/net/promiscuous_mode the include file is not there
* :github:`14767` - ARC: hang in exception handling when CONFIG_LOG is enabled
* :github:`14766` - K_THREAD_STACK_BUFFER() is broken
* :github:`14763` - PCI debug logging cannot work with PCI-enabled NS16550
* :github:`14762` - elf_helper: Call to undefined debug_die() in AggregateTypeMember (wrong class)
* :github:`14753` - nrf52840_pca10056: Get rid of leading spurious 0x00 byte in UART output
* :github:`14743` - Directed advertising to Android does not work
* :github:`14741` - Bluetooth scanning frequent resetting
* :github:`14714` - Mesh network traffic overflow ungraceful stop. (MMFAR Address: 0x0)
* :github:`14698` - USB: usb/console sample does not work for most of the boards
* :github:`14697` - USB: cdc_acm_composite sample might lose characters
* :github:`14693` - ARC: need test coverage for MPU stack guards
* :github:`14691` - samples: telnet: net shell is not working
* :github:`14684` - samples/net/promiscuous _mode : Cannot set promiscuous mode for interface
* :github:`14665` - samples/net/zperf does not work for TCP in qemu_x86
* :github:`14663` - net: echo server sends unknown packets on start
* :github:`14661` - samples/net/syslog_net fails for native_posix
* :github:`14658` - Disabling CONFIG_BT_PHY_UPDATE makes connections stall with iOS
* :github:`14657` - Sample: echo_async: setsockopt fail
* :github:`14654` - Samples: echo_client: No reply packet from the server
* :github:`14647` - IP: Zephyr replies to broadcast ethernet packets in other subnets on the same wire
* :github:`14643` - ARC: tests/kernel/mem_protect/mem_protect/kernel.memory_protection fails on nsim_sem
* :github:`14642` - ARC: tests/posix/common/ and tests/kernel/critical time out on nsim_sem with userspace enabled
* :github:`14641` - ARC: tests/kernel/critical/kernel.common times out on nsim_em and nsim_sem
* :github:`14640` - ARC: tests/cmsis_rtos_v2/portability.cmsis_rtos_v2 fails on nsim_em and nsim_sem
* :github:`14635` - bluetooth: controller: Control procedure collision with Encryption and PHY update procedure
* :github:`14627` - USB HID device only detected after replugging
* :github:`14623` - sanitycheck error when trying to run specific test
* :github:`14622` - net: IPv6: malformed packet in fragmented echo reply
* :github:`14612` - samples/net/sockets/echo_async_select doesn’t work for qemu_x86 target
* :github:`14609` - mimxrt1050_evk Fatal fault in thread tests/kernel/mem_protect/stackprot Fatal fault in thread
* :github:`14608` - Promiscuous mode net sample cannot be build
* :github:`14606` - mimxrt1050_evk tests/kernel/fp_sharing kernel.fp_sharing fails
* :github:`14605` - mimxrt1060_evk cpp_synchronization meets Hardware exception
* :github:`14603` - pyocd can't support more board_runner_args
* :github:`14586` - Sanitycheck shows "FAILED: failed" for successful test: tests/kernel/fifo/fifo_api/kernel.fifo
* :github:`14568` - I2C stm32 LL driver V2 will hang when trying again after an error occurs
* :github:`14566` - mcuboot doesn't link into code-partition
* :github:`14556` - tests/benchmarks/timing_info reports strange values on quark_se_c1000:x86, altera_max10:nios2
* :github:`14554` - UP2 console no output after commit fb4f5e727b.
* :github:`14546` - shell compilation error when disabling CONFIG_SHELL_ECHO_STATUS
* :github:`14542` - STM32F4XX dts_fixup.h error
* :github:`14540` - kernel: message queue MACRO not compatible with C++
* :github:`14536` - out of bounds access in log_backend_rtt
* :github:`14523` - echo-client doesn't close socket if echo-server is offline
* :github:`14510` - USB DFU sample doc outdated
* :github:`14508` - mempool allocator can return with no allocation even if memory is available
* :github:`14504` - mempool can return success if no memory was available
* :github:`14501` - crash in qemu_x86_64:tests/kernel/fifo/fifo_usage/kernel.fifo.usage
* :github:`14500` - sanitycheck --coverage: stack overflows on qemu_x86 and mps2_an385
* :github:`14499` - sanitycheck --coverage on qemu_x86: stack overflows on qemu_x86 and mps2_an385
* :github:`14496` - PyYAML 5.1 breaks DTS parsing
* :github:`14492` - doc: update robots.txt to exclude more old docs
* :github:`14479` - Regression for net_offload API in net_if.c?
* :github:`14477` - tests/crypto/tinycrypt: test_ecc_dh() 's montecarlo_ecdh() hangs when num_tests (1st parameter) is greater than 1
* :github:`14476` - quark_d2000_crb: samples/sensor/bmg160 runs out of ROM (CI failure)
* :github:`14471` - MPU fault during application startup
* :github:`14469` - sanitycheck failures on 96b_carbon due to commit 75164763868ebd604904af3fdbc86845da833abc
* :github:`14462` - tests/kernel/threads/no-multithreading/testcase.yam: Not Booting
* :github:`14460` - python requirements.txt: pyocd and pyyaml conflict
* :github:`14454` - tests/kernel/threads/no-multithreading/:  Single/Repeated delay boot banner
* :github:`14447` - Rename macro functions starting with two or three underscores
* :github:`14422` - [Coverity CID :195758]Uninitialized variables in /drivers/usb/device/usb_dc_nrfx.c
* :github:`14421` - [Coverity CID :195760]Error handling issues in /tests/net/tcp/src/main.c
* :github:`14420` - [Coverity CID :195768]API usage errors in /arch/x86_64/core/xuk-stub32.c
* :github:`14419` - [Coverity CID :195770]Memory - illegal accesses in /drivers/ethernet/eth_native_posix_adapt.c
* :github:`14418` - [Coverity CID :195774]API usage errors in /arch/x86_64/core/xuk-stub32.c
* :github:`14417` - [Coverity CID :195786]Error handling issues in /samples/drivers/CAN/src/main.c
* :github:`14416` - [Coverity CID :195789]Uninitialized variables in /subsys/usb/class/netusb/function_rndis.c
* :github:`14415` - [Coverity CID :195793]Insecure data handling in /drivers/counter/counter_ll_stm32_rtc.c
* :github:`14414` - [Coverity CID :195800]Memory - corruptions in /tests/net/traffic_class/src/main.c
* :github:`14413` - [Coverity CID :195816]Null pointer dereferences in /tests/net/dhcpv4/src/main.c
* :github:`14412` - [Coverity CID :195819]Null pointer dereferences in /tests/net/tcp/src/main.c
* :github:`14411` - [Coverity CID :195821]Memory - corruptions in /tests/net/traffic_class/src/main.c
* :github:`14410` - [Coverity CID :195828]Memory - corruptions in /boards/posix/native_posix/cmdline.c
* :github:`14409` - [Coverity CID :195835]Null pointer dereferences in /tests/net/ipv6/src/main.c
* :github:`14408` - [Coverity CID :195838]Memory - illegal accesses in /samples/subsys/usb/hid-cdc/src/main.c
* :github:`14407` - [Coverity CID :195839]Memory - corruptions in /tests/net/traffic_class/src/main.c
* :github:`14406` - [Coverity CID :195841]Insecure data handling in /drivers/usb/device/usb_dc_native_posix.c
* :github:`14405` - [Coverity CID :195844]Null pointer dereferences in /tests/net/mld/src/main.c
* :github:`14404` - [Coverity CID :195845]Memory - corruptions in /tests/net/traffic_class/src/main.c
* :github:`14403` - [Coverity CID :195847]Memory - corruptions in /tests/net/traffic_class/src/main.c
* :github:`14402` - [Coverity CID :195848]Error handling issues in /samples/net/sockets/echo_async_select/src/socket_echo_select.c
* :github:`14401` - [Coverity CID :195855]Memory - corruptions in /drivers/serial/uart_native_posix.c
* :github:`14400` - [Coverity CID :195858]Incorrect expression in /arch/posix/core/posix_core.c
* :github:`14399` - [Coverity CID :195860]Null pointer dereferences in /tests/net/tcp/src/main.c
* :github:`14398` - [Coverity CID :195867]Memory - corruptions in /arch/posix/core/posix_core.c
* :github:`14397` - [Coverity CID :195871]Integer handling issues in /drivers/counter/counter_ll_stm32_rtc.c
* :github:`14396` - [Coverity CID :195872]Error handling issues in /drivers/serial/uart_native_posix.c
* :github:`14395` - [Coverity CID :195880]Null pointer dereferences in /tests/net/dhcpv4/src/main.c
* :github:`14394` - [Coverity CID :195884]Control flow issues in /arch/x86_64/core/xuk.c
* :github:`14393` - [Coverity CID :195896]Memory - corruptions in /tests/net/traffic_class/src/main.c
* :github:`14392` - [Coverity CID :195897]Error handling issues in /samples/net/sockets/echo_async/src/socket_echo.c
* :github:`14391` - [Coverity CID :195900]Security best practices violations in /drivers/entropy/fake_entropy_native_posix.c
* :github:`14390` - [Coverity CID :195903]Null pointer dereferences in /tests/net/iface/src/main.c
* :github:`14389` - [Coverity CID :195905]Control flow issues in /arch/x86_64/core/x86_64.c
* :github:`14388` - [Coverity CID :195921]Null pointer dereferences in /tests/net/tcp/src/main.c
* :github:`14315` - iamcu has build issues due to lfence
* :github:`14313` - doc: API references such as :c:func:`funcname` aren't creating links
* :github:`14310` - 64 bit print format specifiers not defined with newlib and SDK 0.10.0
* :github:`14297` - mimxrt1020_evk tests/kernel/gen_isr_table test failure
* :github:`14293` - mimxrt1060_evk tests/benchmarks/latency_measure failed
* :github:`14289` - Cannot build GRUB2 boot loader image in Clear Linux
* :github:`14275` - [ci.foundries.io] regression 4344e27 all: Update reserved function names
* :github:`14265` - Bluetooth GATT descriptor discovery returns all attributes
* :github:`14261` - DTS file for the esp32
* :github:`14258` - doc: Recommended SDK version is out of date
* :github:`14247` - tests/net/ieee802154/crypto fails with assertion failure in subsys/net/ip/net_if.c
* :github:`14246` - ./sample/bluetooth/mesh/ always issue an "HARD FALUT"
* :github:`14244` - tests/crypto/rand32/testcase.yaml#crypto.rand32.random_hw_xoroshiro.rand32: Not Booting
* :github:`14235` - Bluetooth connection timeout
* :github:`14209` - unable to flash sam_e70_xplained due to west errors
* :github:`14191` - Logger corrupts itself on rescheduling
* :github:`14186` -  tests/cmsis_rtos_v1 fails on nrf boards
* :github:`14184` - tests/benchmarks: Stuck at  delaying boot banner on quark_se_c1000_ss_board
* :github:`14177` - Spurious Error: "zephyr-no-west/samples/hello_world" is not in a west installation
* :github:`14160` - Bluetooth API documentation - bt_conn_create_slave_le
* :github:`14156` - Mac OSX Documentation Update Needed
* :github:`14141` - USB suspend/resume on board startup
* :github:`14139` - nsim failed in tests/subsys/jwt/libraries_encoding
* :github:`14127` - netusb: TX path doesn't work in RNDIS driver
* :github:`14125` - system calls are vulnerable to Spectre V1 attacks on CPUs with speculative execution
* :github:`14121` - gaps between app shared memory partitions can waste a lot of space
* :github:`14109` - Incorrect documentation for k_work_*() API
* :github:`14105` - Race condition in k_delayed_work_submit_to_queue()
* :github:`14104` - Invalid locking in k_delayed_work_submit_to_queue()
* :github:`14099` - Minnowboard doesn't build tests/kernel/xip/
* :github:`14098` - Test Framework documentation issue
* :github:`14096` - Timeslicing is broken
* :github:`14093` - net: Description of net_pkt_skip() is not clear
* :github:`14087` - serial/stm32: uart_stm32_fifo_fill can't transmit data complete
* :github:`14084` - ADC driver subsystem has no system calls
* :github:`14063` - net: ipv6: Neighbor table management improvements
* :github:`14059` - CONFIG_XUK_64_BIT_ABI is referenced but undefined (outside of tests)
* :github:`14044` - BLE HID sample fails to reconnect on Windows 10 tablets - Wrong Sequence Number (follow-up)
* :github:`14042` - MCUboot fails to boot STM32L4 device
* :github:`14010` - logger: timestamp resets after 35.7 seconds on K64F
* :github:`14001` - drivers: modem: modem receiver is sending extra bytes around \r\n
* :github:`13984` - nucleo_l496zg: samples: console/echo: It doesn't echo
* :github:`13972` - bt_le_scan_stop()  before finding device results in Data Access Violation
* :github:`13964` - rv32m1_vega_ri5cy doesn't build w/o warnings
* :github:`13960` - tests/kernel/lifo/lifo_usage fails on m2gl025_miv
* :github:`13956` - CI scripting doesn't retry modifications to tests on non-default platforms
* :github:`13949` - tests: Ztest problem - not booting properly
* :github:`13943` - net: QEMU Ethernet drivers are flaky
* :github:`13937` - tests/net/tcp: Page fault
* :github:`13934` - tests/kernel/fatal: test_fatal rv equal to TC_FAIL
* :github:`13923` - app shared memory placeholders waste memory
* :github:`13919` - tests/crypto/mbedtls reports some errors without failing
* :github:`13918` - x86 memory domain configuration not always applied correctly on context switch when partitions are added
* :github:`13906` - posix: Recently enabled POSIX+newlib tests fail to build with gnuarmemb
* :github:`13890` - stm32: serial: Data is not read properly at a certain baud rate
* :github:`13889` - ARM: Userspace: should we have default system app partitions?
* :github:`13888` - [Coverity CID :190924]Integer handling issues in /subsys/net/lib/sntp/sntp.c
* :github:`13887` - [Coverity CID :190925]Memory - corruptions in /subsys/bluetooth/controller/hci/hci_driver.c
* :github:`13886` - [Coverity CID :190926]Error handling issues in /drivers/can/stm32_can.c
* :github:`13885` - [Coverity CID :190928]Error handling issues in /samples/net/sockets/echo_async/src/socket_echo.c
* :github:`13884` - [Coverity CID :190929]Integer handling issues in /tests/drivers/hwinfo/api/src/main.c
* :github:`13883` - [Coverity CID :190930]Integer handling issues in /samples/subsys/fs/src/main.c
* :github:`13882` - [Coverity CID :190931]Control flow issues in /subsys/net/lib/lwm2m/lwm2m_rw_json.c
* :github:`13881` - [Coverity CID :190932]Control flow issues in /samples/subsys/ipc/openamp/src/main.c
* :github:`13880` - [Coverity CID :190933]Control flow issues in /drivers/gpio/gpio_intel_apl.c
* :github:`13879` - [Coverity CID :190934]Parse warnings in /tests/drivers/can/stm32/src/main.c
* :github:`13878` - [Coverity CID :190935]Parse warnings in /tests/drivers/can/stm32/src/main.c
* :github:`13877` - [Coverity CID :190936]Uninitialized variables in /tests/subsys/fs/nffs_fs_api/common/test_performance.c
* :github:`13876` - [Coverity CID :190937]Incorrect expression in /tests/drivers/counter/counter_basic_api/src/test_counter.c
* :github:`13875` - [Coverity CID :190938]Parse warnings in /tests/drivers/can/stm32/src/main.c
* :github:`13874` - [Coverity CID :190939]Error handling issues in /tests/subsys/fs/fat_fs_dual_drive/src/test_fat_file.c
* :github:`13873` - [Coverity CID :190940]Memory - corruptions in /soc/arm/microchip_mec/mec1701/soc.c
* :github:`13872` - [Coverity CID :190942]Memory - corruptions in /subsys/mgmt/smp_shell.c
* :github:`13871` - [Coverity CID :190943]Incorrect expression in /tests/kernel/fatal/src/main.c
* :github:`13870` - [Coverity CID :190944]Control flow issues in /subsys/usb/class/usb_dfu.c
* :github:`13869` - [Coverity CID :190945]Parse warnings in /tests/drivers/can/api/src/main.c
* :github:`13868` - [Coverity CID :190946]Null pointer dereferences in /tests/net/utils/src/main.c
* :github:`13867` - [Coverity CID :190948]Null pointer dereferences in /subsys/net/lib/lwm2m/lwm2m_rw_json.c
* :github:`13866` - [Coverity CID :190949]Error handling issues in /tests/subsys/fs/nffs_fs_api/common/test_append.c
* :github:`13865` - [Coverity CID :190950]Integer handling issues in /arch/arm/core/cortex_m/mpu/nxp_mpu.c
* :github:`13864` - [Coverity CID :190951]Control flow issues in /subsys/net/ip/net_context.c
* :github:`13863` - [Coverity CID :190952]Incorrect expression in /tests/drivers/counter/counter_basic_api/src/test_counter.c
* :github:`13862` - [Coverity CID :190953]Error handling issues in /subsys/fs/shell.c
* :github:`13861` - [Coverity CID :190954]Error handling issues in /subsys/bluetooth/controller/ll_sw/nordic/lll/lll_test.c
* :github:`13860` - [Coverity CID :190955]Error handling issues in /tests/subsys/fs/nffs_fs_api/common/nffs_test_utils.c
* :github:`13859` - [Coverity CID :190956]Error handling issues in /samples/net/sockets/can/src/main.c
* :github:`13858` - [Coverity CID :190957]Incorrect expression in /tests/kernel/fatal/src/main.c
* :github:`13857` - [Coverity CID :190958]Control flow issues in /samples/boards/96b_argonkey/microphone/src/main.c
* :github:`13856` - [Coverity CID :190960]Various in /tests/subsys/fs/fcb/src/fcb_test_last_of_n.c
* :github:`13855` - [Coverity CID :190961]Error handling issues in /subsys/bluetooth/host/mesh/prov.c
* :github:`13854` - [Coverity CID :190964]Integer handling issues in /arch/arm/core/cortex_m/mpu/nxp_mpu.c
* :github:`13853` - [Coverity CID :190965]Error handling issues in /subsys/net/ip/ipv4_autoconf.c
* :github:`13852` - [Coverity CID :190966]Error handling issues in /samples/net/sockets/echo_async_select/src/socket_echo_select.c
* :github:`13851` - [Coverity CID :190967]Incorrect expression in /tests/drivers/counter/counter_basic_api/src/test_counter.c
* :github:`13850` - [Coverity CID :190969]Uninitialized variables in /samples/net/sockets/coap_client/src/coap-client.c
* :github:`13849` - [Coverity CID :190970]Uninitialized variables in /subsys/bluetooth/shell/ll.c
* :github:`13848` - [Coverity CID :190971]Null pointer dereferences in /subsys/net/ip/net_pkt.c
* :github:`13847` - [Coverity CID :190972]Control flow issues in /subsys/power/power.c
* :github:`13846` - [Coverity CID :190973]Control flow issues in /subsys/net/ip/net_context.c
* :github:`13845` - [Coverity CID :190974]Integer handling issues in /subsys/net/ip/trickle.c
* :github:`13844` - [Coverity CID :190976]Integer handling issues in /arch/arm/core/cortex_m/mpu/nxp_mpu.c
* :github:`13843` - [Coverity CID :190977]Integer handling issues in /lib/os/printk.c
* :github:`13842` - [Coverity CID :190978]Control flow issues in /drivers/spi/spi_intel.c
* :github:`13841` - [Coverity CID :190980]Parse warnings in /tests/drivers/can/api/src/main.c
* :github:`13840` - [Coverity CID :190981]Error handling issues in /subsys/fs/nffs_fs.c
* :github:`13839` - [Coverity CID :190983]Incorrect expression in /tests/drivers/counter/counter_basic_api/src/test_counter.c
* :github:`13838` - [Coverity CID :190985]Memory - illegal accesses in /arch/x86/core/x86_mmu.c
* :github:`13837` - [Coverity CID :190986]Control flow issues in /subsys/net/lib/sockets/sockets_tls.c
* :github:`13836` - [Coverity CID :190987]Integer handling issues in /arch/arm/core/cortex_m/mpu/nxp_mpu.c
* :github:`13835` - [Coverity CID :190989]Parse warnings in /tests/drivers/can/api/src/main.c
* :github:`13834` - [Coverity CID :190990]Null pointer dereferences in /subsys/net/ip/net_pkt.c
* :github:`13833` - [Coverity CID :190991]Error handling issues in /subsys/bluetooth/controller/ll_sw/ull_conn.c
* :github:`13832` - [Coverity CID :190992]Null pointer dereferences in /subsys/net/ip/dhcpv4.c
* :github:`13831` - [Coverity CID :190993]Various in /subsys/shell/shell.c
* :github:`13830` - [Coverity CID :190995]Control flow issues in /subsys/net/ip/ipv6.c
* :github:`13829` - [Coverity CID :190996]Integer handling issues in /drivers/can/stm32_can.c
* :github:`13828` - [Coverity CID :190997]Integer handling issues in /lib/os/printk.c
* :github:`13827` - [Coverity CID :190998]Incorrect expression in /tests/drivers/uart/uart_async_api/src/test_uart_async.c
* :github:`13826` - [Coverity CID :191001]Control flow issues in /subsys/net/lib/lwm2m/lwm2m_rw_json.c
* :github:`13825` - [Coverity CID :191002]Error handling issues in /tests/net/lib/mqtt_pubsub/src/test_mqtt_pubsub.c
* :github:`13824` - [Coverity CID :191003]Resource leaks in /samples/net/sockets/can/src/main.c
* :github:`13823` - tests/kernel/arm_irq_vector_table: test case cannot quit displaying "isr 0 ran!"
* :github:`13822` - Invalid USB state: powered after cable is disconnected
* :github:`13821` - tests/kernel/sched/schedule_api: Assertion failed for test_slice_scheduling
* :github:`13813` - Test suite mslab_threadsafe fails randomly
* :github:`13783` - tests/kernel/mem_protect/stackprot failure in frdm_k64f due to limited privilege stack size
* :github:`13780` - mimxrt1060_evk tests/crypto/tinycrypt_hmac_prng and test_mbedtls meet Unaligned memory access
* :github:`13779` - mimxrt1060_evk tests/kernel/mem_pool/mem_pool_threadsafe meets Imprecise data bus error
* :github:`13778` - mimxrt1060_evk tests/kernel/pending meets assert
* :github:`13777` - mimxrt1060_evk tests/kernel/profiling/profiling_api meets Illegal use of the EPSR
* :github:`13769` - mimxrt1060_evk tests/kernel/fifo/fifo_timeout and kernel/fifo/fifo_usage meet system error
* :github:`13768` - mimxrt1060_evk tests/kernel/device illegal use of the EPSR
* :github:`13767` - mimxrt1060_evk tests/kernel/context and tests/kernel/critical caught system err
* :github:`13766` - mimxrt1060_evk tests/kernel/fatal meet many unwanted exceptions
* :github:`13765` - mimxrt1060_evk tests/kernel/workq/work_queue meets Illegal use of the EPSR
* :github:`13764` - mimxrt1060_evk test/kernel/mem_slab/mslab_threadsafe meets Imprecise data bus error
* :github:`13762` - mimxrt1060_evk test/lib/c_lib and test/lib/json test/lib/ringbuffer meet  Unaligned memory access
* :github:`13754` - Error: "West version found in path does not support '/usr/bin/make flash', ensure west is installed and not only the bootstrapper"
* :github:`13753` - _UART_NS16550_PORT_{2,3}_ seems to be a (possibly broken) Kconfig/DTS mishmash
* :github:`13752` - The Arduino 101 docs tell people to set CONFIG_UART_QMSI_1_BAUDRATE, which was removed
* :github:`13750` - CONFIG_SPI_3_NRF_SPIS is undefined but referenced
* :github:`13748` - CONFIG_SOC_MCIMX7D_M4 is undefined but referenced
* :github:`13747` - CONFIG_NRFX_UARTE{2-3} are undefined but referenced
* :github:`13735` - mimxrt1020_evk tests/benchmarks/app_kernel meets Illegal use of the EPSR
* :github:`13734` - tests/subsys/settings/fcb/src/settings_test_save_unaligned.c fail with assertion failure on nrf52_pca10040
* :github:`13733` - mimxrt1020_evk samples/net/zperf meets Unaligned memory access
* :github:`13729` - sanitycheck --coverage failed
* :github:`13728` - mimxrt1020_evk tests/subsys/logging/log_core test_log_strdup_gc meets Unaligned memory access
* :github:`13727` - MIMXRT1020_EVK sample/subsys/logging/logger report unaligned memory access
* :github:`13716` - CAN tests don't build
* :github:`13710` - Build failure when using XCC toolchain
* :github:`13703` - Build error w/Flash driver enabled on hexiwear kw40
* :github:`13701` - x86 stack check fail w/posix-lib & newlib
* :github:`13686` - newlib, posix-lib and xtensa/riscv (with sdk-0.9.5) don't build cleanly
* :github:`13680` - XCC install directions need updating in boards/xtensa/intel_s1000_crb/doc/index.rst
* :github:`13665` - amples/subsys/usb/cdc_acm_composite: Stuck at "Wait for DTR"
* :github:`13664` - samples/subsys/usb/cdc_acm_composite: No output beyond "***** Booting Zephyr OS v1.14.0-rc- ....****** banner
* :github:`13662` - samples/subsys/usb/cdc_acm: Stuck at "Wait for DTR"
* :github:`13655` - mimxrt1050_evk test/crypto/rand32 meets Kernel Panic
* :github:`13654` - mimxrt1050_evk test/kernel/mem_protect/stack_random fails on stack fault
* :github:`13642` - stack canaries don't work with user mode threads
* :github:`13624` - ATMEL SAM family UART and USART - functions u(s)art_sam_irq_is_pending doesn't respect IRQ settings
* :github:`13610` - kernel: Non-deterministic and very high ISR latencies
* :github:`13609` - samples: cfb: text is not displayed due to display_blanking_off()
* :github:`13595` - tests/kernel/stack fails to build on nios2 with new SDK 0.10.0-rc3
* :github:`13594` - tests/kernel/mem_protect/mem_protect/kernel.memory_protection build failure on minnowboard with new SDK
* :github:`13585` - CONFIG_BT_HCI_TX_STACK_SIZE is too small
* :github:`13584` - i.MX RT board flashing and debugging sections are out of date
* :github:`13572` - settings: Bluetooth: Failed parse/lookup
* :github:`13567` - tests/subsys/settings/fcb/base64 fails when write-block-size is 8
* :github:`13560` - STM32 USB: netusb: kernel crash when testing example echo_server with nucleo_f412zg  (ECM on Windows)
* :github:`13550` - stm32: i2c: SSD1306 does not work due to write size limitation
* :github:`13547` - tests/drivers/build_all: The Zephyr library 'drivers__adc' was created without source files
* :github:`13541` - sanitycheck errors when device-testing frdm_k64f
* :github:`13536` - test: tests/kernel/mem_slab/mslab_threadsafe fails sporadically on nrf52
* :github:`13522` - BT SUBSCRIBE to characteristic for Indication or WRITE to value results in kernel crash
* :github:`13515` - samples/net/sockets/echo doesn’t link with CONFIG_NO_OPTIMIZATIONS=y
* :github:`13514` - #stm32 creating #gpio #interrupts on 2+ pins with the same pin number failes
* :github:`13502` - tests/benchmarks/timing_info: Output only consist of Delay Boot Banner
* :github:`13489` - frdm_k64f test/net/tcp bus fault after test ends
* :github:`13484` - net: (At least) eth_smsc911x driver is broken in the master
* :github:`13482` - The frdm_k64f board resets itself periodically/A possible NXP MPU bug
* :github:`13481` - Regression in CI coverage for (at least) some Ethernet drivers after net_app code removal
* :github:`13470` - Lack of POSIX compliance for sched_param struct
* :github:`13465` - tests/lib/mem_alloc/testcase.yaml#libraries.libc.minimal: Bus fault at test_malloc
* :github:`13464` - rb.h: macro RB_FOR_EACH_CONTAINER bug
* :github:`13463` - frdm_kl25z samples/basic/threads Kernel Panic
* :github:`13462` - frdm_kl25z samples/basic/disco meet hard fault
* :github:`13458` - galileo I2C bus master names aren't getting set in the build
* :github:`13449` - sanitycheck failure: [nocache] build failures with sdk-ng-0.10.0
* :github:`13448` - OpenOCD support code version not raised on recent additions
* :github:`13437` - 6LoWPAN: ICMP Ping Zephyr -> Linux broken in master [regression, bisected]
* :github:`13434` - Aliases inside dts leads to warnings
* :github:`13433` - Error when rebooting the frdm_k64f board
* :github:`13424` - Logger got recently slower
* :github:`13423` - Default logger stack size insufficient for various samples
* :github:`13422` - Can't use GPIO 2, 3 and 4
* :github:`13421` - tests/drivers/watchdog/wdt_basic_api: test_wdt_no_callback() repeats indefinitely
* :github:`13413` - x86 reports incorrect stack pointer for user mode exceptions
* :github:`13411` - kernel: ASSERTION FAIL [z_spin_lock_valid(l)]
* :github:`13410` - qemu_x86 transient build errors for mmu_tables.o
* :github:`13408` - DT_FLASH_AREA generated seems to be different for Zephyr and MCUBootloader
* :github:`13397` - Function documentation is missing for BSD sockets
* :github:`13396` - Cannot connect to Galaxy S8 via BLE
* :github:`13394` - Missing Documentation for Bluetooth subsystem
* :github:`13384` - linking error of gettimeofday with zephyr-sdk-0.10.0-rc2
* :github:`13380` - sockets: ordering of send() vs. poll() when using socket API + DTLS causes a crash
* :github:`13378` - Missing Documentation for Networking subsystem
* :github:`13361` - nucleo_f103rb blinky example cannot run
* :github:`13357` - Tracing hooks problem on POSIX
* :github:`13353` - z_timeout_remaining should subtract z_clock_elapsed
* :github:`13342` - arm: user thread stack overflows do not report _NANO_ERR_STACK_CHK_FAIL
* :github:`13341` - arc: user thread stack overflows do not report _NANO_ERR_STACK_CHK_FAIL
* :github:`13340` - NRF52 pca10040 boards open the "Flash hardware support" option, the BT Mesh example does not work properly
* :github:`13323` - No USB instance
* :github:`13320` - sanitycheck miss extra_args: OVERLAY_CONFIG parameter
* :github:`13316` - Notification enabled before connection
* :github:`13306` - Checking if UARTE TX complete on nRF52
* :github:`13301` - frdm_k64f: samples/net/sockets/echo_server doesn't work
* :github:`13300` - NET: USB Ethernet tests were removed allowing to submit not compiling code
* :github:`13291` - samples/drivers/watchdog: Fatal fault in ISR
* :github:`13290` - samples/drivers/watchdog: Watchdog setup error
* :github:`13289` - tests/kernel/fifo/fifo_timeout fails on nrf52840_pca10056
* :github:`13287` - Zephyr can no longer apply DT overlays on a per-SoC basis
* :github:`13272` - Catch all bug for build issues with SDK 0.10.0-rc2
* :github:`13257` - Shell not compatible with c++
* :github:`13256` - UART error bitmask broken by new asynchronous UART API
* :github:`13255` - tests/drivers/counter/counter_basic_api: Kernel panic and an assertion error when you run  test_multiple_alarms() after test_single_shot_alarm_top() failed
* :github:`13254` - tests/drivers/counter/counter_basic_api: counter failed to raise alarm after ticks limit reached
* :github:`13253` - tests/drivers/counter/counter_basic_api: nchan  not equal to alarm_cnt
* :github:`13251` - frdm_k64f: samples/net/sockets/echo_server doesn't work
* :github:`13249` - Latest Zephyr HEAD results in a crash in mcuboot tree
* :github:`13247` - tests/drivers/counter/counter_basic_api:  counter_set_top_value() failed
* :github:`13245` - Including module(s):
* :github:`13243` - DT: error in generated_dts_board_fixups.h for board: frdm_k64f
* :github:`13237` - stm32 USB sanitycheck failures with sdk 0.10.0-beta2
* :github:`13236` - Failure tests/kernel/gen_isr_table on some stm32 platforms
* :github:`13223` - I2S transfers causes exception/crash in xtensa/Intel S1000
* :github:`13220` - qemu_x86_64 build failures
* :github:`13218` - tests: intel_s1000_crb: CONFIG_I2C_0_NAME undeclared build error
* :github:`13211` - net/sockets: send/sendto broken when len > MTU
* :github:`13209` - FATAL ERROR: unknown key "posixpath" in format string "{posixpath}"
* :github:`13203` - drivers: wifi: simplelink: Need to translate socket family macro values
* :github:`13194` - soc/arm/nordic_nrf/nrf52/soc_power.h warning spew when CONFIG_SYS_POWER_MANAGEMENT=n
* :github:`13192` - silabs flash_gecko driver warning (will fail in CI)
* :github:`13187` - qemu_x86_64 leaks system headers into the build process
* :github:`13166` - tests/kernel/threads/dynamic_thread test cases are failing on frdm_k64f board
* :github:`13161` - QMSI drivers/counter/counter_qmsi_aon.c doesn't build
* :github:`13147` - net: ICMPv4 echo reply packets do not use default values in the IP header
* :github:`13122` - build for KW40z, KW41z fails to generate isr_tables
* :github:`13113` - Samples fail to build for SimpleLink when CONFIG_XIP=n
* :github:`13110` - MPU fault on performing fifo operations
* :github:`13096` - Remove CONFIG_X86_PAE_MODE from scripts/gen_mmu_x86.py
* :github:`13084` - net: Align interface numbering with POSIX/BSD/Linux
* :github:`13083` - Problem pairing/bonding 2nd device, whilst the first device is still connected using sample project (bluetooth\peripheral)
* :github:`13082` - s1000 board: multiple registrations for irq error
* :github:`13073` - intermittent failure in tests/benchmarks/timing_info
* :github:`13066` - Bug on STM32F2 USB Low Layer HAL
* :github:`13065` - CONFIG_BT leads Fatal fault in ISR on esp32
* :github:`13051` - Two timers are expiring at one time and crashing for platform nrf52_pca10040
* :github:`13050` - net: Zephyr drops TCPv4 packet with extended MAC frame size
* :github:`13047` - Build error while executing tests/kernel/tickless/tickless on quark_se_c1000_devboard
* :github:`13044` - intermittent tests/kernel/workq/work_queue failure
* :github:`13043` - intermittent tests/posix/common/ failure
* :github:`13034` - samples/bluetooth/peripheral_hr: could not connect with reel board
* :github:`13025` - CAN not working on Nucleo L432KC with external transciever
* :github:`13014` - sanitycheck fails to generate coverage report if samples/application_development/external_lib is run
* :github:`13013` - Problem executing 'west flash' outside zephyr directory.
* :github:`13011` - tests/posix/fs/ segfaults randomly in POSIX arch
* :github:`13009` - Coverage broken for nrf52_bsim
* :github:`13005` - syslog_net doc error
* :github:`13001` - app shared memory rules in CMakeLists.txt breaks incremental builds
* :github:`12994` - intermittent failures in tests/net/socket/select/ on qemu_x86
* :github:`12982` - net: Zephyr drops IPv4 packet with extended MAC frame size
* :github:`12967` - settings/fcb-backend: value size might be bigger than expected
* :github:`12959` - Error with cmake build "string sub-command REGEX, mode MATCH needs at least 5 arguments"
* :github:`12958` - Missing LwM2M protocol information in the network section of docs
* :github:`12957` - Need documentation for MQTT
* :github:`12956` - CoAP missing documentation
* :github:`12955` - Missing Documentation for many subsystems and features
* :github:`12950` - tests/kernel/workq/work_queue_api/kernel.workqueue fails on nsim_em
* :github:`12949` - tests/benchmarks/timing_info/benchmark.timing.userspace fails on nsim_em
* :github:`12948` - tests/kernel/mem_protect/stack_random/kernel.memory_protection.stack_random fails on nsim_em
* :github:`12947` - tests/benchmarks/timing_info/benchmark.timing.default_kernel fails on nsim_em
* :github:`12946` - Zephyr/BLE stack: Problem pairing/bonding more than one device, whilst the first device is still connected.
* :github:`12945` - mqtt_socket connect is hung on sam_e70_xplained
* :github:`12933` - MCUboot: high current
* :github:`12908` - Data allocation in sections for quark_se is incorrect
* :github:`12905` - Build improperly does a partial discard of 'const' defined variables
* :github:`12900` - tests/benchmarks/timing_info doesn't print userspace stats
* :github:`12886` - Application development primer docs broken by west merge
* :github:`12873` - Early log panic does not print logs on shell
* :github:`12851` - Early log panic does not print logs
* :github:`12849` - i2c: frdm-k64f and mimxrt1050_evk I2C driver  will cause hardware exception if read/write to a not existing device
* :github:`12844` - ARC MPU version 3 is configured incorrectly
* :github:`12821` - When CPU_STATS is enable with MPU_STACK_GUARD in DEBUG_OPTIMIZATIONS mode, it cause a MPU FAULT /   Instruction Access Violation.
* :github:`12820` - CONFIG_NO_OPTIMIZATION triggers a usage fault
* :github:`12813` - DTS: CONFIG_FLASH_BASE_ADDRESS not being generated for SPI based Flash chip
* :github:`12812` - ninja flash when running without 'west'
* :github:`12810` - Up Squared serial console character output corrupted
* :github:`12804` - tests/drivers/watchdog/wdt_basic_api/: wdt_install_timeout() failed to call callback
* :github:`12803` - tests/drivers/watchdog/wdt_basic_api/: Watchdog setup error
* :github:`12800` - topic-counter: nrfx_*: counter_set_top_value inconsistent behavior
* :github:`12796` - USB Power Event Panic
* :github:`12766` - drivers: gpio: stm32: implementation silently ignores attempts to configure level interrupts
* :github:`12765` - drivers: gpio: intel_apl: implementation silently ignores double-edge interrupt config
* :github:`12764` - drivers: gpio: cmsdk_ahb: implementation silently ignores double-edge interrupt config
* :github:`12763` - drivers: gpio: sch: implementation does not configure interrupt level/edge
* :github:`12758` - doc: Samples and Demos documentation hierarchy looks unintentionally deep
* :github:`12745` - SimpleLink socket functions, on error, sometimes do not set errno and return (-1)
* :github:`12734` - drivers: flash: Recent change in spi_nor.c does not let have multiple flash devs on a board.
* :github:`12726` - Dead loop of the kernel during Bluetooth Mesh pressure communication
* :github:`12724` - SPI CS: in case of multiple slaves, wrong cs-gpio is chosen in DT\_ define
* :github:`12708` - Drivers may call net_pkt_(un)ref from ISR concurrently with other code
* :github:`12696` - CMAKE_EXPORT_COMPILE_COMMANDS is broken
* :github:`12688` - arm: userspace: inconsistent configuration between ARM and NXP MPU
* :github:`12685` - 717aa9cea7 broke use of dtc 1.4.6
* :github:`12657` - subsys/settings: fcb might compress areas more than once
* :github:`12654` - Build error while executing tests/kernel/smp on ESP32
* :github:`12652` - UART console is showing garbage with driver uart_ns15560
* :github:`12650` - drivers: wifi: simplelink: socket() always returns fd of zero on success
* :github:`12640` - CONFIG_ETH_ENC28J60_0_GPIO_SPI_CS=y cause build error
* :github:`12632` - tests/drivers/adc/adc_api/ fails on quark platforms
* :github:`12621` - warning about images when building docs
* :github:`12615` - Network documentation might miss API documentation
* :github:`12611` - Shell does not support network backends
* :github:`12609` - ext: stm32: revert fix https://github.com/zephyrproject-rtos/zephyr/pull/8762
* :github:`12594` - stm32_min_dev board no console output
* :github:`12589` - Several nRF based boards enable both I2C & SPI by default in dts at same MMIO address
* :github:`12574` - Bluetooth: Mesh: 2nd time commissioning configuration details (APP Key) not get saved on SoC flash
* :github:`12571` - No coverage reports are being generated
* :github:`12570` - Zephyr codebase incorrectly uses #ifdef for boolean config values
* :github:`12560` - Using TCP w/ wired NIC results in mismanagement of buffers due to ACK accounting error
* :github:`12559` - tests/kernel/mem_pool/mem_pool_threadsafe/kernel.memory_pool fails sporadically
* :github:`12553` - List of tests that keep failing sporadically
* :github:`12548` - ISR sometimes run with the MPU disabled: breaks __nocache
* :github:`12544` - commit 2fb616e broke I2C on Nucleo F401RE + IKS01A2 shield
* :github:`12543` - doc: Wrong file path for code relocation sample
* :github:`12541` - nrf timer handling exceeds bluetooth hard realtime deadline
* :github:`12530` - DTS: Changes done to support QSPI memory mapped flash breaks intel_s1000 build
* :github:`12528` - a bug in code
* :github:`12501` - nRF52: UARTE lacks pm interface
* :github:`12494` - Logging with CONFIG_LOG_IMMEDIATE=y from ISR leads to assert
* :github:`12488` - RedBear Nano v2 Mesh Instruction Fault
* :github:`12487` - Power management and RTTLogger
* :github:`12479` - arc: the pollution of lp_start,lpend and lp_count will  break down the system
* :github:`12478` - tests/drivers/ipm/peripheral.mailbox failing sporadically on qemu_x86_64 (timeout)
* :github:`12455` - Fatal fault when openthread commissioner starts
* :github:`12454` - doc: Some board images are pretty big (> 1MB)
* :github:`12453` - nrf52 SPIM spi_transceive function occasionally doesn't return
* :github:`12449` - Existing LPN lookup in Mesh Friend Request handling
* :github:`12441` - include: net: Link error when inet_pton() is used and wifi offloading is enabled
* :github:`12429` - Bluetooth samples not working on qemu_x86
* :github:`12419` - cannot flash with segger jlink in windows environment
* :github:`12410` - Assert and printk not printed on RTT
* :github:`12409` - non-tickless kernels incorrectly advance system clock with delayed ticks
* :github:`12395` - Some Bluetooth samples wont run using the latest branch on some boards
* :github:`12369` - WDT: wdt callbacks are not getting triggerred before CPU going for a reboot
* :github:`12362` - BLE HID sample fails to reconnect on Windows 10 tablets
* :github:`12352` - intermittent kernel.mutex sanitycheck with mps2_an385
* :github:`12347` - net ping shell can't show reply
* :github:`12339` - drivers: nordic: usb: missing fragmentation handling for IN transfers, causing buffer overflow
* :github:`12329` - enable CONFIG_NET_DEBUG_HTTP_CONN cause build error
* :github:`12326` - [Coverity CID :158187]Control flow issues in /sanitylog/nrf52840_pca10056/samples/net/echo_server/test_nrf_openthread/zephyr/ext_proj/Source/ot/third_party/mbedtls/repo/library/ecp.c
* :github:`12325` - [Coverity CID :190377]Control flow issues in /sanitylog/nrf52840_pca10056/samples/net/echo_server/test_nrf_openthread/zephyr/ext_proj/Source/ot/examples/platforms/utils/settings_flash.c
* :github:`12324` - [Coverity CID :190380]Insecure data handling in /sanitylog/nrf52840_pca10056/samples/net/echo_server/test_nrf_openthread/zephyr/ext_proj/Source/ot/third_party/mbedtls/repo/library/ssl_tls.c
* :github:`12323` - [Coverity CID :190383]Null pointer dereferences in /sanitylog/nrf52840_pca10056/samples/net/echo_server/test_nrf_openthread/zephyr/ext_proj/Source/ot/third_party/mbedtls/repo/library/ssl_tls.c
* :github:`12322` - [Coverity CID :190611]Control flow issues in /drivers/usb/device/usb_dc_nrfx.c
* :github:`12321` - [Coverity CID :190612]Control flow issues in /subsys/net/lib/lwm2m/lwm2m_rw_json.c
* :github:`12320` - [Coverity CID :190613]Integer handling issues in /subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`12319` - [Coverity CID :190614]Control flow issues in /subsys/shell/shell_utils.c
* :github:`12318` - [Coverity CID :190615]Null pointer dereferences in /subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`12317` - [Coverity CID :190616]Integer handling issues in /subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`12316` - [Coverity CID :190617]Control flow issues in /subsys/net/lib/lwm2m/lwm2m_rw_json.c
* :github:`12315` - [Coverity CID :190618]Code maintainability issues in /drivers/modem/wncm14a2a.c
* :github:`12314` - [Coverity CID :190619]Memory - illegal accesses in /subsys/bluetooth/host/mesh/settings.c
* :github:`12313` - [Coverity CID :190620]Null pointer dereferences in /drivers/wifi/eswifi/eswifi_core.c
* :github:`12312` - [Coverity CID :190621]Memory - corruptions in /subsys/net/lib/lwm2m/lwm2m_rw_oma_tlv.c
* :github:`12311` - [Coverity CID :190622]Memory - corruptions in /drivers/wifi/eswifi/eswifi_offload.c
* :github:`12310` - [Coverity CID :190623]Error handling issues in /drivers/wifi/eswifi/eswifi_core.c
* :github:`12309` - [Coverity CID :190624]Memory - corruptions in /tests/posix/fs/src/test_fs_file.c
* :github:`12308` - [Coverity CID :190625]Integer handling issues in /samples/bluetooth/peripheral/src/main.c
* :github:`12307` - [Coverity CID :190626]Null pointer dereferences in /subsys/net/lib/coap/coap_sock.c
* :github:`12306` - [Coverity CID :190627]Incorrect expression in /samples/net/zperf/src/zperf_tcp_receiver.c
* :github:`12305` - [Coverity CID :190628]Memory - corruptions in /drivers/wifi/eswifi/eswifi_core.c
* :github:`12304` - [Coverity CID :190629]Incorrect expression in /samples/net/zperf/src/zperf_udp_receiver.c
* :github:`12303` - [Coverity CID :190630]Null pointer dereferences in /drivers/modem/wncm14a2a.c
* :github:`12302` - [Coverity CID :190631]Control flow issues in /subsys/net/lib/lwm2m/lwm2m_rw_json.c
* :github:`12301` - [Coverity CID :190632]Memory - corruptions in /drivers/wifi/eswifi/eswifi_offload.c
* :github:`12300` - [Coverity CID :190633]Control flow issues in /subsys/net/lib/lwm2m/lwm2m_rw_plain_text.c
* :github:`12299` - [Coverity CID :190634]Control flow issues in /subsys/net/lib/lwm2m/lwm2m_obj_device.c
* :github:`12298` - [Coverity CID :190635]Integer handling issues in /subsys/net/lib/coap/coap_link_format_sock.c
* :github:`12297` - [Coverity CID :190636]Incorrect expression in /samples/subsys/usb/console/src/main.c
* :github:`12296` - [Coverity CID :190637]Null pointer dereferences in /subsys/net/lib/lwm2m/lwm2m_rw_plain_text.c
* :github:`12295` - [Coverity CID :190638]Control flow issues in /samples/portability/cmsis_rtos_v2/timer_synchronization/src/main.c
* :github:`12294` - [Coverity CID :190639]Null pointer dereferences in /subsys/net/ip/route.c
* :github:`12293` - [Coverity CID :190640]Null pointer dereferences in /drivers/wifi/eswifi/eswifi_core.c
* :github:`12292` - [Coverity CID :190641]Null pointer dereferences in /lib/cmsis_rtos_v2/kernel.c
* :github:`12291` - [Coverity CID :190642]Error handling issues in /drivers/console/telnet_console.c
* :github:`12290` - [Coverity CID :190644]Memory - illegal accesses in /drivers/modem/wncm14a2a.c
* :github:`12282` - "make htmldocs" reports a cmake warning
* :github:`12274` - Assert crash in logger's out_func
* :github:`12268` - doc: nightly published AWS docs are not as generated
* :github:`12265` - doc: Some API documentation is not being generated
* :github:`12257` - The latest GNU ARM Embedded Toolchain can't produce hex files on Windows
* :github:`12252` - potential non-termination of k_mem_pool_alloc
* :github:`12250` - DTS: Inconsistencies seen for SPI node with size-cells
* :github:`12249` - Can't flash CC3220 with OpenOCD
* :github:`12243` - Build error with I2C driver for nucleo_f103rb: DT_ST_STM32_I2C_V1_40005400_BASE_ADDRESS' undeclared here
* :github:`12241` - logging: log output busy loops if log output function can not process buffer
* :github:`12224` - POSIX api are incompatible with arm gcc 2018q2 toolchain
* :github:`12203` - openthread: bind IP is randomly selected causing off-mesh communication problems
* :github:`12201` - Enabling CONFIG_LOG is throwing exceptions on intel_s1000
* :github:`12192` - include: __assert: enabling assert causes build errors
* :github:`12190` - Mbedtls MBEDTLS_PLATFORM_STD_SNPRINTF issue
* :github:`12186` - net: arp: Zephyr does not use MAC address from unicast ARP reply with wrong target MAC
* :github:`12179` - net: arp: Zephyr does not use MAC address from unicast ARP reply
* :github:`12171` - ot: ping ten times will crash
* :github:`12170` - logging: previous changes to dropped message notification break wifi sample debug
* :github:`12164` - net: icmpv4: Zephyr drops ICMPv4 packet with correct checksum 0
* :github:`12162` - net: icmpv4: Zephyr replies to ICMPv4 echo request with broadcast destination IPv4 address
* :github:`12159` - bt_gatt_attr_read_chrc: no characteristic for value
* :github:`12154` - 802.15.4 6LowPAN stopped working with multiple samples
* :github:`12147` - tests/kernel/interrupt fails on quark_se_c1000_ss_devboard
* :github:`12146` - tests/kernel/arm_irq_vector_table fails on NRF5 boards
* :github:`12144` - There's lots of references to undefined Kconfig symbols
* :github:`12123` - samples: cmsis_rtos_v1/philosophers faults with a FATAL EXCEPTION on esp32
* :github:`12122` - system.settings.nffs fails on NRF5 boards
* :github:`12120` - Can't pair with nrf52832 running zephyr
* :github:`12118` - printf doesn't work on qemu_cortex_m3 with newlib and arm gcc 2018q2 toolchain
* :github:`12092` - Default Virtual COM port on nucleo_l4r5zi is lpuart1
* :github:`12091` - tests/subsys/settings/fcb/base64 fails on NRF5 boards
* :github:`12089` - Unexpected fault during test does not cause test failure
* :github:`12069` - iwdg_stm32.c: potential failure in wdt_set_config()
* :github:`12066` - Observing a Python path Issue with Git based Compile
* :github:`12065` - enormous .BSS size while building tests/subsys/fs/nffs_fs_api
* :github:`12059` - kernel.memory_protection.stack_random fails on quark_d2000_crb board
* :github:`12058` - Lots of tests don't honor CONFIG_TEST_EXTRA_STACK_SIZE
* :github:`12051` - LOG_PANIC never returns if RTT is disconnected
* :github:`12045` - frdm_k64f + shell breaks Ethernet driver
* :github:`12043` - logger: Invalid pointer deference in samples/subsys/logging/logger/
* :github:`12040` - Log messages are dropped even with huge log buffer
* :github:`12037` - nrf52840 ram retention example failed
* :github:`12033` - samples/application_development/code_relocation fails on mps2_an385
* :github:`12029` - CONFIG_STACK_CANARIES doesn't work on x86_64
* :github:`12016` - Crash while disconnecting device from USB
* :github:`12006` - valgrind detected issue in tests/net/ipv6/
* :github:`11999` - CONFIG_TEXT_SECTION_OFFSET doesn't seem to work on x86
* :github:`11998` - intermittent failures in tests/kernel/common: test_timeout_order: (poll_events[ii].state not equal to K_POLL_STATE_SEM_AVAILABLE)
* :github:`11989` - Reel Board mesh_badge sample | Light sensor bug
* :github:`11980` - cmake: Build fails in an environment without 'python' binary
* :github:`11967` - nrfx SPI driver blocks indefinitely when transferring
* :github:`11961` - Python warning in process_gperf.py
* :github:`11935` - Invalid usage of k_busy_wait()
* :github:`11916` - ISR table (_sw_isr_table) generation is fragile and can result in corrupted binaries
* :github:`11914` - NXP documentation: frdm-k64f links dead
* :github:`11904` - blinky example can't work on nucleo-f070rb and nucleo-f030r8 platform
* :github:`11894` - zephyr_env.sh failes to correctly set $ZEPHYR_BASE with zsh hooks bound to cd
* :github:`11889` - Race between SimpleLink WiFi driver FastConnect and networking app startup.
* :github:`11859` - sanitycheck failures on lpcxpresso54114_m4: Error: Aborting due to non-whitelisted Kconfig warning
* :github:`11857` - cmake does not detect most recent python, it is fixed somehow to 3.4.x
* :github:`11844` - rtc_ll_stm32.c:232: undefined reference to 'LL_RCC_LSE_SetDriveCapability'  #stm32 #rtc
* :github:`11841` - Assert invoked in exception in ctrl.c line 7653 (zephyr 1.13.00)
* :github:`11827` - gPTP Sample Application fails on frdm_k64f board ,and PDelay Response Receipt Timeout
* :github:`11815` - nxp_kinetis: system reset leaves interrupts enabled during startup
* :github:`11806` - gpio_nrf mishandles level interrupts
* :github:`11798` - A thread may lock a mutex before it is fully unlocked.
* :github:`11794` - cmake: CMake 3.13 doesn't like INTERFACE with target_link_libraries
* :github:`11792` - drivers: nrf: UARTE interrupt driven cannot be compiled
* :github:`11780` - Bluetooth: Mesh: initialisation delay after disabling CONFIG_BT_DEBUG_LOG
* :github:`11779` - watchdog sample application not being built on Nordic devices
* :github:`11767` - tests: counter api: Failed when MPU enabled
* :github:`11763` - logger drops log messages without any indication
* :github:`11754` - NRFX TWI driver does not compile with newlib
* :github:`11752` - tests/lib/ringbuffer: build error on Xtensa ESP32
* :github:`11749` - logging: default log thread priority is too high
* :github:`11744` - Regression: nNRF52840 HW dies at 8 minutes 20 seconds in various samples using IPSP
* :github:`11741` - mqtt_publisher sample not working with BLE IPSP and has outdated net setup/prj configs
* :github:`11723` - tests/cmsis_rtos_v1 fails on nrf52840_pca10056
* :github:`11722` - tests/kernel/timer/timer_api fails on nrf52_pca10040 board
* :github:`11721` - tests/kernel/sched/schedule_api fails on nrf5* boards
* :github:`11719` - Legacy MQTT sample app breaking
* :github:`11698` - net: ipv4: Zephyr replies to ICMPv4 packets with incorrect checksums
* :github:`11694` - system clock problem on NRF52 boards
* :github:`11682` - Mesh Friend replies to initial Friend Poll with "old" security credentials
* :github:`11675` - Cannot set UART device name in menuconfig
* :github:`11674` - Wrong assert condition @@zephyr/kernel/sched.c:345
* :github:`11651` - Time consumption is not constant during the pend and unpend operation with 0(1) pending queue
* :github:`11633` - logging: CONFIG_LOG_INPLACE_PROCESS is not synchronous
* :github:`11626` - k_busy_wait exits early on Nordic chips
* :github:`11625` - Problem with printk and LOG_ERR used with shell enabled
* :github:`11618` - net: icmpv4: Zephyr drops valid echo request
* :github:`11617` - net: ipv4: udp: broadcast delivery not supported
* :github:`11612` - i2c_burst_write on nrf51 is not a burst write
* :github:`11586` - mimxrt1050_evb board: Can't get Ethernet to work
* :github:`11579` - net: arp: MAC address not updated if target address is not its own
* :github:`11565` - gpio_callback ambiguity in union pin vs pin_mask
* :github:`11564` - Bluetooth: settings: Invalid base64 value written to flash
* :github:`11562` - net: arp: Zephyr does use MAC address from broadcast ARP reply
* :github:`11561` - Ping through net shell seems to be broken
* :github:`11533` - Energy-efficient BLE controller on the nRF52
* :github:`11532` - drivers: serial: uart_msp432p4xx.c not compile for interrupt API
* :github:`11531` - Networking samples documentation updates
* :github:`11513` - drivers: SPI NOR: Inconsistency with flash interface docs
* :github:`11502` - Delayed works are not scheduled when system is busy
* :github:`11495` - Printk processed by logger overwrites each other
* :github:`11489` - net: arp: Zephyr replies to localhost address
* :github:`11488` - [Coverity CID :183483]Incorrect expression in /sanitylog/v2m_beetle/tests/ztest/test/base/testing.ztest.verbose_2/zephyr/priv_stacks_hash.gperf
* :github:`11487` - [Coverity CID :185396]Incorrect expression in /sanitylog/v2m_beetle/tests/posix/fs/portability.posix/zephyr/kobject_hash.gperf
* :github:`11486` - [Coverity CID :186197]Parse warnings in /sanitylog/lpcxpresso54114_m4/samples/subsys/ipc/openamp/test/openamp_remote-prefix/src/openamp_remote-build/CMakeFiles/CheckIncludeFiles/HAVE_FCNTL_H.c
* :github:`11485` - [Coverity CID :189738]Null pointer dereferences in /subsys/net/lib/dns/llmnr_responder.c
* :github:`11484` - [Coverity CID :189739]Parse warnings in /subsys/usb/class/netusb/function_eem.c
* :github:`11483` - [Coverity CID :189740]Control flow issues in /samples/boards/reel_board/mesh_badge/src/reel_board.c
* :github:`11482` - [Coverity CID :189741]Memory - illegal accesses in /samples/boards/reel_board/mesh_badge/src/reel_board.c
* :github:`11481` - [Coverity CID :189742]Program hangs in /drivers/usb/device/usb_dc_sam.c
* :github:`11480` - [Coverity CID :189743]Incorrect expression in /drivers/usb/device/usb_dc_sam.c
* :github:`11465` - UART_INTERRUPT_DRIVEN broken on SOC_SERIES_IMX7_M4 and SOC_SERIES_IMX_6X_M4
* :github:`11464` - where is PM_CONTROL_OS defined in Kconfig?
* :github:`11462` - tests: intel_s1000: flash test will fail or erase code when booting from flash
* :github:`11461` - tests: intel_s1000: build error due to offsets lib
* :github:`11447` - Build error for lwm2m_client w/ modem overlay
* :github:`11425` - MISRA C Do not use recursions
* :github:`11417` - logging doesn't log, again
* :github:`11409` - Device crash on BLE reconnection
* :github:`11394` - GPIO API: Do not blindly re-install an already installed callback
* :github:`11393` - printk (RTT backend) from isr can deadlock the system
* :github:`11390` - soc: intel_s1000: Fix the tests for dma and i2s drivers
* :github:`11384` - CI ignores result of multiple test suites in one test application
* :github:`11383` - tests/drivers/i2s/i2s_api failing on sam_e70_xplained
* :github:`11373` - Move STM32 RTC to new counter API
* :github:`11339` - zephyr stm32f4 startup freezes at startup in PRINT_BOOT_BANNER() #stm32 #uart #boot
* :github:`11329` - net: arp: Zephyr replies to multicast address cause of malfored ARP request
* :github:`11310` - Wifi scan crashes on disco_l475_iot1
* :github:`11301` - KW41Z crash on echo apps (ieee802154)
* :github:`11281` - uart shell: ring buffer usage bug
* :github:`11276` - nrf51_pca10028:tests/kernel/interrupt/arch.interrupt registers IRQ 24 twice
* :github:`11275` - frdm_k64f/samples/net/gptp/test: irq line 85 registered twice
* :github:`11268` - soc: intel_s1000: linker section size updates
* :github:`11266` - intel_s1000: usb_hid: load/store alignment exception
* :github:`11260` - Detection of an MSYS environment sometimes fails
* :github:`11255` - Pinging sam_e70 leads to unresponsive ethernet device at some point
* :github:`11250` - 'net help' or 'kernel help' shell commands lead to a fatal fault
* :github:`11244` - Turning on SystemView causes error
* :github:`11232` - nrf52: reel_board: USB unable to send fragmented packets through Control endpoint
* :github:`11226` - Logging: duplicate log messages for some samples when building for qemu_x86
* :github:`11202` - recent "shell: shell_uart" commit breaks shell samples on cc3220sf
* :github:`11187` - cmake/toolchain/clang: selecting clang doesn't select the correct compiler file
* :github:`11182` - qemu_xtensa tests fail spuriously under sanitycheck/CI
* :github:`11179` - Minor Typo "dirver" in UART of Native Posix
* :github:`11171` - usb: nrf: usb_nrfx is in endless loop when USB cable in not connected
* :github:`11170` - kernel.profiling.call_stacks_analyze_idle: assertion fails on em_starterkit_em7d_v22
* :github:`11167` - Building failing in arm cortex-m0
* :github:`11166` - quark_se_c1000_devboard has no docs
* :github:`11147` - subsys: logging: rtt: undeclared macro
* :github:`11146` - ext: debug: segger: rtt: SEGGER_RTT_Init() missing return statement
* :github:`11136` - drivers/spi: WARNING: Unsigned expression compared with zero
* :github:`11135` - subsys/net/lib/: WARNING: Unsigned expression compared with zero
* :github:`11134` - drivers/flash/soc_flash_nios2_qspi.c: WARNING: Unsigned expression compared with zero
* :github:`11133` - lib/posix/mqueue.c: WARNING: Unsigned expression compared with zero
* :github:`11129` - CONFIG_TICKLESS_KERNEL makes z_clock_set_timeout doesn't work
* :github:`11103` - The wrong Python version can be detected on Windows
* :github:`11102` - [Coverity CID :189504]Error handling issues in /samples/net/sockets/echo/src/socket_echo.c
* :github:`11101` - [Coverity CID :189505]Error handling issues in /drivers/sensor/lis2mdl/lis2mdl_trigger.c
* :github:`11100` - [Coverity CID :189506]Control flow issues in /subsys/net/ip/net_shell.c
* :github:`11099` - [Coverity CID :189507]Parse warnings in /samples/drivers/flash_shell/src/main.c
* :github:`11098` - [Coverity CID :189508]Control flow issues in /drivers/usb/device/usb_dc_nrfx.c
* :github:`11097` - [Coverity CID :189509]Integer handling issues in /drivers/sensor/ms5837/ms5837.c
* :github:`11096` - [Coverity CID :189510]Memory - corruptions in /subsys/bluetooth/host/monitor.c
* :github:`11095` - [Coverity CID :189511]Code maintainability issues in /subsys/settings/src/settings_fcb.c
* :github:`11094` - [Coverity CID :189512]Null pointer dereferences in /subsys/logging/log_msg.c
* :github:`11093` - [Coverity CID :189513]Control flow issues in /subsys/bluetooth/shell/gatt.c
* :github:`11092` - [Coverity CID :189514]Possible Control flow issues in /drivers/display/ssd1673.c
* :github:`11091` - [Coverity CID :189515]Incorrect expression in /drivers/usb/device/usb_dc_stm32.c
* :github:`11090` - [Coverity CID :189516]Null pointer dereferences in /drivers/wifi/simplelink/simplelink_sockets.c
* :github:`11089` - [Coverity CID :189517]Control flow issues in /drivers/sensor/fxos8700/fxos8700.c
* :github:`11088` - [Coverity CID :189518]Memory - illegal accesses in /samples/boards/reel_board/mesh_badge/src/reel_board.c
* :github:`11087` - [Coverity CID :189519]Error handling issues in /samples/net/sockets/echo/src/socket_echo.c
* :github:`11086` - [Coverity CID :189520]Error handling issues in /samples/net/sockets/echo/src/socket_echo.c
* :github:`11077` - Compiler warning at include/misc/util.h when building for ESP32
* :github:`11047` - doc: missing Doxygen generated documentation for any function which name starts with an underscore
* :github:`11046` - Enabling SYS_POWER_MANAGEMENT results in a linker error.
* :github:`11034` - Enabling config XTENSA_ASM2 is causing system crash on intel_s1000
* :github:`11022` - mempool allocator is broken
* :github:`11019` - About printk configuration
* :github:`11016` - nRF52840-PCA10056/59: Cannot bring up HCI0 when using HCI_USB sample
* :github:`11008` - net-related logging: Don't see the expected output + crash in logging
* :github:`11007` - logging: "log_strdup pool empty!" is confusing
* :github:`10994` - upsquared sample gpio_counter sample.yaml is malformatted/sanitycheck doens't build samples/boards/up_squared/gpio_counter
* :github:`10993` - upsquared sample gpio_counter doesnt build
* :github:`10990` - ext: lib: crypto: mbedTLS: warning in platform.h glue
* :github:`10971` - net: ipv6: Zephyr replies to ICMPv6 packets with incorrect checksums
* :github:`10970` - net: ipv6: Zephyr replies to malformed ICMPv6 message
* :github:`10967` - FRDM-K64F and MCR-20A configuration does not work
* :github:`10961` - net: ipv6: Zephyr replies to a packet with organization local multicast destination address
* :github:`10960` - net: ipv6: Zephyr replies to a packet with site-local multicast destination address
* :github:`10959` - net: ipv6: Zephyr replies to a packet with interface-local multicast destination
* :github:`10958` - net: ipv6: Zephyr replies to a packet with destination multicast with scope zero
* :github:`10955` - I can't see the output using the minicom when flash on STM32F429IGT6
* :github:`10952` - Bluetooth Mesh: Disabling "CONFIG_BT_MESH_DEBUG_ACCESS" leads to wrong OpCode attached to sent messages
* :github:`10917` - Convert iwdg_stm32 STM32 Watchdog driver to new API
* :github:`10916` - Convert CMSDK APB Watchdog driver to new API
* :github:`10914` - Convert Atmel SAM0 Watchdog driver to new API
* :github:`10913` - Convert Atmel SAM Watchdog driver to new API
* :github:`10909` - Missing stm32l433.dtsi file
* :github:`10899` - nRF52840 DevKit generates USB TX timeout when using HCI_USB
* :github:`10894` - Add dts support to SPI slave drivers & tests
* :github:`10882` - Web instructions: quotes are wrong in build chain for Windows
* :github:`10878` - errno for error codes for key management break on newlib
* :github:`10845` - ext: hal: nxp: Use ARRAY_SIZE helper macro
* :github:`10825` - The DesignWare SPI driver should be hidden on unsupported platforms
* :github:`10817` - Getting Started Guide script error
* :github:`10811` - crash while connecting a USB cable
* :github:`10803` - Compiler warning at /kernel/sched.c
* :github:`10801` - Compiler warning in soc/xtensa/esp32/include/_soc_inthandlers.h
* :github:`10788` - riscv_machine_timer driver don't follow the spec ?
* :github:`10775` - Build error in zephyr.git/include/toolchain/gcc.h leading to shippable failures
* :github:`10772` - tests/drivers/adc/adc_api results into build failure on nRF platforms
* :github:`10760` - Flash Shell compilation error without BT
* :github:`10758` - [Coverity CID :188881]Control flow issues in /samples/net/zperf/src/zperf_shell.c
* :github:`10757` - [Coverity CID :188882]Incorrect expression in /tests/kernel/interrupt/src/nested_irq.c
* :github:`10756` - [Coverity CID :188883]Memory - illegal accesses in /subsys/bluetooth/host/hci_core.c
* :github:`10755` - [Coverity CID :188885]Error handling issues in /lib/ring_buffer/ring_buffer.c
* :github:`10754` - [Coverity CID :188886]Error handling issues in /tests/posix/common/src/mqueue.c
* :github:`10753` - [Coverity CID :188887]Error handling issues in /tests/posix/common/src/pthread.c
* :github:`10752` - [Coverity CID :188888]Incorrect expression in /tests/kernel/interrupt/src/nested_irq.c
* :github:`10751` - [Coverity CID :188889]Memory - illegal accesses in /subsys/bluetooth/host/gatt.c
* :github:`10750` - [Coverity CID :188890]Insecure data handling in /drivers/watchdog/wdt_qmsi.c
* :github:`10747` - tests/kernel/tickless/tickless_concept fails on ARC boards
* :github:`10733` - Logger thread should be started earlier than _init_static_threads() in init.c
* :github:`10720` - SSD1673 driver it setting a Kconfig symbol in dts_fixup.h
* :github:`10707` - tests/benchmarks/latency_measure fails on sam_e70_xplained
* :github:`10693` - samples/subsys/usb/console: Console routed to UART of no USB
* :github:`10685` - Missing # for else in segger conf
* :github:`10678` - nRF52840: hci_usb timeout during initialization by bluez
* :github:`10672` - nRF52: UARTE: uart_fifo_fill return value unreliable
* :github:`10671` - Callback function le_param_req() is never called.
* :github:`10668` - nRF52832: RTT vs. Bluetooth
* :github:`10662` - "Quick start - Integration testing" does not work as intended.
* :github:`10658` - Cannot use SPI_0 on nRF52810
* :github:`10649` - bug when build project which new c lib
* :github:`10639` - make run for native_posix has dependency issues
* :github:`10636` - quark_se is missing SPI_DRV_NAME symbol
* :github:`10635` - tests/kernel/mem_pool/mem_pool_api#test_mpool_alloc_timeout crashes on qemu_riscv32
* :github:`10633` - tests/kernel/mem_protect/syscall crashes on freedom_k64f, sam_e70_xplained and nrf52_pca10040
* :github:`10632` - tests/kernel/poll faults on freedom_k64f, sam_e70_xplained and nrf52_pca10040
* :github:`10623` - tests/drivers/watchdog/wdt_basic_api fails on Quark platforms
* :github:`10619` - [Zephyr] Firmware upgrade through UART [Nordic 52840 - pca 10056 Board]
* :github:`10617` - net: shell: Converting net and wifi shells to new shell breaks samples/net/wifi app for SimpleLink WiFi driver
* :github:`10611` - STM32L4 GPIOC Missing Interrupt support? (nucleo_l476rg)
* :github:`10610` - i2s driver for stm still uses old logger
* :github:`10609` - RISC-V timer incorrectly written
* :github:`10600` - [Coverity CID :174409]Memory - illegal accesses in /ext/hal/ti/simplelink/source/ti/drivers/net/wifi/source/fs.c
* :github:`10599` - [Coverity CID :174410]Memory - corruptions in /ext/hal/ti/simplelink/source/ti/drivers/net/wifi/source/wlan.c
* :github:`10598` - [Coverity CID :174412]Incorrect expression in /ext/hal/ti/simplelink/source/ti/drivers/net/wifi/source/wlan.c
* :github:`10597` - [Coverity CID :174414]Error handling issues in /ext/hal/ti/simplelink/source/ti/drivers/net/wifi/source/driver.c
* :github:`10596` - [Coverity CID :174417]Code maintainability issues in /ext/hal/ti/simplelink/source/ti/drivers/net/wifi/source/device.c
* :github:`10595` - [Coverity CID :174418]Error handling issues in /ext/hal/ti/simplelink/source/ti/drivers/net/wifi/source/fs.c
* :github:`10594` - [Coverity CID :188729]Uninitialized variables in /subsys/bluetooth/shell/bt.c
* :github:`10593` - [Coverity CID :188730]Memory - corruptions in /drivers/sensor/lis2dh/lis2dh_trigger.c
* :github:`10592` - [Coverity CID :188731]Memory - illegal accesses in /ext/hal/ti/simplelink/source/ti/drivers/net/wifi/source/driver.c
* :github:`10591` - [Coverity CID :188732]Integer handling issues in /subsys/net/lib/http/http_server.c
* :github:`10590` - [Coverity CID :188733]Error handling issues in /drivers/sensor/lis2dh/lis2dh_trigger.c
* :github:`10589` - [Coverity CID :188734]Control flow issues in /drivers/sensor/lis2dh/lis2dh.c
* :github:`10588` - [Coverity CID :188735]Uninitialized variables in /subsys/bluetooth/shell/bt.c
* :github:`10587` - [Coverity CID :188738]Memory - illegal accesses in /subsys/bluetooth/host/conn.c
* :github:`10586` - [Coverity CID :188739]Program hangs in /tests/posix/common/src/posix_rwlock.c
* :github:`10585` - [Coverity CID :188740]Error handling issues in /drivers/sensor/isl29035/isl29035_trigger.c
* :github:`10584` - [Coverity CID :188741]Control flow issues in /subsys/shell/shell_utils.c
* :github:`10583` - [Coverity CID :188742]Incorrect expression in /subsys/net/ip/connection.c
* :github:`10582` - [Coverity CID :188743]Program hangs in /tests/posix/common/src/posix_rwlock.c
* :github:`10581` - [Coverity CID :188744]Memory - corruptions in /drivers/sensor/lis2dh/lis2dh_trigger.c
* :github:`10580` - [Coverity CID :188745]Error handling issues in /drivers/wifi/winc1500/wifi_winc1500.c
* :github:`10579` - [Coverity CID :188747]Security best practices violations in /subsys/shell/shell.c
* :github:`10578` - [Coverity CID :188748]Memory - corruptions in /subsys/bluetooth/host/gatt.c
* :github:`10577` - [Coverity CID :188749]Null pointer dereferences in /subsys/bluetooth/controller/ll_sw/ctrl.c
* :github:`10576` - [Coverity CID :188750]Memory - illegal accesses in /subsys/bluetooth/shell/bt.c
* :github:`10575` - [Coverity CID :188752]Security best practices violations in /subsys/shell/shell.c
* :github:`10574` - [Coverity CID :188753]Incorrect expression in /subsys/net/ip/connection.c
* :github:`10573` - [Coverity CID :188754]Control flow issues in /samples/basic/userspace/shared_mem/src/enc.c
* :github:`10572` - [Coverity CID :188755]Control flow issues in /subsys/shell/shell_utils.c
* :github:`10571` - [Coverity CID :188756]Memory - corruptions in /drivers/sensor/lis2dh/lis2dh_trigger.c
* :github:`10570` - [Coverity CID :188757]Memory - illegal accesses in /subsys/bluetooth/shell/bt.c
* :github:`10569` - [Coverity CID :188758]Incorrect expression in /drivers/wifi/winc1500/wifi_winc1500.c
* :github:`10568` - [Coverity CID :188759]Integer handling issues in /subsys/net/l2/ethernet/gptp/gptp_mi.c
* :github:`10567` - [Coverity CID :188760]Error handling issues in /drivers/wifi/winc1500/wifi_winc1500.c
* :github:`10537` - nRF52 regression: random resets noted across several boards / use cases
* :github:`10535` - Failure on tests/posix/common/ due to uninitialized memory access
* :github:`10531` - Default idle thread stack sizes too small when OS controlled power management is used
* :github:`10524` - PM_CONTROL_OS doesn't work in combination with certain I2C drivers in a single thread context
* :github:`10518` - drivers/modem/modem_receiver.c needs LOG_MODULE_REGISTER(mdm_receiver)
* :github:`10517` - Compatibility problem with increased BLE Tx buffers
* :github:`10515` - tests/benchmarks/timing_info faults on ARM platforms
* :github:`10509` - tests/kernel/interrupt is failing on NRFx boards
* :github:`10508` - tests/posix/common fails randomly on all platforms
* :github:`10493` - native_posix: Warnings during link on orphan sections after #10368
* :github:`10476` - kernel/threads/thread_apis tests crashes on ARM boards
* :github:`10475` - tests/kernel/threads/dynamic_thread test cases are failing on ARM boards
* :github:`10474` - tests/kernel/pipe/pipe test cases are failing on ARM boards
* :github:`10473` - tests/kernel/mem_protect/mem_protect tests are failing on ARM boards
* :github:`10460` - Bluetooth: settings: No space to store CCC config after successful pairing
* :github:`10453` - dma_stm32 driver has been broken by commit 07ff2d5
* :github:`10444` - tests/subsys/logging/log_core fails on few ARM platforms
* :github:`10439` - Logger calls will execute functions even though the LOG_LEVEL is masked
* :github:`10428` - logging: Weird output from log_strdup()
* :github:`10420` - gcc: "Exec format error" - WSL in Windows 10 1803
* :github:`10415` -  logging: Unaligned memory access in log_free
* :github:`10413` - Shell: trying to browse history freezes shell on disco_l475_iot1
* :github:`10402` - Crash with new logger and with new shell
* :github:`10389` - Conversion of net core to new logger breaks WiFi driver builds
* :github:`10382` - samples/sensor/apds9960/ results into build failure on multiple platforms.
* :github:`10369` - Logger crashes shell when boot banner is enabled
* :github:`10348` - valgrind detected issue in logger, during msg free
* :github:`10345` - The OpenAMP remote build is for wrong board
* :github:`10344` - SPI Chip Select usage is not unambiguous
* :github:`10329` - SystemView overflow event
* :github:`10320` - arm: mpu: mpu_config and mpu_regions to be declared/defined as const
* :github:`10318` - It is not documented what YAML bindings do
* :github:`10316` - net: sockets: Close doesn't unblock recv
* :github:`10313` - net: sockets: Packets are leaked on TCP abort connection
* :github:`10312` - Bluetooth: settings: CCC not stored on device reset
* :github:`10271` - Unexpected Kconfig warnings during documentation build
* :github:`10243` - native_posix linking issues under Ubuntu 18.04 after upgrade
* :github:`10241` - scripts/requirements.txt needs updating for west
* :github:`10234` - There is one too many "Shields" entry in the root Kconfig menu
* :github:`10231` - zephyr supported targets fail to flash with "ImportError: No module named 'colorama'"
* :github:`10207` - Shell should accept either CR or LF as line delimiter
* :github:`10204` - net: ipv6: corrupted UDP header after forwarding over 6lo iface
* :github:`10195` - Shell dereferences invalid pointer when printing demo command help
* :github:`10192` - SHELL asserts when pressing tab
* :github:`10191` - The new shell uses CONSOLE kconfig options, even though it does not use CONSOLE.
* :github:`10190` - The new shell can only be compiled on boards with SERIAL but it does not set the dependency in its Kconfig
* :github:`10186` - GPIO callback disable has no effect due to _gpio_fire_callbacks
* :github:`10164` - logger sample fails on qemu_xtensa due to lack of backend
* :github:`10152` - gitlint complains of apostrophe in user name
* :github:`10146` - [bluetooth][PTS] Getting Connection failed to be established occasionally
* :github:`10143` - Why does BT_SETTINGS require PRINTK?
* :github:`10137` - sample/basic/button should configure expected pin configuration
* :github:`10134` - sensor: vl53l0x: Warning message when building in ext/hal/st/lib/sensor/vl53l0x
* :github:`10130` - sanitycheck errors with "not well-formed text" warning
* :github:`10127` - Cannot use new shell with native_posix
* :github:`10102` - /tests/subsys/logging/log_core compilation fails on nios2 platform
* :github:`10096` - [Coverity CID :188167] Concurrent data access violations in /lib/posix/pthread.c
* :github:`10095` - [Coverity CID :188168] Concurrent data access violations in /lib/posix/pthread.c
* :github:`10094` - [Coverity CID :188169] Null pointer dereferences in /subsys/net/ip/rpl.c
* :github:`10093` - [Coverity CID :188170] Concurrent data access violations in /lib/posix/pthread.c
* :github:`10092` - [Coverity CID :188171] Null pointer dereferences in /lib/cmsis_rtos_v1/cmsis_thread.c
* :github:`10091` - [Coverity CID :188172] Null pointer dereferences in /subsys/net/ip/route.c
* :github:`10090` - [Coverity CID :188173] Null pointer dereferences in /subsys/net/ip/route.c
* :github:`10089` - [Coverity CID :188174] Control flow issues in /tests/crypto/mbedtls/src/mbedtls.c
* :github:`10058` - The test mem_pool_threadsafe sporadically hangs forever
* :github:`10055` - nRF52: MPU Fault issue
* :github:`10054` - Logger thread spins forever if there is no backend
* :github:`10043` - RISCv32 qemu configuration does not work with upstream qemu
* :github:`10038` - Inversed logic in fade_led sample for nRF boards
* :github:`10037` - fade_led sample doesn't work with pwm_nrfx driver
* :github:`10035` - nrfx PWM driver breaking API contract
* :github:`10034` - Possible regression of #8815 ("Nordic: Directly accessing GPIOTE might create unstable firmware"...)
* :github:`10028` - MCUBoot using W25Q SPI Flash not working (use of Zephyr semaphore)
* :github:`10026` - undefined reference to '__log_current_const_data_get' when using LOG_MODULE_DECLARE
* :github:`10013` - MISRA C - Review the use of memcpy, memcmp and memmove
* :github:`10012` - MISRA-C Do not use feature from stdarg.h
* :github:`10003` - Bluetooth: Identity creation is incomplete through vendor HCI
* :github:`9993` - Few error codes in POSIX API implementation is not supported
* :github:`9992` - cmake compiler cache failures in CI
* :github:`9975` - tests/lib/mem_alloc fails to build on Arduino Due after SoC move
* :github:`9972` - Porting to a new architecture needs to be documented
* :github:`9971` - doc: DT: use-prop-name is not documented
* :github:`9970` - Native posix port drivers need to be documented
* :github:`9960` - Stack check test fails - qemu_x86:tests/kernel/fatal/kernel.common.stack_protection
* :github:`9956` - Build failed when CONFIG_STM32_ARM_MPU_ENABLE=y
* :github:`9954` - samples/hello_world build failed on Windows/MSYS
* :github:`9953` - wrong behavior in pthread_barrier_wait()
* :github:`9949` - Make West launcher scripts in mainline zephyr compatible with multi- and mono-repo
* :github:`9936` - docs: Ring Buffers docs are hidden away under unexpected title
* :github:`9935` - sockets: Connect always binds the context to the default interface
* :github:`9932` - Invalid documentation link in README.rst
* :github:`9926` - samples/net/sockets/echo_client/server: No docs for testing TLS mode
* :github:`9924` - Segger Systemview + newlib does not compile
* :github:`9922` - Networking: Neighbour Discovery may be breaking an ongoing UDP Transmission
* :github:`9912` - Group posix tests
* :github:`9901` - Default nrfx PWM interrupt prio results in assert
* :github:`9892` - MISRA C Avoid dynamic memory allocation
* :github:`9879` - Portability: Arithmetic on void pointers
* :github:`9867` - Bluetooth LESC debug keys support (BT_USE_DEBUG_KEYS) is broken
* :github:`9861` - samples/subsys/usb/hid/  test hangs on quark_se_c1000_devboard
* :github:`9849` - samples/drivers/i2c fails with error writing to FRAM sensor
* :github:`9843` - tests/kernel/sched/deadline fails on NRF5x boards
* :github:`9830` - ASSERTION FAILURE : /tests/drivers/adc/adc_api fails on arduino_101 and quark_se_c1000 platforms
* :github:`9816` - DHT driver fetch fail
* :github:`9812` - drivers: eth: gmac: Fix incorrect cache coherency handling code
* :github:`9777` - tests/kernel/mem_pool/mem_pool_api:mpool_alloc_timeout crashes on qemu_riscv32 with boot delay
* :github:`9767` - [Coverity CID :187903] Uninitialized variables in /subsys/fs/nvs/nvs.c
* :github:`9765` - [Coverity CID :187905] Incorrect expression in /arch/arc/core/thread.c
* :github:`9763` - samples/net/http_client: Failed to send Head requrest
* :github:`9749` - NRF52 : NFFS file system : use of write function returns 0 but fails
* :github:`9743` - tests/posix/fs crashes with BUS FAULT on nRF52
* :github:`9741` - tests/kernel/spinlock:kernel.multiprocessing.spinlock_bounce crashing on ESP32
* :github:`9720` - samples\bluetooth\mesh_demo crash with real payload
* :github:`9708` - tests/kernel/tickless/tickless_concept fails on nRF5x with 1000msec boot delay
* :github:`9704` - tests/lib/mem_alloc/testcase.yaml#libraries.libc.newlib @ minnowboard:x86 results into exception
* :github:`9703` - tests/kernel/threads/no-multithreading: kernel.threads.no-multithreading failing on nrf52_pca10040 and qemu_x86 with boot delay
* :github:`9695` - Deprecated configurations around 'SPI_CS_GPIO'
* :github:`9689` - Multiple tests are failing on sam_e70_xplained once the cache is enabled
* :github:`9666` - tests/benchmarks/timing_info/testcase.yaml#benchmark.timing.default_kernel crashes on Arduino 101 / ARC
* :github:`9656` -  tests/kernel/fp_sharing failing on sam_e70_xplained
* :github:`9653` - tests/lib/mem_alloc/testcase.yaml#libraries.libc.newlib @ esp32:xtensa BUILD failed
* :github:`9651` - Shell_module@mimxrt1050_evk runs failure on R1.13_RC1
* :github:`9650` - latency_measure@mimxrt1050_evk meets hard fault for R1.13 RC1
* :github:`9590` - Template with C linkage in util.h:41
* :github:`9587` - System stack usage analysis code seems to be broken
* :github:`9582` - Cannot find g++ when CONFIG_CPLUSPLUS is set to y
* :github:`9560` - Failed test: mem_protect/usespace failed in nsim_sem and em_starterkit_em7d_v22
* :github:`9510` - zephyr/doc/security/security-overview.rst needs update
* :github:`9509` - Unable to upload firmware over serial with mcumgr
* :github:`9498` - Invalid argument saved on IRQ_CONNECT
* :github:`9463` - bt_le_scan_start Fails with Error -5 after 128 scan start/stop cycles
* :github:`9432` - Overriding 'LOG_LEVEL' could crash the firmware
* :github:`9411` - checkpatch.pl generates warning messages during execution for tests/kernel/static_idt/src/main.c
* :github:`9340` - Failed test: kernel.common.errno.thread_context on em_starterkit_em7d_v22 and minnowboard
* :github:`9290` - [Coverity CID :187325] Control flow issues in /samples/boards/nrf52/mesh/onoff_level_lighting_vnd_app/src/mesh/device_composition.c
* :github:`9289` - [Coverity CID :187326] Control flow issues in /samples/boards/nrf52/mesh/onoff_level_lighting_vnd_app/src/mesh/device_composition.c
* :github:`9174` - STM32L4 I2C read polling hang on NACK in stm32_i2c_msg_read()
* :github:`9076` - doc: correct SMP server sample imgtool.py sign usage
* :github:`9043` - New logging subsystem's timestamps wrap a little before the 3-minute mark
* :github:`9015` - eth_sam_gmac driver (and BOARD=sam_e70_xplained using it) sets unduly high memory requirements on the IP stack overall
* :github:`9003` - [Coverity CID :187062] Incorrect expression in /samples/net/sockets/echo_server/src/udp.c
* :github:`8999` - [Coverity CID :187067] Memory - corruptions in /subsys/logging/log_output.c
* :github:`8988` - [Coverity CID :187079] Integer handling issues in /subsys/net/l2/ethernet/gptp/gptp.c
* :github:`8979` - Failed test:  tests/subsys/dfu/mcuboot/dfu.bank_erase failing on nrf52840_pca10056
* :github:`8915` - STM32 I2C hang
* :github:`8914` - Failed test: net.app.no-ipv6 (/tests/net/app/) on sam_e70_xplained
* :github:`8869` - uart: Problems with interrupt-driven UART in QEMU and some hw boards
* :github:`8838` - hello_world not working on frdm_k64f/qemu_cortex_m3 with newlib and arm gcc embedded 2018q2
* :github:`8810` - Cannot flash board stm32f4_disco
* :github:`8804` - esp32 cannot get character from UART port
* :github:`8796` - Bluetooth: controller: assert on conn update in slave role under max. throughput usecases
* :github:`8789` - tests/ztest/test/mock fails to complete on max10/nios2
* :github:`8746` - net_app DTLS Client net/pkt ERR log when doing handshake
* :github:`8684` - driver: i2c_mcux: unable to perform more than one write transfer like i2c_burst_write
* :github:`8683` - issue with nrf52840_pca10040 and peripheral sample
* :github:`8631` - memory leak in mbedtls using net_app DTLS client
* :github:`8614` - cmake: Zephyr wrapped functions does not allow keywords on zephyr_link_libraries
* :github:`8470` - Broken Arduino 101 Bluetooth Core flashing
* :github:`8409` - Pin interrupt not handled when two pin ints fires in quick succession
* :github:`8376` - DTS: 'boolean' type value was defined as True, not 1
* :github:`8374` - arm: core: cortex_m: wrongly assumes double precision FPU on Cortex-M7
* :github:`8348` - sanitycheck: localized make error message leads to false "passed" result
* :github:`8339` - crypto: drivers use cipher_aead_pkt.tag differently
* :github:`8208` - tests/crypto/ecc_dsa hangs in montecarlo_signverify test on nrf51_pca10028:arm
* :github:`8197` - kernel.memory_protection.stack_random: Stack pointer randomization fails on em_starterkit_em7d_v22
* :github:`8190` - scheduler: thread priorities need to be cleaned up
* :github:`8187` - QEMU serial output is not reliable, may affect SLIP and thus network testing
* :github:`8160` - BUILD_ASSERT doesn't work outside gcc
* :github:`8159` - tests/kernel/fifo/fifo_timeout fails on nrf51_pca10028 and nrf52_pca10040
* :github:`8131` - net_if_tx sends a 0 Reference counter(pkt->ref == 0) packet
* :github:`8116` - tests/kernel/profiling/profiling_api fails to complete on minnowboard
* :github:`8115` - tests/kernel/xip crasshes on minnowboard
* :github:`8108` - make 'rom_report' misreports _sw_isr_table
* :github:`8104` - open-amp: enable C library cause open-amp\open-amp\lib\common\sh_mem.c compile error
* :github:`8097` - tests/drivers/watchdog/wdt_basic_api fails on Quark SE / x86 and esp32
* :github:`8057` - samples/net/: Experiencing the delayed response from zephyr networking stack
* :github:`7999` - HCI UART with Linux host cannot connect to nrf52 6lowpan peripheral
* :github:`7986` - The scripts (debug, debugserver and flash) are not working for Intel S1000 board
* :github:`7818` - big_http_download stuck during download on qemu_x86
* :github:`7817` - Confusing macros: SECONDS vs K_SECONDS, MSEC vs K_MSEC
* :github:`7760` - cmake failure using ExternalProject and dependencies w/ninja
* :github:`7706` - ARM NXP hardware stack overflows do not report _NANO_ERR_STACK_CHK_FAIL or provide MPU fault information
* :github:`7638` - get FAULT when fuzzing sys_ring_buf\_ put and sys_ring_bug_get APIs
* :github:`7510` - [Coverity CID :185395] Memory - corruptions in /samples/net/mbedtls_sslclient/src/mini_client.c
* :github:`7441` - newlib support in zephyr is untested and very broken
* :github:`7409` - Networking examples crash on optimization levels different than -Os
* :github:`7390` - pinmux subsystem API is undocumented and does not enforce validation
* :github:`7381` - gpio_mcux driver needs to validate pin parameters
* :github:`7291` - intermittent issue with tests/kernel/fatal
* :github:`7196` - kernel: CONFIG_INIT_STACKS : minor documentation & dependency update
* :github:`7193` - tickless and timeslicing don't play well together
* :github:`7179` - _vprintk incorrectly prints 64-bit values
* :github:`7048` - Tickless Kernel Timekeeping Problem
* :github:`7013` - cleanup device tree warnings on STM32
* :github:`6874` - Not able to join OpenThread BorderRouter or a ot-ftd-cli network
* :github:`6866` - build: requirements: No module named yaml and elftools
* :github:`6696` - [Coverity CID: 183036] Control flow issues in /drivers/gpio/gpio_sam.c
* :github:`6695` - [Coverity CID: 183037] Memory - corruptions in /samples/net/mbedtls_dtlsclient/src/dtls_client.c
* :github:`6666` - [Coverity CID: 183066] Error handling issues in /tests/kernel/mbox/mbox_api/src/test_mbox_api.c
* :github:`6585` - kernel: re-delete event list node [bug]
* :github:`6452` - Jumbled Console log over USB
* :github:`6365` - "dummy-flash" device tree bug
* :github:`6319` - Missing documentation for zephyr API to query kernel version: sys_kernel_version_get
* :github:`6276` - assert when running sys_kernel on disco_l475_iot1 (with asserts enabled)
* :github:`6226` - IRC-bot sample eventually stops responding to IRC commands
* :github:`6180` - CONFIG_IS_BOOTLOADER is poorly named and documented
* :github:`6147` - "ninja flash" cannot be used with DFU-capable applications
* :github:`5781` - Initial TLS connection failure causes TLS client handler to stop and fail endlessly w/ EBUSY
* :github:`5735` - mpu_stack_guard_test fails on many arm platforms, including qemu
* :github:`5734` - samples/drivers/crypto fails on qemu_nios2
* :github:`5702` - usb subsystem doc not pulling from header doxygen comments
* :github:`5678` - USB: DW driver does not work properly with mass storage class
* :github:`5634` - The dependency between the Kconfig source files and the Kconfig output is missing
* :github:`5605` - Compiler flags added late in the build are not exported to external build systems
* :github:`5603` - Bluetooth logging is hardcoded with level 4 (debug)
* :github:`5485` - tests: kernel/xip: CONFIG_BOOT_DELAY issue for qemu_riscv32
* :github:`5426` - [kconfig] Allow user-input when new options are detected
* :github:`5411` - Secondary repos are missing licenses
* :github:`5387` - Many of the samples using mbedtls_ssl_conf_psk() dont check for error
* :github:`5376` - No way to get clock control subsystem type
* :github:`5343` - cmake: libapp.a in unexpected location
* :github:`5289` - IPv6 over BLE: IPSP sample crashes and ble controller gets disconnected
* :github:`5244` - UART continuous interrupt
* :github:`5226` - Compiling with -O0 causes the kobject text area to exceed the limit (linker error)
* :github:`5006` - syscalls: properly fix how unit testing deals with __syscall
* :github:`4977` - USB: documentation needs to be updated
* :github:`4927` - Hard Fault when no device driver is setup for ENTROPY results
* :github:`4888` - LwM2M: Fix BT device pending / retry errors
* :github:`4836` -  connection disconnected due to Le timeout(0x22)
* :github:`4682` - http_server example fails for ESP32
* :github:`4324` - samples/net/http_client: error in detecting and processing the message received
* :github:`4082` - remove _current NULL check in ARM's _is_thread_user()
* :github:`4042` - net: NET_CONN_CACHE relevant
* :github:`4002` - Can not compile C++ project without setting -fpermissive
* :github:`3999` - qemu_xtensa crash while running sample/net/sockets/echo with ipv6 enabled
* :github:`3997` - *** ERROR *** pkt 0x2000c524 is freed already by prepare_segment():388 (mqtt_tx_publish():242)
* :github:`3906` - Static code scan (coverity CID: 151975, 173645, 173658 ) issues seen
* :github:`3847` - Fix LwM2M object firmware pull block transfer mode to select size via interface type (BT, ethernet, etc)
* :github:`3796` - Multiple issues with http_server library design and implementation
* :github:`3670` - ARC: timeslice not reset on interrupt-induced swap
* :github:`3669` - xtensa: timeslice not reset for interrupt-induced swaps
* :github:`3606` - _NanoFatalErrorHandler and other internal kernel APIs are inconsistently defined
* :github:`3524` - Bluetooth data types missing API documentation
* :github:`3522` - New Zephyr-defined types missing API documentation
* :github:`3464` - xtensa hifi_mini SOC does not build
* :github:`3375` - Debugging difficulties on Cortex-M with frame pointer missing
* :github:`3288` - Fatal fault in SPI ISR when using multiple interfaces
* :github:`3244` - Ataes132a fails to encrypt/decrypt with mode ECB and CCM mode
* :github:`3226` - ATT channel gets disconnected on ATT timeout but GATT API doesn't check for it
* :github:`3198` - ARC: Boot_time test not functioning
* :github:`3132` - frdm_k64f: Sometimes, there may be 1000+ms roundtrip for pings and other Ethernet packets (instead of sub-ms)
* :github:`3129` - frdm_k64f: Ethernet may become stuck in semi-persistent weird state, not transferring data, potentially affecting host Ethernet
* :github:`2984` - frdm_k64f bus exception bug due to peculiar RAM configuration
* :github:`2907` - make menuconfig .config easily overwritten
* :github:`2627` - LE L2CAP CoC transfers less octets that claims to be
* :github:`2108` - Stack alignment on ARM doesn't always follow Procedure Call Standard
* :github:`1677` - sys_*_bit and sys_bitfield_*_bit APIs are not implemented on ARM
* :github:`1550` - problem with pci_bus_scan resulting in an endless loop
* :github:`1533` - It is not documented how to discover, install, and use external toolchains
* :github:`1495` - esp32: newlibc errors
* :github:`1429` - LWM2M configs not defined
* :github:`1381` - HMC5883L driver config error
* :github:`729` - TCP SYN backlog change likely has concurrent global var access issues
