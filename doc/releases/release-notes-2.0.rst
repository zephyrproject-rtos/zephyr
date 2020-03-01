:orphan:

.. _zephyr_2.0:

Zephyr 2.0.0
############

We are pleased to announce the release of Zephyr RTOS version 2.0.0.

Major enhancements with this release include:

* The kernel now supports both 32- and 64-bit architectures.
* We added support for SOCKS5 proxy. SOCKS5 is an Internet protocol that
  exchanges network packets between a client and server through a proxy server.
* Introduced support for 6LoCAN, a 6Lo adaption layer for Controller Area
  Networks.
* We added support for :ref:`Point-to-Point Protocol (PPP) <ppp>`. PPP is a
  data link layer (layer 2) communications protocol used to establish a direct
  connection between two nodes.
* We added support for UpdateHub, an end-to-end solution for large scale
  over-the-air device updates.
* We added support for ARM Cortex-R Architecture (Experimental).

The following sections provide detailed lists of changes by component.

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

Kernel
******

* New kernel API for per-thread disabling of Floating Point Services for
  ARC, ARM Cortex-M, and x86 architectures.
* New system call to set the clock frequency at runtime.
* Additional support for compatibility with 64-bit architectures.
* Userspace mutexes are now supported through the new k_futex primitive.
* Improvements to the slab allocator.
* Fixed the implementation of k_thread_name_set() with userspace enabled.
* Boosted the default tick rate for tickless kernels to improve the
  precision of timeouts.

Architectures
*************

* ARM:

  * Added initial support for ARM Cortex-R architecture (Experimental)
  * We enhanced the support for Floating Point Services in Cortex-M
    architecture, implementing and enabling lazy-stacking for FPU
    capable threads and fixing stack overflow detection for FPU
    capable supervisor threads
  * Added QEMU support for ARMv8-M Mainline architecture
  * Optimized the IRQ locking time in thread context switch
  * Fixed several critical bugs in User Mode implementation
  * Added test coverage for ARM-specific kernel features
  * Improved support for linking TrustZone Secure Entry functions into
    Non-Secure firmware

* ARC:

  * Added support for ARC HS architecture
  * Added SMP support for ARC HS architecture
  * Added support for ARC SecureShield based TEE (Experimental)
  * Fixed several critical bugs in interrupt and exception handling
  * Enhance the support for Floating Point Services

* POSIX:

  * Fixed race condition with terminated threads which had never been
    scheduled by kernel. On very loaded systems it could cause swap errors.

* x86:

  * Dropped support for the Intel Quark microcontroller family.
  * A new lightweight PCI implementation has been introduced which supports
    MSI and other features required for PCIe devices. The previous PCI
    implementation has been deprecated and will be removed in 2.1.

* RISC-V:

  * Added support for the SiFive HiFive1 Rev B development board.
  * Added support for LiteX VexRiscv soft core.
  * Added support for 64-bit RISC-V, renaming the architecture from "riscv32"
    to "riscv".
  * Added 64-bit QEMU support.
  * Added DeviceTree bindings for RISC-V memory devices, CPU interrupt
    controllers, and platform-local interrupt controllers.

Boards & SoC Support
********************

* Added native_posix_64: A 64 bit variant of native_posix
* Added support for these ARC boards:

  .. rst-class:: rst-columns

     * emsdp
     * hsdk
     * nsim for hs

* Added support for these ARM boards:

  .. rst-class:: rst-columns

     * atsamr21_xpro
     * cc1352r1_launchxl
     * cc26x2r1_launchxl
     * holyiot_yj16019
     * lpcxpresso55s69
     * mec15xxevb_assy6853
     * mikroe_mini_m4_for_stm32
     * mimxrt1015_evk
     * mps2_an521
     * nrf51_pca10031
     * nrf52811_pca10056
     * nucleo_g071rb
     * nucleo_wb55rg
     * qemu_cortex_r5
     * stm32h747i_disco
     * stm32mp157c_dk2
     * twr_ke18f
     * v2m_musca_b1
     * 96b_avenger96
     * 96b_meerkat96
     * 96b_wistrio

* Added support for these RISC-V boards:

  .. rst-class:: rst-columns

     * hifive1_revb
     * litex_vexriscv
     * qemu_riscv64

* Added support for the gpmrb x86 board

* Added support for these following shields:

  .. rst-class:: rst-columns

     * frdm_cr20a
     * link_board_can
     * sparkfun_sara_r4
     * wnc_m14a2a
     * x_nucleo_iks01a3

* Removed support for these boards:

  .. rst-class:: rst-columns

     * arduino_101
     * arduino_101_sss
     * curie_ble
     * galileo
     * quark_d2000_crb
     * quark_se_c1000_devboard
     * quark_se_c1000_ss_devboard
     * quark_se_c1000_ble
     * tinytile
     * x86_jailhouse

Drivers and Sensors
*******************

* ADC

  * Added API to support calibration
  * Enabled ADC on STM32WB
  * Removed Quark D2000 ADC driver
  * Added NXP ADC12 and SAM0 ADC drivers
  * Added ADC shell

* Audio

  * Added support for two microphones (stereo) in the mpxxdtyy driver

* CAN

  * Added support for canbus Ethernet translator
  * Added 6LoCAN implementation
  * Added MCP2515, NXP FlexCAN, and loopback drivers
  * Added CAN shell

* Clock Control

  * Added NXP Kinetis MCG, SCG, and PCC drivers
  * Added STM32H7, STM32L1X, and STM32WB support
  * Removed Quark SE driver

* Counter

  * Added optional flags to alarm configuration structure and extended set channel alarm flags
  * Added top_value setting configuration structure to API
  * Enabled counter for STM32WB
  * Added NXP GPT, "CMOS" RTC, SiLabs RTCC, and SAM0 drivers
  * Removed Quark D2000 support from QMSI driver

* Display

  * Added ST7789V based LCD driver
  * Renamed ssd1673 driver to ssd16xx
  * Added framebuffer driver with multiboot support
  * Added support for Seeed 2.8" TFT touch shield v2.0

* DMA

  * Added API to retrieve runtime status
  * Added SAM0 DMAC driver
  * Removed Quark SE C1000 support from QMSI driver

* Entropy

  * Added TI CC13xx / CC26xx driver

* ESPI

  * Added Microchip XEC driver

* Ethernet

  * Added LiteEth driver

* Flash

  * Removed Quark SE C1000 driver
  * Removed support for Quark D2000 from QMSI driver
  * Added STM32G0XX and STM32WB support to STM32 driver
  * Added RV32M1 and native POSIX drivers

* GPIO

  * Added stm32f1x SWJ configuration
  * Removed Quark SE C1000 and D2000 support from DesignWare driver
  * Added support for STM32H7, STM32L1X, and STM32WB to STM32 driver
  * Added Microchip XEC and TI CC13x2 / CC26x2 drivers
  * Added HT16K33 LED driver
  * Added interrupt support to SAM0 driver

* Hardware Info

  * Added ESP32 and SAM0 drivers

* I2C

  * Added support for STM32MP1, STM32WB, and STM32L1X to STM32 driver
  * Added STM32F10X slave support
  * Added power management to nrf TWI and TWIM drivers
  * Added TI CC13xx / CC26xx, Microchip MEC, SAM0, and RV32M1 drivers
  * Rewrote DesignWare driver for PCI(e) support

* IEEE 802.15.4

  * Fixed KW41z fault and dBm mapping

* Interrupt Controller

  * Added initial support for ARC TCC
  * Added GIC400, LiteX, and SAM0 EIC drivers
  * Added support for STM32G0X, STM32H7, STM32WB, and STM32MP1 to STM32 driver
  * Removed MVIC driver

* IPM

  * Removed Quark SE driver
  * Added MHU and STM32 drivers

* LED

  * Added Holtek HT16K33 LED driver

* Modem

  * Introduced socket helper layer
  * Introduced command handler and UART interface driver layers
  * Introduced modem context helper driver
  * Added u-blox SARA-R4 modem driver

* Pinmux

  * Added SPI support to STM32MP1
  * Enabled ADC, PWM, I2C, and SPI pins on STM32WB
  * Added Microchip XEC and TI CC13x2 / CC26x2 drivers

* PWM

  * Added NXP PWM driver
  * Added support for STM32G0XX to STM32 driver

* Sensor

  * Added STTS751 temperature sensor driver
  * Added LSM6DSO and LPS22HH drivers
  * Renamed HDC1008 driver to ti_hdc and added support for 1050 version
  * Added LED current, proximity pulse length, ALS, and proximity gain configurations to APDS9960 driver
  * Reworked temperature and acceleration conversions, and added interrupt handling in ADXL362 driver
  * Added BME680 driver and AMS ENS210 drivers

* Serial

  * Added Xilinx ZynqMP PS, LiteUART, and TI CC12x2 / CC26x2 drivers
  * Added support for virtual UARTS over RTT channels
  * Added support for STM32H7 to STM32 driver
  * Removed support for Quark D2000 from QMSI driver
  * Enabled interrupts in LPC driver
  * Implemented ASYNC API in SAM0 driver
  * Added PCI(e) support to NS16550 driver

* SPI

  * Added support for STM32MP1X and STM32WB to STM32 driver
  * Removed support for Quark SE C1000 from DesignWare driver
  * Added TI CC13xx / CC26xx driver
  * Implemented ASYNC API in SAM0 driver

* Timer

  * Added Xilinx ZynqMP PS ttc driver
  * Added support for SMP to ARC V2 driver
  * Added MEC1501 32 KHZ, local APIC timer, and LiteX drivers
  * Replaced native POSIX system timer driver with tickless support
  * Removed default selection of SYSTICK timer for ARM platforms

* USB

  * Added NXP EHCI driver
  * Implemented missing API functions in SAM0 driver

* WiFi

  * Implemented TCP listen/accept and UDP support in eswifi driver

Networking
**********

* Added support for `SOCKS5 proxy <https://en.wikipedia.org/wiki/SOCKS>`__.
  See also `RFC1928 <https://tools.ietf.org/html/rfc1928>`__ for details.
* Added support for 6LoCAN, a 6Lo adaption layer for Controller Area Networks.
* Added support for :ref:`Point-to-Point Protocol (PPP) <ppp>`.
* Added support for UpdateHub, an end-to-end solution for large scale
  over-the-air update of devices.
  See `UpdateHub.io <https://updatehub.io/>`__ for details.
* Added support to automatically register network socket family.
* Added support for ``getsockname()`` function.
* Added SO_PRIORITY support to ``setsockopt()``
* Added support for VLAN tag stripping.
* Added IEEE 802.15.4 API for ACK configuration.
* Added dispatching support to SocketCAN sockets.
* Added user mode support to PTP clock API.
* Added user mode support to network interface address functions.
* Added AF_NET_MGMT socket address family support. This is for receiving network
  event information in user mode application.
* Added user mode support to ``net_addr_ntop()`` and ``net_addr_pton()``
* Added support for sending network management events when DNS server is added
  or deleted.
* Added LiteEth Ethernet driver.
* Added support for ``sendmsg()`` API.
* Added `civetweb <https://civetweb.github.io/civetweb/>`__ HTTP API support.
* Added LWM2M IPSO Accelerometer, Push Button, On/Off Switch and Buzzer object
  support.
* Added LWM2M Location and Connection Monitoring object support.
* Added network management L4 layer. The L4 management events are used
  when monitoring network connectivity.
* Allow net-mgmt API to pass information length to application.
* Removed network management L1 layer as it was never used.
* By default a network interface is set to UP when the device starts.
  If this is not desired, then it is possible to disable automatic start.
* Allow collecting network packet TX throughput times in the network stack.
  This information can be seen in net-shell.
* net-shell Ping command overhaul.
* Accept UDP packet with missing checksum.
* 6lo compression rework.
* Incoming connection handling refactoring.
* Network interface refactoring.
* IPv6 fragmentation fixes.
* TCP data length fixes if TCP options are present.
* SNTP client updates.
* Trickle timer re-init fixes.
* ``getaddrinfo()`` fixes.
* Fixes in DHCPv4, LWM2M, gPTP, and MQTT
* DNS fixes for non-compressed answers.
* mDNS and LLMNR resolver fixes.
* Ethernet ARP fixes.
* OpenThread updates and fixes.
* Network device driver fixes for:

  .. rst-class:: rst-columns

     - Ethernet e1000
     - Ethernet enc28j60
     - Ethernet mcux
     - Ethernet stellaris
     - Ethernet gmac
     - Ethernet stm32
     - WiFi eswifi
     - IEEE 802.15.4 kw41z
     - IEEE 802.15.4 nrf5

Bluetooth
*********

* Host:

  * GATT: Added support for database hashes, Read Using Characteristic
    UUID, static services, disabling the dynamic database, and notifying
    and indicating by UUID
  * GATT: Simplified the bt_gatt_notify_cb() API
  * GATT: Added additional attributes to the Device Information Service
  * GATT: Several protocol and database fixes
  * Settings: Transitioned to new optimized settings model and support for custom backends
  * Completed support for directed advertising and Out-Of-Band (OOB) pairing
  * Added support for fine-grained control of security establishment, including
    forcing a pairing procedure in case of key loss
  * Switched to separate, dedicated pools for discardable events and number of
    completed packets events
  * Extended and improved the Bluetooth shell with several commands
  * BLE qualification up to the 5.1 specification
  * BLE Mesh: Several fixes and improvements

* BLE split software Controller:

  * The split software Controller is now the default
  * Added support for the Data Length Update procedure
  * Improved and documented the ticker packet scheduler for improved conflict resolution
  * Added support for out-of-tree user-defined commands and events,
    Zephyr Vendor Specific Commands, and user-defined protocols
  * Converted several control procedures to be queueable
  * Nordic: Decorrelated address generation from resolution
  * Nordic: Added support for Controller-based privacy, fast encryption
    setup, RSSI, low-latency ULL processing of messages, nRF52811 IC BLE
    radio, PA/LNA on Port 1 GPIO pins, and radio event abort
  * BLE qualification up to the 5.1 specification

* BLE legacy software Controller:

  * BLE qualification up to the 5.1 specification
  * Multiple control procedures fixes and improvements

Build and Infrastructure
************************

* The devicetree Python scripts have been rewritten to be more robust and
  easier to understand and change. The new scripts are these three files:

  - :zephyr_file:`scripts/dts/dtlib.py` -- a low-level :file:`.dts` parsing
    library

  - :zephyr_file:`scripts/dts/edtlib.py` -- a higher-level library that adds
    information from bindings

  - :zephyr_file:`scripts/dts/gen_defines.py` -- generates a C header from the
    devicetree files for the board

  The new scripts verify ``category: optional/required`` and ``type:`` settings
  given in bindings for nodes, and add some new types, like ``phandle-array``.
  Error messages and other output is now more helpful.

  See the updated documentation in :zephyr_file:`dts/binding-template.yaml`.

  The old scripts are kept around to generate a few deprecated ``#define``\ s.
  They will be removed in the Zephyr 2.2 release.

* Changed ARM Embedded toolchain to default to nano variant of newlib


Libraries / Subsystems
***********************

* File Systems: Added support for littlefs

HALs
****

* HALs are now moved out of the main tree as external modules and reside
  in their own standalone repositories.

Documentation
*************

* We've made many updates to component, subsystem, and process
  documentation bringing our documentation up-to-date with code changes,
  additions, and improvements, as well as new supported boards and
  samples.

Tests and Samples
*****************

* We have implemented additional tests and significantly expanded the
  amount of test cases in existing tests to increase code coverage.

Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.14.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`18964` - [Coverity CID :203911]Memory - corruptions in /tests/bluetooth/uuid/src/main.c
* :github:`18963` - [Coverity CID :203910]Memory - corruptions in /tests/bluetooth/uuid/src/main.c
* :github:`18959` - [Coverity CID :203907]Parse warnings in /include/bluetooth/conn.h
* :github:`18923` - (BLE) Dynamic TX Power Control
* :github:`18906` - Problem on build when calling objcopy to generate isrList.bin
* :github:`18865` - Fatal Usage Fault When Bluetooth And OpenThread Are Enabled On NRF52840 Multiprotocol Support
* :github:`18828` - Bluetooth: controller: crash terminating link during encryption procedure
* :github:`18821` - Documentation: parent vs child in DeviceTree nodes
* :github:`18819` - Bluetooth: LL split assert upon disconnection
* :github:`18814` - Module Request: LoRaMac-Node
* :github:`18813` - fs: nvs: Cannot delete entries
* :github:`18808` - Docs for gpmrb board incorrectly refer to up_squared board
* :github:`18804` - Channel Selection Algorithm Modification In Zephyr
* :github:`18802` - Bluetooth: UUID: Missing tests and confusing documentation
* :github:`18799` - bt_uuid_create_le() and bt_uuid_create() have endianness issues, and only one of them is needed
* :github:`18795` - FS:NVS: garbage collection when restart
* :github:`18784` - Can not build link_board_can shield
* :github:`18774` - (nRF51) NVS example doesn't work
* :github:`18765` - LwM2M: DNS handling via offload socket API is broken
* :github:`18760` - hello_world sample instructions don't work
* :github:`18739` - k_uptime_get_32() does not behave as documented
* :github:`18732` - net: mDNS name resolving issue between 2 Zephyr nodes
* :github:`18726` - arc: should not rely on that ERET has a copy of ilink
* :github:`18725` - arc: the IRM bit of SEC_STAT is not handled corrected
* :github:`18724` - arc: interrupt stack is not  switched correctly
* :github:`18717` - USB broken on disco l475 iot board
* :github:`18705` - SMP fails to allocate buffer and pairing times out
* :github:`18693` - POSIX: Some headers were missing from PR #16621
* :github:`18687` - [Coverity CID :203623]Memory - illegal accesses in /tests/subsys/settings/fcb_init/src/settings_test_fcb_init.c
* :github:`18686` - [Coverity CID :203622]Parse warnings in /opt/zephyr-sdk-0.10.3/nios2-zephyr-elf/nios2-zephyr-elf/include/c++/8.3.0/bits/refwrap.h
* :github:`18685` - [Coverity CID :203621]Parse warnings in /opt/zephyr-sdk-0.10.3/nios2-zephyr-elf/nios2-zephyr-elf/include/c++/8.3.0/bits/refwrap.h
* :github:`18684` - [Coverity CID :203620]Parse warnings in /opt/zephyr-sdk-0.10.3/nios2-zephyr-elf/nios2-zephyr-elf/include/c++/8.3.0/bits/refwrap.h
* :github:`18683` - [Coverity CID :190988]Memory - illegal accesses in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/drivers/imx/fsl_elcdif.c
* :github:`18682` - [Coverity CID :190984]Memory - illegal accesses in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/drivers/imx/fsl_elcdif.c
* :github:`18681` - [Coverity CID :190979]Memory - illegal accesses in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/drivers/imx/fsl_elcdif.c
* :github:`18680` - [Coverity CID :190959]Memory - illegal accesses in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/drivers/imx/fsl_elcdif.c
* :github:`18679` - [Coverity CID :198643]Incorrect expression in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/devices/MKE18F16/fsl_clock.c
* :github:`18678` - [Coverity CID :198642]Incorrect expression in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/devices/MKE18F16/fsl_clock.c
* :github:`18677` - [Coverity CID :198641]Incorrect expression in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/devices/MKE18F16/fsl_clock.c
* :github:`18676` - [Coverity CID :190994]Incorrect expression in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/devices/MK64F12/fsl_clock.c
* :github:`18675` - [Coverity CID :190982]Incorrect expression in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/devices/MK64F12/fsl_clock.c
* :github:`18674` - [Coverity CID :190962]Incorrect expression in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/devices/MK64F12/fsl_clock.c
* :github:`18673` - [Coverity CID :190947]Incorrect expression in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/devices/MK64F12/fsl_clock.c
* :github:`18672` - [Coverity CID :198948]Control flow issues in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/devices/LPC55S69/fsl_clock.c
* :github:`18671` - [Coverity CID :198947]Integer handling issues in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/devices/LPC55S69/fsl_clock.c
* :github:`18670` - [Coverity CID :182600]Integer handling issues in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/devices/LPC54114/fsl_clock.c
* :github:`18669` - [Coverity CID :158891]Memory - illegal accesses in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nxp/mcux/components/phyksz8081/fsl_phy.c
* :github:`18668` - [Coverity CID :203544]Integer handling issues in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nordic/nrfx/drivers/src/nrfx_usbd.c
* :github:`18667` - [Coverity CID :203513]Integer handling issues in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nordic/nrfx/drivers/src/nrfx_usbd.c
* :github:`18666` - [Coverity CID :203506]Integer handling issues in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nordic/nrfx/drivers/src/nrfx_usbd.c
* :github:`18665` - [Coverity CID :203436]Memory - illegal accesses in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/hal/nordic/nrfx/drivers/src/nrfx_usbd.c
* :github:`18664` - [Coverity CID :203416]Uninitialized variables in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/fs/littlefs/lfs.c
* :github:`18663` - [Coverity CID :203413]Null pointer dereferences in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/fs/littlefs/lfs.c
* :github:`18662` - [Coverity CID :61908]Insecure data handling in /home/aasthagr/zephyrproject-external-coverity-new/zephyrproject/modules/crypto/mbedtls/library/ssl_tls.c
* :github:`18658` - Bluetooth BR/EDR encryption key negotiation vulnerability
* :github:`18654` - cc3220sf_launchxl fails tests/kernel/interrupt/arch.interrupt
* :github:`18645` - Disconnect because of data packets during encryption procedure
* :github:`18615` - sam e70 xplained failed to build hello world
* :github:`18599` - tests/kernel/fifo/fifo_timeout fails on cc3220sf_launchxl
* :github:`18598` - tests/net/trickle failed on multiple plartforms
* :github:`18595` - USB CDC endless loop with BLE on NRF52
* :github:`18593` - tests/arch/arm/arm_zero_latency_irqs fails on cc3220sf_launchxl
* :github:`18592` - (nRF51) The RSSI signal does not rise above -44 dBm
* :github:`18590` - tests/kernel/fatal/kernel.common.stack_sentinel fails on FRDM-KW41Z
* :github:`18587` - tests/kernel/fifo/fifo_timeout/kernel.fifo.timeout fails to run on lpcxpresso54114_m4
* :github:`18584` - BT LL assert on LL/CON/ADV/BV-04-C
* :github:`18580` - Bluetooth: Security fail on initial pairing
* :github:`18574` - Some platforms: “reel_board”, “frdm_k64f” and “sam_e70_xplained”  are be built failure
* :github:`18572` - Bluetooth: GATT: Unable to indicate by UUID
* :github:`18563` - log_strdup missing error messages seen when running wifi sample
* :github:`18547` - Bluetooth: GATT: Fix using variable size storage for CCC
* :github:`18546` - Hard Fault when connecting to BLE device
* :github:`18524` - No disconnection event during "heavy" indication stream
* :github:`18522` - BLE: Mesh: When transport send seg_msg to LPN
* :github:`18521` - BLE: Mesh: when friend send msg to LPN
* :github:`18508` - tests/net/trickle failed on frdm_k64f board
* :github:`18476` - Custom module with west
* :github:`18462` - potential buffer overrun in logging infrastructure
* :github:`18461` - [Coverity CID :203487]Parse warnings in /usr/lib/gcc/x86_64-redhat-linux/8/include/stdint-gcc.h
* :github:`18460` - [Coverity CID :203527]Parse warnings in /usr/include/unistd.h
* :github:`18459` - [Coverity CID :203509]Null pointer dereferences in /tests/subsys/usb/desc_sections/src/desc_sections.c
* :github:`18458` - [Coverity CID :203422]Memory - illegal accesses in /tests/subsys/fs/littlefs/src/testfs_util.c
* :github:`18457` - [Coverity CID :203419]Security best practices violations in /tests/net/traffic_class/src/main.c
* :github:`18456` - [Coverity CID :203401]Security best practices violations in /tests/net/traffic_class/src/main.c
* :github:`18455` - [Coverity CID :203490]Error handling issues in /tests/net/socket/net_mgmt/src/main.c
* :github:`18454` - [Coverity CID :203499]Null pointer dereferences in /tests/net/icmpv6/src/main.c
* :github:`18453` - [Coverity CID :203480]Null pointer dereferences in /tests/net/context/src/main.c
* :github:`18446` - [Coverity CID :203532]Incorrect expression in /tests/kernel/sched/schedule_api/src/user_api.c
* :github:`18445` - [Coverity CID :203507]Error handling issues in /tests/kernel/mutex/sys_mutex/src/main.c
* :github:`18444` - [Coverity CID :203516]Memory - corruptions in /tests/kernel/mem_protect/userspace/src/main.c
* :github:`18443` - [Coverity CID :203454]Error handling issues in /tests/kernel/mem_protect/sys_sem/src/main.c
* :github:`18442` - [Coverity CID :203465]Memory - corruptions in /tests/kernel/mem_protect/protection/src/main.c
* :github:`18439` - [Coverity CID :203437]Incorrect expression in /tests/kernel/fp_sharing/float_disable/src/k_float_disable.c
* :github:`18438` - [Coverity CID :203407]Incorrect expression in /tests/kernel/fp_sharing/float_disable/src/k_float_disable.c
* :github:`18437` - [Coverity CID :203478]Program hangs in /tests/kernel/common/src/sflist.c
* :github:`18436` - [Coverity CID :203424]Control flow issues in /tests/kernel/common/src/sflist.c
* :github:`18434` - [Coverity CID :203486]Memory - corruptions in /tests/bluetooth/uuid/src/main.c
* :github:`18433` - [Coverity CID :203431]Memory - corruptions in /tests/bluetooth/uuid/src/main.c
* :github:`18432` - [Coverity CID :203502]Error handling issues in /tests/bluetooth/tester/src/gap.c
* :github:`18431` - [Coverity CID :203391]Null pointer dereferences in /tests/bluetooth/gatt/src/main.c
* :github:`18430` - [Coverity CID :203540]Incorrect expression in /tests/arch/arm/arm_zero_latency_irqs/src/arm_zero_latency_irqs.c
* :github:`18429` - [Coverity CID :203525]Incorrect expression in /tests/arch/arm/arm_thread_swap/src/arm_thread_arch.c
* :github:`18428` - [Coverity CID :203479]Incorrect expression in /tests/arch/arm/arm_thread_swap/src/arm_thread_arch.c
* :github:`18427` - [Coverity CID :203392]Incorrect expression in /tests/arch/arm/arm_thread_swap/src/arm_thread_arch.c
* :github:`18426` - [Coverity CID :203455]Incorrect expression in /tests/arch/arm/arm_ramfunc/src/arm_ramfunc.c
* :github:`18424` - [Coverity CID :203489]Memory - corruptions in /tests/application_development/gen_inc_file/src/main.c
* :github:`18423` - [Coverity CID :203473]Null pointer dereferences in /subsys/usb/usb_descriptor.c
* :github:`18421` - [Coverity CID :203504]Uninitialized variables in /subsys/net/lib/sockets/sockets_net_mgmt.c
* :github:`18420` - [Coverity CID :203468]Control flow issues in /subsys/net/lib/sockets/sockets_net_mgmt.c
* :github:`18419` - [Coverity CID :203397]Control flow issues in /subsys/net/lib/sockets/sockets_net_mgmt.c
* :github:`18418` - [Coverity CID :203445]Error handling issues in /subsys/net/lib/sockets/getnameinfo.c
* :github:`18417` - [Coverity CID :203501]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_timer.c
* :github:`18416` - [Coverity CID :203475]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_timer.c
* :github:`18415` - [Coverity CID :203420]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_timer.c
* :github:`18414` - [Coverity CID :203496]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_push_button.c
* :github:`18413` - [Coverity CID :203488]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_push_button.c
* :github:`18412` - [Coverity CID :203482]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_push_button.c
* :github:`18411` - [Coverity CID :203450]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_onoff_switch.c
* :github:`18410` - [Coverity CID :203448]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_onoff_switch.c
* :github:`18409` - [Coverity CID :203427]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_onoff_switch.c
* :github:`18408` - [Coverity CID :203533]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_light_control.c
* :github:`18407` - [Coverity CID :203519]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_light_control.c
* :github:`18406` - [Coverity CID :203511]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_buzzer.c
* :github:`18405` - [Coverity CID :203426]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_buzzer.c
* :github:`18404` - [Coverity CID :203414]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_buzzer.c
* :github:`18403` - [Coverity CID :203539]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_accelerometer.c
* :github:`18402` - [Coverity CID :203530]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_accelerometer.c
* :github:`18401` - [Coverity CID :203438]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_accelerometer.c
* :github:`18400` - [Coverity CID :203483]Control flow issues in /subsys/net/lib/conn_mgr/events_handler.c
* :github:`18399` - [Coverity CID :203457]Control flow issues in /subsys/net/l2/ppp/lcp.c
* :github:`18398` - [Coverity CID :203514]Control flow issues in /subsys/net/l2/ppp/ipv6cp.c
* :github:`18397` - [Coverity CID :203512]Memory - corruptions in /subsys/net/l2/ppp/ipv6cp.c
* :github:`18396` - [Coverity CID :203435]Error handling issues in /subsys/net/l2/ppp/fsm.c
* :github:`18395` - [Coverity CID :203471]Memory - corruptions in /subsys/net/l2/ethernet/gptp/gptp_mi.c
* :github:`18394` - [Coverity CID :203464]Memory - corruptions in /subsys/net/l2/ethernet/gptp/gptp_mi.c
* :github:`18393` - [Coverity CID :203541]Integer handling issues in /subsys/net/ip/6lo.c
* :github:`18392` - [Coverity CID :203494]Integer handling issues in /subsys/fs/littlefs_fs.c
* :github:`18391` - [Coverity CID :203403]Memory - corruptions in /subsys/disk/disk_access_usdhc.c
* :github:`18390` - [Coverity CID :203441]Null pointer dereferences in /subsys/bluetooth/mesh/transport.c
* :github:`18389` - [Coverity CID :203396]Null pointer dereferences in /subsys/bluetooth/mesh/access.c
* :github:`18388` - [Coverity CID :203545]Memory - corruptions in /subsys/bluetooth/host/smp.c
* :github:`18387` - [Coverity CID :203536]Memory - corruptions in /subsys/bluetooth/host/smp.c
* :github:`18385` - [Coverity CID :203534]Memory - corruptions in /subsys/bluetooth/host/hci_core.c
* :github:`18384` - [Coverity CID :203495]Control flow issues in /subsys/bluetooth/host/gatt.c
* :github:`18383` - [Coverity CID :203447]Memory - corruptions in /subsys/bluetooth/host/att.c
* :github:`18382` - [Coverity CID :203524]Incorrect expression in /subsys/bluetooth/controller/ticker/ticker.c
* :github:`18381` - [Coverity CID :203393]Control flow issues in /subsys/bluetooth/controller/ll_sw/ull_conn.c
* :github:`18380` - [Coverity CID :203461]Null pointer dereferences in /subsys/bluetooth/controller/ll_sw/ull.c
* :github:`18379` - [Coverity CID :203493]Control flow issues in /soc/arm/st_stm32/stm32h7/soc_m7.c
* :github:`18377` - [Coverity CID :203535]Error handling issues in /samples/net/sockets/civetweb/src/main.c
* :github:`18376` - [Coverity CID :203462]Error handling issues in /samples/net/sockets/civetweb/src/main.c
* :github:`18375` - [Coverity CID :203440]Null pointer dereferences in /samples/net/nats/src/main.c
* :github:`18374` - [Coverity CID :203523]Error handling issues in /samples/drivers/counter/alarm/src/main.c
* :github:`18372` - [Coverity CID :203543]Memory - illegal accesses in /samples/bluetooth/eddystone/src/main.c
* :github:`18371` - [Coverity CID :203542]Error handling issues in /lib/posix/pthread.c
* :github:`18370` - [Coverity CID :203469]Memory - corruptions in /drivers/wifi/eswifi/eswifi_core.c
* :github:`18369` - [Coverity CID :203425]Memory - corruptions in /drivers/wifi/eswifi/eswifi_core.c
* :github:`18368` - [Coverity CID :203411]Memory - corruptions in /drivers/wifi/eswifi/eswifi_core.c
* :github:`18367` - [Coverity CID :203409]Memory - corruptions in /drivers/wifi/eswifi/eswifi_core.c
* :github:`18366` - [Coverity CID :203452]Control flow issues in /drivers/timer/xlnx_psttc_timer.c
* :github:`18365` - [Coverity CID :203434]Control flow issues in /drivers/timer/xlnx_psttc_timer.c
* :github:`18364` - [Coverity CID :203467]Memory - corruptions in /drivers/sensor/lis2dh/lis2dh_trigger.c
* :github:`18363` - [Coverity CID :203492]Memory - corruptions in /drivers/net/ppp.c
* :github:`18362` - [Coverity CID :203412]Control flow issues in /drivers/net/ppp.c
* :github:`18361` - [Coverity CID :203515]Uninitialized variables in /drivers/flash/flash_stm32l4x.c
* :github:`18360` - [Coverity CID :203531]Memory - corruptions in /drivers/espi/espi_mchp_xec.c
* :github:`18359` - [Coverity CID :203521]Memory - illegal accesses in /drivers/espi/espi_mchp_xec.c
* :github:`18358` - [Coverity CID :203497]Memory - corruptions in /drivers/espi/espi_mchp_xec.c
* :github:`18357` - [Coverity CID :203485]Memory - illegal accesses in /drivers/espi/espi_mchp_xec.c
* :github:`18356` - [Coverity CID :203430]Integer handling issues in /drivers/espi/espi_mchp_xec.c
* :github:`18355` - [Coverity CID :203466]Memory - illegal accesses in /drivers/can/can_mcux_flexcan.c
* :github:`18354` - [Coverity CID :203449]Memory - illegal accesses in /boards/posix/native_posix/cmdline_common.c
* :github:`18353` - [Coverity CID :203522]Null pointer dereferences in /arch/arm/core/cortex_m/fault.c
* :github:`18352` - devicetree: support multiple values in io-channels
* :github:`18334` - DNS resolution is broken for some addresses in master/2.0-pre
* :github:`18326` - Bluetooth: Mesh: LPN: Remove msg from cache on rejection Enhancement
* :github:`18320` - tests/drivers/can/api/peripheral.can fail on FRDM-K64F
* :github:`18306` - Unable to reconnect paired devices with controller privacy disabled (host privacy enabled)
* :github:`18301` - menuconfig target can corrupt build configuration
* :github:`18298` - Unable to build mesh-demo for BBC micro:bit
* :github:`18292` - tests/net/lib/dns_addremove failed on frdm_k64f board.
* :github:`18284` - tests/kernel/fp_sharing/float_disable and tests/kernel/mutex/mutex_api and tests/kernel/sleep  fails on twr_ke18f
* :github:`18283` - tests/crypto/tinycrypt_hmac_prng and tests/crypto/mbedtls  and tests/posix/fs  build failure on mimxrt1015_evk
* :github:`18281` - tests/kernel/mem_protect/protection fails on LPC54114_m4
* :github:`18272` - xtensa ASM2 has no support for dynamic interrupts
* :github:`18269` - Documentation improvement for macOS
* :github:`18263` - flash sector erase fails on stm32f412
* :github:`18261` - CONFIG_TIMESLICING=n breaks kernel
* :github:`18258` - sys_get_be64() is missing from sys/byteorder.h
* :github:`18253` - Network samples echo_client doesn't work if only IPv4 enabled.
* :github:`18246` - Build failures with current tree
* :github:`18238` - drivers/modem/modem_socket: modem_socket_put() sock_fd parameter not handled correctly
* :github:`18232` - drivers: can: mcux: TX callback and can_detach don't work propperly
* :github:`18231` - MCUBoot not cleaning up properly before booting Zephyr?
* :github:`18228` - stm32h747i_disco: Fix SYS_CLOCK_TICKS_PER_SEC
* :github:`18212` - README file missing for civetweb sample
* :github:`18205` - tests/net/socket/udp fails when code coverage is enabled on qemu_x86
* :github:`18202` - Disable Duplicate scan, no longer available
* :github:`18201` - bug: west flash with --hex-file param used to work w/o path specified
* :github:`18198` - SDK 0.10.2 rv32m1_vega samples/subsys/logging/logger build fails
* :github:`18194` - [zephyr 1.14][MESH/NODE/CFG/HBP/BV-05-C] Zephyr does not send Heartbeat message on friendship termination
* :github:`18188` - [zephyr 1.14] Re-enabling CCC gets broken when used along with Robust Caching
* :github:`18183` - [zephyr 1.14][GATT/SR/GAS/BV-07-C] GATT Server does not inform change-unaware client about DB changes
* :github:`18181` - Some platforms(e.g. sam_e70_xplained) will be flashed failure if the platforms have not generated HEX file although they are built successfully.
* :github:`18178` - BLE Mesh When Provisioning Use Input OOB Method
* :github:`18171` - gen_defines creates identical labels for multicell pwms definition
* :github:`18155` - i2c_ll_stm32_v1: I2C_CR1:STOP is not cleared
* :github:`18154` - Qemu: mps2+: missing documentation
* :github:`18150` - [zephyr 1.14] Host does not change the RPA
* :github:`18141` - arc: the caculation of exception stack is wrong
* :github:`18140` - xtensa passes NULL esf on fatal error
* :github:`18132` - getting_started should indicate upgrade rather than just install west
* :github:`18131` - devicetree should check input against declared type
* :github:`18092` - Assert in BT controller on RPA timeout
* :github:`18090` - [zephyr 1.14][MESH/NODE/FRND/FN/BV-08-C] Mesh Friend queues more messages than indicates it's Friend Cache
* :github:`18080` - LWM2M bootstrap issue
* :github:`18059` - k_busy_wait passed milliseconds instead of microseconds
* :github:`18052` - z_fatal_error missing log_strdup
* :github:`18048` - [zephyr 1.14] Zephyr with privacy does not disconnect device with unresolvable RPA
* :github:`18042` - Only corporate members can join the slack channel
* :github:`18034` - It's impossible to build Zephyr via cmake/make with west 0.6.0 installed
* :github:`18029` - why kconfiglib.py doesn't throw error for file drivers/serial/Kconfig.nrfx
* :github:`18021` - Socket vtable can access null pointer callback function
* :github:`18019` - BT scan via shell fatal error
* :github:`18013` - BLE Mesh On Net Buffer free issue
* :github:`18011` - arc:  the offset generation of accl_regs is wrong
* :github:`18009` - Dead link in documentation
* :github:`18005` - BLE Mesh When Friend Clear Procedure Timeout
* :github:`18002` - Flash using open source stlink, instead of SEGGER jlink?
* :github:`17997` - fix extern "C" use throughout Zephyr
* :github:`17996` - BUILD_ASSERT not active in three of five shippable platforms
* :github:`17990` - BLE Mesh When IV update test procedure
* :github:`17979` - Security level cannot be elevated after re-connection with privacy
* :github:`17977` - BLE Mesh When IV Update Procedure
* :github:`17971` - [zephyr 1.14] Unable to register GATT service that was unregistered before
* :github:`17967` - drivers/pwm/pwm_api test failed on frdm_k64f board.
* :github:`17965` - kernel/sleep/ test failed on reel_board.
* :github:`17962` - BLE Mesh Recommended memory allocation due to who is assigned who releases the strategy
* :github:`17956` - Is POSIX I/O supported on peripheral?
* :github:`17951` - RFC: update FS API for readdir consistency
* :github:`17948` - Bluetooth: privacy: Reconnection issue
* :github:`17944` - [zephyr 1.14]  LE Enhanced Connection Complete indicates Resolved Public once connected to Public peer address
* :github:`17936` - Bluetooth: Mesh: The canceled buffer is not free, causing a memory leak
* :github:`17932` - BLE Mesh When Friend Send Seg Message To LPN
* :github:`17926` - CAN | nrf52 | device tree error: zephyrproject/zephyr/dts/bindings/can/microchip,mcp2515.yaml (in 'reg'): 'category' from !included file overwritten ('required' replaced with 'optional')
* :github:`17923` - SPI1 on nrf52_pca10040 is dead by default
* :github:`17922` - Driver: modem helper should make it easier to implement a modem
* :github:`17920` - Bluetooth: Security problem
* :github:`17907` - BLE Mesh when resend use GATT bearer
* :github:`17899` - tests/kernel/mem_protect/stackprot/kernel.memory_protection fails on nsim_em
* :github:`17897` - k_busy_wait not working when using 32KHz timer driver
* :github:`17891` - fs/nvs: nvs_init can hang if no nvs_ate available
* :github:`17882` - [zephyr 1.14]  Database Out of Sync error is not returned as expected
* :github:`17880` - Unable to re-connect to privacy enabled peer when using stack generated Identity
* :github:`17876` - BME680 sensor sample not building
* :github:`17870` - Incorrect report received lenght and offset in async API
* :github:`17869` - Unlocking nested k_sched_lock() cause forced preemption
* :github:`17864` - cpp_synchronization sample not working on nucleo_l476rg
* :github:`17861` - Tester application lacks BTP Discover All Primary Services handler
* :github:`17857` - GATT: Incorrect byte order for GATT database hash
* :github:`17853` - kernel panic in tests/kernel/sched/schedule_api
* :github:`17851` - riscv/m2gl025: timer tests broken
* :github:`17843` - Bluetooth: controller: v1.14.x release conformance test failures
* :github:`17821` - Mesh Bug on access.c
* :github:`17820` - Mesh  bug report In access.c
* :github:`17816` - LVGL V5.3 build error if CONFIG_LVGL_COLOR_16_SWAP=y
* :github:`17812` - pthread_cond_timedwait interprets timeout wrong
* :github:`17809` - Bluetooth Mesh message cached too early when LPN
* :github:`17802` - [zephyr 1.14] Address type 0x02 is used by LE Create Connection in device privacy mode
* :github:`17800` - Bluetooth: GATT: Write Without Reponse to invalid handle asserts
* :github:`17794` - Timeutil_api test fails with sanitycheck on iotdk board.
* :github:`17790` - MEC1501 configure warnings in eSPI (dts)
* :github:`17789` - Bluetooth: host: conn.c missing parameter copy
* :github:`17787` - openocd unable to flash hello_world to cc26x2r1_launchxl
* :github:`17784` - failing network tests with code coverage enabled in qemu_x86 not failing when run with gdb
* :github:`17783` - network tests failing with code coverage enabled in qemu_x86 (coverage.c)
* :github:`17782` - network tests failing with 'unexpected eof' with code coverage enabled in qemu_x86 (TSS)
* :github:`17778` - Microchip XEC rtos Timer breaks gpios and k_sleep?
* :github:`17772` - Compilation error of soc/arm/nxp_imx/rt/soc.c
* :github:`17764` - Broken link to latest development version of docs
* :github:`17751` - build is broken for mec15xxevb_assy6853
* :github:`17738` - STATIC_ASSERT no longer defined when CONFIG_NEWLIB_LIBC is enabled
* :github:`17732` - cannot use bt_conn_security in connected callback
* :github:`17727` - how to make zephyr as a ble mesh provisioner to other BLE based board having ble mesh
* :github:`17726` - How to make Zephyr as a ble mesh provisioner ?
* :github:`17723` - Advertiser never clears state flags
* :github:`17715` - Missing 'reg-names' string in riscv32-litex-vexriscv.dtsi
* :github:`17703` - Add prop 'clock-frequency' to STM32  targets
* :github:`17697` - usb_dc_nrfx driver gets stuck after USB3CV HID Tests are performed on hid sample
* :github:`17692` - Proper way for joining a multicast group (NRF52840/OpenThread)
* :github:`17690` - samples/subsys/shell/fs does not work?
* :github:`17671` - ADC not supported by nrf52840_pca10059 DTS file
* :github:`17665` - Missing 'label' on most nodes with 'compatible = "pwm-leds"'
* :github:`17664` - Missing 'clocks' on most nodes with 'compatible = "nxp,kinetis-usbd"'
* :github:`17663` - Missing 'label' on most nodes with 'compatible = "fixed-clock"'
* :github:`17662` - Missing 'label' on nodes with 'compatible = "jedec,spi-nor"'
* :github:`17657` - subsis/disk/disk_access_spi_sdhc: response data eaten by idle byte consumption
* :github:`17650` - devicetree: missing preferred instance presence macro
* :github:`17635` - UnicodeDecodeError is raised while executing west build
* :github:`17630` - efr32mg_sltb004a tick clock error
* :github:`17613` - POSIX arch: occasional failures of tests/kernel/sched/schedule_api on CI
* :github:`17608` - NMP timeout when uploading image with mcumgr over BLE under Linux
* :github:`17600` - Enable Mesh Friend support in Bluetooth tester application
* :github:`17595` - two userspace tests fail if stack canaries are enabled in board configuration
* :github:`17591` - ARM: z_arch_except() is too permissive in user mode
* :github:`17590` - ARC: unable to induce kernel_oops or stack check fail errors from user mode
* :github:`17586` - stack canary storage area should be read-only to user mode?
* :github:`17584` - k_mutex is not SMP-safe
* :github:`17581` - linker script packing failure with subsys/fb fonts and native_posix_64
* :github:`17564` - Missing 'stdlib.h' include when C++ standard library is used.
* :github:`17559` - Assertion failed: zephyr toolchain variant invalid
* :github:`17557` - samples/net/wifi fails to build on cc3220sf_launchxl
* :github:`17555` - CONFIG_LOG doesn't work on x86_64 due to no working backends
* :github:`17554` - pyocd flash does not support the -b option for board ID so that the sanitycheck script can’t specified the board ID to flash when the host connected with multiple boards.
* :github:`17550` - SimpleLink WiFi host driver should revert back to using static memory model
* :github:`17547` - incorrect documentation for debugging nsim_em / nsim_sem
* :github:`17543` - dtc version 1.4.5 with ubuntu 18.04 and zephyr sdk-0.10.1
* :github:`17534` - Race condition in GATT API.
* :github:`17532` - List of missing device tree properties with 'category: required' in the binding for the node
* :github:`17525` - L2CAP: On insufficient authentication error received, Zephyr does unauthenticated pairing
* :github:`17511` - _bt_br_channels_area  section missing in sanitycheck whitelist
* :github:`17508` - RFC: Change/deprecation in display API
* :github:`17507` - system timer drivers using the "new" API should not be configured with CONFIG_TICKLESS_KERNEL
* :github:`17497` - Bluetooth: Mesh: How to Write provision and configure data to flash?
* :github:`17488` - CDC_ACM USB on nRF device fails after suspend
* :github:`17487` - v1.14-branch: SDK 0.10.1 support?
* :github:`17486` - nRF52: SPIM: Errata work-around status?
* :github:`17485` - sanitycheck: Over-zealous checking for binary sections
* :github:`17483` - mec15xxevb_assy6853 board documentation is erroneous
* :github:`17480` - holyiot_yj16019 cannot compile IEEE 802.15.4 L2
* :github:`17478` - net/buf test fails for qemu_x86_64
* :github:`17475` - [RTT] compile error when RTT console enabled
* :github:`17463` - Bluetooth: API limits usage of MITM flags in Pairing Request
* :github:`17460` - sample: gui/lvgl
* :github:`17450` - net: IPv6/UDP datagram with unspecified addr and zero hop limit causes Zephyr to quit
* :github:`17439` - sanitycheck: nrf52840-pca10056 (dev kit) picks up sample/drivers items which will fail due to missing HW
* :github:`17427` - net: IPv4/UDP datagram with zero src addr and TTL causes Zephyr to segfault
* :github:`17419` - arch:arc: remove the extra vairables used in irq and exception handling
* :github:`17415` - Settings Module - settings_line_val_read() returning -EINVAL instead of 0 for deleted setting entries
* :github:`17410` - k_work should have a user_data field
* :github:`17408` - LwM2M: engine doesn't support offloaded TLS stack
* :github:`17401` - LwM2M: requires that CONFIG_NET_IPV* be enabled (can't use 100% offloaded IP stack)
* :github:`17399` - LwM2M: Can't use an alternate mbedtls implementation
* :github:`17381` - DTS compatible property processing assumes specific driver exists
* :github:`17379` - Wrong hex file generated for MCUboot
* :github:`17378` - samples: net: echo-server: no return packet
* :github:`17376` - device tree diagnostic failure in enum
* :github:`17368` - Time Slicing cause system sleep short time
* :github:`17366` - Regression: sanitycheck coverage generation defaults will error out for POSIX arch targets
* :github:`17365` - Documentation: sanitycheck coverage generation instructions lead to errors and no coverage report for POSIX boards
* :github:`17363` - SPI driver does not reset master mode fault on STM32
* :github:`17353` - Configuring with POSIX_API disables NET_SOCKETS_POSIX_NAMES
* :github:`17342` - CODEOWNERS is broken (III)
* :github:`17340` - Bluetooth Mesh: Unable to receive messages when RPL is full.
* :github:`17338` - kernel objects address check in elf_helper.py
* :github:`17313` - drivers: usb_dc_mcux_ehci does not compile
* :github:`17307` - device tree bindings disallow strings that begin with integers
* :github:`17294` - DB corruption when adding/removing service
* :github:`17288` - Bluetooth: controller: Fix handling of L2CAP start frame with zero PDU length
* :github:`17284` - unrecognized binary sections: ['_settings_handlers_area']
* :github:`17281` - sanitycheck error on mimxrt1050_evk samples/gui/lvgl/sample.gui.lvgl with no network connection
* :github:`17280` - How to use UART1 for nrf52_pca10040
* :github:`17277` - no code coverage for k_float_disable() in user mode
* :github:`17266` - CDC_ACM USB not recognized by windows as serial port
* :github:`17262` - insufficient code coverage for lib/os/base64.c
* :github:`17251` - w25q: erase operations must be erase-size aligned
* :github:`17250` - After first GC operation the 1st sector had become scratch and the 2nd sector had became write sector.
* :github:`17231` - Posix filesystem wrapper leaks internal FS desc structures
* :github:`17226` - [Coverity CID :61894]Security best practices violations in /home/aasthagr/zephyrproject/modules/crypto/mbedtls/library/rsa.c
* :github:`17225` - [Coverity CID :61905]Insecure data handling in /home/aasthagr/zephyrproject/modules/crypto/mbedtls/library/ssl_cli.c
* :github:`17224` - [Coverity CID :78542]Null pointer dereferences in /home/aasthagr/zephyrproject/modules/crypto/mbedtls/library/rsa.c
* :github:`17223` - [Coverity CID :149311]Control flow issues in /home/aasthagr/zephyrproject/modules/crypto/mbedtls/library/cipher.c
* :github:`17222` - [Coverity CID :173947]Uninitialized variables in /home/aasthagr/zephyrproject/modules/lib/mcumgr/cborattr/src/cborattr.c
* :github:`17221` - [Coverity CID :173979]Control flow issues in /home/aasthagr/zephyrproject/modules/lib/tinycbor/src/cborparser.c
* :github:`17220` - [Coverity CID :173986]Control flow issues in /home/aasthagr/zephyrproject/modules/lib/mcumgr/cborattr/src/cborattr.c
* :github:`17219` - [Coverity CID :174014]Incorrect expression in /home/aasthagr/zephyrproject/modules/lib/tinycbor/src/cborparser.c
* :github:`17218` - [Coverity CID :186031]Control flow issues in /home/aasthagr/zephyrproject/modules/lib/mcumgr/cmd/fs_mgmt/src/fs_mgmt.c
* :github:`17217` - [Coverity CID :186038]Control flow issues in /home/aasthagr/zephyrproject/modules/lib/mcumgr/cmd/img_mgmt/src/img_mgmt.c
* :github:`17216` - [Coverity CID :186052]Control flow issues in /home/aasthagr/zephyrproject/modules/lib/mcumgr/cmd/fs_mgmt/src/fs_mgmt.c
* :github:`17215` - [Coverity CID :186054]Control flow issues in /home/aasthagr/zephyrproject/modules/lib/mcumgr/cmd/img_mgmt/src/img_mgmt_state.c
* :github:`17214` - [Coverity CID :186060]Control flow issues in /home/aasthagr/zephyrproject/modules/lib/mcumgr/cmd/img_mgmt/src/img_mgmt_state.c
* :github:`17213` - [Coverity CID :186188]Memory - illegal accesses in /home/aasthagr/zephyrproject/modules/lib/open-amp/open-amp/lib/rpmsg/rpmsg.c
* :github:`17212` - [Coverity CID :187076]Control flow issues in /home/aasthagr/zephyrproject/modules/hal/silabs/gecko/emlib/src/em_cmu.c
* :github:`17211` - [Coverity CID :188746]Memory - illegal accesses in /home/aasthagr/zephyrproject/modules/hal/cypress/PDL/3.1.0/drivers/source/cy_syslib.c
* :github:`17210` - [Coverity CID :190643]Error handling issues in /home/aasthagr/zephyrproject/modules/debug/segger/systemview/SEGGER_SYSVIEW.c
* :github:`17209` - [Coverity CID :190927]Uninitialized variables in /home/aasthagr/zephyrproject/modules/lib/open-amp/open-amp/lib/remoteproc/remoteproc.c
* :github:`17208` - [Coverity CID :190941]Insecure data handling in /home/aasthagr/zephyrproject/modules/crypto/mbedtls/library/ssl_tls.c
* :github:`17207` - [Coverity CID :190963]Code maintainability issues in /home/aasthagr/zephyrproject/modules/fs/nffs/src/nffs_restore.c
* :github:`17206` - [Coverity CID :190975]Memory - illegal accesses in /home/aasthagr/zephyrproject/modules/lib/open-amp/open-amp/lib/include/openamp/rpmsg.h
* :github:`17205` - [Coverity CID :190999]Insecure data handling in /home/aasthagr/zephyrproject/modules/lib/open-amp/open-amp/lib/rpmsg/rpmsg_virtio.c
* :github:`17204` - [Coverity CID :191000]Memory - corruptions in /home/aasthagr/zephyrproject/modules/lib/open-amp/open-amp/lib/remoteproc/remoteproc.c
* :github:`17203` - [Coverity CID :198951]Code maintainability issues in /home/aasthagr/zephyrproject/modules/debug/segger/systemview/SEGGER_SYSVIEW.c
* :github:`17202` - [Coverity CID :199436]Uninitialized variables in /subsys/net/lib/sockets/sockets.c
* :github:`17201` - [Coverity CID :199437]Null pointer dereferences in /tests/net/ip-addr/src/main.c
* :github:`17200` - [Coverity CID :199438]Memory - illegal accesses in /drivers/interrupt_controller/exti_stm32.c
* :github:`17190` - net-mgmt should pass info element size to callback
* :github:`17188` - k_uptime_delta returns wrong times
* :github:`17182` - "tests/subsys/usb/device/" fails on reel_board.
* :github:`17177` - ARM: userspace/test_bad_syscall fails on ARMv8-M
* :github:`17176` - deprecated counter_set_alarm is referenced in documentation
* :github:`17172` - insufficient code coverage for lib/os/mempool.c
* :github:`17170` - x86_64 crash with spinning child thread
* :github:`17167` - ARC crash with spinning user thread
* :github:`17166` - arch/x86: eliminate support for CONFIG_REALMODE
* :github:`17158` - Bluetooth: Update PICS for latest PTS 7.4.1
* :github:`17147` - UARTE device has no API when run on nrf52810
* :github:`17114` - drivers: usb_dc_stm32 broken after west update
* :github:`17111` - nucleo_f030r8 build error
* :github:`17095` - Building with Xtensa toolchain fails
* :github:`17092` - Bluetooth: GAP/IDLE/NAMP/BV-01-C requires Read by UUID
* :github:`17065` - Misspelled CONFIG use in is_rodata() for CONFIG_RISCV32
* :github:`17063` - tests/kernel/tickless/tickless_concept (qemu_x86) fails even outside of CI
* :github:`17057` - Bluetooth: Mesh: Implementation doesn't conform to latest errata and 1.0.1 version
* :github:`17055` - net: Incorrect data length after the connection is established
* :github:`17053` - Bluetooth Mesh: Periodic Publishing
* :github:`17043` - compile "hello-world" sample for esp32 board error
* :github:`17041` - [1.14] Bluetooth: Mesh: RPL handling is not in line with the spec
* :github:`17038` - code relocation generating different memory layout cause user mode not working
* :github:`17037` - MQTT with TLS support over SOCKS
* :github:`17031` - Compiler warnings in settings module in Zephyr 1.14
* :github:`17017` - #16827 Breaks Ethernet on FRDM-K64F
* :github:`17015` - #15910 Breaks Ethernet on STM32F7
* :github:`17013` - Bluetooth: Add error reason to pairing failed callbacks
* :github:`17007` - USB mass demo format fails on frdm_k64f
* :github:`16989` - Errors when building application in Eclipse
* :github:`16971` - DFU supported for hci_uart sample ?
* :github:`16946` - characteristic value handle vs characteristic handle
* :github:`16944` - Insufficient test coverage for lib/os/json.c
* :github:`16943` - Missing test coverage for lib/os/crc*.c
* :github:`16934` - drivers: flash: stm32l4: Erase wait time is not enough
* :github:`16931` - logging: Assertion when in panic mode
* :github:`16926` - NXP LPC54102（LPC54114）: question about dual core(M4 and M0) running on flash
* :github:`16924` - Add DNS server added/removed events to net_mgmt
* :github:`16915` - stack_sentinel: rare ASSERTION FAIL [!(z_arch_curr_cpu()->nested != 0U)] @ ZEPHYR_BASE/kernel/thread.c:429  Threads may not be created in ISRs
* :github:`16911` - tests/kernel/sched/schedule_api crash on qemu_x86_64 with SCHED_MULTIQ enabled
* :github:`16907` - native_posix build fails with X86_64 on macOS
* :github:`16901` - No test coverage for CONFIG_ZERO_LATENCY_IRQS
* :github:`16899` - fs/nvs: might loop-up if storage was not erased before first run
* :github:`16898` - bluetooth stack change affects timer behavior
* :github:`16894` - ARM: alignment problems in libc/newlib
* :github:`16893` - Bluetooth: Multiple local IDs, privacy problem
* :github:`16887` - ARM: threads' privilege stack alignment is not optimal
* :github:`16872` - Bluetooth: LL: Peripheral crashes after a while with multiple Centrals
* :github:`16864` - Bluetooth: Mesh: Rx buffer exhaustion causes deadlock
* :github:`16862` - arc: -mfpu=fpuda_all is not set when CONFIG_FLOAT is configured
* :github:`16861` - nRF52: UARTE: Data corruption right after resuming device
* :github:`16830` - Bluetooth: controller: Follow up on ticker conflict resolution
* :github:`16823` - k_busy_wait() on nRF5x boards isn't waiting long enough
* :github:`16803` - Deferred bt_conn_tx causes sysworkq deadlock
* :github:`16799` - Bluetooth: L2CAP: Interpretation of SCID and DCID in Disconnect is wrong
* :github:`16797` - [Zephyr v1.14.0] stm32: MCUboot bootloader results in Hardware exception
* :github:`16793` - kernel timeout_list repeatedly add a thread
* :github:`16787` - [Coverity CID :198945]Null pointer dereferences in /subsys/bluetooth/controller/ll_sw/ull_conn.c
* :github:`16786` - [Coverity CID :198946]Memory - corruptions in /subsys/bluetooth/host/gatt.c
* :github:`16785` - [Coverity CID :198949]Error handling issues in /tests/net/socket/register/src/main.c
* :github:`16779` - [Zephyr v1.14] ARM: fix the start address of MPU guard in stack-fail checking (when building with no user mode)
* :github:`16778` - Build failures in various mimxrt boards
* :github:`16773` - DTS: generated output for each flash-controller
* :github:`16770` - Complete FP support for ARC
* :github:`16761` - nrf52840 usb driver with openthread
* :github:`16760` - K_THREAD_STACK_EXTERN() confuses gen_kobject_list.py
* :github:`16750` - counter:  lack of interrupt when CC=0
* :github:`16749` - IRQ_CONNECT and irq_enable calls in the SiFive UART driver is misconfigured
* :github:`16747` - bluetooth: peripheral: RX buffer size issues
* :github:`16746` - boards: nrf52840_pca10059: Configure NFC pins as GPIOs by default
* :github:`16745` - PTHREAD_MUTEX_DEFINE(): don't store into the _k_mutex section
* :github:`16739` - spi: stm32: pinmux: default configuration does not opt for low power consumption
* :github:`16734` - Bluetooth: GATT: Writing 1 byte to a CCC access invalid memory
* :github:`16733` - soc/stm32: Remove useless package digit for STM32 SoC Kconfig symbols
* :github:`16720` - drivers/loapic_timer.c is buggy, needs cleanup
* :github:`16716` - soc: stm32: Is the setting of NUM_IRQS in the F3 series wrong?
* :github:`16707` - Problem with k_sleep
* :github:`16695` - code coverage: kernel/device.c
* :github:`16687` - basic disco sample fails
* :github:`16678` - LPN establishment of Friendship never completes if there is no response to the initial Friend Poll
* :github:`16676` - Settings enhancements
* :github:`16672` - nrf: spi: Excess current
* :github:`16670` - Memory reports do not work when Nordic proprietary LL is selected
* :github:`16661` - Symmetric multiprocessing (SMP) for ARC HS cores
* :github:`16639` - eth: pinging frdm k64f eventually leads to unresponsive ethernet device
* :github:`16634` - GATT indicate API inconsistent when using characteristic declaration attribute as argument
* :github:`16631` - SDK_VERSION
* :github:`16624` - Building Grub fails when using gcc9
* :github:`16623` - Building with Openthread fails
* :github:`16607` - Building hello_world fails for xtensa: xt-xcc ERROR parsing -Wno-address-of-packed-member:  unknown flag
* :github:`16606` - Fault in CPU stats
* :github:`16604` - Zephyr fails to build with CPU load measurement enabled
* :github:`16603` - Bluetooth: Gatt Discovery: BT_GATT_DISCOVER_PRIMARY returns all services while BT_GATT_DISCOVER_SECONDARY returns none
* :github:`16602` - Bluetooth: GATT Discovery: Descriptor Discovery by range Seg Fault
* :github:`16600` - Bluetooth: Mesh: Proxy SAR timeout is not implemented
* :github:`16594` - net: dns: Zephyr is unable to unpack mDNS answers produced by another Zephyr node
* :github:`16584` - [Coverity CID :198863]Error handling issues in /subsys/net/lib/sntp/sntp.c
* :github:`16583` - [Coverity CID :198864]Parse warnings in /subsys/logging/log_backend_rtt.c
* :github:`16582` - [Coverity CID :198865]Null pointer dereferences in /drivers/usb/device/usb_dc_stm32.c
* :github:`16581` - [Coverity CID :198866]Null pointer dereferences in /subsys/net/lib/dns/llmnr_responder.c
* :github:`16580` - [Coverity CID :198867]Parse warnings in /tests/subsys/fs/nffs_fs_api/common/nffs_test_system_01.c
* :github:`16579` - [Coverity CID :198868]Parse warnings in /drivers/watchdog/wdt_qmsi.c
* :github:`16578` - [Coverity CID :198869]Parse warnings in /subsys/shell/shell_rtt.c
* :github:`16577` - [Coverity CID :198870]Error handling issues in /subsys/net/lib/lwm2m/lwm2m_obj_firmware_pull.c
* :github:`16576` - [Coverity CID :198871]Parse warnings in /drivers/i2c/i2c_qmsi_ss.c
* :github:`16575` - [Coverity CID :198872]Parse warnings in /tests/subsys/settings/nffs/src/settings_setup_nffs.c
* :github:`16574` - [Coverity CID :198873]Incorrect expression in /tests/drivers/uart/uart_async_api/src/test_uart_async.c
* :github:`16573` - [Coverity CID :198874]Null pointer dereferences in /drivers/usb/device/usb_dc_stm32.c
* :github:`16572` - [Coverity CID :198875]Memory - corruptions in /drivers/flash/flash_simulator.c
* :github:`16571` - [Coverity CID :198876]Parse warnings in /tests/subsys/fs/multi-fs/src/test_nffs.h
* :github:`16570` - [Coverity CID :198877]Null pointer dereferences in /subsys/net/ip/net_if.c
* :github:`16569` - [Coverity CID :198878]Error handling issues in /samples/net/sockets/echo_server/src/tcp.c
* :github:`16568` - [Coverity CID :198879]Parse warnings in /tests/subsys/fs/fat_fs_dual_drive/src/test_fat_mount.c
* :github:`16567` - [Coverity CID :198880]Possible Control flow issues in /samples/net/lwm2m_client/src/lwm2m-client.c
* :github:`16566` - [Coverity CID :198881]Parse warnings in /drivers/serial/uart_qmsi.c
* :github:`16565` - [Coverity CID :198882]Parse warnings in /drivers/console/rtt_console.c
* :github:`16564` - [Coverity CID :198883]Parse warnings in /drivers/gpio/gpio_qmsi_ss.c
* :github:`16563` - [Coverity CID :198884]Parse warnings in /drivers/counter/counter_qmsi_aon.c
* :github:`16524` - FXOS8700 is not well supported in twr_ke18f
* :github:`16519` - USAGE FAULT occurs when i2c_write is called
* :github:`16518` - USB_UART_DTR_WAIT not working on nrf52840_pca10059
* :github:`16508` - tests/subsys/storage/flash_map  Instruction bus error on frdmk64 board
* :github:`16506` - tests/posix/fs missing ff.h
* :github:`16501` - Code Coverage for qemu_x86 is not getting generated due to a build error
* :github:`16493` - [Coverity CID :198640]Resource leaks in /tests/net/socket/register/src/main.c
* :github:`16492` - [Coverity CID :198644]Incorrect expression in /tests/drivers/uart/uart_async_api/src/test_uart_async.c
* :github:`16487` - tests/kernel/timer/timer_api/kernel.timer  sporadically (high frequency) fails in CI on qemu-xtensa
* :github:`16483` - net: ipv6: udp: Zephyr replies to datagram with illegal checksum 0
* :github:`16478` - Bluetooth: Improper bonded peers handling
* :github:`16470` - Superfluous USB suspend after USB configured
* :github:`16463` - tests/subsys/settings/fcb_init fails on second run
* :github:`16453` - sockets: getaddrinfo: AF_UNSPEC handling was recently broken
* :github:`16432` - Weird link error of the console sample!
* :github:`16428` - samples/gui/lvgl does not work on PCA10056
* :github:`16426` - Missing included dependencies in many header files
* :github:`16419` - Bluetooth: XTAL feature regression
* :github:`16418` - drivers: watchdog: sam0: check if timeout is valid
* :github:`16417` - issues with can filter mode set
* :github:`16416` - sram size for RT1015 and RT1020 needs to be update.
* :github:`16415` - Build errors with C++
* :github:`16414` - Backport west build --pristine
* :github:`16413` - Missing dependency in cmake
* :github:`16412` - on reel_board the consumption increases because TX pin is floating
* :github:`16411` - bad regex for west version check in host-tools.cmake
* :github:`16389` - ninja flash to intel quark d2000 zephyr
* :github:`16387` - STM32wb55 bluetooth samples fail
* :github:`16379` - net: ipv6: udp: Zephyr replies with illegal UDP checksum zero
* :github:`16375` - net: ipv4: udp: Zephyr does not reply to a valid datagram with checksum zero
* :github:`16366` - Build error on QEMU x86 and quark_se_c1000_devboard
* :github:`16365` - lwm2m: enable with CONFIG_NET_RAW_MODE
* :github:`16363` - Error building x_nucleo_iks01a1 sample on nucleo_wb55rg after activating I2C Bus
* :github:`16360` - ARM: Implement configurable MPU-based stack overflow guards
* :github:`16354` - net: ipv6: Zephyr does not reply to fragmented packet
* :github:`16341` - Bluetooth: GATT Server failed to send Service Change Indication
* :github:`16339` - openthread: off-by-one error when calculating ot_flash_offset for settings
* :github:`16327` - doc: networking: overview has out of date info for LwM2M
* :github:`16326` - USB3CV Chapter 9 Tests failures
* :github:`16323` - net: ipv6: tcp: unexpected reply to malformed HBH in TCP/IPv6 SYN
* :github:`16318` - net: Network Offloading: Particle Boron
* :github:`16316` - ST modules organization
* :github:`16313` - LMP Response Timeout / LL Response Timeout (0x22) after ~40s when using LE Secure Connections
* :github:`16307` - cannot move location counter backwards error happen
* :github:`16303` - mbedtls: config-tls-generic.h: MBEDTLS_PLATFORM_NO_STD_FUNCTIONS seems ungrounded
* :github:`16296` - dts generation in correct for 2 registers and no-size
* :github:`16289` - Driver: modem ublox-sara-r4 not compiling
* :github:`16278` - [Zepyhr v1.14.0] Unable to update FW with mcumgr over UART
* :github:`16276` - net: ipv4: Zephyr replies to link-layer broadcast packet
* :github:`16275` - setting_init crashes on qemu_x86 when setting BT_SETTINGS
* :github:`16273` - Calling k_work_submit can reenable interrupts
* :github:`16272` - bluetooth mesh proxy filter
* :github:`16268` - Add 32K RAM support for nRF51822 REVC/microbit board
* :github:`16257` - net: icmpv4: Zephyr sends echo reply with multicast source address
* :github:`16243` - std::vector push_back() not working correctly
* :github:`16240` - USB Bluetooth and DFU classes cannot be enabled simultaneously on nRF52840
* :github:`16238` - k_cycle_get_32() API is useless in some Zephyr configurations
* :github:`16236` - [docs] Windows installation guide, git part, is installed with non-intended configuration
* :github:`16234` - tests/benchmarks/latency_measure can not calculate the real time thread switch for twr_ke18f
* :github:`16229` - tests/kernel/common fails at test_atomic on twr_ke18f board
* :github:`16227` - Zephyr env: unset var in conditional activation
* :github:`16226` - ARM: IsInIsr(): inconsistencies between doc and implementation
* :github:`16225` - tests/kernel/msgq/msgq_api twr_ke18f fails with assert
* :github:`16224` - tests/subsys/storage/flash_map meet mpu hardfault in twr_ke18f board.
* :github:`16216` - tests/kernel/timer/timer_api fails on nrf51_pca10028 board
* :github:`16215` - FIFO queue data seems to get overwritten
* :github:`16211` - NVS: sector erase at startup (2-sectors configuration)
* :github:`16204` - Build STM32 : generate hex file fail
* :github:`16191` - boards/arm/{olimexino_stm32, stm32_min_dev}: USB pinmux setup is skipped
* :github:`16185` - Compile error using entropy.h in C++ code
* :github:`16177` - STM32: Could not compile with CONFIG_PINMUX=n
* :github:`16170` - CI fails because warning in LOG_ERR() in drivers/i2s_ll_stm32.c
* :github:`16164` - [Coverity CID :198584]Uninitialized variables in /drivers/led/ht16k33.c
* :github:`16163` - [Coverity CID :198587]Incorrect expression in /tests/subsys/usb/desc_sections/src/desc_sections.c
* :github:`16162` - [Coverity CID :198588]Control flow issues in /drivers/gpio/gpio_cc13xx_cc26xx.c
* :github:`16161` - [Coverity CID :198589]Control flow issues in /drivers/i2c/i2c_sam0.c
* :github:`16160` - [Coverity CID :198590]Control flow issues in /drivers/i2c/i2c_sam0.c
* :github:`16159` - [Coverity CID :198591]Control flow issues in /drivers/sensor/adxl362/adxl362.c
* :github:`16158` - LwM2M: Fix incorrect last_block handling in the firmware write callback
* :github:`16156` - Remove the LWM2M maximum number of instances limitation
* :github:`16155` - drivers: can: wrong value used for filter mode set
* :github:`16154` - Fix various issues with handling of floating values within the LWM2M subsystem
* :github:`16148` - ARM: Enable building with TRUSTED_EXECUTION_SECURE
* :github:`16145` - question: Using OpenThread API in Zephyr application
* :github:`16143` - posix: clock_settime calculates the base time incorrectly
* :github:`16142` - NET: llmnr responder sends malformed packets
* :github:`16141` - posix: CONFIG_POSIX_API and CONFIG_NET_SOCKETS_POSIX_NAMES don't make sense to use together, and conflict when doing so
* :github:`16138` - is this right for clock announcing in every CORE?
* :github:`16132` - The nRF mesh APP report “Invalid Publish Parameters”
* :github:`16110` - net: arp: request from own hardware but different IP address not dropped
* :github:`16107` - Using bt_gatt_read() with 'by_uuid' method returns 3 extra bytes
* :github:`16103` - nrf5 802.15.4 driver requires Log subsys to be enabled
* :github:`16098` - net: arp: sender hardware address not used by ICMP/TCP/UDP
* :github:`16090` - mpu align support for code relocation on non-XIP system
* :github:`16089` - Mcux Ethernet driver does not detect carrier anymore (it's alway on)
* :github:`16080` - Zephyr UART shell crashes on start if main() is blocked
* :github:`16079` - SAM0/SAMR SERIAL subsystem broken?
* :github:`16078` - Shell subsystem or SERIAL no longer works on SAM0/SAMR
* :github:`16072` - boards/up_squared: k_sleep() too long with local APIC timer
* :github:`16054` - Bluetooth sample app 'peripheral' failing to build for nRF52840
* :github:`16052` - Adafruit Trinket M0 Bossac Offset is Wrong
* :github:`16046` - modules are being processed too late.
* :github:`16042` - NDP should be enhanced with Security, RFC 3971
* :github:`16027` - support for no-flash systems
* :github:`16025` - webusb example app not reading data
* :github:`16012` - Source IP address for DHCP renewal messages is unset
* :github:`16010` - Coverage reporting fails on many tests
* :github:`16006` - The ArgonKey board documentation needs to align to the official information
* :github:`16002` - the spi base reg address in arc_iot.dtsi has an error
* :github:`16001` - ARC iotdk supports MPU and fpu in hardware but not enabled in kconfig
* :github:`16000` - We need a CI check for commas in CODEOWNERS
* :github:`15998` - CODEOWNERS is broken (Again)
* :github:`15997` - Fix compile warning in samples/net/sockets/dumb_http_server
* :github:`15996` - tests/kernel/sched/schedule_api/testcase.yaml#kernel.sched.slice_reset fails on nrf52840_pca10056, nrf52_pca10040, nrf51_pca10028
* :github:`15991` - [Coverity CID :198389]Memory - illegal accesses in /subsys/settings/src/settings_runtime.c
* :github:`15990` - [Coverity CID :198390]Memory - illegal accesses in /subsys/settings/src/settings_runtime.c
* :github:`15989` - [Coverity CID :198391]Memory - illegal accesses in /subsys/settings/src/settings_runtime.c
* :github:`15988` - [Coverity CID :198392]Insecure data handling in /tests/net/socket/getaddrinfo/src/main.c
* :github:`15987` - [Coverity CID :198393]Error handling issues in /tests/net/socket/socket_helpers.h
* :github:`15986` - [Coverity CID :198394]Error handling issues in /tests/net/socket/socket_helpers.h
* :github:`15985` - [Coverity CID :198395]Memory - corruptions in /soc/arm/microchip_mec/mec1501/soc.c
* :github:`15983` - Kernel tests assume SYS_CLOCK_TICKS_PER_SEC=100
* :github:`15981` - ARM: k_float_disable() as system call
* :github:`15975` - Openthread - fault with dual network interfaces
* :github:`15971` - Fail to connect sample bluetooth HID with Tizen OS (BT_HCI_ERR_DIFF_TRANS_COLLISION)
* :github:`15970` - samples: microbit pong demo
* :github:`15964` - ARM: Cortex-M: enhance Sharing Floating-Point Registers Mode
* :github:`15961` - bug:  west: 'west flash' doesn't use specified hex file
* :github:`15941` - Stale 1.3.99 documentation under /latest
* :github:`15924` - Bluetooth: PTS: GATT server tests fail after merge of #15524
* :github:`15922` - BLE mesh:The Provisioner APP can't find the micro:bit which is running the mesh sample
* :github:`15918` - stm32f7 GPIO Ports F & G Disabled by Default
* :github:`15917` - USB disconnect/reconnect
* :github:`15916` - [BLE] Mesh example qemu kernel panic
* :github:`15911` - Stack size is smaller than it should be
* :github:`15909` - stm32f7: DTCM included in sram0
* :github:`15906` - WEST ERROR: extension command build was improperly defined
* :github:`15904` - concerns with use of CONFIG_BT_MESH_RPL_STORE_TIMEOUT in examples
* :github:`15893` - code coverage is not tested in CI
* :github:`15884` - tests/net/socket/getaddrinfo fails on mps2_an385
* :github:`15878` - tests/net/lib/mqtt_publisher/net.mqtt.tls fails to build on sam_e70_xplained
* :github:`15877` - all qemu_x86_64 tests hang on Ubuntu 18.04
* :github:`15864` - disk partitioning should not specified in DTS
* :github:`15844` - Network management API should support user space
* :github:`15842` - cdc_acm: stm32: uart_fifo_fill() can't transmit data out
* :github:`15835` - "#if XIP" block in qemu_x86 DTS always evaluates to false
* :github:`15831` - qemu_x86 DTS does not reflect actual emulated hardware layout
* :github:`15827` - ARM: Update ARM CMSIS to latest  version
* :github:`15823` - Build failure for spi_loopback on atsamr21_xpro
* :github:`15817` - nrf52: HFXO is not turned off as expected
* :github:`15814` - [Coverity CID :186196]Unchecked return value in  samples/sensor/lsm6dsl/src/main.c
* :github:`15794` - mps2_an385 crashes if CONFIG_INIT_STACKS=y and CONFIG_COVERAGE=y
* :github:`15789` - Networking documentation missing
* :github:`15778` - [Coverity CID :198001]Control flow issues in /subsys/bluetooth/host/mesh/settings.c
* :github:`15777` - [Coverity CID :198002]Null pointer dereferences in /subsys/net/l2/ethernet/arp.c
* :github:`15776` - [Coverity CID :198003]Error handling issues in /tests/net/net_pkt/src/main.c
* :github:`15775` - [Coverity CID :198005]Memory - corruptions in /subsys/bluetooth/shell/gatt.c
* :github:`15774` - [Coverity CID :198006]Control flow issues in /subsys/bluetooth/host/settings.c
* :github:`15773` - [Coverity CID :198007]Memory - corruptions in /subsys/bluetooth/host/hci_core.c
* :github:`15772` - [Coverity CID :198009]Memory - corruptions in /subsys/bluetooth/shell/gatt.c
* :github:`15771` - [Coverity CID :198010]Control flow issues in /samples/boards/nrf52/mesh/onoff_level_lighting_vnd_app/src/storage.c
* :github:`15770` - [Coverity CID :198011]Incorrect expression in /tests/subsys/usb/desc_sections/src/desc_sections.c
* :github:`15769` - [Coverity CID :198012]Memory - corruptions in /drivers/flash/flash_simulator.c
* :github:`15768` - [Coverity CID :198013]Control flow issues in /subsys/bluetooth/host/mesh/settings.c
* :github:`15767` - [Coverity CID :198014]Memory - corruptions in /drivers/flash/flash_simulator.c
* :github:`15766` - [Coverity CID :198016]Security best practices violations in /subsys/settings/src/settings_runtime.c
* :github:`15765` - [Coverity CID :198018]Control flow issues in /subsys/bluetooth/host/mesh/settings.c
* :github:`15764` - [Coverity CID :198019]Security best practices violations in /subsys/settings/src/settings_runtime.c
* :github:`15763` - [Coverity CID :198021]Control flow issues in /drivers/clock_control/clock_stm32_ll_mp1x.c
* :github:`15762` - [Coverity CID :198022]Security best practices violations in /subsys/settings/src/settings_runtime.c
* :github:`15759` - usb: cdc_acm: uart_line_ctrl_set(dev, LINE_CTRL_DTR, &dtr) should always return 0 if USB port is not opened by host
* :github:`15751` - Incorrect flash map
* :github:`15749` - [question] errors using custom command in CMakeLists.txt
* :github:`15748` - 'ninja flash' does not work for IMXRT1052 target
* :github:`15736` - Generalize and improve async context for SPI, ADC, etc.
* :github:`15734` - Power management doesn't work with CONFIG_I2C=y on nRF52
* :github:`15733` - Bluetooth: controller: Central Encryption setup overlaps Length Request procedure
* :github:`15728` - tests/benchmarks/timing_info: wrong value for context switch duration
* :github:`15720` - "z_clock_elapsed" implementation seems to be missing #linking #sched
* :github:`15719` - tests/ztests/mock/ : Stuck at test_parameter_tests
* :github:`15714` - samples/bluetooth/peripheral: could not connect with disco_l475_iot1 board
* :github:`15710` - [question] how about the current consumption on NRF52DK running power_mgr sample?
* :github:`15709` - CODEOWNERS ignored in GitHub
* :github:`15706` - tunslip6: main: open: No such file or directory
* :github:`15698` - bluetooth: bt_conn: No proper ID handling
* :github:`15696` - [question] why bt_setting is dependant of printk in menuconfig?
* :github:`15679` - Can GPTP support multiple slave nodes
* :github:`15678` - Watchdog peripheral api docs aren't generated correctly.
* :github:`15675` - DTS question about pinmix/GPIO
* :github:`15672` - net: socket send return error(-110) when http request 100 times
* :github:`15668` - Support request: Issue with documentation warning
* :github:`15664` - Zephyr modules failure report
* :github:`15652` - document the mailing list for nightly build results
* :github:`15639` - [question] how to get the bd_addr from scan callback as shown on nrf-connect app?
* :github:`15637` - Support of device tree gpio-map
* :github:`15627` - Application compiled with CONFIG_POSIX_API doesn't have access to POSIX headers
* :github:`15625` - target_compile_features in CMakeLists.txt triggers an error
* :github:`15622` - NXP RT10XX: Load code to ITCM
* :github:`15612` - bt_set_id_addr() to allow public address as argument
* :github:`15608` - [question] my board won't boot without debugger attached but no issue using nordic SDK
* :github:`15607` - nRF52: 2.4 GHz proprietary RF support
* :github:`15606` - trickle.c can't work for multiple triggerings
* :github:`15605` - Unaligned memory access by ldrd
* :github:`15601` - pwm: nRF default prescalar value is wrong
* :github:`15597` - [question] How to include mesh related header files in my own source file
* :github:`15596` - net: Zephyr's SNTP API time precision is not adequate
* :github:`15594` - net shell "net tcp send" command failed when repeated many times
* :github:`15588` - Does zephyr support different time slices for each thread?
* :github:`15587` - Zephyr was unable to find the toolchain
* :github:`15580` - SAMD21 Adafruit examples no longer run on boards
* :github:`15571` - Fix sanitycheck failures for v2m_musca_b1_nonsecure
* :github:`15570` - Unbonded peripheral gets 'Tx Buffer Overflow' when erasing bond on master
* :github:`15565` - undefined references to 'sys_rand32_get'
* :github:`15558` - support for power-of-two MPUs on non-XIP systems
* :github:`15551` - CMake enables -fmacro-prefix-map on GCC 7
* :github:`15549` - [FCB question] Is it true that fcb storage won't overwrite old records which limits the max num?
* :github:`15546` - tests/kernel/mem_protect/protection/: Reached unreachable code
* :github:`15526` - Unhandled identity in bt_conn_create_slave_le
* :github:`15522` - Extra padding in IPv4 link local ARP packets
* :github:`15520` - tests/ztest/mock: test_multi_value_test:  Unused mocked return value
* :github:`15516` - Implementation of CONFIG_MAX_PTHREAD_COUNT
* :github:`15513` - nRF timer unnecessary configuration?
* :github:`15508` - No space to store CCC cfg
* :github:`15507` - NRF52840: usb composite MSC + HID (with CONFIG_ENABLE_HID_INT_OUT_EP)
* :github:`15504` -  Can I use one custom random static bd_addr before provision?
* :github:`15501` - smp_svr build issue
* :github:`15499` - gpio_intel_apl: gpio_pin_read() pin value doesn't match documentation
* :github:`15497` - USB DFU: STM32: usb dfu mode doesn't work
* :github:`15495` - tests/drivers/spi/spi_loopback/peripheral.spi fails to build on several boards
* :github:`15486` - usb composite MSC + HID (with CONFIG_ENABLE_HID_INT_OUT_EP)
* :github:`15483` - add mpu and fpu support for arc iotdk
* :github:`15481` - object_access in common-rom.ld missed GROUP_LINK_IN(ROMABLE_REGION)
* :github:`15477` - Zephyr network stack will up the Ethernet interface after its driver initialized (regardless of actual status)
* :github:`15472` - DNS resolver sample sends only one A query
* :github:`15465` - Fix build failures for test_newlib &   tests/lib/mem_alloc/libraries.libc.newlib w/ARM gcc
* :github:`15451` - doc: settings Misleading examples
* :github:`15448` - help to use zephyr-ninja flash of st_nucleo_l476rg
* :github:`15447` - sanitycheck --coverage -p qemu_x86: Fatal failures with tests/kernel/pipe/pipe/kernel.pipe
* :github:`15446` - ssanitycheck --coverage -p mps2_an385: Some remaing test cases that are still failing
* :github:`15444` - Error initiating sdhc disk
* :github:`15443` - usb_dc_stm32: Missing semaphore initialization and missing pin remapping configuration
* :github:`15419` - reset and halt function
* :github:`15411` - tests/kernel/critical:  Continuous reboot at test_critical
* :github:`15408` - [Coverity CID :197596]Memory - corruptions in /tests/net/lib/mqtt_packet/src/mqtt_packet.c
* :github:`15391` - [Coverity CID :197613]Possible Control flow issues in /subsys/net/ip/net_core.c
* :github:`15374` - PR 15230 introduces test failure for particle_argon
* :github:`15373` - IPv4 link local packets are not sent with ARP ethernet type
* :github:`15355` - Driver for U-Blox SARA modem (used by Particle Boron)
* :github:`15354` - counter: stm32: Issue with LSE clock source selection
* :github:`15339` - RISC-V: RV32M1: Load access fault when accessing GPIO port E
* :github:`15334` - Unable to reset nRF52840 from nRF9160 on pca10090 DK
* :github:`15331` - CONFIG_CODE_DATA_RELOCATION does not work with Cache enabled MCUs.
* :github:`15316` - printf causing usage fault
* :github:`15315` - doc: simplify cmake examples thanks to cmake's new -B option
* :github:`15305` - add QEMU target for armv8-M with MPU support
* :github:`15282` - Enhance networking tests/net/all tests
* :github:`15279` - mempool alignment might cause a memory block allocated twice
* :github:`15272` - sanitycheck reports build errors as "handler crash"
* :github:`15246` - doc: confusion about dtc version
* :github:`15232` - tests/bluetooth/tester fails build if CONFIG_TEST_USERSPACE=n
* :github:`15193` - tests/net/socket/getaddrinfo tests too little
* :github:`15180` - testing flash driver clients in CI
* :github:`15159` - undefined reference to 'bsearch' #stdlib
* :github:`15156` - GH labels reorg
* :github:`15144` - West: how to set up for external modules
* :github:`15139` - implement sys_sem that can reside in user memory
* :github:`15133` - Is the level2 interrupt supported for ARM cortex-M0P core?
* :github:`15119` - GPIO callback not disabled from an interrupt
* :github:`15116` - nrfx_twim: (nRF52) driver unable to perform i2c_burst_write() correctly
* :github:`15115` - ARM: Cortex-M: enhance non-sharing floating point services
* :github:`15074` - ARM: Fix/Enhance Floating Point for ARM Cortex-M architecture
* :github:`15046` - Native_posix: command line handling: Hint users why an option does not exist
* :github:`15003` - init_iface in net_if.c should check api->init
* :github:`14997` - Convert samples/sensor/bme280/src/main.c to use printk instead of printf
* :github:`14970` - samples/drivers/watchdog: Not showing "Waiting for reset..." for nrf boards
* :github:`14918` - olimexino stm32 CAN errors
* :github:`14904` - Sanitycheck report not clear if built or ran a test
* :github:`14828` - ARM: MPU-based HW thread stack protection not working properly when building with CONFIG_FLOAT
* :github:`14826` - USB reset in suspended state
* :github:`14791` - Bluetooth: GATT Notification callbacks should have conn as argument
* :github:`14769` - kernel/userspace.c: Improve code coverage to 100%
* :github:`14744` - ARM: Core Stack Improvements/Bug fixes for 2.0 release
* :github:`14736` - kernel/include/kswap.h: Improve code coverage to 100%
* :github:`14734` - kernel/include/kernel_structs.h: Improve code coverage to 100%
* :github:`14733` - kernel/work_q.c: Improve code coverage to 100%
* :github:`14731` - kernel/userspace_handler.c: Improve code coverage to 100%
* :github:`14730` - kernel/userspace.c: Improve code coverage to 100%
* :github:`14729` - kernel/timer.c: Improve code coverage to 100%
* :github:`14728` - kernel/timeout.c: Improve code coverage to 100%
* :github:`14727` - kernel/thread_abort.c: Improve code coverage to 100%
* :github:`14726` - kernel/thread.c: Improve code coverage to 100%
* :github:`14725` - kernel/system_work_q.c: Improve code coverage to 100%
* :github:`14724` - kernel/stack.c: Improve code coverage to 100%
* :github:`14723` - kernel/sem.c: Improve code coverage to 100%
* :github:`14722` - kernel/sched.c: Improve code coverage to 100%
* :github:`14721` - kernel/queue.c: Improve code coverage to 100%
* :github:`14720` - kernel/poll.c: Improve code coverage to 100%
* :github:`14719` - kernel/pipes.c: Improve code coverage to 100%
* :github:`14718` - kernel/mutex.c: Improve code coverage to 100%
* :github:`14717` - kernel/msg_q.c: Improve code coverage to 100%
* :github:`14716` - kernel/mempool.c: Improve code coverage to 100%
* :github:`14715` - kernel/mem_slab.c: Improve code coverage to 100%
* :github:`14713` - kernel/mem_domain.c: Improve code coverage to 100%
* :github:`14712` - kernel/mailbox.c: Improve coverage to 100%
* :github:`14711` - kernel/idle.c: Improve code coverage to 100%
* :github:`14710` - kernel/errno.c: Improve code coverage to 100%
* :github:`14709` - kernel/device.c: Improve code coverage to 100%
* :github:`14708` - kernel/include/ksched.h: Improve code coverage to 100%
* :github:`14707` - kernel/include/kernel_offsets.h: Improve code coverage to 100%
* :github:`14706` - kernel/include/kernel_internal.h: Imrpove code coverage to 100%
* :github:`14705` - kernel/smp.c: Improve code coverage to 100%
* :github:`14704` - kernel/int_latency_bench.c: Improve code coverage to 100%
* :github:`14703` - kernel/compiler_stack_protect.c: Improve code coverage to 100%
* :github:`14702` - kernel/atomic_c.c : Improve code coverage to 100%
* :github:`14675` - Bluetooth: Controller privacy support (platforms with HW acceleration)
* :github:`14652` - Gitlint is more strict than checkpatch.pl
* :github:`14633` - undefined reference to 'mbedtls_debug_set_threshold' when MBEDTLS_DEBUG is enabled
* :github:`14605` - mimxrt1060_evk cpp_synchronization meets Hardware exception
* :github:`14604` - BLE disconnection caused by channel map request or connection parameter update request
* :github:`14599` - Can you add a ADC sample?
* :github:`14588` - static IP support on cc3220sf_launchxl
* :github:`14547` - Kconfig shell prompt configuration
* :github:`14517` - LPCXpresso55S69 board support
* :github:`14493` - implement Zephyr futexes
* :github:`14467` - New HTTP API implementation
* :github:`14459` - usb: samples: mass: doesn't build with FLASH overlay
* :github:`14292` - Typo "QPSI" in QSPI macros in some stm32 drivers
* :github:`14283` - tests/drivers/watchdog/wdt_basic_api fails on test_wdt_callback_1() for Ardruino and quark_se_c1000_ss_devboard:arc
* :github:`14123` - Particle boards need a board initialization module for antenna configuration
* :github:`14082` - Update Segger host library to V2.52f
* :github:`14037` - Generic device driver object type
* :github:`14034` - Support for PPP protocol
* :github:`13963` - up_squared: evaluate removal of SBL-related special configurations
* :github:`13935` - tests/crypto/rand32/crypto.rand32.random_hw_xoroshiro: Usage fault "Fatal fault in ISR! Spinning..."
* :github:`13897` - CONFIG_LOG_IMMEDIATE leads to unobvious faults in unrelated rotines due to stack overflow
* :github:`13817` -  tests/ztest/test/mock fails to complete on nios2
* :github:`13799` - usb cdc acm fails when writing big chunks of data on stm32
* :github:`13766` - mimxrt1060_evk tests/kernel/fatal meet many unwanted exceptions
* :github:`13749` - Can CONFIG_SOC_WATCH-related code be removed?
* :github:`13610` - kernel: Non-deterministic and very high ISR latencies
* :github:`13602` - How to get the port number for an ephemeral (randomly assigned) port
* :github:`13574` - Missing documentation for fcb and nffs
* :github:`13560` - STM32 USB: netusb: kernel crash when testing example echo_server with nucleo_f412zg  (ECM on Windows)
* :github:`13444` - Build failure when including both socket.h and posix/time.h
* :github:`13441` - optimize x86 userspace page table memory usage
* :github:`13347` - tests/drivers/watchdog/wdt_basic_api fails on test_wdt_callback_1() for Quark SE / arc
* :github:`13316` - Notification enabled before connection
* :github:`13288` - Disable JTAG debug port for free use GPIO PA15, PB3, PB4 on STM32F1 series
* :github:`13197` - LwM2M: support Connectivity Monitoring object (object id 4)
* :github:`13148` - Build on nucleo_stm32f429zi
* :github:`13097` - openthread default configuration
* :github:`13075` - Review memory protection Kconfig policies for consistency and sanity
* :github:`13065` - CONFIG_BT leads Fatal fault in ISR on esp32
* :github:`13003` - context switch in x86 memory domains needs optimization
* :github:`12942` - openthread: sleepy end device not supported by zephyr router
* :github:`12825` - SystemView Feature Not working
* :github:`12728` - docs: Hard to find guidelines for ext/ maintenance
* :github:`12681` - BLE Split Link Layer
* :github:`12633` - tests/boards/altera_max10/i2c_master failed with "failed to read HPD control" on altera_max10
* :github:`12602` - STM32F415RG Support
* :github:`12553` - List of tests that keep failing sporadically
* :github:`12542` - nrf timers unstable with ticks faster than 100 Hz
* :github:`12478` - tests/drivers/ipm/peripheral.mailbox failing sporadically on qemu_x86_64 (timeout)
* :github:`12261` - i2c_ll_stm32_v1 driver gets stuck
* :github:`12257` - GNU Arm Embedded Toolchain 8-2018-q4-major can't produce hex files on Windows
* :github:`12245` - LWM2M registration timeout
* :github:`12228` - 	How to build images for client or server in bluetooth mesh examples?
* :github:`12160` - settings: NVS back-end
* :github:`12129` - Include: Clean up header file namespace
* :github:`12119` - 64bit architecture support
* :github:`12044` - [Question] [Blutooth Mesh] How proxy client create the proxy PDU
* :github:`12039` - clock_control: API should allow callback to be specified
* :github:`11993` - drivers: Supporting driver-specific extensions to existing generic APIs
* :github:`11922` - ADC generic driver missing features: calibration, reference voltage value
* :github:`11918` - Dynamic pin configuration
* :github:`11740` - LL_ASSERT in event_common_prepare in Zephyr v1.13
* :github:`11712` - Add support for newlib-nano
* :github:`11681` - Mesh Friend does not reply within ReceiveWindow
* :github:`11626` - k_busy_wait exits early on Nordic chips
* :github:`11617` - net: ipv4: udp: broadcast delivery not supported
* :github:`11535` - serial: uart_irq_tx_ready() needs better documentation or correction of the test
* :github:`11455` - cdc_acm uart_fifo_fill sample app doesn't comply with the documentation
* :github:`11107` - clock_control: API should support asynchronous and synchronous variants of turning on clocks
* :github:`10965` - wncm14a2a modem should be moved to a shield configuration
* :github:`10942` - Declaring an API Stable
* :github:`10935` - Support all UARTs on ESP32 WROOM module
* :github:`10915` - ESP 32 failed to boot while running  crypto/rand32 tests
* :github:`10896` - Add STM32 ADC driver
* :github:`10739` - Cannot flash to STM32 Nucleo F446RE with SEGGER JLink
* :github:`10664` - FOTA: nRF52: integration of samples/bluetooth/mesh & smp_srv
* :github:`10657` - tests/net/ieee802154/crypto does not work as expected
* :github:`10603` - Hard to find "todo"
* :github:`10463` - can: stm32: Update to new API
* :github:`10450` - usb/stm32: use dts extracted information to populate clock settings
* :github:`10420` - gcc: "Exec format error" - WSL in Windows 10 1803
* :github:`10150` - Getting LLVM work on ARM
* :github:`9954` - samples/hello_world build failed on Windows/MSYS
* :github:`9898` - 1.14 Release Checklist
* :github:`9762` - Bluetooth mesh reliability issue?
* :github:`9570` - Network stack cleanup: deep TCP cleanup
* :github:`9509` - Unable to upload firmware over serial with mcumgr
* :github:`9333` - Support for STM32 L1-series
* :github:`9247` - Get ST Disco and Eval boards compliant with default configuration guidelines
* :github:`9070` - bluetooth ：BR ：The sample is incomplete
* :github:`9047` - Interrupt APIs.
* :github:`8978` - soc: Intel S1000: add low power memory management support
* :github:`8851` - Allow creation of Zephyr libraries outside of Zephyr tree
* :github:`8734` - USB: DFU timeout on nRF52840 due to long flash erase
* :github:`8728` - Network stack cleanup: Net IF
* :github:`8726` - Network stack cleanup: TCP
* :github:`8725` - Network stack cleanup: Net Context
* :github:`8723` - Network stack cleanup
* :github:`8722` - Network stack cleanup: connection
* :github:`8464` - sdk_version file missing
* :github:`8419` - Bluetooth: tester: LPN Poll issue
* :github:`8404` - Bluetooth: controller: Reuse get_entropy_isr API
* :github:`8275` - when zephyr can support popular IDE develop?
* :github:`8125` - About BMI160 reading issue.
* :github:`8081` - Bluetooth: Deprecate TinyCrypt and use mbedTLS
* :github:`8062` - [Coverity CID :186196] Error handling issues in /samples/sensor/mcp9808/src/main.c
* :github:`7908` - tests/boards/altera_max10/qspi fails on max10
* :github:`7589` - Migrate Websocket to BSD sockets
* :github:`7585` - Migrate HTTP to socket API
* :github:`7503` - LwM2M client: No registration retry after error?
* :github:`7462` - Convert mcr20a to be a shield when that functionality is merged
* :github:`7403` - RAM flash driver emulator
* :github:`7316` - native_posix supersedes ztest mocking
* :github:`6906` - QM (Quality Managed) level qualification
* :github:`6823` - Optimize rendering of Kconfig documentation
* :github:`6817` - Question: Is supported for stack sharing among tasks  in Zephyr? Or any plan?
* :github:`6816` - Question: Is KProbes supported in Zephyr? Or any plan?
* :github:`6773` - Publish doxygen-generated API pages
* :github:`6770` - Multiple Git Repositories
* :github:`6636` - Enable hardware flow control in mcux uart shim driver
* :github:`6605` - Question: Does WAMP protocol has any usecase for Zephyr
* :github:`6545` - ARM Cortex R Architecture support
* :github:`6370` - I can't find adc name which is f429zi board
* :github:`6183` - sensor: Handling sensors with long measurement times
* :github:`6086` - Ideas/requirements for improved TLS support in Zephyr
* :github:`6039` - Implement interrupt driven USART on LPC54114
* :github:`5579` - Address untested options in kernel
* :github:`5529` - Explore Little File System (littlefs) support
* :github:`5460` - scripts: extract_dts_includes: check yaml dts binding property "category: "
* :github:`5457` - Add SDHC card support
* :github:`5423` - [nRF] on-target tests accidentally run twice
* :github:`5365` - Revisit DHCPv4 test, convert to ztest
* :github:`4628` - Sample app enabling sensors on disco_l475_iot1 board
* :github:`4506` - rework GPIO flags
* :github:`4505` - sanitycheck should distinguish compile errors from link errors
* :github:`4375` - Provide a script to find files not owned per the CODEOWNERS file
* :github:`4178` - Openweave Support
* :github:`3936` - BT: bt_conn_create_le(): No way to find out error cause
* :github:`3930` - Need a CPU frequency driver for ESP-32
* :github:`3928` - [TIMER] k_timer_start should take 0 value for duration parameter
* :github:`3846` - to use {k_wakeup} to cancel the delayed startup of one thread
* :github:`3805` - [test] test scheduling of threads along with  networking conditions
* :github:`3767` - reconsider k_mem_pool APIs
* :github:`3759` - reconsider k_mbox APIs
* :github:`3754` - Support static BT MAC address
* :github:`3749` - ESP32: Deep Sleep
* :github:`3722` - Enable Flash Cache for ESP32
* :github:`3710` - 802.15.4 Soft MAC 2015 version support
* :github:`3678` - Implement the Read Static Addresses Command
* :github:`3673` - reconsider k_queue APIs
* :github:`3651` - add tickless idle and kernel support to RISCV32 pulpino
* :github:`3648` - Ability to unpair devices
* :github:`3640` - BLE central scan ignores changes in payload
* :github:`3547` - IP stack: Ideas for optimizations
* :github:`3500` - ESP8266 Architecture Configuration
* :github:`3499` - evaluate Emul8 as a replacement for QEMU in sanitycheck
* :github:`3442` - TCP connection locally isn't possible
* :github:`3426` - Only supports classic Bluetooth compilation options
* :github:`3418` - Bluetooth True Wireless Stereo
* :github:`3417` - BT Phonebook Access Profles
* :github:`3399` - Texas Instruments CC2538 Support
* :github:`3396` - LLDP: Implement local MIB support with advertisement of mandatory TLVs
* :github:`3295` - Advanced Power Management
* :github:`3286` - Native Zephyr IP Stack Advanced Features
* :github:`3155` - xtensa: fix tests/kernel/mem_protect/stackprot
* :github:`3040` - Add support for the Arduino Ethernet Shield V2
* :github:`2994` - The build system crashes when GCCARMEMB_TOOLCHAIN_PATH has a space in it
* :github:`2984` - frdm_k64f bus exception bug due to peculiar RAM configuration
* :github:`2933` - Add zephyr support to openocd upstream
* :github:`2900` - Support for the BBC micro:bit Bluetooth Profile
* :github:`2856` - Customer: Floating Point samples
* :github:`2780` - Set the _Swap() return value on ARC the same way it is now done on ARM ?
* :github:`2766` - BMI160 Oversampling Configuration
* :github:`2682` - Add support for UART A1 for TI CC3200 SoC
* :github:`2623` - nRF52 UART behaviour sensitive to timing of baud rate initialization.
* :github:`2611` - SDP client
* :github:`2587` - Support for BR/EDR SSP out-of-band pairing
* :github:`2586` - Support for LE SC out-of-band pairing
* :github:`2401` - 802.15.4 - LLDN frame support
* :github:`2400` - 802.15.4 - Multipurpose frame support
* :github:`2399` - 802.15.4 - TSCH Radio protocol support
* :github:`2398` - 802.15.4 - Update existing Management commands
* :github:`2397` - 802.15.4 - IE list support
* :github:`2396` - 802.15.4 - Management service: FFD level support
* :github:`2333` - Get IPv6 Ready approval
* :github:`2049` - Enable ARM M4F FPU lazy stacking
* :github:`2046` - Add a driver to support timer functions using the PWM H/W periphral timers
* :github:`1933` - nios2: implement nested interrupts
* :github:`1860` - Add support for getting OOB data
* :github:`1843` - nios2: enable interrupt driven serial console for JTAG UART
* :github:`1766` - nios2: implement asm atomic operations
* :github:`1392` - No module named 'elftools'
* :github:`335` - images for the wiki
