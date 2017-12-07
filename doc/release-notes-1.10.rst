:orphan:

.. _zephyr_1.10:

Zephyr Kernel 1.10.0
#####################

We are pleased to announce the release of Zephyr kernel version 1.10.0.

Major enhancements with this release include:

* Initial alpha-quality thread-level memory protection on x86, userspace and memory
  domains
* Major overhaul to the build system and a switch from Kbuild to CMake.
* Newtron Flash Filesystem (NFFS) Support
* Increased testsuite coverage and migrated majority of testcases to use ztest
* Integration with MCUBOOT Bootloader
* Additional SoC, platform and driver support for many of the already supported
  platforms.

The following sections provide detailed lists of changes by component.

Kernel
******

* Remove deprecated k_mem_pool_defrag code
* Initial alpha-quality thread-level memory protection on x86, userspace and memory
  domains:

  * Same kernel & driver APIs for kernel and user mode threads
  * System calls for privilege elevation
  * Stack overflow protection
  * Kernel object and device driver permission tracking
  * Simple app vs. kernel memory separation
  * Memory domain APIs for fine-tuning memory region permissions
  * Stack memory protection from other threads

* Add the following application-facing memory domain APIs:

  * k_mem_domain_init() - to initialize a memory domain
  * k_mem_domain_destroy() - to destroy a memory domain
  * k_mem_domain_add_partition() - to add a partition into a domain
  * k_mem_domain_remove_partition() - to remove a partition from a domain
  * k_mem_domain_add_thread() - to add a thread into a domain
  * k_mem_domain_remove_thread() - to remove a thread from a domain
* add k_calloc() which uses kernel heap to implement traditional calloc()
  semantics.
* Introduce object validation mechanism: All system calls made from userspace,
  which involve pointers to kernel objects (including device drivers), will need
  to have those pointers validated; userspace must never be able to crash the
  kernel by passing it garbage.

Architectures
*************

* nrf52: Add support for LOW_POWER state and SYSTEM_OFF
* Architecture specific memory domain APIs added
* Tickless Kernel Implementation for Xtensa
* Added support for the following ARM SoCs:

  * NXP i.MX RT1052
  * Silabs EFM32WG
  * STM F0
  * TI MSP432P4xx

Boards
******
* Jailhouse port: The port will enable Zephyr to run as a guest OS on x86-64
  systems. It comes with a test on QEMU to validate that, thus this new board
  introduction.
* Power Management for nrf52 series SOC
* Added support for the following ARM boards:

  * 96b_neonkey
  * efm32wg_stk3800
  * mimxrt1050_evk
  * msp_exp432p401r_launchxl
  * nucleo_f030r8
  * nucleo_f091rc
  * stm32f411e_disco
  * stm32f412g_disco
  * stm32l476g_disco
  * usb_kw24d512

Drivers and Sensors
*******************

* timer: Add Support for TICKLESS KERNEL in xtensa_sys_timer
* Rename `random` to `entropy`
* Add Atmel SAM I2S (SSC) driver
* Add Atmel SAM DMA (XDMAC) driver
* Add plantower PMS7003 Driver
* Add Altera shim driver for JTAG UART soft IP
* Add Altera shim driver for timer soft IP
* Introduce mcux ccm driver
* Introduce mcux igpio shim driver

Networking
**********

* HTTP API changed to use net-app API. Old HTTP API is deprecated.
* Loopback network interface support added. This is used in testing only.
* LWM2M multi-fragment network packet support added.
* New CoAP library implementation, supporting longer network packets.
* Deprecated ZoAP library.
* mDNS (multicast DNS) support added.
* SNTP (Simple Network Time Protocol) client library added.
* Various fixes for: TCP, RPL, ARP, DNS, LWM2M, Ethernet, net-app API, Network
  shell, and BSD socket API
* Network management API fixes.
* Networking sample application fixes.
* 6lo IPv6 header compression fixes.
* IEEE 802.15.4 generic fixes.
* IEEE 802.15.4 mcr20a driver fixes.
* IEEE 802.15.4 kw41z driver fixes.
* IEEE 802.15.4 nrf5 driver fixes.

Bluetooth
*********

* Multiple qualification-related fixes for Bluetooth Mesh
* Support for Bluetooth Mesh Friend Node role
* Support for Bluetooth Mesh Foundation Client Models
* New Bluetooth Mesh shell module and test application
* Support for PA/LNA amplifiers in the BLE Controller
* Support for additional VS commands in the BLE Controller
* Multiple stability fixes for the BLE Controller

Build and Infrastructure
************************

* The Zephyr project has migrated to CMake, an important step in a
  larger effort to make Zephyr easier to use for application developers
  working on different platforms with different development environment
  needs.  This change retains Kconfig as-is, and replaces all Makefiles
  with corresponding CMakeLists.txt.  The DSL-like Make language that
  KBuild offers is replaced by a set of CMake extensions that provide
  either simple one-to-one translations of KBuild features or introduce
  new concepts that replace KBuild concepts. Please re-read the Getting
  Started guide
  (http://docs.zephyrproject.org/getting_started/getting_started.html)
  with updated instructions for setting up and developing on your host-OS.
  You *will* need to port your own out-of-tree scripts and Makefiles to
  CMake.

Libraries / Subsystems
***********************

* The implementation for sys_rand32_get() function has been moved to a new
  "random" subsystem. There are new implementations for this function, one based
  in the Xoroshift128+ PRNG (using a hardware number generator to seed), and
  another that obtains random numbers directly from a hardware number generator
  driver. Hardware number generator drivers have been moved to a
  "drivers/entropy" directory; these drivers only expose the interface provided
  by include/entropy.h.
* TinyCrypt updated to version 0.2.8

HALs
****

* Add Altera HAL for support NIOS-II boards
* Add mcux 2.3.0 for mimxrt1051 and mimxrt1052
* stm32cube: HAL/LL static library for stm32f0xx v.1.9.
* Add support for STM32 family USB driver
* Add Silabs Gecko SDK for EFM32WG SoCs
* Simplelink: Update cc32xx SDK to version 1.50.00.06

Documentation
*************

* Missing API documentation caused by doxygen subgroups and missing
  Sphinx directives now included.
* Note added to all released doc pages mentioning more current content could
  be available from the master branch version of the documentation.
* Documentation updated to use CMake (vs. Make) in all examples, and
  using a new Sphinx extension to keep examples consistent.
* Getting Started Guide material updated to include CMake dependencies
  and build instructions required for version 1.10.
* Instead of hiding all expected warnings from the document build
  process (there are some known doxygen/sphinx issues), the build
  now outputs all warnings, and then reports
  if any new/unexpected warnings or errors were detected.
* Obsolete V1 to V2 porting material removed.
* Continued updates to documentation for new board support, new samples,
  and new features.
* Integration of documentation with new zephyrproject.org website.
* Documentation moved to docs.zephyrproject.org site (with redirection
  from zephyrproject.org/doc)

Tests and Samples
*****************

* Benchmarking: cleanup of the benchmarking code
* Add userspace protection tests
* Move all tests to ztest and cleanup coding style and formatting

Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.9.0 tagged
release:

.. comment  List derived from Jira/GitHub Issue query: ...

* :github:`779` - CI: shippable - provide some means to allow users to rebuild
* :github:`1166` - Keeping reusable components under samples/ leads to build issues
* :github:`1236` - Cleanup CONFIG_EXECUTION_BENCHMARKING
* :github:`1241` - tests/net/ipv6/ FAILED on qc1000:x86
* :github:`1242` - tests/kernel/mutex/mutex/ FAILED @ esp32
* :github:`1256` - [cmake] A board should support multiple configurations (variants)
* :github:`1270` - Issue : Information CC3220SF LaunchXL
* :github:`1280` - shell on Arduino Due prints "shell>" before the delayed boot banner
* :github:`1289` - C++ 11 support!
* :github:`1332` - sanitycheck builds too many duplicates in CI, make it smarter
* :github:`1392` - No module named 'elftools'
* :github:`1397` - no serialport output
* :github:`1415` - Problem with forcing new line in generated documentation.
* :github:`1416` - Regression added by commit cd35742a (net/ethernet/arp: Let ethernet L2 managing pkt's reference while sending)
* :github:`1419` - test, please ignore
* :github:`1425` - spi.h and spi_legacy.h documentation conflicts
* :github:`1428` - networking defines being used but not defined anywhere
* :github:`1435` - Could not connect to Eclipse Leshan Demo Server
* :github:`1445` - doc: groups of items in API documentation not displaying
* :github:`1450` - make kconfig help is difficult to understand
* :github:`1474` - tests/net/ipv6_fragment build failure, missing testcase.yaml
* :github:`1487` - net/lib/dns doesn't respect CONFIG_NET_IPV6=n
* :github:`1488` - Replacing Make/Kbuild with CMake
* :github:`1499` - doc: replace Mac OS with macOS
* :github:`1501` - doc: Fix link title
* :github:`1510` - "make debugserver" broken for qemu_xtensa
* :github:`1522` - "make qemu" may not regenerate .config after changes to prj.conf
* :github:`1524` - doc: Remove "Changes from Version 1 Kernel" document
* :github:`1527` - make htmldocs failed
* :github:`1538` - esp32: is broken for the latest esp-idf version
* :github:`1542` - filter-known-issues.py fails if input file is empty
* :github:`1543` - doc: add process documentation for importing non-Apache2.0 licensed code
* :github:`1544` - regression: net: K64F: DHCP seems to fail a lot after 91041f9e
* :github:`1558` - Master reports itself as if it was 1.9.0 release
* :github:`1571` - Update to latest tinycrypt: v0.2.8
* :github:`1573` - tests/net/lib/http_header_fields/ fails with CONFIG_HTTP_PARSER_STRICT enabled
* :github:`1580` - checkpatch output in shippable log displays without line breaks
* :github:`1581` - two tests fail in qemu_cortex_m3 with new SDK
* :github:`1597` - remove deprecated k_mem_pool_defrag()
* :github:`1626` - Bluetooth LE dual mode topology
* :github:`1628` - Bluetooth LE data length extension
* :github:`1629` - LE privacy 1.2
* :github:`1630` - E2E tests for connection
* :github:`1632` - Implement Environmental Sensing Profile sample app
* :github:`1653` - enable stack canaries on ARC so we can run test_stackprot
* :github:`1670` - Add Reject command handling
* :github:`1853` - Review all Kconfig variables used and Simplify
* :github:`1880` - Zephyr Build Management
* :github:`1883` - Audio Codec
* :github:`1885` - Display Interface
* :github:`1902` - uWeave
* :github:`2011` - tcf: add support for running altera_max10 binaries
* :github:`2035` - doc: remove workaround for sphinx issue once 1.5 is released
* :github:`2202` - sporadic bad RAM pointer error under qemu_nios2
* :github:`2277` - Update to a more recent version of micro-ecc in Zephyr
* :github:`2281` - purge usage of platform_whitelist
* :github:`2411` - Look into supporting additional file systems under Zephyr FS API
* :github:`2580` - Failure in test_nano_work
* :github:`2723` - QEMU NIOS2 sporadic FAIL in tests/legacy/kernel/test_context
* :github:`2775` - Ability to make Security / Vulnerability bugs non-public
* :github:`2793` - entropy subsystem
* :github:`2818` - Add disk access based on flash on freedom board to interface with file system
* :github:`2853` - Customer: Zephyr Tutorial
* :github:`2855` - Customer: Sample code
* :github:`2858` - Customer: Training / Webinar / Video
* :github:`2942` - Support for NXP KW2xD MCU
* :github:`3039` - Simple Network Time Protocol support
* :github:`3058` - no good way to include library code outside of $(PROJECT_BASE)
* :github:`3064` - Symmetric multiprocessing (SMP)
* :github:`3070` - Add Atmel SAM family DMA (XDMAC) driver
* :github:`3139` - Zephyr Tutorials and Training
* :github:`3142` - [PTS] GAP/TC_SEC_AUT_BV_12_C fails
* :github:`3143` - [PTS] GAP/TC_SEC_AUT_BV_14_C fails
* :github:`3144` - [PTS] GAP/TC_PRIV_CONN_BV_11_C fails
* :github:`3146` - [PTS] SM/SLA/PROT/BV-02-C fails
* :github:`3147` - [PTS] SM/SLA/SIE/BV-01-C fails
* :github:`3158` - Add support for Panther board based on Quark SE C1000
* :github:`3184` - xtensa: Zephyr SDK build and emulation support
* :github:`3201` - Add Device Tree Documentation
* :github:`3268` - Add tickless kernel support in xtensa_sys_timer timer
* :github:`3274` - Lauterbach Debug Tools Support
* :github:`3275` - Tickless Kernel and Frequency Scaling
* :github:`3290` - introduce shared metadata for boards, samples and tests
* :github:`3294` - Application Development
* :github:`3297` - ROM-able
* :github:`3313` - [RESEARCH] Memory Protection Unit support
* :github:`3353` - Missing board documentation for arm/quark_se_c1000_ble
* :github:`3355` - Missing board documentation for arm/nucleo_f103rb
* :github:`3357` - Missing board documentation for arm/stm32_mini_a15
* :github:`3360` - Missing board documentation for x86/panther
* :github:`3364` - Missing board documentation for arc/panther_ss
* :github:`3368` - Can Zephyr support SNMP (Simple Network Management Protocol)?
* :github:`3378` - Zephyr will not build with icecream
* :github:`3383` - Work up linker-based system call prototype for MPU enabling
* :github:`3412` - Provide a sample application for kernel_event_logger
* :github:`3415` - Building FS for Arduino 101
* :github:`3432` - Port Zephyr to Silabs EFM32WG-STK3800
* :github:`3484` - Provide stm32cube LL based UART driver
* :github:`3485` - Provide stm32cube LL based I2C driver
* :github:`3486` - Provide stm32cube LL based SPI driver
* :github:`3587` - Move board related device tree files where the board is defined
* :github:`3588` - Move all X86 boards and related SoCs to device tree
* :github:`3600` -  Build warnings [-Wpointer-sign] with LLVM/icx (tests/unit/bluetooth/at)
* :github:`3601` - Use QMSI mailbox driver for Quark SE
* :github:`3604` - the http_client sample app cannot send GET request on Qemu x86
* :github:`3608` - Add functionality of Gesture Sensor
* :github:`3621` - Design system call interface for drivers
* :github:`3625` - Validation mechanism for user-supplied kernel object pointers
* :github:`3627` - x86: implement system calls
* :github:`3628` - implement APIs for dropping threads to unprivileged mode
* :github:`3630` - use API to validate user-supplied kernel buffers
* :github:`3632` - define set of architecture-specific memory protection APIs
* :github:`3635` - Device Driver Access Control
* :github:`3641` - define kernel system calls
* :github:`3643` - [PTS] PTS server stops working while executing  TC_SEC_CSIGN_BV_01_C test case
* :github:`3646` - Zoap message to use more than one fragment
* :github:`3682` - incremental builds do not work properly in Windows
* :github:`3683` - unable to follow directions to install Crosstool-NG on OS X
* :github:`3688` - OS X Setup Instructions Not Working on macOS Sierra
* :github:`3690` - Move to CMake or similar instead of Kbuild
* :github:`3697` - Use CMSIS __NVIC_PRIO_BITS consistently
* :github:`3716` - define / implement application-facing memory domain APIs
* :github:`3728` - ESP32 i2c Driver Support
* :github:`3772` - test_mem_pool_api crashes qemu_x86 if CONFIG_DEBUG=y
* :github:`3781` - iwdg: provide independent watchdog driver compliant with STM32Cube LL API
* :github:`3783` - Add mbedtls Crypto API shim driver
* :github:`3829` - PTS test case GATT/SR/GPA/BV-02-C crashes tester in QEMU
* :github:`3832` - ARM: implement API to validate user buffer
* :github:`3844` - Fix LWM2M header calculation in lwm2m_engine.c
* :github:`3851` - Port SPI HCI driver on new SPI API
* :github:`3852` - x86: implement memory domain interface
* :github:`3892` - Add support for STM32F429I_DISC1 board
* :github:`3897` - Static code scan (coverity) issues seen
* :github:`3922` - [PTS] GATT/SR/GAT/BV-01-C INCONC
* :github:`3923` - boards: provide support for Nucleo-64 F030R8
* :github:`3939` - Add Atmel SAM family I2S (Inter-IC Sound) driver based on SSC module
* :github:`3941` - x86: implement option for PAE-formatted page tables with NX bit
* :github:`3942` - x86: scope SMEP support in Zephyr
* :github:`3984` - Build warning: [-Wpointer-bool-conversion] with LLVM/icx (samples/bluetooth/mesh_demo)
* :github:`3985` - Build warning:  [-Wpointer-bool-conversion] with LLVM/icx (samples/bluetooth/mesh)
* :github:`4001` - GENERATED_KERNEL_OBJECT_FILES end up in application memory
* :github:`4004` - integrate printk() with console subsystem
* :github:`4009` - I2C API is mixing two incompatible definitions of bit-fields
* :github:`4014` - memory protection: implicit kernel object permissions
* :github:`4016` - bluetooth linker not connected
* :github:`4022` - net: "queue: Use k_poll if enabled" commit regressed BSD Sockets performance
* :github:`4026` - CC3220 WiFi Host Driver support
* :github:`4027` - extra unref happening on net_context
* :github:`4029` - TinyTILE bluetooth app flash
* :github:`4030` - Coverity issue seen with CID: 175366 , in file: /subsys/bluetooth/host/smp.c
* :github:`4031` - Coverity issue seen with CID: 175365 , in file: /subsys/bluetooth/controller/hci/hci.c
* :github:`4032` - Coverity issue seen with CID: 175364 , in file: /subsys/bluetooth/host/mesh/proxy.c
* :github:`4033` - Coverity issue seen with CID: 175363 , in file: /subsys/bluetooth/host/smp.c
* :github:`4034` - Coverity issue seen with CID: 175362 , in file: /subsys/bluetooth/host/smp.c
* :github:`4035` - Coverity issue seen with CID: 175361 , in file: /samples/bluetooth/eddystone/src/main.c
* :github:`4036` - Coverity issue seen with CID: 175360 , in file: /subsys/bluetooth/host/mesh/prov.c
* :github:`4037` - Coverity issue seen with CID: 175359 , in file: /subsys/bluetooth/host/hci_core.c
* :github:`4038` - Coverity issue seen with CID: 175358 , in file: /subsys/bluetooth/host/hci_core.c
* :github:`4041` - flashing tinytile and use of minicom
* :github:`4043` - Add new user CONFIG to project
* :github:`4044` - Livelock in SMP pairing failed scenario
* :github:`4046` - BLE Central and BLE Peripheral roles at a moment on nRF52832
* :github:`4048` - HTTP Request Timeout Not Working
* :github:`4049` - AMP - Multi-core
* :github:`4050` - zephyr.git/tests/kernel/obj_validation/testcase.yaml#test :Evaluation failure
* :github:`4051` - Coverity issue seen with CID: 177219 , in file: /drivers/flash/flash_stm32f4x.c
* :github:`4054` - [CID: 177215 ], in file: /tests/subsys/dfu/mcuboot/src/main.c
* :github:`4055` - Coverity issue seen with CID: 177214 , in file: /samples/boards/microbit/pong/src/ble.c
* :github:`4056` - Coverity issue seen with CID: 177213 , in file: /tests/net/ipv6_fragment/src/main.c
* :github:`4057` - Coverity issue seen with CID: 170744, in file: /samples/boards/microbit/pong/src/ble.c
* :github:`4058` - samples/net/http_client: The HTTP client failed to  send the GET request
* :github:`4059` -  zephyr.git/tests/net/ipv6/testcase.yaml#test  :evaluation failed
* :github:`4068` - [BLE, nRF51822] Error -ENOMEM when use  bt_gatt_write_without_response function
* :github:`4099` - Add some docs to samples/net/ieee802154/hw
* :github:`4131` - gen_syscalls.py may choke on non-ascii chars
* :github:`4135` - checkpatch.pl generates warning messages when run w/ perl-5.26
* :github:`4149` - Transition message on jira.zephyrproject.org needed
* :github:`4162` - build error in http_get sample
* :github:`4165` - ieee802154_uart_pipe.c: warning: return from incompatible pointer type
* :github:`4182` - NET_APP_SETTINGS for 15.4 doesn't seem to work (if to trust 15.4 shell)
* :github:`4186` - tcf.git/examples/test_network_linux_zephyr.py#_test  :Compilation failure
* :github:`4188` - samples /net/echo_server:failed to send packets to client
* :github:`4189` - ieee802154_settings.c is duplicated in the codebase
* :github:`4190` - samples/net/echo_client :failed to send data
* :github:`4193` - Zephyr libc(snprintf) is not comply with ISO standard.
* :github:`4195` - tests/net/udp/test_udp.py#_ipv4_udp : evaluation failed
* :github:`4239` - unit tests broken in sanitycheck
* :github:`4249` - where is auto-pts py script of zephyr?
* :github:`4258` - samples/net/zoap_server : unable to communicate between zoap client and server
* :github:`4264` - Getting started guide for windows: small error
* :github:`4289` - samples/mpu/mem_domain_apis_test is broken
* :github:`4292` - net: tcp.c: prepare_segment() may unrightly unref packet in case of error
* :github:`4295` - Error flashing board STM32373C-EVAL
* :github:`4301` - checkpatch.pl false positives block PR merge
* :github:`4310` - unable to flash quark_se_c1000_devboard
* :github:`4312` - GDB: Ubuntu's default GDB package does not support arm
* :github:`4323` - net: tcp.c: prepare_segment() may leak fragments in case of error
* :github:`4325` - samples/net/http_client:  unable to send the proper http request to Apache server
* :github:`4327` - NET_PKT_TX_SLAB_DEFINE, NET_PKT_DATA_POOL_DEFINE description and usage are confusing
* :github:`4347` - net: BSD Sockets UDP sendto() impl broke tests/net/socket/udp/
* :github:`4353` - VM-VM qemu networking example crashes often
* :github:`4358` - k_queue_poll returns NULL with K_FOREVER
* :github:`4366` - memory corruption in test_pipe_api
* :github:`4377` - Sniffing traffic in a VM-VM qemu setup crashes with a segfault in the monitor application
* :github:`4392` - zephyr/tests/benchmarks/footprint :build Failed
* :github:`4394` - Coverity issue seen with CID: 178058
* :github:`4395` - Coverity issue seen with CID: 178059
* :github:`4396` - Coverity issue seen with CID: 178060
* :github:`4397` - Coverity issue seen with CID: 178064
* :github:`4398` - zephyr/tests/crypto/ccm_mode :-Evaluation failed due to esp32
* :github:`4419` - 6LoWPAN - source address uncompress corner case
* :github:`4421` - net: Duplicated functionality between net_pkt_get_src_addr() and net_context.c:create_sockaddr()
* :github:`4424` - Turning on network debug message w/ LwM2M sample client will result in stack check failure
* :github:`4429` - I2C: stm32-i2c-v2 Driver (F0/F3/F7) gets stuck in endless loop when handling restart conditions
* :github:`4442` - samples: net: ieee802154: Sample is not working on nRF52840 platform
* :github:`4459` - i2c: stm32-i2c-(v1/v2) don't handle i2c_burst_write like expected
* :github:`4463` - Some tests and samples are missing a .yaml file
* :github:`4466` - warnings building echo_client with nrf5
* :github:`4469` - CI problem with check-compliance.py
* :github:`4476` - Multiple build failures with i2c_ll_stm32.c driver
* :github:`4480` - Compilation failure for qemu_x86 with CONFIG_DEBUG_INFO=y
* :github:`4481` - Build failure with CONFIG_NET_DEBUG_APP=y
* :github:`4503` - CONFIG_STACK_SENTINEL inconsistencies
* :github:`4538` - Coverity issue seen with CID:174928
* :github:`4539` - Coverity issue seen with CID:173658
* :github:`4540` - Coverity issue seen with CID: 173657
* :github:`4541` - Coverity issue seen with CID: 173653
* :github:`4544` - [BLE Mesh] Error: Failed to advertise using Node ID
* :github:`4563` - [BLE Mesh]: How to handle the 'Set" and 'Get' callbacks
* :github:`4565` - net_context_recv always fails with timeout=K_FOREVER
* :github:`4567` - [BLE Mesh]: Multiple elements in a node
* :github:`4569` - LoRa:  support LoRa
* :github:`4579` - [CID: 178249] Parse warnings in samples/mpu/mem_domain_apis_test/src/main.c
* :github:`4580` - Coverity issue seen with CID: 178248
* :github:`4581` - Coverity issue seen with CID: 178247
* :github:`4582` - Coverity issue seen with CID: 178246
* :github:`4583` - [CID: 178245] Parse warnings in samples/mpu/mem_domain_apis_test/src/main.c
* :github:`4584` - Coverity issue seen with CID: 178244
* :github:`4585` - Coverity issue seen with CID: 178243
* :github:`4586` - [CID: 178242]: Parse warnings samples/mpu/mem_domain_apis_test/src/main.c
* :github:`4587` - Coverity issue seen with CID: 178241
* :github:`4588` - Coverity issue seen with CID: 178240
* :github:`4589` - [coverity] Null pointer dereferences in tests/net/app/src/main.c
* :github:`4591` - [CID: 178237] memory corruption in drivers/ieee802154/ieee802154_mcr20a.c
* :github:`4592` - Coverity issue seen with CID: 178236
* :github:`4593` - Coverity issue seen with CID: 178235
* :github:`4594` - Coverity issue seen with CID: 178234
* :github:`4595` - Coverity issue seen with CID: 178233
* :github:`4600` - drivers:i2c_ll_stm32_v2: Interrupt mode uses while loops
* :github:`4607` - tests/net/socket/udp/ is broken, again
* :github:`4630` - Sample app 'coaps_server' fails to parse coap pkt
* :github:`4637` - [Coverity CID: 178334] Null pointer dereferences in /subsys/usb/class/netusb/function_ecm.c
* :github:`4638` - build is failing when newlib  is enabled
* :github:`4644` - Kconfig warnings when building any sample for nRF5x
* :github:`4652` - Document "make flash" in the "application development primer"
* :github:`4654` - Wrong file name for drivers/aio/aio_comparator_handlers.o
* :github:`4667` - x86 boards need device trees
* :github:`4668` - drivers/random/random_handlers.c is built when no random driver has been kconfig'ed into the build
* :github:`4683` - net: tcp  tcp_retry_expired cause assert
* :github:`4695` - samples/net/ieee802154 needs documentation
* :github:`4697` - [regression] net: echo_server doesn't accept IPv4 connections
* :github:`4738` - ble-mesh: proxy.c : Is clients-> conn a clerical error? it should be client-> conn?
* :github:`4744` - tests/net/ieee802154/l2/testcase.yaml#test : unable to acknowledge data from receiver
* :github:`4757` - kw41z-frdm: assertion failure while setting IRQ priority
* :github:`4759` - [PTS] GATT/CL/GAW/BV-02-C fails with INCONC
* :github:`4760` - stm32f4_disco and frdm_k64f  samples/basic/blink_led ,Choose supported PWM driver
* :github:`4766` - tests: mem_pool: Fixed memory pool test case failure on quark d2000
* :github:`4780` - [Coverity CID: 178794] Error handling issues in /tests/subsys/dfu/mcuboot/src/main.c
* :github:`4781` - [Coverity CID: 178793] Incorrect expression in /tests/kernel/static_idt/src/static_idt.c
* :github:`4782` - [Coverity CID: 178792] Memory - illegal accesses in /subsys/net/lib/http/http_app_server.c
* :github:`4783` - [Coverity CID: 178791] Incorrect expression in /tests/kernel/static_idt/src/static_idt.c
* :github:`4784` - [Coverity CID: 178790] Memory - corruptions in /samples/net/http_server/src/main.c
* :github:`4785` - [Coverity CID: 178789] Null pointer dereferences in /samples/net/http_server/src/main.c
* :github:`4786` - [Coverity CID: 178788] Control flow issues in /tests/net/context/src/main.c
* :github:`4787` - [Coverity CID: 178787] Null pointer dereferences in /subsys/net/ip/net_context.c
* :github:`4788` - [Coverity CID:178786] Memory - corruptions in /samples/net/http_server/src/main.c
* :github:`4789` - [Coverity CID: 178785] Incorrect expression in /tests/kernel/static_idt/src/static_idt.c
* :github:`4791` - rpl-node uses testcase.ini instead of sample.yaml format
* :github:`4825` - Bluetooth IPSP error with qemu_x86
* :github:`4827` - Ping command crashes kernel over qemu_x86
* :github:`4841` - fix doc/devices/dts/device_tree.rst path and Make references
* :github:`4844` - cmake: can't flash stm32 with openocd
* :github:`4847` - custom 404 error page not being shown on docs.zephyrproject.org
* :github:`4853` - cmake: building unit test cases ignore EXTRA_* settings
* :github:`4864` - cmake: hts221 sensor sample not working anymore
* :github:`4881` - device_get_binding() returns failure in sample/drivers/crypto
* :github:`4889` - Flashing EM Starterkit with EM7D fails on master
* :github:`4899` - Convert opensda doc to CMake
* :github:`4901` - net: tcp: RST is sent after last ack is received
* :github:`4904` - cmake: BOOT_BANNER disappeared
* :github:`4905` - cmake: flashing for quark_se_devboard is broken
* :github:`4910` - BT host CMakeLists.txt code should be agnostic to the FS implementation
* :github:`4912` - Not using the Zephyr SDK is broken
* :github:`4925` - application_development test pollutes source directory
* :github:`4936` - net: 15.4 MAC addresses are shown differently between shell "net iface" and "ieee15_4 get_ext_addr"
* :github:`4937` - ESP32 can't boot
* :github:`4975` - Getting started documentation for Mac OS X inconsistent
* :github:`5004` - Normalize IEEE802514 driver "raw" mode.
* :github:`5008` - system call headers are not properly regenerated in CMake on incremental builds
* :github:`5009` - cmake creates too many build artifacts
* :github:`5014` - samples/drivers/crypto :Unable to find crypto device
* :github:`5019` - tests/kernel/mem_protect/stackprot : input string is long stack overflow
* :github:`5025` - arduino_due not generating proper config with cmake (crash)
* :github:`5026` - k_poll() documentation is wrong
* :github:`5040` - Bluetooth mesh API documentation incomplete
* :github:`5047` - document error: getting_started.rst
* :github:`5051` - Verify doxygen documented APIs are in the generated docs
* :github:`5055` - [Coverity CID: 179254] Possible Control flow issues in /samples/net/zperf/src/zperf_udp_receiver.c
* :github:`5056` - [Coverity CID: 179253] Control flow issues in /samples/net/zperf/src/zperf_tcp_receiver.c
* :github:`5057` - [Coverity CID: 179252] Null pointer dereferences in /samples/net/zperf/src/zperf_udp_receiver.c
* :github:`5058` - [Coverity CID: 179251] Control flow issues in /samples/net/zperf/src/zperf_udp_receiver.c
* :github:`5059` - [Coverity CID: 179250] Null pointer dereferences in /samples/net/zperf/src/zperf_udp_uploader.c
* :github:`5060` - [Coverity CID: 179249] Incorrect expression in /tests/kernel/fatal/src/main.c
* :github:`5061` - [Coverity CID: 179248] Control flow issues in /samples/net/zperf/src/zperf_tcp_receiver.c
* :github:`5062` - [Coverity CID: 179247] Incorrect expression in /tests/kernel/fatal/src/main.c
* :github:`5063` - samples/bluetooth: central_hr sample app is not connecting with peripheral sample app on arduino_101.
* :github:`5085` - bug: dts: stm32f1: wrong pinctrl base address
* :github:`5087` - Samples/bluetooth: Failed to connect with eddystone sample app on arduino_101.
* :github:`5090` - no makefile in zephyr/samples/bluetooth/mesh examples
* :github:`5097` - zephyr_library_*() configuration has lower precedence than global zephyr_*()  configuration
* :github:`5107` - Default partition addressing for nrf52_pca10040 is incompatible
* :github:`5116` - [Coverity CID: 179986] Null pointer dereferences in /subsys/bluetooth/host/mesh/access.c
* :github:`5117` - [Coverity CID: 179985] Error handling issues in /subsys/bluetooth/host/mesh/cfg_srv.c
* :github:`5118` - [Coverity CID: 179984] Memory - corruptions in /drivers/ethernet/eth_mcux.c
* :github:`5119` - [Coverity CID: 179983] Error handling issues in /subsys/bluetooth/host/mesh/cfg_srv.c
* :github:`5120` - [Coverity CID: 179982] Error handling issues in /subsys/bluetooth/host/mesh/cfg_srv.c
* :github:`5121` - [Coverity CID: 179981] Incorrect expression in /drivers/ieee802154/ieee802154_kw41z.c
* :github:`5122` - [Coverity CID: 179980] Integer handling issues in /subsys/bluetooth/host/mesh/friend.c
* :github:`5123` - [Coverity CID: 179979] Error handling issues in /subsys/bluetooth/host/mesh/cfg_srv.c
* :github:`5124` - [Coverity CID: 179978] Error handling issues in /subsys/bluetooth/host/mesh/health_srv.c
* :github:`5125` - [Coverity CID: 179976] Error handling issues in /subsys/bluetooth/host/mesh/cfg_srv.c
* :github:`5126` - [Coverity CID: 179975] Error handling issues in /subsys/bluetooth/host/mesh/health_srv.c
* :github:`5127` - [Coverity CID: 179974] Error handling issues in /subsys/bluetooth/host/mesh/cfg_srv.c
* :github:`5128` - [Coverity CID: 179973] Error handling issues in /subsys/bluetooth/host/mesh/cfg_srv.c
* :github:`5140` - CMake migration regressed (changed behavior) of QEMU_PTY=1
* :github:`5144` - BUG: cmake: make doesn't update zephyr.hex file
* :github:`5145` - samples/bluetooth: Connection failure on peripheral CSC with Arduino 101
* :github:`5152` - CONFIG_CPLUSPLUS is incompatible with the zephyr_get_* API
* :github:`5159` - [nrf] Button example for nrf5x boards latches GPIO after 1 button press
* :github:`5176` - zephyr-v1.9.2 tag missing
* :github:`5177` - hci_usb: Linking error
* :github:`5184` - kernel system call handlers missing due to -Wl,--no-whole-archive
* :github:`5186` - gen_syscall_header_py is being run at the wrong time
* :github:`5189` - tests/subsys/fs/nffs_fs_api:-Evaluation failed
* :github:`5207` - Bluetooth subsystem uses acl_in_pool even for controllers not supporting flow control
* :github:`5211` - Kconfig: CPU_HAS_FPU dependencies problem
* :github:`5223` - CMake: Recent changes broke 3rd-party build system integration again
* :github:`5265` - ROM size increase due Zephyr compile options not stripping unused functions
* :github:`5266` - Ensure the Licensing page is up-to-date for the release
* :github:`5286` - NET_L2: Symbols from the L2 implementation layer are exposed to users of L2
* :github:`5298` - Endless loop in uart pipe
