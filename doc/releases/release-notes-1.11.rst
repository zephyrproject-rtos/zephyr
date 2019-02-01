:orphan:

.. _zephyr_1.11:

Zephyr Kernel 1.11.0
#####################

We are pleased to announce the release of Zephyr kernel version 1.11.0.

Major enhancements with this release include:

* Thread-level memory protection on x86, ARC and Arm, userspace and memory
  domains
* Symmetric Multi Processing (SMP) support on the Xtensa architecture.
* Initial Armv8-M architecture support.
* Native development environment on Microsoft Windows.
* Native build target on POSIX platforms.
* POSIX PSE52 partial support.
* Thread support via integration with OpenThread.
* Firmware over-the-air (FOTA) updates over BLE using MCUmgr.
* Lightweight flash storage layer for constrained devices.
* Additional SoC, platform and driver support for many of the already supported
  platforms.

The following sections provide detailed lists of changes by component.

Kernel
******

* Initial Symmetric Multi Processing (SMP) support added:

  * SMP-aware scheduler
  * SMP timer and idling support
  * Available on the Xtensa architecture
* POSIX PSE52 support:

  * Timer, clock, scheduler and pthread APIs

Architectures
*************

* User space and system call related changes:

  * Added ARC user space implementation
  * Added Arm user space implementation
  * Fixed a few MPU related issues with stack guards
* Armv8-M initial architecture support, including the following cores:

  * Arm Cortex-M23
  * Arm Cortex-M33
* New POSIX architecture for native GNU/Linux and macOS build targets:

  * Targets native executables that can be run on the host operating system

Boards
******

* New native_posix board for the POSIX architecture:

  * Includes a template for hardware models
  * Adds support for console and logging
  * Interrupts and timers are simulated in several different configurations
* Added support for the following Arm boards:

  * adafruit_trinket_m0
  * arduino_zero
  * lpcxpresso54114
  * nrf52_sparkfun
  * nucleo_f429zi
  * stm32f072_eval
  * stm32f072b_disco
* Removed Panther board support, which included boards/x86/panther and
  boards/arc/panther_ss
* Refactored dts.fixup so common SoC-related fixes are in arch/<*>/soc
  and board dts.fixup is only used for board-specific items.

Drivers and Sensors
*******************

* New LED PWM driver for ESP32 SoC
* Fixed ESP32 I2C driver
* Added I2C master, QSPI flash, and GPIO drivers for nios-II
* Added PinMux, GPIO, serial drivers for LPC54114
* Added PinMux, GPIO, serial, SPI, and watchdog drivers for sam0
* Added APA102 and WS2821B led_strip drivers
* Added native entropy driver
* Moved some sensors to dts
* Added AMG88xx, CCS811, and VL53L0x sensor drivers
* Redefined SENSOR_CHAN_HUMIDITY in percent

Networking
**********

* Generic OpenThread support added
* OpenThread support to nRF5 IEEE 802.15.4 driver added
* NXP MCUX ethernet driver IPv6 multicast join/leave enhancements
* Ethernet STM32 fixes
* IEEE 802.15.4 Sub-GHz TI CC1200 chip support added
* IEEE 802.15.4 test driver (upipe) hw filtering support added
* IEEE 802.15.4 radio API enhancements
* Net loopback driver fixes
* Net management API event enhancements
* IPv6 neighbor addition and removal can be monitored
* Static IPv4 and DHCPv4 configuration enhancements
* Bluetooth IPSP disconnect fix
* Network buffer enhancements
* ICMPv4 and ICMPv6 error checking fixes
* Network interface address handling enhancements
* Add routing support between network interfaces
* LWM2M fixes and enhancements
* Old legacy HTTP API removed
* Old legacy ZoAP API removed
* CoAP fixes
* TCP fixes
* HTTP fixes
* RPL fixes
* Net-app API fixes
* Net-shell fixes
* BSD socket API fixes

Bluetooth
*********

* Multiple fixes to the controller
* Fixed potential connection transmission deadlock issue with the help
  of a dedicated fragment pool
* Multiple fixes to Mesh support
* Added test automation for Mesh (for tests/bluetooth/tester)

Build and Infrastructure
************************

* Native development environment on Microsoft Windows:

  * Uses CMake and Kconfiglib to avoid requiring an emulation layer
  * Package management support with Chocolatey for simple setup
  * Build time now comparable to Linux and macOS using Ninja

Libraries / Subsystems
***********************

* New management subsystem based on the cross-RTOS MCUmgr:

  * Secure Firmware Updates over BLE and serial
  * Support for file system access and statistics
  * mcumgr cross-platform command-line tool

* FCB (File Circular Buffer) lightweight storage layer:

  * Wear-leveling support for NOR flashes
  * Suitable for memory constrained devices

HALs
****

* Updated Arm CMSIS from version 4.5.0 to 5.2.0
* Updated stm32cube stm32l4xx from version 1.9.0 to 1.10.0
* Updated stm32cube stm32f4xx from version 1.16.0 to 1.18.0
* Added Atmel SAMD21 HAL
* Added mcux 2.2.1 for LPC54114
* Added HAL for VL53L0x sensor from STM
* Imported and moved to nRFx 0.8.0 on Nordic SoCs
* Added QSPI Controller HAL driver

Documentation
*************

* Added MPU specific stack and userspace documentation
* Improved docs for Native (POSIX) support
* Docs for new samples and supported board
* General documentation clarifications and improvements
* Identify daily-built master-branch docs as "Latest" version
* Addressed Sphinx-generated intra-page link issues
* Updated doc generation tools (Doxygen, Sphinx, Breathe, Docutils)

Tests and Samples
*****************

* Added additional tests and test improvements for user space testing

Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.10.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...

* :github:`1082` - build all tests have issues for devices that don't exist on a given board
* :github:`1281` - spi_ll_stm32 driver does not support stm32f1soc
* :github:`1291` - Initial Posix PSE52 Support
* :github:`1460` - 1.10 Release Checklist
* :github:`1462` - rename nano_internal.h to kernel_internal.h
* :github:`1526` - Bluetooth:mesh:prov_start: Invalid authentication
* :github:`1532` - There are no RISC-V boards in the list of supported boards
* :github:`1727` - Support out-of-tree board definitions
* :github:`1793` - I2S device APIs and Drivers
* :github:`1868` - Build System cleanup and Kernel / Application build separation
* :github:`1877` - Provide single point of notification for new data on multiple sockets.
* :github:`1890` - Memory Management
* :github:`1891` - Native Port
* :github:`1892` - NFC Stack
* :github:`1893` - Unified Kernel
* :github:`1921` - Bluetooth LE 4.2 Hardware Support
* :github:`1926` - build system does not re-link if linker script changed
* :github:`1930` - bluetooth tester shall support logging on Arduino101
* :github:`2007` - C++ compiler flags are not managed correctly
* :github:`2072` - create abstraction layer to directly use Altera HAL drivers for Nios II IP blocks
* :github:`2107` - handle configuration changes with more code coverage
* :github:`2239` - sporadic illegal instruction exception on Nios II in test_errno
* :github:`2244` - LE Controller: remove util folder
* :github:`2280` - Move defaults.tc and .known-issue under tests/
* :github:`2347` - Thread Protocol v1.1 Dependencies on the IP Stack
* :github:`2365` - Port IOT Protocols to the new IP Stack
* :github:`2477` - no unit tests exist for CONFIG_DEBUG_INFO
* :github:`2620` - object files created outside of $O directory when obj-XYZ path is relative
* :github:`2722` - QEMU NIOS2 sporadic FAIL in tests/legacy/kernel/test_timer/nanokernel
* :github:`2760` - Implement Virtual USB ethernet Adapter support
* :github:`2819` - legacy/kernel/test_task_priv randomly fails on EMSK ARC
* :github:`2889` - [ARC] legacy/benchmark/latency_measure not measuring RIRQ/FIRQ
* :github:`2891` - implement _tsc_read equivalent for all architectures
* :github:`2912` - Develop Guideline for Handling Configuration Changes with More Code Coverage
* :github:`2937` - Thread-level Memory Protection Support
* :github:`2939` - Add Xtensa Port
* :github:`2943` - Support for the KW22D512 Kinetis MCU based USB Dongle
* :github:`2971` - I2C High-Speed Mode is not implemented
* :github:`2994` - The build system crashes when GCCARMEMB_TOOLCHAIN_PATH has a space in it
* :github:`3069` - XML encoding/decoding library
* :github:`3081` - Concise Binary Object Representation (CBOR)
* :github:`3083` - I2C problem Zephyr OS sensor example on NRF51 and F401re
* :github:`3127` - IP stack does not implement multicasting requirements of IPv6 RFCs and network driver model lacks features to implement it properly
* :github:`3240` - Unnecessary code footprint bloat due to "static inline"
* :github:`3279` - Reclaiming Memory
* :github:`3283` - Split net_buf parsing context from the actual data
* :github:`3302` - samples/subsys/logging/logger-hook needs to be a test case
* :github:`3308` - [TAHI] No "ICMPv6 error message" is received while sending echo request with parameter problem header
* :github:`3316` - [IPv6 TAHI] Section 1: RFC 2460 - IPv6 Specification
* :github:`3317` - [IPv6 TAHI]Section 4: RFC 1981 - Path MTU Discovery for IPv6
* :github:`3318` - [IPv6 TAHI]Section 5: RFC 4443 - ICMPv6
* :github:`3322` - [IPv6 TAHI] Section 3: RFC 4862 - IPv6 Stateless Address Autoconfiguration
* :github:`3323` - [IPv6 TAHI] Section 2: RFC 4861 - Neighbor Discovery for IPv6
* :github:`3329` - Build warnings [-Wpointer-sign] with LLVM/icx (bluetooth_ipsp)
* :github:`3345` - Missing board documentation for riscv32/zedboard_pulpino
* :github:`3346` - Missing board documentation for riscv32/qemu_riscv32
* :github:`3351` - Missing board documentation for arm/bbc_microbit board
* :github:`3352` - Missing board documentation for arm/nrf51_blenano
* :github:`3439` - IP stack: No MTU handling on send()
* :github:`3440` - IP stack: No TCP receive window handling
* :github:`3483` - Unify STM32 configuration files
* :github:`3546` - Enabling networking for targets w/o network hw causes hang on boot
* :github:`3565` - Symmetric multiprocessing (SMP) for Xtensa architecture
* :github:`3597` - Build warnings [-Wpointer-sign] with LLVM/icx (tests/net/ieee802154/crypto)
* :github:`3614` - cdc-acm error when printing one byte at a time
* :github:`3617` - Build warnings [-Wshift-overflow] with LLVM/icx (K_MEM_POOL_DEFINE)
* :github:`3667` - _IsInIsr doesn't work properly on Cortex M0
* :github:`3685` - Test suite cleanup and Test Coverage
* :github:`3704` - Move all X86 boards to device tree
* :github:`3707` - intermittent work_queue test failure
* :github:`3712` - RPL client node version bogus incrementation
* :github:`3718` - Mpu stack guard is not set when reaching main
* :github:`3747` - tests/kernel/mem_slab/test_mslab_threadsafe/testcase.ini#test type:qemu-zephyr-arm
* :github:`3809` - Build warnings [-Wimplicit-function-declaration] with LLVM/icx (tests/drivers/pci_enum)
* :github:`3833` - make device_get_binding() more efficient
* :github:`3834` - CDC_ACM is limited to 4 bytes at a time for Arduino 101
* :github:`3838` - Some  tests end up with  0 platforms due to bad filtering
* :github:`3850` - SPI fails on Nucleo_f334r8
* :github:`3855` - Support board files located in application directory
* :github:`3856` - LwM2M: Support write attributes (section 5.4.4 of spec)
* :github:`3858` - Enable OpenThread support for Zephyr
* :github:`3859` - Create OpenThread platform adaptation for Zephyr
* :github:`3860` - Create OpenThread network interface driver
* :github:`3862` - Verify that the OpenThread requirements are fulfilled by Zephyr 15.4 radio driver
* :github:`3870` - Create a shell to configure OpenThread stack
* :github:`3872` - build on windows failed " error: unrecognized command line option '-no-pie'"
* :github:`3918` - Build error [use of undeclared identifier]with LLVM/icx (samples/net/nats)
* :github:`4000` - xtensa-vectors.S builds wrong versions of ISRs based on HAL information
* :github:`4010` - [CID: 174928]: Control flow issues in /tests/kernel/mem_slab/mslab/src/slab.c
* :github:`4025` - Upgrade to TinyCrypt 0.2.7 has significant API changes
* :github:`4045` - convert to ztest for files in tests/kernel
* :github:`4105` - Sensors: move all the drivers using SPI bus to new SPI API
* :github:`4106` - Flash: move w25qxxdv driver to new SPI API
* :github:`4216` - samples:net:sockets:echo : communication blocks between client and server after few packets transmission
* :github:`4351` - arduino_101: USB device is not listed after flashing with a Zephyr sample
* :github:`4401` - tests/net/ipv6/test.yaml :--Cannot add multicast IPv6 address
* :github:`4445` - sanitycheck --platform-limit is broken
* :github:`4513` - parsetab.py is getting corrupted when multiple instance of sanitycheck is executed simultaneously
* :github:`4549` - tests/crypto/mbedtls/testcase.yaml#test :Build failed
* :github:`4566` - k_busy_wait( ) gives compilation error when CONFIG_SYS_CLOCK_TICKS_PER_SEC is set to 0
* :github:`4568` - dts generation incorrect
* :github:`4576` - no testcase.yaml for tests/drivers/spi/spi_loopback
* :github:`4578` - tests/net/socket/udp/testcase.yaml#test : Build failed on esp32
* :github:`4596` - tests/net/mgmt/testcase.yaml#test :failed due to un-handled exception
* :github:`4597` - tests/drivers/ipm/testcase.yaml#test :unable to print the Expected output
* :github:`4603` - sanitycheck either conceals information from user or spams it
* :github:`4606` - usb mass storage : config waits for Vendor ID and Product ID from user during building
* :github:`4633` - Implement flash page layout api in the Kinetis flash driver
* :github:`4635` - xtensa-esp32-elf-gcc.exe: error: unrecognized command line option '-no-pie'
* :github:`4653` - net: tcp->recv_max_ack isn't used
* :github:`4666` - x86 device trees need interrupt controller nodes
* :github:`4687` - Windows: Remove all dependencies on MSYS2
* :github:`4699` - PWM LED Driver for ESP32
* :github:`4705` -  tests/net/socket/tcp/ undefined reference to __getreent
* :github:`4709` - tests/kernel/fatal/testcase.yaml#stack-sentinel : Kernel Panic
* :github:`4724` - sanitycheck build_only option can be confusing
* :github:`4772` - doc: add contributing info about shippable failures
* :github:`4777` - need a testcase for version number of the kernel and version.h
* :github:`4779` - net: tcp: FIN isn't sent when performing active close.
* :github:`4826` - Bluetooth IPSP example does not reach main() on qemu_cortex_m3
* :github:`4828` - device tree: Introduce bus objects (i2c-device)
* :github:`4851` - cmake does not show verbose output of build tools
* :github:`4885` - cmake: IS_TEST guessing gets thrown off by symlinks
* :github:`4924` - dumb_http_server pollutes the source directory
* :github:`4934` - net: 15.4 network interfaces use incorrect MTU setting of 127
* :github:`4941` - LwM2M: support discovery other than '/'
* :github:`4983` - ARMv8-M basic support
* :github:`4989` - Bluetooth: PTS fails to connect to Zephyr
* :github:`5010` - changes to included linker scripts are not picked up by the build system
* :github:`5017` - Genuino/Arduino 101 Sensor Sample BMI160: Gyro Device not found
* :github:`5091` - MPU fault at drivers/flash/soc_flash_nrf5.c:493 with NFFS enabled on nrf52840_pca10056
* :github:`5101` - LwM2M: device hang after requesting a firmware update to a loopback device IP
* :github:`5109` - yaml: fix key/value syntax to 'mapping' instead of 'series'
* :github:`5130` - include guards missing in toolchain/gcc.h and toolchain/common.h
* :github:`5136` - "Distinguishing Features" section in docs is outdated and needs an update
* :github:`5143` - Cmake ignores setting CONFIG_ETH_MCUX_0=n in prj.conf
* :github:`5148` - Lightweight flash storage layer
* :github:`5162` - Reduce duplication in UUID definitions?
* :github:`5184` - kernel system call handlers missing due to -Wl,--no-whole-archive
* :github:`5221` - Build doesn't fail if total RAM usage is greater than the RAM available on the board
* :github:`5226` - Compiling with -O0 causes the kobject text area to exceed the limit (linker error)
* :github:`5228` - The build fails when building echo_server with nrf52840_pca10056
* :github:`5240` - I2C is enabled by default on ESP32
* :github:`5247` - Object tracing test case fails in NRF boards
* :github:`5256` - _nano_tick_delta, sys_tick_delta, sys_tick_delta_32: not used or tested
* :github:`5270` - Not all IEEE802154_MCR20A_RAW references were removed
* :github:`5282` - net: IPv6 DAD is incorrect, wouldn't work ("always succeed") on mcast medium like Ethernet
* :github:`5283` - reference to non-existing functions in arch/x86/core/intstub.S
* :github:`5305` - flash: use generated FLASH_WRITE_BLOCK_SIZE in flash_stm32_api
* :github:`5317` - IPSP deadlock during disconnect -> net_if_down
* :github:`5326` - IPSP ping fails
* :github:`5328` - build system should try and create conf if not found for non-zephyr SDK
* :github:`5334` - CMake: Ninja support is broken
* :github:`5345` - Cmake: ext: Header file include error: No such file or directory
* :github:`5348` - rom_report is broken for some environments
* :github:`5351` - Some libraries should automatically be linked with 'app'
* :github:`5355` - qemu_x86/qemu_x86_nommu hangs on big executable files
* :github:`5370` - [Coverity CID: 180699] Error handling issues in /tests/bluetooth/tester/src/gatt.c
* :github:`5374` - merge_config.sh can behave differently from merge_config.py
* :github:`5379` - sample: net/socket/http_get: no printf output
* :github:`5382` - P2P Device Firwmare Update (FOTA) over BLE and Serial
* :github:`5391` - drivers: stm32 clock control: F0 Series with PREDIV1 Support uses wrong PLLSOURCE define for HSI clock
* :github:`5401` - delta_ticks_from_prev become negative and waiting tasks never scheduled
* :github:`5406` - UART1 on STM32F0 Series not working: errors in makro to enable clock
* :github:`5418` - Provide a python based replacement for gperf
* :github:`5419` - Provide a python based kconfig processing script, replacement for conf/mconf..
* :github:`5428` - can not build for esp32
* :github:`5444` - bluetooth controller fails when building with -Wshadow
* :github:`5448` - STM32: Entropy test could not build
* :github:`5449` - STM32: provide default configuration for entropy sample
* :github:`5453` - gitlint should allow tabs in commit messages
* :github:`5458` - [ESP32] Make error
* :github:`5466` - sanitycheck: "CMake Error: : System Error: File name too long"
* :github:`5467` - NFFS file system does no build when newlib libc is used
* :github:`5471` - cmake errors at -B containing @
* :github:`5476` - Native port (posix) to write own PID into a file
* :github:`5477` - Native port (posix) to support receiving signals
* :github:`5483` - Native port (POSIX) should accept command-line arguments
* :github:`5484` - net: ARP/ND: Possibility for deadlocks and DoS
* :github:`5486` - Bluetooth: Cannot connect to prevoiusly disconnected device when BT_PRIVACY is enabled
* :github:`5488` - target_ld_options will apply flags that should be skipped
* :github:`5493` - NFFS does not work with STM32L4 devices due to flash restrictions
* :github:`5497` - cmake: allow to link external libraries with --whole-archive
* :github:`5499` - config BT_CTLR_DEBUG_PINS
* :github:`5504` - net: Incorrect logic for TCP "ackerr" statistics
* :github:`5530` - [Coverity CID: 181848] Null pointer dereferences in /subsys/bluetooth/host/mesh/access.c
* :github:`5531` - [Coverity CID: 181847] Incorrect expression in /samples/drivers/crypto/src/main.c
* :github:`5539` - tests/kernel/fatal/stack-sentinel fails when asserts are enabled
* :github:`5546` - (Stupid) questions about coverage reports
* :github:`5548` - coverage should be collected from code built with -O0
* :github:`5557` - Cloning Zephyr with git's core.autocrlf=true leads to obscure errors
* :github:`5565` - net: Kconfig: NET_BUF_LOG and NET_BUF_SIMPLE_LOG unrightly select STDOUT_CONSOLE
* :github:`5566` - kconfig: STDOUT_CONSOLE unrightly stuffed under subsys/debug
* :github:`5576` - None of the :github:'XYZ' links work in the 1.10 release notes
* :github:`5589` - Issue with using generic gcc cross compile with cmake
* :github:`5601` - docs for cc3220sf_launchxl are out of date/incorrect
* :github:`5608` - Bluetooth LE continous scan weird behaviour
* :github:`5619` - zephyr.git/tests/misc/test_build/testcase.yaml#test_newlib @ esp32:xtensa BUILD failed
* :github:`5626` - Building samples failed
* :github:`5640` - MacOS compile error with -std=gnu89
* :github:`5645` - build failures with asserts enabled/newlib: arch/arm/core/cortex_m/mpu/nxp_mpu.c
* :github:`5646` - userbuffer_validate test fails with double fault if CONFIG_USERSPACE disabled
* :github:`5650` - i2c driver test on ESP32 fails with error
* :github:`5651` - [In Progress] arch: arm: linkder: vt must be linked at address 0x00000000 for Cortex-M0
* :github:`5660` - doc: make: make htmldocs fails in genrest.py/kconfiglib.py
* :github:`5673` - kconfig regression: Existing configuration is overwritten on reconfiguration
* :github:`5687` - docs: Confusing treatment of "Sensor Drivers" in Zephyr subsystem docs
* :github:`5692` - sensors: struct sensor_value::val2 is confusingly defined
* :github:`5693` - sensors: SENSOR_CHAN_HUMIDITY confusingly defined in "milli percent", SENSOR_CHAN_DISTANCE inconsistently defined in millimeters
* :github:`5696` - net_app: Static vs DHCPv4 behavior appears to be not as described
* :github:`5699` - Zephyr installs a broken pyOCD
* :github:`5717` - CONTRIBUTING instructions are Linux-specific and don't work for Windows
* :github:`5719` - need a zephyr-env.sh equivalent for Windows developers
* :github:`5720` - Add CONFIG_NOOPTIMIZATIONS option
* :github:`5722` - dts board configuration is incompatible with "build all" kind of test
* :github:`5724` - [Windows] Instructions for setting up a bash-less environment uses bashisms
* :github:`5726` - CI should use the same generator as the sanitycheck default
* :github:`5737` - [Coverity CID: 182195] Error handling issues in /subsys/fs/fcb/fcb_walk.c
* :github:`5740` - [Coverity CID: 181923] Incorrect expression in /subsys/bluetooth/controller/ll_sw/ctrl.c
* :github:`5741` - [Coverity CID: 181922] Incorrect expression in /subsys/bluetooth/controller/ll_sw/ctrl.c
* :github:`5743` - Windows and Linux are writing .config files with options re-ordered
* :github:`5749` - Exception and Interrupt vector forwarding
* :github:`5753` - Bluetooth: controller: In nRF5 radio. c RATEBOOST event not cleared in ISRs
* :github:`5755` - Support flash in jlink runner
* :github:`5756` - MCUboot-compatible builds in Zephyr
* :github:`5760` - doc: device.h defines device_power_management_api group twice
* :github:`5761` - NRF5 BLE radio driver: PPI18 is cleared unconditionally
* :github:`5762` - Windows 10 WSL does not supports Native POSIX Boards
* :github:`5766` - boards: nucleo_f413zh: Likely outdated OpenOCD info in docs
* :github:`5771` - Linking issues with host cross compile with cmake
* :github:`5772` - sanitycheck crashes if ZEPHYR_BASE has symlinks in its path
* :github:`5778` - Add/fix flash controller nodes for NXP kinetis SoCs
* :github:`5779` - bluetooth test_controller_4_0 fails to build on nrf52840_pca10056
* :github:`5784` - make rom_report fails for qemu_x86 (not finding zephyr.bin)
* :github:`5794` - wiki/Development-Model is out of date
* :github:`5808` - msys2 getting started instructions are missing Ninja install step
* :github:`5817` - socket.h: Using #define for POSIX redefinition of zsock\_ functions has unintended consequences
* :github:`5821` - [MSYS2] Unable to build Zephyr
* :github:`5823` - Bluetooth: Collision during Start Encryption procedure
* :github:`5836` - spi: stm32: convert remaining boards that support SPI to using dts
* :github:`5853` - Using newlibc in a project breaks 'rom_report' and 'ram_report' targets.
* :github:`5866` - ram_report not working for qemu targets
* :github:`5877` - sensors: Cleanup Kconfig for address, driver & bus name
* :github:`5881` - enabling THREAD_MONITOR causes tests to fail
* :github:`5886` - [Coverity CID: 182602] Integer handling issues in /drivers/interrupt_controller/system_apic.c
* :github:`5887` - [Coverity CID: 182597] Control flow issues in /drivers/sensor/vl53l0x/vl53l0x.c
* :github:`5888` - [Coverity CID: 182594] Control flow issues in /drivers/sensor/lsm6ds0/lsm6ds0.c
* :github:`5889` - [Coverity CID: 182593] Control flow issues in /drivers/sensor/vl53l0x/vl53l0x.c
* :github:`5890` - [Coverity CID: 182588] Integer handling issues in /drivers/sensor/hts221/hts221.c
* :github:`5903` - Code coverage reports seem wrong
* :github:`5919` - Remove obsolete FLASH_DRIVER_NAME
* :github:`5938` - Incorrectly reported coverage changes
* :github:`5952` - API k_delayed_work_submit_to_queue() make a delayed_work unusable
* :github:`5958` - "Ninja flash" swallows user prompts
* :github:`5968` - datastructure for LIFO
* :github:`5982` - nRF5x  subscribe will cause HardFault while disconnect and reconnect
* :github:`5989` - workstation setup instructions are broken for Fedora
* :github:`5992` - doc: Discrepancy in Zephyr memory domain API documentation
* :github:`5994` - samples/bluetooth/ipsp: build failed for MICRO-BIT & NRF51-PCA10028 HW
* :github:`5996` - Need a "ps aux" like command to list all running threads and their attributes
* :github:`6010` - Removal of old HTTP API is causing errors and faults
* :github:`6013` - updated workstations setup breaks FC27
* :github:`6023` - Bluetooth: Invalid behaviour of Transport Layer after Incomplete timer expiration
* :github:`6025` - mbedTLS: Buffer overflow security issue, requires upgrade to 2.7.0
* :github:`6050` - IPSP sample failed: Cannot bind IPv6 mcast (-2)
* :github:`6062` - build failure in tests/boards/altera_max10/i2c_master with sys log enabled
* :github:`6064` - k_is_in_isr() returns false inside "direct" interrupts on several arches
* :github:`6081` - echo server crash from corrupt ICMPv4 packet
* :github:`6083` - Bluetooth: Regression in MESH tests
* :github:`6091` - [Coverity CID: 182780] Error handling issues in /samples/net/sockets/http_get/src/http_get.c
* :github:`6092` - [Coverity CID: 182779] Memory - corruptions in /drivers/flash/soc_flash_nios2_qspi.c
* :github:`6102` - [Coverity CID: 182769] Error handling issues in /subsys/bluetooth/host/mesh/beacon.c
* :github:`6121` - doc: unit tests documentation refers to non existing sample code
* :github:`6127` - net: Semantics of CONFIG_NET_BUF_POOL_USAGE changed (incorrectly)
* :github:`6131` - mbedtls: Name of config-mini-tls1_2.h contradicts description
* :github:`6135` - build error with gcc 7.3
* :github:`6164` - timer: cortex_m: Incorrect read of clock cycles counter after idle tickless period
* :github:`6185` - [MSYS2] Unable to build hello_world sample
* :github:`6194` - K64F ethernet regression since f7ec62eb
* :github:`6197` - echo server crash from corrupt ICMPv6 packet
* :github:`6204` - bluetooth controller: crc init is not random
* :github:`6217` - echo server crash from corrupt ICMPv6 NS packet
* :github:`6229` - Bluetooth, nRF51: ticker_success_assert during flash erase
* :github:`6231` - samples/bluetooth/eddystone: failed to connect with central device
* :github:`6232` - samples/bluetooth/central_hr: Run time HARD fault occurs
* :github:`6233` - samples/bluetooth/central: Run time HARD fault occurs
* :github:`6235` - echo server crash from ICMPv6 NS source link layer address anomaly
* :github:`6238` - spi: stm32f0 IRQ priority is invalid
* :github:`6240` - "Previous execution" and "Next execution" display problem.
* :github:`6257` - test, please ignore
* :github:`6261` - [Coverity CID: 182887] Control flow issues in /drivers/gpio/gpio_esp32.c
* :github:`6263` - ARC: Implement userspace
* :github:`6264` - ARM: Implement Userspace
* :github:`6279` - Add doc to samples/bluetooth/mesh & samples/bluetooth/mesh_demo
* :github:`6284` - docs.zephyrproject.org should be served with HTTPS
* :github:`6309` - Non-blocking BSD sockets not working as expected
* :github:`6312` - The shell sample does not working on k64f board
* :github:`6315` - echo server crash from malformed ICMPv6 NA
* :github:`6322` - shell crashes when enter is pressed
* :github:`6323` - "SPI master port SPI_1 not found* when porting spi ethernet device enc28j60 on stm32_min_dev board
* :github:`6324` - doc: Project coding standards: page not found
* :github:`6333` - How do I implement GPIO on the f429zi board?
* :github:`6339` - samples/drivers/gpio Sample doesn't work on ESP32 after SMP support was added
* :github:`6346` - ESP-32 preemption regressions with asm2
* :github:`6382` - echo server: crash from unsolicited RA reachable time anomaly
* :github:`6393` - echo server: crash from NS flood
* :github:`6429` - How to add custom driver subdirectory to application project?
* :github:`6432` - daily doc build pages don't indicate their version
* :github:`6439` - ESP32 doesn't compile in master
* :github:`6444` - tests/kernel/mem_protect/obj_validation/ fails to build
* :github:`6455` - k_sleep() fails on ESP32 sometimes
* :github:`6469` - tests/crypto/ecc_dsa results in FATAL EXCEPTION on esp32
* :github:`6470` - tests/crypto/ecc_dh results in FATAL EXCEPTION on esp32
* :github:`6471` - tests/crypto/aes results in Assertion failure on esp32
* :github:`6472` - tests/crypto/sha256 results in Assertion failure on esp32
* :github:`6505` - Userspace support: stack corruption for ARC em7d v2.3

