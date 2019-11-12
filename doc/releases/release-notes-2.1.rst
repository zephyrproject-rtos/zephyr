:orphan:

.. _zephyr_2.1:

Zephyr 2.1.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr kernel version 2.1.0.

Major enhancements with this release include:

* TBD

The following sections provide detailed lists of changes by component.

Kernel
******

* TBD

Architectures
*************

* TBD

Boards & SoC Support
********************

* TBD

Drivers and Sensors
*******************

* TBD

Networking
**********

* Added new TCP stack implementation. The new TCP stack is still experimental
  and is turned off by default. Users wanting to experiment with it can set
  :option:`CONFIG_NET_TCP2` Kconfig option.
* Added support for running MQTT protocol on top of a Websocket connection.
* Added infrastructure for testing network samples in automatic way.
* Added support for enabling DNS in LWM2M.
* Added support for resetting network statistics in net-shell.
* Added support for getting statistics about the time it took to receive or send
  a network packet.
* Added support for sending a PPP Echo-Reply packet when a Echo-Request packet
  is received.
* Added CC13xx / CC26xx device drivers for IEEE 802.15.4.
* Added TCP/UDP socket offload with TLS for eswifi network driver.
* Added support for sending multiple SNTP requests to increase reliability.
* Added support for choosing a default network protocol in socket() call.
* Added support for selecting either native IP stack, which is the default, or
  offloaded IP stack. This can save ROM and RAM as we do not need to enable
  network functionality that is not going to be used in the network device.
* Added support for LWM2M client initiated de-register.
* Updated the supported version of OpenThread.
* Updated OpenThread configuration to use mbedTLS provided by Zephyr.
* Various fixes to TCP connection establishment.
* Fixed delivery of multicast packets to all listening sockets.
* Fixed network interface initialization when using socket offloading.
* Fixed initial message id seed value for sent CoAP messages.
* Fixed selection of network interface when using "net ping" command to send
  ICMPv4 echo-request packet.
* Networking sample changes for:

  .. rst-class:: rst-columns

     - http_client
     - dumb_http_server_mt
     - dumb_http_server
     - echo_server
     - mqtt_publisher
     - zperf

* Network device driver changes for:

  .. rst-class:: rst-columns

     - Ethernet enc424j600 (new driver)
     - Ethernet enc28j60
     - Ethernet stm32
     - WiFi simplelink
     - Ethernet DesignWare (removed)

Bluetooth
*********

* TBD

Build and Infrastructure
************************

* Deprecated kconfig functions dt_int_val, dt_hex_val, and dt_str_val.
  Use new functions that utilize eDTS info such as dt_node_reg_addr.
  See :zephyr_file:`scripts/kconfig/kconfigfunctions.py` for details.

* Deprecated direct use of the ``DT_`` Kconfig symbols from the generated
  ``generated_dts_board.conf``.  This was done to have a single source of
  Kconfig symbols coming from only Kconfig (additionally the build should
  be slightly faster).  For Kconfig files we should utilize functions from
  :zephyr_file:`scripts/kconfig/kconfigfunctions.py`.  See
  :ref:`kconfig-functions` for usage details.  For sanitycheck yaml usage
  we should utilize functions from
  :zephyr_file:`scripts/sanity_chk/expr_parser.py`.  Its possible that a
  new function might be required for a particular use pattern that isn't
  currently supported.

* Various parts of the binding format have been simplified. The format is
  better documented now too.

  See :ref:`legacy_binding_syntax` for more information.

Libraries / Subsystems
***********************

* TBD

HALs
****

* TBD


Documentation
*************

* TBD


Tests and Samples
*****************

* TBD

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.0.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`20558` - Build failure for samples/bluetooth/peripheral_hr/sample.bluetooth.peripheral_hr_rv32m1_vega_ri5cy on rv32m1_vega_ri5cy
* :github:`20480` - i2c driver for cc13xx/cc26xx is configured with incorrect frequency
* :github:`20438` - Kernel timeout API does not document well accepted values
* :github:`20422` - Device with bonds should not accept new keys without user awareness
* :github:`20417` - BME280 wrong pressure unit?
* :github:`20406` - misc.app_dev.libcxx test fails to build for qemu_x86_64
* :github:`20371` - Sanitycheck filtering broken
* :github:`20313` - Zperf documentation points to wrong iPerf varsion
* :github:`20310` - SDHC : Could not enable SPI clock on nucleo_f091rc
* :github:`20299` - bluetooth: host: Connection not being unreferenced when using CCC match callback
* :github:`20284` - zephyr-env.sh  Is this supposed to be unsetopt posixargzero ?
* :github:`20274` - Kconfig new libc changes cause echo server cmake error
* :github:`20260` - logging system call
* :github:`20246` - Module Request: hal_unisoc
* :github:`20244` - mesh: demo: BT fails it init
* :github:`20232` - Bluetooth: Kernel panic on gatt discover in shell app
* :github:`20225` - [TOPIC-GPIO] sam_e70_xplained fails 2-pin active-low pull test
* :github:`20224` - [TOPIC-GPIO] rv32m1_vega_ri5cy fails 2-pin double-edge detection test
* :github:`20223` - [TOPIC-GPIO] efr32mg_sltb004a fails 2-pin double-edge detection test
* :github:`20202` - tests/arch/arm/arm_interrupt failed on sam_e70_xplained board.
* :github:`20177` - sanitycheck error with tests/benchmarks/timing_info.
* :github:`20167` - posix clock: unexpected value for CLOCK_REALTIME when used with newlib
* :github:`20163` - doc: storage settings not clear
* :github:`20135` - Bluetooth: controller: split: Missing initialization of master terminate_ack flag
* :github:`20122` - Deadlock in ASAN leak detection on exit
* :github:`20110` - Crash in hci_driver.c when create_connection_cancel is issued after create connection
* :github:`20109` - altera_nios2 support decision required
* :github:`20105` - tests/subsys/fs/fcb/ Using uninitialised memory/variables
* :github:`20086` - Broken-looking duplicated ESPI_XEC symbol
* :github:`20071` - Incompatible pointer types in Nordic Driver
* :github:`20049` - Build warnings in several unit tests
* :github:`20045` - z_sched_abort: sched_spinlock should be released before k_busy_wait
* :github:`20042` - Telnet can connect only once
* :github:`20032` - Make it clear in HTML docs what monospaced text is a link
* :github:`20030` - stm32 can: zcan_frame from fifo uninitialized
* :github:`20022` - sanitycheck is not failing on build warnings
* :github:`20016` - STM32F4: cannot erase sectors from bank2
* :github:`20010` - Cannot flash mimxrt1050_evk board
* :github:`20007` - tests/net/mld failed on mimxrt1050_evk board.
* :github:`19969` - [TOPIC-GPIO] mcux driver problems with pull configuration
* :github:`19963` - settings test tests/subsys/settings/fcb/raw failing
* :github:`19917` - Bluetooth: Controller: Missing LL_ENC_RSP after HCI LTK Negative Reply
* :github:`19915` - tests/net/icmpv6 failed on sam_e70 board.
* :github:`19914` - tests/net/shell failed on sam_e70 board.
* :github:`19910` - Bluetooth: Mesh: Thread stack can reduce by use malloc&free function
* :github:`19898` - CONFIG_NET_ROUTE_MCAST and CONFIG_NET_ROUTING can't be enabled
* :github:`19889` - Buffer leak in GATT for Write Without Response and Notifications
* :github:`19877` - Broken partition size
* :github:`19871` - display/ssd1306: allow "reverse display" in kconfig or dts
* :github:`19867` - modem: ublox-sara-r4/u2 build error
* :github:`19848` - stm32wb MPU failure
* :github:`19841` - MIPI Sys-T logging/tracing support
* :github:`19837` - SS register is 0 when taking exceptions on qemu_x86_long
* :github:`19833` - missing or empty reg/ranges property when trying to build blink_led example
* :github:`19820` - Bluetooth: Host: Unable to use whitelist in peripheral only build
* :github:`19818` - Compiler error for counter example (nRF52_pca10040)
* :github:`19811` - native_posix stack smashing
* :github:`19802` - Zephyr was unable to find the toolchain after update to zephyr version 1.13.0
* :github:`19795` - bt_gatt_attr_next returns first attribute in table for attributes with static storage.
* :github:`19791` - How to use CMSIS DSP Library on nRF52832 running zephyr LTS Version(V1.14) ?
* :github:`19783` - floating point in C++ on x86_64 uses SSE
* :github:`19775` - net_calc_chksum: Use of un-initialized memory on 64 bit targets
* :github:`19769` - CONFIG_FLASH_SIZE should be CONFIG_FLASH_END and specified in hex
* :github:`19767` - Bluetooth: Mesh: Provision Random buffer has too small size
* :github:`19762` - tests/net/lib/tls_credentials failed on sam_e70_xplained board.
* :github:`19759` - z_arch_switch() passed pointer to NULL outgoing switch handle on dummy thread context switch
* :github:`19748` - k_sleep(K_FOREVER) behavior unexpected
* :github:`19734` - "make gdbserver" doesn't work properly for qemu_x86_long
* :github:`19724` - Bluetooth: Mesh: Receiving an access message
* :github:`19722` - Settings: settings_file_save_priv() use of uninitialized variable
* :github:`19721` - samples/bluetooth/ipsp does not respond to pings from Linux
* :github:`19717` - Add provisions for supporting multiple CMSIS variants
* :github:`19701` - mem_pool_threadsafe sporadic failures impacting CI
* :github:`19697` - tests/subsys/fs/fat_fs_api uses unitialized variables
* :github:`19692` - [TOPIC-GPIO] gpi_api_1pin test failures
* :github:`19683` - nrf: clock reimplementation breaks test
* :github:`19678` - Noticeable delay between processing multiple client connection requests (200ms+)
* :github:`19660` - missing file reference in samples/sensor/ti_hdc doc
* :github:`19649` - [TOPIC-GPIO]: Replace GPIO_INT_DEBOUNCE with GPIO_DEBOUNCE
* :github:`19638` - Bluetooth: Mesh: Provisioning Over PB-ADV
* :github:`19629` - tinycbor buffer overflow causing mcumgr image upload failure
* :github:`19612` - ICMPv6 packet is routed to wrong interface when peer is not found in neighbor cache
* :github:`19604` - Bluetooth: ATT does not release all buffers on disconnect
* :github:`19603` - addition to winbond,w25q16.yaml required for SPI CS to be controlled by driver
* :github:`19592` - Request new repository to host the Eclipse plugin for building Zephyr applications
* :github:`19569` - nRF RTC Counter with compile time decision about support of custom top value
* :github:`19560` - Console on CDC USB  crashes when CONFIG_USB_COMPOSITE_DEVICE=y
* :github:`19552` - [TOPIC-GPIO]: Support for legacy interrupt configuration breaks new API contract
* :github:`19550` - drivers/pcie: `pcie_get_mbar()` should return a `void *` not `u32_t`
* :github:`19549` - kernel/mem_protection/stackprot fails on NXP RT series platforms on v1.14.1-rc3 release
* :github:`19544` - make usb power settings in "Configuration Descriptor" setable
* :github:`19543` - net: tcp: echo server stops if CONFIG_POSIX_MAX_FDS is not set
* :github:`19539` - Support MQTT over Websocket
* :github:`19536` - devicetree bindings path misinterpreted
* :github:`19525` -  Can't change the slave latency on a connection.
* :github:`19515` - Bluetooth: Controller: assertion failed
* :github:`19509` - Bluetooth: stm32wb55: Unable to pair with privacy-enabled peer
* :github:`19490` - Bluetooth: split: 'e' assert during disconnect
* :github:`19484` - Bluetooth: split:  bt_set_name() asserts due to flash and radio coex
* :github:`19472` - drivers: usb_dc_stm32:  shows after some time errors and warnings
* :github:`19459` - Bluetooth: Mesh:  Mesh Model State Binding.
* :github:`19456` - arch/x86: make use of z_bss_zero() and z_data_copy()
* :github:`19452` - Bluetooth: Mesh: Mesh model implementation?
* :github:`19447` - SEGGER_RTT.h: No such file or directory
* :github:`19437` - tests/kernel/sched/schedule_api tests fail to build
* :github:`19432` - nrfx: nrf52840_pca10056 SPIM1 cannot be selected without SPIM3
* :github:`19420` - power: system power management sleep duration
* :github:`19415` - typo in nucleo_l496zg.dts
* :github:`19413` - Not able to scan and connect to other ble devices with HCI commands
* :github:`19398` - net: ENC28J60 driver does not respond to ping
* :github:`19385` - compilation error
* :github:`19381` - 'k_yield()' exhibits different behavior with 'CONFIG_SMP'
* :github:`19376` - Build on a ARM host
* :github:`19374` - net: echo server: TCP add support for multiple connections
* :github:`19370` - bugs in kernel/atomic_c
* :github:`19367` - net: TCP/IPv4: TCP stops working after dropping segment with incorrect checksum
* :github:`19363` - arc: bug in _firq_enter
* :github:`19353` - arch/x86: QEMU doesn't appear to support x2APIC
* :github:`19347` - Bluetooth: BL654 USB dongle not found after flashing
* :github:`19342` - Bluetooth: Mesh: Persistent storage of Virtual Addresses
* :github:`19319` - tests/kernel/spinlock only runs on ESP32
* :github:`19317` - need a minimal log implementation that maps to printk()
* :github:`19307` - \_interrupt_stack is defined in the kernel, but declared in arch headers
* :github:`19299` - kernel/spinlock: A SMP race condition in SPIN_VALIDATE
* :github:`19284` - Service Changed indication not being sent in some cases
* :github:`19270` - GPIO: STM32: Migration to new API
* :github:`19267` - Service changed not notified upon reconnection.
* :github:`19265` - Bluetooth: Mesh: Friend Send model message to LPN
* :github:`19263` - Bluetooth: Mesh: Friend Clear Procedure Timeout
* :github:`19250` - NVS: Overwriting an item with a shorter matching item fails
* :github:`19239` - tests/kernel/common failed on iotdk board.
* :github:`19235` - move drivers/timer/apic_timer.c to devicetree
* :github:`19227` - IOTDK uses QMSI DT binding
* :github:`19226` - Device Tree Enhancements in 2.1
* :github:`19219` - drivers/i2c/i2c_dw.c is not 64-bit clean
* :github:`19216` - Ext library for WIN1500: different values of AF_INET
* :github:`19198` - Bluetooth: LL split assert on connect
* :github:`19191` - problem with implementation of sock_set_flag
* :github:`19186` - BLE: Mesh: IVI Initiator When ivi in progress timeout
* :github:`19181` - sock_set_flag implementation in sock_internal.h does not work for 64 bit pointers
* :github:`19178` - Segmentation fault when running echo server
* :github:`19177` - re-valuate commit 0951ce2
* :github:`19176` - NET: LLMNR: zephyr drops IPV4 LLMNR packets
* :github:`19167` - Message queues bug when using C++
* :github:`19165` - zephyr_file generates bad links on branches
* :github:`19164` - compiling native_posix64 with unistd.h & net/net_ip.h fail
* :github:`19144` - arch/x86: CONFIG_BOOT_TIME_MEASUREMENT broken
* :github:`19135` - net: ipv4: udp: echo server sends malformed data bytes in reply to broadcast packet
* :github:`19133` - Scheduler change in #17369 introduces crashes
* :github:`19103` - zsock_accept_ctx blocks even when O_NONBLOCK is specified
* :github:`19098` - Failed to flash on ESP32
* :github:`19096` - No error thrown for device tree node with missing required property of type compound
* :github:`19079` - Enable shield sample on stm32mp157c_dk2
* :github:`19078` - search for board specific shield overlays doesn't always work
* :github:`19066` - Build error with qemu_x86_64
* :github:`19065` - Build error with stm32h747i_disco_m4
* :github:`19064` - Correct docs for K_THREAD_DEFINE
* :github:`19059` - i2c_ll_stm32_v2: nack on write is not handled correctly
* :github:`19051` - [Zephyr v2.0.0 nrf52840] Unable to reconnect to recently bonded peripheral
* :github:`19039` - Bluetooth: Qualification test case GATT/SR/UNS/BI-02-C fails
* :github:`19015` - Bluetooth: Mesh: Node doesn't respond to "All Proxies" address
* :github:`19013` - [Zephyr 1.14]: NetUsb and Ethernet work together
* :github:`19004` - problems in sanitycheck/CI infrastructure revealed by post-release change
* :github:`18999` - assignment in assert in test of arm_thread_arch causes build failures
* :github:`18988` - BLE Central auto enables indications and notifies
* :github:`18986` - DTS: transition from alias to node label as the standard prefix
* :github:`18973` - z_arch_system_halt() does not block interrupts
* :github:`18961` - [Coverity CID :203912]Error handling issues in /samples/net/sockets/coap_client/src/coap-client.c
* :github:`18957` - NET_L2: modem drivers (offloaded) aren't assigned a net_l2 which causes a crash in net_if_up()/net_if_down()
* :github:`18956` - memory protection for x86 dependent on XIP
* :github:`18935` - [Zephyr 1.14] drivers: flash: spi_nor: Problematic write with page boundaries
* :github:`18880` - boards: mec15xxevb_assy6853: consider moving ARCH_HAS_CUSTOM_BUSY_WAIT to SoC definition
* :github:`18873` - zsock_socket() should support proto==0
* :github:`18870` - zsock_getaddrinfo() returns garbage values if IPv4 address is passed and hints->ai_family == AF_INET6
* :github:`18858` - Runner support for stm32flash utility
* :github:`18832` - Doc: contact-us page should use slack invite (not zephyrproject.slack.com)
* :github:`18824` - tests/subsys/usb/device/ failed on sam_e70 board.
* :github:`18816` - ssd1306 driver can't work with lvgl
* :github:`18807` - Support the Ubuntu Cross Toolchain
* :github:`18803` - LTS - support time
* :github:`18787` - arch/x86: retire loapic_timer.c driver in favor of new apic_timer.c
* :github:`18749` - Avenger96 regressed in mainline for U-Boot M4 boot
* :github:`18695` - Watchdog: stm32: Wrong timeout value when watchdog started at boot
* :github:`18657` - drivers/timer/hpet.c should use devicetree, not CONFIG_* for MMIO/IRQ data
* :github:`18652` - Optimization flags from CMAKE_BUILD_TYPE are not taken into account
* :github:`18592` - (nRF51) The RSSI signal does not rise above -44 dBm
* :github:`18585` - STM32G4 support
* :github:`18583` - hci_usb: NRF52840 connecting addtional peripheral fails
* :github:`18540` - MEC1501 ADC is missing in HAL
* :github:`18539` - MEC1501 PWM is missing in HAL
* :github:`18488` - Bluetooth: Mesh: Friend queue message seqnum order
* :github:`18480` - Microchip's MEC1501 HAL is broken (watchdog part)
* :github:`18465` - timeutil_timegm() has undefined behavior
* :github:`18440` - [Coverity CID :203439]Memory - corruptions in /tests/kernel/mem_protect/protection/src/main.c
* :github:`18373` - [Coverity CID :203399]API usage errors in /samples/boards/olimex_stm32_e407/ccm/src/main.c
* :github:`18340` - settings: make NVS the default backend
* :github:`18308` - net: TCP/IPv6 set of fragmented packets causes Zephyr to quit
* :github:`18305` - Native Posix target can not use features with newlib dependencies
* :github:`18297` - Bluetooth: SMP: Pairing issues
* :github:`18282` - tests/kernel/sched/schedule_api/ fails on LPC54114_m4
* :github:`18160` - Cleanup dts compatible for "nxp,kinetis-sim" on nxp_ke1xf
* :github:`18143` - stm32f SPI Slave TX does not work correctly, but occurs OVERRUN err
* :github:`18138` - xtensa arch has two different implementations
* :github:`18105` - BSD socket offload with IPv4 and IPv6 disabled breaks many client-based net samples
* :github:`17998` - STM32 (Nucleo L476RG) SPI pins floating
* :github:`17983` - Bluetooth: Re-establish security before notifications/indications can be sent
* :github:`17949` - stm32 i2c driver has problems with AHB_PRESCALER, APB1_PRESCALER, APB2_PRESCALER
* :github:`17892` - arch/x86: clean up segmentation.h
* :github:`17888` - arch/x86: remove IAMCU ABI support
* :github:`17832` - x86: update mmustructs.h and x86_mmu.c to support long mode
* :github:`17829` - support default property values in devicetree bindings
* :github:`17781` - Question:Is it possible to connect the device on internet using bluetooth connection?
* :github:`17625` - driver: gpio: PCAL9535A: can't write to register (read is possible)
* :github:`17548` - Can't set thread name with k_thread_create prevents useful tracing information
* :github:`17546` - Bluetooth: Central Scan fails continuously if last connect attempt failed to complete
* :github:`17443` - Kconfig: move arch-specific stack sizes to arch trees?
* :github:`17430` - arch/x86: drivers/interrupt_controller/system_apic.c improperly classifies IRQs
* :github:`17361` - _THREAD_QUEUED overlaps with x86 _EXC_ACTIVE in k_thread.thread_state
* :github:`17337` - ArmV7-M mpu sub region alignment
* :github:`17239` - Too many open files crash when running "sanitycheck" with no arguments
* :github:`17234` - CONFIG_KERNEL_ENTRY appears to be superfluous
* :github:`17133` - arch/x86: x2APIC EOI should be inline
* :github:`17104` - arch/x86: fix -march flag for Apollo Lake
* :github:`17064` - drivers/serial/uart_ns16550: CMD_SET_DLF should be removed
* :github:`16900` - Inline assembly in Arm z_arch_switch_to_main_thread missing clobber list
* :github:`16880` - Systematic *-zephyr-eabi/bin/ld: warning: toolchain_is_ok cannot find entry symbol _start; defaulting to 000::00XXXXX
* :github:`16797` - [Zephyr v1.14.0] stm32: MCUboot bootloader results in Hardware exception
* :github:`16791` - build system does not see changes in DTS dependencies
* :github:`16721` - PCIe build warnings from devicetree
* :github:`16599` - drivers: usb_dc_nrfx: unstable handling of hosts suspend/resume
* :github:`16452` - drivers: ethernet: stm32, sam, mcux: LAA bit not set
* :github:`16376` - posix ext: Implement eventfd()
* :github:`16320` - The routing option CONFIG_NET_ROUTING needs clarification
* :github:`16223` - stm32: Unable to send 64 byte packet over control endpoint
* :github:`16167` - Implement interrupt driven GPIO on LPC families
* :github:`16097` - STM32 Ethernet driver should be able to detect the carrier state
* :github:`16032` - Socket UDP: Low transmission efficiency
* :github:`16031` - Toolchain abstraction
* :github:`15912` - add Reject as an option to pull request reviews
* :github:`15906` - WEST ERROR: extension command build was improperly defined
* :github:`15841` - Support AT86RF233
* :github:`15604` - Suspicious PCI and build_on_all default test coverage
* :github:`15603` - Unable to use C++ Standard Library
* :github:`15598` - Standard devicetree connectors for boards
* :github:`15551` - CMake enables -fmacro-prefix-map on GCC 7
* :github:`15494` - 2.0 Release Checklist
* :github:`15359` - The docs incorrectly state that common.dts integrates with mcuboot
* :github:`15323` - blink_led sample does not work on most of the nRF boards
* :github:`15246` - doc: confusion about dtc version
* :github:`15196` - logging: Support for blocking deferred logging
* :github:`15027` - doc: PDF generation broken
* :github:`14906` - USB: NXP Device controller does not pass testusb tests
* :github:`14810` - DT_<Driver Name>_LABEL undeclared
* :github:`14744` - ARM: Core Stack Improvements/Bug fixes for 2.0 release
* :github:`14683` - need end-to-end memory protection samples
* :github:`13725` - drivers: ssd1306: When 128x32 is used, only half of the screen is output.
* :github:`13000` - sanitycheck serializes running tests on ARC simulator
* :github:`12969` - settings: loading key-value pairs for given subtree
* :github:`12965` - POSIX subsys: Need more fine-grained enable options
* :github:`12961` - ARM Memory Protection functions not invoked in SWAP for ARMv6/ARMv8-M Baseline
* :github:`12933` - MCUboot: high current
* :github:`12813` - DTS: CONFIG_FLASH_BASE_ADDRESS not being generated for SPI based Flash chip
* :github:`12703` - how to configure interrupt signals on shields via device tree?
* :github:`12677` - USB: There are some limitations for users to process descriptors
* :github:`12535` - Bluetooth: suspend private address (RPA) rotating
* :github:`12509` - Fix rounding in _ms_to_ticks()
* :github:`12504` - STM32: add USB_OTG_HS example
* :github:`12206` - OpenThread apps want to download and build OpenThread every time!
* :github:`12114` - assertion using nRF5 power clock with BLE and nRF5 temp sensor
* :github:`11743` - logging: add user mode access
* :github:`11717` - qemu_x86 's SeaBIOS clears the screen every time it runs
* :github:`11655` - Alleged multiple design and implementation issues with logging
* :github:`10748` - Work waiting on pollable objects
* :github:`10701` - API: Prefix (aio_) conflict between POSIX AsyncIO and Designware AnalogIO Comparator
* :github:`10503` - User defined USB function & usb_get_device_descriptor()
* :github:`10338` - Add PyLint checking of all python scripts in CI
* :github:`10256` - Add support for shield x-nucleo-idb05a1
* :github:`10149` - Make tickless kernel the default mode
* :github:`9509` - Unable to upload firmware over serial with mcumgr
* :github:`9482` - Enable mpu on lpc54114
* :github:`9249` - Get non ST, STM32 Based boards compliant with default configuration guidelines
* :github:`9248` - Get Olimex boards compliant with default configuration guidelines
* :github:`9245` - Get TI SoC based boards compliant with default configuration guidelines
* :github:`9244` - Get SILABS board compliant with default configuration guidelines
* :github:`9243` - Get NXP SoC based boards compliant with default configuration guidelines
* :github:`9241` - Get ATMEL SoC based boards compliant with default configuration guidelines
* :github:`9240` - Get ARM boards compliant with default configuration guidelines
* :github:`9239` - Get NIOS boards compliant with default configuration guidelines
* :github:`9237` - Get RISCV boards compliant with default configuration guidelines
* :github:`9236` - Get X86 boards compliant with default configuration guidelines
* :github:`9235` - Get XTENSA boards compliant with default configuration guidelines
* :github:`9193` - STM32: Move DMA driver to LL/HAL and get it STM32 generic
* :github:`8758` - All nRF drivers: migrate configuration from Kconfig to DTS
* :github:`7213` - DTS should use (one or more) prefixes on all defines
* :github:`6858` - Default board configuration guidelines
* :github:`6446` - sockets: Accept on non-blocking socket is currently blocking
* :github:`5633` - Optimize the CMake configuration time by reducing the time spent testing the toolchain
* :github:`5138` - dts: boards: provide generic dtsi file for 'generic' boards
* :github:`3981` - ESP32 uart driver does not support Interrupt/fifo mode
* :github:`3877` - Use mbedtls from Zephyr instead of openthread
* :github:`3056` - arch-specific inline functions cannot manipulate \_kernel
* :github:`2686` - Add qemu_cortex_m0/m0+ board.
