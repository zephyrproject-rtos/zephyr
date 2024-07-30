:orphan:

.. _zephyr_2.2:
.. _zephyr_2.2.1:

Zephyr 2.2.1
#############

This is a maintenance release for Zephyr 2.2 with fixes.

See :ref:`zephyr_2.2.0` for the previous version release notes.

Security Vulnerability Related
******************************

The following security vulnerabilities (CVE) were addressed in this release:

  * Fix CVE-2020-10028
  * Fix CVE-2020-10060
  * Fix CVE-2020-10063
  * Fix CVE-2020-10066

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Issues Fixed
************

These GitHub issues were addressed since the previous 2.2.0 tagged
release:

* :github:`23494` - Bluetooth: LL/PAC/SLA/BV-01-C fails if Slave-initiated Feature Exchange is disabled
* :github:`23485` - BT: host: Service Change indication sent regardless of whether it is needed or not.
* :github:`23482` - 2M PHY + DLE and timing calculations on an encrypted link are wrong
* :github:`23070` - Bluetooth: controller: Fix ticker implementation to avoid catch up
* :github:`22967` - Bluetooth: controller: ASSERTION FAIL on invalid packet sequence
* :github:`24183` - [v2.2] Bluetooth: controller: split: Regression slave latency during connection update
* :github:`23805` - Bluetooth: controller: Switching to non conn adv fails for Mesh LPN
* :github:`24086` - Bluetooth: SMP: Existing bond deleted on pairing failure
* :github:`24211` - [v2.2.x] lib: updatehub: Not working on Zephyr 2.x
* :github:`24601` - Bluetooth: Mesh: Config Client's net_key_status pulls two key indexes, should pull one.
* :github:`25067` - Insufficient ticker nodes for vendor implementations
* :github:`25350` - Bluetooth: controller: Data transmission delayed by slave latency
* :github:`25483` - Bluetooth: controller: split: feature exchange not conform V5.0 core spec
* :github:`25478` - settings_runtime_set() not populating bt/cf
* :github:`25447` - cf_set() returns 0 when no cfg is available

.. _zephyr_2.2.0:

Zephyr 2.2.0
############

We are pleased to announce the release of Zephyr RTOS version 2.2.0.

Major enhancements with this release include:

* We added initial support for 64-bit ARMv8-A architecture (Experimental).
* CANopen protocol support through 3rd party CANopenNode stack
* LoRa support was added through integration of the Semtech LoRaWAN endpoint
  stack and addition of a new SX1276 LoRa modem driver.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following security vulnerabilities (CVEs) were addressed in this release:

  * Fix CVE-2020-10019
  * Fix CVE-2020-10021
  * Fix CVE-2020-10023
  * Fix CVE-2020-10024
  * Fix CVE-2020-10026
  * Fix CVE-2020-10027
  * Fix CVE-2020-10028
  * Fix CVE-2020-10058

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

Deprecated in this release
==========================

* Settings

  * SETTINGS_USE_BASE64, encoding values in base64 is marked for removal.

Stable API changes in this release
==================================

* GPIO

  * GPIO API has been reworked to support flags known from Linux DTS GPIO
    bindings. They will typically be defined in the board DTS file

    - GPIO_ACTIVE_LOW, GPIO_ACTIVE_HIGH used to set pin active level
    - GPIO_OPEN_DRAIN, GPIO_OPEN_SOURCE used to configure pin as open drain or
      open source
    - GPIO_PULL_UP, GPIO_PULL_DOWN used to configure pin bias

  * Reading / writing of pin logical level is supported by gpio_pin_get,
    gpio_pin_set functions.
  * Reading / writing of pin physical level is supported by gpio_pin_get_raw,
    gpio_pin_set_raw functions.
  * New set of port functions that operate simultaneously on multiple pins
    that belong to the same controller.
  * Interrupts should be configured by a dedicated
    gpio_pin_interrupt_configure() function. Configuring interrupts via
    gpio_pin_configure() is still supported but this feature will be removed
    in future releases.
  * New set of flags allows to set arbitrary interrupt configuration (if
    supported by the driver) based on pin physical or logical levels.
  * New set of flags to configure pin as input, output or in/out as well as set
    output initial state.
  * Majority of the old GPIO API has been deprecated. While the care was taken
    to preserve backward compatibility due to the scope of the work it was not
    possible to fully achieve this goal. We recommend to switch to the new GPIO
    API as soon as possible.
  * Areas where the deprecated API may behave differently to the original old
    implementation are:

    - Configuration of pin interrupts, especially involving GPIO_INT_ACTIVE_LOW
      and GPIO_POL_INV flags.
    - Behavior of gpio_pin_configure() when invoked without interrupt related
      flags. In the new implementation of this deprecated functionality the
      interrupts remain unmodified. In the original implementation some of the
      GPIO drivers would disable the interrupts.

  * Several drivers that rely on the functionality provided by the GPIO API
    were reworked to honor pin active level. Any external users of these
    drivers will have to update their DTS board files.

    - bluetooth/hci/spi.c
    - display/display_ili9340.c
    - display/ssd1306.c
    - ieee802154/ieee802154_mcr20a.c
    - ieee802154/ieee802154_rf2xx.c
    - lora/sx1276.c
    - wifi/eswifi/eswifi_core.c
    - majority of the sensor drivers

* PWM

  * The pwm_pin_set_cycles(), pwm_pin_set_usec(), and
    pwm_pin_set_nsec() functions now take a flags parameter. The newly
    introduced flags are PWM_POLARITY_NORMAL and PWM_POLARITY_INVERTED
    for specifying the polarity of the PWM signal. The flags parameter
    can be set to 0 if no flags are required (the default is
    PWM_POLARITY_NORMAL).
  * Similarly, the pwm_pin_set_t PWM driver API function function now
    takes a flags parameter. The PWM controller driver must check the
    value of the flags parameter and return -ENOTSUP if any
    unsupported flag is set.

* USB

  * The usb_enable() function, which was previously invoked automatically
    by the USB stack, now needs to be explicitly called by the application
    in order to enable the USB subsystem.
  * The usb_enable() function now takes a parameter, usb_dc_status_callback
    which can be set by the application to a callback to receive status events
    from the USB stack. The parameter can also be set to NULL if no callback is required.

* nRF flash driver

  * The nRF Flash driver has changed its default write block size to 32-bit
    aligned. Previous emulation of 8-bit write block size can be selected using
    the CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS Kconfig option.
    Usage of 8-bit write block size emulation is only recommended for
    compatibility with older storage contents.

* Clock control

  * The callback prototype (clock_control_cb_t) has now additional argument
    (clock_control_subsys_t) indicating which clock subsystem got started.

Removed APIs in this release
============================

* Shell

  * SHELL_CREATE_STATIC_SUBCMD_SET (deprecated), replaced by
    SHELL_STATIC_SUBCMD_SET_CREATE
  * SHELL_CREATE_DYNAMIC_CMD (deprecated), replaced by SHELL_DYNAMIC_CMD_CREATE

* Newtron Flash File System (NFFS) was removed. NFFS was removed since it has
    serious issues, not fixed since a long time. Where it was possible
    NFFS usage was replaced by LittleFS usage as the better substitute.

Kernel
******

* Addressed some race conditions observed on SMP-enabled systems
* Propagate a distinct error code if a workqueue item is submitted that
  has already been completed
* Disable preemption when handing fatal errors
* Fix an issue with the system call stack frame if the system call is
  preempted and then later tries to Z_OOPS()
* add k_thread_stack_space_get() system call for analyzing thread stack
  space. Older methods which had problems in some cases or on some
  architectures like STACK_ANALYZE() are now deprecated.
* Many kernel object APIs now optionally return runtime error values
  instead of relying on assertions. Whether these return values, fail
  assertions, or do no checking at all is controlled by the new
  Kconfig options ASSERT_ON_ERRORS, NO_RUNTIME_CHECKS, RUNTIME_ERROR_CHECKS.
* Cleanups to the arch_cpu_start() API
* Spinlock validation now dumps the address of the incorrectly used spinlock
* Various improvements to the assertion mechanism
* k_poll() may be passed 0 events, in which case it just puts the caller to
  sleep
* Add k_thread_foreach_unlocked() API
* Add an assertion if k_sleep() is called from an ISR
* Numerous 64-bit fixes, mostly related to data type sizes
* k_mutex_unlock() is now correctly a rescheduling point
* Calling k_thread_suspend() on the current thread now correctly invokes
  the scheduler
* Calling k_thread_suspend() on any thread cancels any pending timeouts for
  that thread
* Fix edge case in meta-IRQ preemption of co-operative threads

Architectures
*************

* ARC:

  * Fixed several irq-handling related issues

* ARM:

  * Added initial support for ARMv8-A 64-bit architecture (Experimental)
  * Added support for Direct Dynamic Interrupts in ARM Cortex-M
  * Fixed several critical bugs in ARM Cortex-R architecture port
  * Fixed several critical bugs in Stack Limit checking for ARMv8-M
  * Added QEMU emulation support for ARM Cortex-A53
  * Enhanced QEMU emulation support for ARM Cortex-R architecture
  * Enhanced test coverage for ARM-specific kernel features
  * Added support for GIC SGI and PPI interrupt types
  * Refactored GIC driver to support multiple GIC versions

* POSIX:

  * N/A

* RISC-V:

  * N/A

* x86:

  * Fix an issue with Kconfig values larger than INT_MAX
  * Fix an issue where callee-saved registers could be unnecessarily
    saved on the stack when handling exceptions on x86_64
  * Fix a potential race with saving RFLAGS on context switch on x86_64
  * Enable 64-bit mode and X2APIC for the 'acrn' target
  * Add a poison value of 0xB9 to RIP if a thread is dispatched on multiple
    cores
  * Implement CONFIG_USERSPACE on x86_64
  * Fix an issue where reserved memory could be overwritten when loading the
    Zephyr image on qemu_x86_64
  * x86_64 will now exit QEMU when encountering a fatal error, much like
    32-bit already does
  * Cleanups and improvements to exception debug messages

Boards & SoC Support
********************

* Added support for these SoC series:

.. rst-class:: rst-columns

   * Atmel SAM4E
   * Atmel SAMV71
   * Broadcom BCM58400
   * NXP i.MX RT1011
   * Silicon Labs EFM32GG11B
   * Silicon Labs EFM32JG12B
   * ST STM32F098xx
   * ST STM32F100XX
   * ST STM32F767ZI
   * ST STM32L152RET6
   * ST STM32L452XC
   * ST STM32G031
   * Intel Apollolake Audio DSP

* Added support for these Xtensa boards:

  .. rst-class:: rst-columns

   * Up Squared board Audio DSP

* Added support for these ARM boards:

  .. rst-class:: rst-columns

   * Atmel SAM 4E Xplained Pro
   * Atmel SAM E54 Xplained Pro
   * Atmel SAM V71 Xplained Ultra
   * Broadcom BCM958401M2
   * Cortex-A53 Emulation (QEMU)
   * Google Kukui EC
   * NXP i.MX RT1010 Evaluation Kit
   * Silicon Labs EFM32 Giant Gecko GG11
   * Silicon Labs EFM32 Jade Gecko
   * ST Nucleo F767ZI
   * ST Nucleo G474RE
   * ST Nucleo L152RE
   * ST Nucleo L452RE
   * ST STM32G0316-DISCO Discovery kit
   * ST STM32VLDISCOVERY

* Removed support for these ARM boards:

  .. rst-class:: rst-columns

     * TI CC2650


* Added support for these following shields:

  .. rst-class:: rst-columns

     * ST7789V Display generic shield
     * TI LMP90100 Sensor Analog Frontend (AFE) Evaluation Board (EVB)

* Removed support for these following shields:

  .. rst-class:: rst-columns

     * Link board CAN

Drivers and Sensors
*******************

* ADC

  * Added LMP90xxx driver with GPIO

* Audio

  * N/A

* Bluetooth

  * Update SPI driver to new GPIO API
  * Minor fixes to H:5 (Three-wire UART) driver

* CAN

  * Support for CAN_2 on STM32, but no simultaneous use of CAN_1 and CAN_2.
  * Support for STM32F3 and STM32F4 series
  * Added SocketCAN support to mcux flexcan driver
  * Fixed bit timing conversion in stm32 driver
  * Introduced can-primary device tree alias

* Clock Control

  * Modified driver for nRF platform to use single device with multiple
    subsystems, one for each clock source.

* Console

  * N/A

* Counter

  * The counter_read() API function is deprecated in favor of
    counter_get_value(). The new API function adds a return value for
    indicating whether the counter was read successfully.
  * Added missing syscalls

* Crypto

  * Added AES GCM, ECB, and CBC support to crypto_mtls_shim
  * Added stm32 CRYP driver

* Debug

  * N/A

* Display

  * Added generic display driver sample
  * Added support for BGR565 pixel format
  * Added support for LVGL v6.1
  * Introduced KSCAN based ft5336 touch panel driver
  * Added support for LVGL touch input device

* DMA

  * dw: renaming cavs drivers into DesignWare
  * stm32: improvements over channels support

* EEPROM

  * Added EEPROM driver for STM32L0 and STM32L1 SoC series
  * Added EEPROM simulator (replacing native_posix EEPROM driver)

* Entropy

  * Added support for sam0
  * Added LiteX PRBS module driver

* ESPI

  * N/A

* Ethernet

  * Support for SiLabs Giant Gecko GG11 Ethernet driver
  * Fixed Ethernet networking for LiteX VexRiscv

* Flash

  * Added Nordic JEDEC QSPI NOR flash driver
  * Unified native_posix flash driver with drivers/flash/flash_simulator
  * fixed: erase native_posix flash in initialization
  * extend MCUX flash drive to support LPC55xxx devices
  * stm32: Replace register accesses for Flash driver to use STM32Cube
  * Nios2: qspi unaligned read support
  * sam0: Add support for SAME54
  * Added the flash driver of the stm32f1x family

* GPIO

  * Updated all drivers to the new API
  * Added LiteX GPIO driver

* Hardware Info

  * N/A

* I2C

  * Enabled interrupts by default in stm32 driver
  * Added I2C shell with scan command
  * Added LiteX I2C controller driver
  * Added STM32G0X support to stm32 driver
  * Added support for bus idle timeout property to mcux lpspi driver
  * Added support for SAME54 to sam0 driver

* I2S

  * N/A

* IEEE 802.15.4

  * Add support for IEEE 802.15.4 rf2xxx driver

* Interrupt Controller

  * Added support for multiple GIC versions
  * Renamed s1000 driver to cavs
  * Added SweRV Programmable Interrupt Controller driver
  * Fixed invalid channel bug for RV32M1 interrupt controller

* IPM

  * N/A

* Keyboard Scan

  * Added ft5336 touch panel driver

* LED

  * N/A

* LED Strip

  * Fixed up ws2812 driver

* LoRa

  * Added APIs and drivers needed to support LoRa technology by reusing the
    LoRaMac-node library.

* Modem

  * Add support for generic GSM modem

* Neural Net

  * N/A

* PCIe

  * N/A

* Pinmux

  * Removed CC2650 driver

* PS/2

  * N/A

 * PTP Clock

   * N/A

* PWM

  * Added RV32M1 timer/PWM driver
  * Added LiteX PWM peripheral driver
  * Added support for intverted PWM signals

* Sensor

  * Fixed DRDY interrupt in lis3mdl driver
  * Added nxp kinetis temperature sensor driver
  * Reworked ccs811 driver
  * Fixed tmp007 driver to use i2c_burst_read
  * Introduced sensor shell module
  * Added ms5607 driver

* Serial

  * nRF UARTE driver support TX only mode with receiver permanently disabled.
  * Enabled shared interrupts support in uart_pl011 driver
  * Implemented configure API in ns16550 driver
  * Removed cc2650 driver
  * Added async API system calls

* SPI

  * Added support for samv71 to sam driver
  * Added support for same54 support to sam0 driver
  * Added PM busy state support in DW driver
  * Added Gecko SPI driver
  * Added mcux flexcomm driver

* Timer

  * Optimized reads of MTIME/MTIMECMP on 64-bit RISC-V
  * Added per-core ARM architected timer driver
  * Added support for same54 to sam0 rtc timer driver

* USB

  * Add support for SAMV71 SoC
  * Add support for SAME54 SoC
  * Extend USB device support to all NXP IMX RT boards

* Video

  * N/A

* Watchdog

  * Added SiLabs Gecko watchdog driver
  * Added system calls
  * Fixed callback call on stm32 wwdg enable

* WiFi

  * Reworked offloading mechanism in eswifi and simplelink drivers

Networking
**********

* Add support to configure OpenThread Sleepy End Device (SED)
* Add 64-bit support to net_buf APIs
* Add support for IEEE 802.15.4 rf2xxx driver
* Add TLS secure renegotiation support
* Add support for Timestamp and Record Route IPv4 options.
  They are only used for ICMPv4 Echo-Request packets.
* Add sample cloud application that shows how to connect to Azure cloud
* Add optional timestamp resource to some of the LWM2M IPSO objects
* Add support to poll() which can now return immediately when POLLOUT is set
* Add support to PPP for enabling connection setup to Windows
* Add signed certificate support to echo-server sample application
* Add support for handling multiple simultaneous mDNS requests
* Add support for SiLabs Giant Gecko GG11 Ethernet driver
* Add support for generic GSM modem which uses PPP to connect to data network
* Add UTC offset and timezone support to LWM2M
* Add RX time statistics support to packet socket
* Update ACK handling in IEEE 802.154 nrf5 driver and OpenThread
* Update MQTT PINGREQ count handling
* Update wpan_serial sample to support more boards
* Update Ethernet e1000 driver debugging prints
* Update OpenThread to use settings subsystem
* Update IPv6 to use interface prefix in routing
* Update socket offloading support to support multiple registered interfaces
* Fix checks when waiting network interface to come up in configuration
* Fix zperf sample issue when running out of network buffers
* Fix PPP IPv4 Control Protocol (IPCP) handling
* Fix native_posix Ethernet driver to read data faster
* Fix PPP option handling
* Fix MQTT to close connection faster
* Fix 6lo memory corruption during uncompression
* Fix echo-server sample application accept handling
* Fix Websocket to receive data in small chunks
* Fix Virtual LAN (VLAN) support to add link local address to network interface
* Various fixes to new TCP stack implementation
* Remove NATS sample application

CAN Bus
*******

* CANopen protocol support through 3rd party CANopenNode stack.
* Added native ISO-TP subsystem.
* Introduced CAN-PRIMARY alias.
* SocketCAN for MCUX flexcan.

Bluetooth
*********

* Host:

  * GAP: Add dynamic LE scan listener API
  * GAP: Pre-allocate connection objects for connectable advertising and
    whitelist initiator.
  * GAP: Fixes for multi-identity support
  * GAP: RPA timeout handling fixes
  * GAP: Add remote version information
  * GATT: Add return value to cfg_write callback
  * L2CAP: move channel processing to the system workqueue
  * L2CAP: multiple fixes for credit-based flowcontrol
  * SMP: Add pairing_accept callback
  * SMP: Fix Security Manager timeout handling

* Mesh:

  * Add support for Mesh Configuration Database
  * Multiple fixes to Friendship feature
  * Add support for sending segmented control messages
  * Add support for sending reliable model publication messages

* BLE split software Controller:

  * Multiple fixes, including all those required to pass qualification
  * Implemented software-deferred privacy for platforms without built-in
    address resolution support
  * Added dynamic TX power control, including a set of vendor-specific commands
    to read and write the TX power
  * Added a Kconfig option, BT_CTLR_PARAM_CHECK, to enable additional parameter
    checking
  * Added basic support for SMI (Stable Modulation Index)
  * Ticker: Implemented dynamic rescheduling
  * Nordic: switched to using a single clock device for clock control
  * openisa: Added encryption and decryption support

* BLE legacy software Controller:

  * Multiple fixes
  * Added dynamic TX power control support

USB Device Stack
****************

* Stack:

  * API: Add support for user device status callback
  * Rework switching to alternate interface
  * Make USB Descriptor power options configurable
  * Derive USB device Serial Number String from HWINFO (required by USB MSC)
  * Move USB transfer functions to appropriate file as preparation for
    the rework
  * Windows OS compatibility: Set USB version to 2.1 when using BOS descriptor
  * Convert VBUS control to new GPIO API

* Classes:

  * CDC ACM: Memory and performance improvements, avoid ZLP during IN transactions
  * DFU: Limit upload length during DFU_UPLOAD to the request buffer size
  * Loopback: Re-trigger usb_write after interface configuration event

Build and Infrastructure
************************

* The minimum Python version supported by Zephyr's build system and tools is
  now 3.6.
* Renamed :file:`generated_dts_board.h` and :file:`generated_dts_board.conf` to
  :file:`devicetree.h` and :file:`devicetree.conf`, along with various related
  identifiers. Including :file:`generated_dts_board.h` now generates a warning
  saying to include :file:`devicetree.h` instead.

Libraries / Subsystems
***********************

* LoRa

  * LoRa support was added through official LoRaMac-node reference
    implementation.

* Logging

  * Improvements in immediate mode: less interrupts locking, better RTT usage,
    logging from thread context.
  * Improved notification about missing log_strdup.

* mbedTLS updated to 2.16.4

HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.

Documentation
*************

* settings: include missing API subgoups into the documentation
* Documentation for new boards and samples.
* Improvements and clarity of API documentation.

Tests and Samples
*****************

* Added sample for show settings subsystem API usage

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.1.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`23351` - boards: nucle_g474re: west flash doesn't work
* :github:`23321` - Bluetooth: LE SC OOB authentication in central connects using different RPA
* :github:`23310` - GUI: LVGL: possible NULL dereference
* :github:`23281` - UART console input does not work on SAM E5x
* :github:`23268` - Unnecessary privileged stacks with CONFIG_USERSPACE=y
* :github:`23244` - kernel.scheduler fails on frdmkw41z
* :github:`23231` - RISCV Machine Timer consistently interrupts long running system after soft reset
* :github:`23221` - status register value always reads 0x0000 in eth_mcux_phy_setup
* :github:`23209` - Bug in tls_set_credential
* :github:`23208` - Can not flash test images into up_squared board.
* :github:`23202` - Macro value for 10 bit ADC is wrong in MEC driver.
* :github:`23198` - rf2xx driver uses mutex in ISR
* :github:`23173` - west flash --nobuild,   west flash-signed
* :github:`23172` - Common west flash, debug arguments like --hex-file can't be used from command line
* :github:`23169` - "blinky" sample fails to build for BBC MicroBit (DT_ALIAS_LED0_GPIOS_CONTROLLER undefined)
* :github:`23168` - Toolchain docs: describe macOS un-quarantine procedure
* :github:`23165` - macOS setup fails to build for lack of "elftools" Python package
* :github:`23148` - bme280 sample does not compile
* :github:`23147` - tests/drivers/watchdog/wdt_basic_api failed on mec15xxevb_assy6853 board.
* :github:`23121` - Bluetooth: Mesh: Proxy servers only resends segments to proxy
* :github:`23110` - PTS: Bluetooth: GATT/SR/GAS/BV-07-C
* :github:`23109` - LL.TS Test LL/CON/SLA/BV-129-C fails (split)
* :github:`23072` - #ifdef __cplusplus missing in tracking_cpu_stats.h
* :github:`23069` - Bluetooth: controller: Assert in data length update procedure
* :github:`23050` - subsys/bluetooth/host/conn.c: conn->ref is not 0 after disconnected
* :github:`23047` - cdc_acm_composite sample doesn't catch DTR from second UART
* :github:`23035` - dhcpv4_client sample not working on sam e70
* :github:`23023` - Bluetooth: GATT CCC problem (GATT Server)
* :github:`23015` - Ongoing LL control procedures fails with must-expire latency (BT_CTLR_CONN_META)
* :github:`23004` - Can't use west to flash test images into up_squared board.
* :github:`23002` - unknown type name 'class'
* :github:`22999` - pend() assertion can allow user threads to crash the kernel
* :github:`22985` - Check if Zephyr is affected by SweynTooth vulnerabilities
* :github:`22982` - PTS: Test framework: Bluetooth: GATT/SR/GAS/BV-01-C,  GATT/SR/GAS/BV-07-C - BTP Error
* :github:`22979` - drivers: hwinfo: Build fails on some SoC
* :github:`22977` - ARM Cortex-M4 stack offset when not using Floating point register sharing
* :github:`22968` - Bluetooth: controller: LEGACY: ASSERTION failure on invalid packet sequence
* :github:`22967` - Bluetooth: controller: ASSERTION FAIL on invalid packet sequence
* :github:`22945` - Bluetooth: controller: ASSERTION FAIL Radio is on during flash operation
* :github:`22933` - k_delayed_work_submit_to_queue returns error code when resubmitting previously completed work.
* :github:`22931` - GPIO callback is not triggered for tests/drivers/gpio/gpio_basic_api on microchip mec15xxevb_assy6853 board
* :github:`22930` - PTS: Test Framework :Bluetooth: SM/MAS/PKE/BV-01-C INCONCLUSIV
* :github:`22929` - PTS: Test Framework :Bluetooth: SM/SLA/SIP/BV-01-C Error
* :github:`22928` - PTS: Test Framework: Bluetooth: SM/MAS/SIGN/BV-03-C, SM/MAS/SIGN/BI-01-C - INCONCLUSIV
* :github:`22927` - PTS: Test Framework: Bluetooth:  SM/MAS/SIP/BV-02-C-INCONCLUSIV
* :github:`22926` - Bluetooth: Cannot establish security and discover GATT when using Split LL
* :github:`22914` - tests/arch/arm/arm_irq_vector_table crashes for nRF5340
* :github:`22912` - [Coverity CID :208406] Macro compares unsigned to 0 in subsys/net/l2/ppp/ppp_l2.c
* :github:`22902` - eth_mcux_phy_setup called before ENET clock being enabled causes CPU to hang
* :github:`22893` - Problem using 3 instances of SPIM on NRF52840
* :github:`22890` - IP networking does not work on ATSAME70 Rev. B
* :github:`22888` - Can't flash test image into iotdk board.
* :github:`22885` - Sanitycheck timeout all test cases on mec15xxevb_assy6853 board.
* :github:`22874` - sanitycheck: when someone instance get stuck because of concurrent.futures.TimeoutErro exception, it always stuck
* :github:`22858` - WDT_DISABLE_AT_BOOT, if enabled by default, degrades functionality of the watchdog
* :github:`22855` - drivers: enc28j60: waits for wrong interrupt
* :github:`22847` - Test gpio_basic_api hangs on cc3220sf_launchxl
* :github:`22828` - kernel: fatal: interrupts left locked in TEST mode
* :github:`22822` - mesh: typo in condition in comp_add_elem of cfg_srv
* :github:`22819` - #define _current in kernel_structs.h leaks into global namespace
* :github:`22814` - mcuboot doesn't build with zephyr v2.1.0
* :github:`22803` - k_delayed_work_cancel documentation inconsistent with behavior
* :github:`22801` - Bluetooth: Split LL: Reconnection problem
* :github:`22786` - Bluetooth: SM/MAS/PROT/BV-01-C FAIL
* :github:`22784` - system hangs in settings_load() nrf52840 custom board
* :github:`22774` - Set USB version to 2.1 when CONFIG_USB_DEVICE_BOS is set
* :github:`22730` - CONFIG_BT_SETTINGS writes bt/hash to storage twice
* :github:`22722` - posix: redefinition of symbols while porting zeromq to zephyr
* :github:`22720` - armv8-m: userspace: some parts in userspace enter sequence need to be atomic
* :github:`22698` - log_stack_usage: prints err: missinglog_strdup()
* :github:`22697` - nrf52 telnet_shell panic. Mutex using in ISR.
* :github:`22693` - net: config: build break when CONFIG_NET_NATIVE=n
* :github:`22689` - driver: modem: sara-u2  error when connecting
* :github:`22685` - armv8-m: userspace: syscall return sequence needs to be atomic
* :github:`22682` - arm: cortex-a: no default board for testing
* :github:`22660` - gpio: legacy level interrupt disable API not backwards compatible
* :github:`22658` - [Coverity CID :208189] Self assignment in soc/xtensa/intel_apl_adsp/soc.c
* :github:`22657` - [Coverity CID :208191] Dereference after null check in subsys/canbus/isotp/isotp.c
* :github:`22656` - [Coverity CID :208192] Out-of-bounds access in tests/subsys/canbus/isotp/implementation/src/main.c
* :github:`22655` - [Coverity CID :208193] Unchecked return value in tests/bluetooth/mesh/src/microbit.c
* :github:`22654` - [Coverity CID :208194] Arguments in wrong order in tests/subsys/canbus/isotp/implementation/src/main.c
* :github:`22653` - [Coverity CID :208196] Out-of-bounds access in drivers/eeprom/eeprom_simulator.c
* :github:`22652` - [Coverity CID :208197] Pointless string comparison in tests/drivers/gpio/gpio_basic_api/src/main.c
* :github:`22651` - [Coverity CID :208198] Logical vs. bitwise operator in boards/xtensa/up_squared_adsp/bootloader/boot_loader.c
* :github:`22650` - [Coverity CID :208199] Arguments in wrong order in tests/subsys/canbus/isotp/conformance/src/main.c
* :github:`22649` - [Coverity CID :208200] Bad bit shift operation in drivers/interrupt_controller/intc_exti_stm32.c
* :github:`22648` - [Coverity CID :208201] Out-of-bounds write in soc/xtensa/intel_apl_adsp/soc.c
* :github:`22647` - [Coverity CID :208202] Arguments in wrong order in samples/subsys/canbus/isotp/src/main.c
* :github:`22646` - [Coverity CID :208203] Missing break in switch in drivers/interrupt_controller/intc_exti_stm32.c
* :github:`22645` - [Coverity CID :208204] Arguments in wrong order in samples/subsys/canbus/isotp/src/main.c
* :github:`22644` - [Coverity CID :208205] Improper use of negative value in tests/subsys/canbus/isotp/implementation/src/main.c
* :github:`22642` - [Coverity CID :208207] Arguments in wrong order in tests/subsys/canbus/isotp/conformance/src/main.c
* :github:`22641` - [Coverity CID :208208] Arguments in wrong order in tests/subsys/canbus/isotp/implementation/src/main.c
* :github:`22640` - [Coverity CID :208209] 'Constant' variable guards dead code in drivers/gpio/gpio_sx1509b.c
* :github:`22636` - Provide Linux-style IS_ERR()/PTR_ERR()/ERR_PTR() helpers
* :github:`22626` -  tests/drivers/counter/counter_basic_api failed on frdm_k64f board.
* :github:`22624` - tests/kernel/semaphore/semaphore failed on iotdk board.
* :github:`22623` - tests/kernel/timer/timer_api failed on mimxrt1050_evk board.
* :github:`22616` - Zephyr doesn't build if x86_64 SDK toolchain isn't install
* :github:`22584` - drivers: spi: spi_mcux_dspi: bus busy status ignored in async
* :github:`22563` - Common west flash/debug etc. arguments cannot be set in CMake
* :github:`22559` - crash in semaphore tests on ARC nsim_em and nsim_sem
* :github:`22557` - document guidelines/principles related to DT usage in Zephyr
* :github:`22556` - document DT macro generation rules
* :github:`22543` - No way to address a particular FTDI for OpenOCD
* :github:`22542` - GEN_ABSOLUTE_SYM cannot handle value larger than INT_MAX on qemu_x86_64
* :github:`22539` - bt_gatt: unable to save SC: no cfg left
* :github:`22535` - drivers: lora: Make the SX1276 driver independent of loramac module
* :github:`22534` - sanitycheck qemu_x86_coverage problem with SDK 0.11.1
* :github:`22532` - Doc build warning lvgl/README.rst
* :github:`22525` - stm32f7xx.h: No such file or directory
* :github:`22522` - GPIO test code tests/drivers/gpio/gpio_basic_api does not compile for microchip board mec15xxevb_assy6853
* :github:`22519` - sanitycheck failures for native_posix
* :github:`22514` - Bluetooth: gatt: CCC cfg not flushed if device was previously paired
* :github:`22510` - Build warnings in samples/net/cloud/google_iot_mqtt
* :github:`22489` - Request to enable CONFIG_NET_PKT_RXTIME_STATS for SOCK_RAW
* :github:`22486` - Do we have driver for Texas Instruments DRV2605 haptic driver for ERM and LRA actuators?
* :github:`22484` - Linker error when building google_iot_mqtt sample with zephyr-sdk 0.11.1
* :github:`22482` - Unable to use LOG_BACKEND_DEFINE macro from log_backend.h using C++
* :github:`22478` - Bluetooth - peripheral_dis - settings_runtime_set not working
* :github:`22474` - boards that have Kconfig warnings on hello_world.
* :github:`22466` - Add hx711 sensor
* :github:`22462` - onoff: why client must be reinitialized after each transition
* :github:`22455` - How to assign USB endpoint address manually in stm32f4_disco for CDC ACM class driver
* :github:`22452` - not driver found in can bus samples for olimexino_stm32
* :github:`22447` - samples: echo_client sample breaks for UDP when larger than net if MTU
* :github:`22444` - [Coverity CID :207963] Argument cannot be negative in tests/net/socket/websocket/src/main.c
* :github:`22443` - [Coverity CID :207964] Dereference after null check in subsys/canbus/canopen/CO_driver.c
* :github:`22442` - [Coverity CID :207965] Missing break in switch in drivers/i2c/i2c_ll_stm32_v1.c
* :github:`22440` - [Coverity CID :207970] Out-of-bounds access in samples/net/sockets/websocket_client/src/main.c
* :github:`22439` - [Coverity CID :207971] Negative array index read in subsys/net/l2/ppp/ipcp.c
* :github:`22438` - [Coverity CID :207973] Out-of-bounds access in tests/net/socket/websocket/src/main.c
* :github:`22437` - [Coverity CID :207974] Out-of-bounds read in tests/net/socket/websocket/src/main.c
* :github:`22436` - [Coverity CID :207975] Logically dead code in subsys/net/l2/ppp/ipcp.c
* :github:`22435` - [Coverity CID :207977] Logically dead code in subsys/canbus/canopen/CO_driver.c
* :github:`22434` - [Coverity CID :207978] Dereference after null check in subsys/canbus/canopen/CO_driver.c
* :github:`22433` - [Coverity CID :207980] Untrusted loop bound in tests/net/socket/websocket/src/main.c
* :github:`22432` - [Coverity CID :207982] Explicit null dereferenced in tests/lib/onoff/src/main.c
* :github:`22430` - [Coverity CID :207985] Argument cannot be negative in subsys/net/lib/websocket/websocket.c
* :github:`22424` - RFC: API Change: clock_control
* :github:`22417` - Build warnings with atsamr21_xpro
* :github:`22410` - arch: arm64: ARM64 port not working on real target
* :github:`22390` - Unable to build http_get with TLS enabled on cc32xx
* :github:`22388` - Build warnings in http_get on cc3220sf_launchxl
* :github:`22366` - Bug in sockets.c (subsys\net\lib\sockets)
* :github:`22363` - drivers: clock_control: clock_stm32_ll_h7.c Move Power Configuration code
* :github:`22360` - test_mqtt_disconnect in mqtt_pubsub fails
* :github:`22356` - An application hook for early init
* :github:`22343` - stm32f303 - irq conflict between CAN and USB
* :github:`22317` - samples/arc_secure_services fails on nsim_sem
* :github:`22316` - samples/philosophers coop_only scenario times out on nsim_sem and nsim_em
* :github:`22307` - net: ip: net_pkt_pull(): packet corruption when using CONFIG_NET_BUF_DATA_SIZE larger than 256
* :github:`22304` - ARM Cortex-M STMF401RE: execution too slow
* :github:`22299` - The file flash_stm32wbx.c generates compilation error
* :github:`22297` - nucleo_wb55rg:samples/bluetooth/peripheral/sample.bluetooth.peripheral fails to build on master
* :github:`22290` - ARC crashes due to concurrent system calls
* :github:`22280` - incorrect linker routing
* :github:`22275` - arm: cortex-R & M: CONFIG_USERSPACE: intermittent Memory region write access failures
* :github:`22272` - aggregated devicetree source file needs to be restored to build directory
* :github:`22268` - timer not working when duration is too high
* :github:`22265` - Simultaneous BLE pairings getting the same slot in keys structure
* :github:`22259` - Bluetooth: default value 80 on BT_ACL_RX_COUNT clamped to 64
* :github:`22258` - sanitycheck fails to merge OVERLAY_CONFIG properly
* :github:`22257` - test wdt_basic_api failed on nucleo_f746zg
* :github:`22245` - STM32G4xx: Wrong SystemCoreClock variable
* :github:`22243` - stm32g431rb: PLL setting result to slow exccution
* :github:`22210` - Bluetooth -  bt_gatt_get_value_attr_by_uuid
* :github:`22207` - Bluetooth ：Mesh：Provison init should after proxy
* :github:`22204` - CONFIG_BT_DEBUG_LOG vs atomic operations
* :github:`22202` - bt_rand() is called over HCI when BT_HOST_CRYPTO=y, even if BT_CTLR_LE_ENC=n
* :github:`22197` - dts: gen_defines.py bails out on new path property type
* :github:`22188` - drivers: espi: xec : eSPI driver should not send VWire SUS_ACK automatically in all cases
* :github:`22177` - Adafruit M0 boards are not set up to correctly flash in their code partitions
* :github:`22171` - West bossac runner inorrectly tries to include an offset parameter when flashing
* :github:`22128` - frdm_k82f:samples/drivers/spi_fujitsu_fram/sample.drivers.spi.fujitsu_fram fails
* :github:`22107` - mdns support with avahi as client
* :github:`22106` - intermittent emulator exit on samples/userspace/shared_mem on qemu_x86_64
* :github:`22088` - Bluetooth Mesh friendship is cleared due to no Friend response reception
* :github:`22086` - L2CAP/SMP: Race condition possible in native posix central when bonding.
* :github:`22085` - HCI/CCO/BV-07-C & HCI/GEV/BV-01-C failing in EDTT
* :github:`22066` - tests/kernel/mem_pool/mem_pool_threadsafe fails reliably on m2gl025_miv
* :github:`22062` - Adafruit Feather M0 does not flash correctly - incorrect flash code offset and bossa version incompatibility
* :github:`22060` - Build fails with gnuarmemb under windows
* :github:`22051` - Bluetooth Central: Discovery of 128bit primary service fails with later versions of gcc.
* :github:`22048` - Failing LL.TS Data Length Update Tests (split)
* :github:`22037` - qemu_cortex_r5 excludes too many tests
* :github:`22036` - sanitycheck for qemu_cortex_r5 fails
* :github:`22026` - west: openocd runner fails for boards without support/openocd.cfg
* :github:`22014` - RTC prescaler overflow on nRF(52)
* :github:`22010` - Bluetooth 'central' failure on native_posix
* :github:`22003` - 'central' failure on nrf52_pca10040
* :github:`21996` - Native POSIX or QEMU X86 emulation does not detect Bluetooth HCI Vendor-Specific Extensions
* :github:`21989` - websocket: recv_msg always returns full message length on last call
* :github:`21974` - make include hierarchy consistent with expected usage
* :github:`21970` - net: dns: mDNS resolving fails when responder is also enabled
* :github:`21967` - json: json_obj_parse will modify the input string
* :github:`21962` - drivers: usb: usb_dc_stm32: does not compile for stm32f3_disco board
* :github:`21949` - net: TCP: echo server deadlock from TCP packet
* :github:`21935` - SPI - STM32: transceive() should handle null tx buffer
* :github:`21917` - cmake error with CONFIG_COUNTER and CONFIG_BT both enabled (nrf52 board)
* :github:`21914` - net: dns: Answers to multiple mDNS queries sent in parallel aren't properly handled
* :github:`21888` - Print unmet Kconfig dependency
* :github:`21875` - sanitycheck warning for silabs,gecko-spi-usart.yaml
* :github:`21869` - IPv6 neighbors get added too eagerly
* :github:`21859` - Bluetooth LE Disconnect event not received
* :github:`21854` - HCI-UART: Bluetooth ACL data packets with 251 bytes not acknowledged
* :github:`21846` - RFC: API: Counter: counter_read() has no way of indicating failure
* :github:`21837` - net: socket: Add dependency to mbedtls
* :github:`21813` - tests/kernel/timer/timer_api failed on frdm_k64f board.
* :github:`21812` - tests/arch/arm/arm_irq_advanced_features failed on reel_board.
* :github:`21800` - Xtensa doesn't save SCOMPARE1 register on context switch
* :github:`21790` - tests/kernel/timer/timer_api fails on nucleo_g071rb board
* :github:`21789` - Merge topic-gpio back to master
* :github:`21784` - sanitycheck prints some build errors directly to the console
* :github:`21780` - OpenThread fails on nRF52840 Dongle (nrf52840_pca10059)
* :github:`21775` - echo_server and 802154 not build for NRF52811
* :github:`21768` - Make [CONFIG_NET_SOCKETS_SOCKOPT_TLS] dependent on [CONFIG_MBEDTLS] in menuconfig
* :github:`21764` - [SARA-R4] MQTT publisher not working - Impossible to connect to broker
* :github:`21763` - at86rf2xx radio driver does not report whether a TX was ACKed
* :github:`21756` - tests/kernel/obj_tracing failed on mec15xxevb_assy6853 board.
* :github:`21755` - tests/drivers/adc/adc_api  failed on  mec15xxevb_assy6853 board.
* :github:`21745` - tests: counter_basic_api: Failed on stm32 based boards
* :github:`21744` - dumb_http_server_mt with overlay-tls.conf does not connect
* :github:`21735` - ARM: Cortex-M: IRQ lock/unlock() API non-functional but accessible from user mode
* :github:`21716` - nucleo_g431rb: Hello world not working
* :github:`21715` - nucleo_g431rb: Blinky too slow / wrong clock setup?
* :github:`21713` - CDC ACM USB class issue with high transfer rate and ZLP
* :github:`21702` - [Coverity CID :206599] Out-of-bounds access in tests/bluetooth/uuid/src/main.c
* :github:`21700` - [Coverity CID :206606] Out-of-bounds access in tests/bluetooth/uuid/src/main.c
* :github:`21699` - [Coverity CID :206608] Dereference null return value in tests/net/icmpv4/src/main.c
* :github:`21695` - Documentation issues on v1.14-branch block backport
* :github:`21681` - nucleo_g431rb / STM32G4: Flashing works only once
* :github:`21679` - SPI broken on stm32f412 on master
* :github:`21676` - [Coverity CID :206389] Logically dead code in subsys/testsuite/ztest/src/ztest.c
* :github:`21674` - [Coverity CID :206392] Side effect in assertion in tests/kernel/timer/starve/src/main.c
* :github:`21673` - [Coverity CID :206393] Unintentional integer overflow in drivers/sensor/ms5607/ms5607.c
* :github:`21672` - [Coverity CID :206394] Logically dead code in subsys/testsuite/ztest/src/ztest.c
* :github:`21660` - Sample projects do not build for Nucleo WB55RG
* :github:`21659` - at86rf2xx radio driver not (reliably) sending ACKs
* :github:`21650` - _TEXT_SECTION_NAME_2 on ARM Cortex-R
* :github:`21637` - sanitycheck failed issue in parallel running.
* :github:`21629` - error with 'west update' on Windows 10
* :github:`21623` - DT: accept standard syntax for phandle in chosen node
* :github:`21618` - CI failing to complete tests
* :github:`21617` - Allow per module prj.conf
* :github:`21614` - host toolchain for x86 fails on empty CMAKE_C_FLAGS
* :github:`21607` - BME680 Sensor is not building
* :github:`21601` - '!radio_is_ready()' failed
* :github:`21599` - CONFIG_HEAP_MEM_POOL_SIZE and k_malloc, k_free not working in nrf51_pca10028
* :github:`21597` - sht3xd build error on olimexino_stm32
* :github:`21591` - Timeout error for the Microchip board during Sanitycheck
* :github:`21586` - Bluetooth Mesh fail to transmit messages after some time on nRF52840
* :github:`21581` - GNU ARM Embedded link broken in Getting Started
* :github:`21571` - CONFIG_BT_CENTRAL doesnot work fine with nrf51_pca10028
* :github:`21570` - how to select usb mps for SAME70 board
* :github:`21568` - mps2_an385:tests/kernel/tickless/tickless_concept/kernel.tickless.concept  fail
* :github:`21552` - Constant disconnects while attempting BT LE multi-central application.
* :github:`21551` - gpio: xec: GPIO Interrupt is not triggered for range GPIO240_276
* :github:`21546` - SPI broken for STM32L1
* :github:`21536` - tests/subsys/fs/fat_fs_api fails on native_posix_64
* :github:`21532` - can not build the image ,No targets specified and no makefile found
* :github:`21514` - Logging - strange behaviour with RTT on nRF53
* :github:`21510` - re-v
* :github:`21493` - System tick is not running
* :github:`21483` - sanitycheck messages in CI are not informative anymore
* :github:`21475` - sanitycheck: hardware map generation unexpected exit during the first attempt
* :github:`21466` - doc: extract_content.py not copying images in a table
* :github:`21450` - sample.net.cloud.google_iot_mqtt test is failing for frdm_k64f
* :github:`21448` - nrf52840 errata_98 / 89 mixup
* :github:`21443` - "HCI_USB" sample doesn't compile with "nucleo_wb55rg" board
* :github:`21438` - sanitycheck reports "FAILED: N/A" for failed or hung tests
* :github:`21432` - watchdog subsystem has no system calls
* :github:`21431` - missing async uart.h system calls
* :github:`21429` - Impossible to override syscalls
* :github:`21426` - civetweb triggers an error on Windows with Git 2.24
* :github:`21422` - Added nucleo-f767zi board support and would like to share
* :github:`21419` - RFC: API Change: usb: Make users call usb_enable. Provide global status callback.
* :github:`21418` - Crash when suspending system
* :github:`21410` - bt_ctlr_hci: Tx Buffer Overflow on LL/CON/MAS/BV-04-C, LL/CON/SLA/BV-05-C & LL/CON/SLA/BV-06-C
* :github:`21409` - sanitycheck: cmd.exe colorized output
* :github:`21385` - board frdm_kl25z build passed, but can't flash
* :github:`21384` - RFC: API Change: PWM: add support for inverted PWM signals
* :github:`21379` - Bluetooth: Mesh: Node Reset Not Clear Bind Key Information
* :github:`21375` - GATT: gatt_write_ccc_rsp with error (0x0e) removes always beginning from subscriptions head
* :github:`21365` - implicit casts in API headers must be replaced for C++ support
* :github:`21351` - tests/drivers/counter/counter_basic_api  failed on mimxrt1050_evk board.
* :github:`21341` - conditions required for safe call of kernel operations from interrupts
* :github:`21339` - Expired IPv6 router causes an infinite loop
* :github:`21335` - net: TCP: Socket echo server does not accept incoming connections when TLS is enabled
* :github:`21328` - Apparent network context leak with offloading driver (u-blox Sara r4)
* :github:`21325` - Where should the Digital-Input, Output, ADC driver be added?
* :github:`21321` - error update for project civetweb
* :github:`21318` - CONFIG_SYS_POWER_MANAGEMENT Makes Build Fail for nRF5340 and nRF9160
* :github:`21317` - intermittent SMP crashes on x86_64
* :github:`21306` - ARC: syscall register save/restore needs backport to 1.14
* :github:`21301` - Coverage report generated for qemu_x86 board is incomplete
* :github:`21300` - pyocd flash failing on bbc_microbit
* :github:`21299` - bluetooth: Controller does not release buffer on central side after peripheral reset
* :github:`21290` - Compiler warnings in flash.h: invalid conversion from 'const void*' to 'const flash_driver_api*'
* :github:`21281` - logging: msg_free may erroneously call log_free
* :github:`21278` - How to use pwm in nrf52832 for rgb led
* :github:`21275` - kl2x soc fixup is missing I2C_1 labels
* :github:`21257` - tests/net/net_pkt failed on mimxrt1050_evk board.
* :github:`21240` - Error west flash
* :github:`21229` - cc1plus: warning: '-Werror=' argument '-Werror=implicit-int' is not valid for C++
* :github:`21202` - Required upgrade of HAL
* :github:`21186` - Gatt discover callback gives invalid pointer to primary and secondary service UUID.
* :github:`21185` - zero-latency IRQ behavior is not documented?
* :github:`21181` - devicetree should support making properties with defaults required
* :github:`21177` - Long ATT MTU reports wrong length field in write callback.
* :github:`21171` - Module Request: Optiga Trust X
* :github:`21167` - libraries.libc.newlib test fails
* :github:`21165` - Bluetooth: Mesh: Friend Clear message from a Friend node
* :github:`21162` - Sanitycheck corrupted test case names in test-report.xml files
* :github:`21161` - question: openthread with other boards
* :github:`21148` - nrf51: uart_1 does not compile
* :github:`21139` - west: runners: blackmagicprobe: Keyboard Interrupt shouldn't kill the process
* :github:`21131` - Bluetooth: host: Subscriptions not removed upon unpair
* :github:`21126` - drivers: spi_nrfx_spim: Incorrect handling of extended SPIM configuration
* :github:`21123` - sanitycheck halt some test cases with parallel running.
* :github:`21121` - netusb: RNDIS host support
* :github:`21115` - Request a new repository for the Xtensa HAL
* :github:`21105` - Bluetooth API called before finished initialization.
* :github:`21103` - Bluetooth: host: Reduce overhead of GATT subscriptions
* :github:`21099` - echo server qemu_x86 e1000 cannot generate coverage reports
* :github:`21095` - [Coverity CID :206086] Out-of-bounds access in drivers/timer/cortex_m_systick.c
* :github:`21094` - native_posix doesn't call main function that's defined in C++
* :github:`21082` - tests/kernel/timer/timer_api failing on several nRF5x SoCs
* :github:`21074` - Enhance 802.1Qav documentation
* :github:`21058` - BLE: Enable/Disable Automatic sending of Connection Parameter update request on Timeout.
* :github:`21057` - BLE: No Valid Parameter check in send_conn_le_param_update()
* :github:`21045` - log_backend.h missing include for UTIL_CAT in LOG_BACKEND_DEFINE macro
* :github:`21036` - Add SMP function similar to bt_conn_get_info
* :github:`21025` - sam_e70_xplained reboots after 35secs
* :github:`20981` - mempool: MPU fault
* :github:`20974` - file resources exceeded with sanitycheck
* :github:`20953` - usb: nrf: usb on reel board becomes unavailable if USB cable is not connected at first
* :github:`20927` - ztest_1cpu_user_unit_test() doesn't work
* :github:`20915` - doc: Kconfig section in board_porting.rst should be moved or removed
* :github:`20904` - kernel.timer.tickless is failed due to missing TEST_USERSPACE flag
* :github:`20886` - [Coverity CID :205826] Memory - corruptions in tests/subsys/fs/nffs_fs_api/common/nffs_test_utils.c
* :github:`20885` - [Coverity CID :205819] Memory - corruptions in tests/subsys/fs/nffs_fs_api/common/nffs_test_utils.c
* :github:`20884` - [Coverity CID :205799] Memory - corruptions in tests/subsys/fs/nffs_fs_api/common/nffs_test_utils.c
* :github:`20877` - [Coverity CID :205823] Null pointer dereferences in tests/kernel/fifo/fifo_timeout/src/main.c
* :github:`20802` - reschedule not done after mutex unlock
* :github:`20770` - irq locking in logging backend can cause missing interrupts
* :github:`20755` - mcuboot: add as module and verify functionality
* :github:`20749` - samples:sample.net.dns_resolve.mdns:frdmk64f ipv4dns handler has not result
* :github:`20748` - build warnings on lpcxpresso54114_m0/m4 board
* :github:`20746` - Bluetooth: Mesh: Friend node Adding another Friend Update
* :github:`20724` - Packed pointer warning in LL Controller
* :github:`20698` - Bluetooth: host: Skip pre-scan done by bt_conn_create_le if not needed
* :github:`20697` - Confusing warning during cmake
* :github:`20673` - guiconfig not working properly?
* :github:`20640` -  Bluetooth: l2cap do not recover when faced with long packets and run out of buffers
* :github:`20629` - when CONFIG_BT_SETTINGS is enabled, stack stores id in flash memory each power up of device (call to bt_enable)
* :github:`20618` - Can unicast address be relayed when send message over gatt proxy?
* :github:`20576` - DTS overlay files must include full path name
* :github:`20561` - Crypto API: Separate IV from ciphertext based on struct cipher_ctx::flags
* :github:`20535` - [Coverity CID :205619]Null pointer dereferences in /tests/net/ieee802154/fragment/src/main.c
* :github:`20497` - [Coverity CID :205638]Integer handling issues in /drivers/pwm/pwm_mchp_xec.c
* :github:`20490` - [Coverity CID :205651]Uninitialized variables in /drivers/dma/dma_stm32.c
* :github:`20484` - Tests/kernel/gen_isr_table failing when enabling WDT driver
* :github:`20426` - sensors: grove temperature and light drivers out of date
* :github:`20414` - nRF51 issues with the split link layer
* :github:`20411` - samples: lis3mdl trigger not working with x_nucleo_iks01a1
* :github:`20388` - Allow for runtime reconfiguration of SPI master / slave
* :github:`20355` - west build for zephyr/samples/net/sockets/echo_server/ on qemu_xtensa target outputs elf with panic
* :github:`20315` - zperf TCP uploader fails
* :github:`20286` - Problem building for ESP32
* :github:`20278` - Something is wrong when trying ST7789V sample
* :github:`20264` - Bluetooth: Delay advertising events instead of dropping them on collision
* :github:`20256` - settings subsystem sample
* :github:`20217` - Extend qemu_cortex_r5 test coverage
* :github:`20172` - devicetree support for compound elements
* :github:`20161` - Facing issue to setup zephyr on ubuntu
* :github:`20153` - BLE small throughput
* :github:`20140` - CMake: syscall macro's are not generated for out of tree DTS_ROOT
* :github:`20125` - Add system call to enter low power mode and reduce latency for deep sleep entry
* :github:`20026` - sanitycheck corrupts stty in some cases
* :github:`20017` - Convert GPIO users to new GPIO API
* :github:`19982` - Periodically wake up log process thread consume more power
* :github:`19922` - Linear time to give L2CAP credits
* :github:`19869` - Implement tickless capability for xlnx_psttc_timer
* :github:`19761` - tests/net/ieee802154/fragment failed on reel board.
* :github:`19737` - No Function In Zephyr For Reading BLE Channel Map?
* :github:`19666` - remove kernel/include and ``arch/*/include`` from default include path
* :github:`19643` - samples/boards/arc_secure_services fails on nsim_sem
* :github:`19545` - usb: obtain configuration descriptor's bmAttributes and bMaxPower from DT
* :github:`19540` - Allow running and testing network samples in automatic way
* :github:`19492` - sanitycheck: unreliable/inconsistent catch of ASSERTION FAILED
* :github:`19488` - Reference and sample codes to get started with the friendship feature in ble mesh
* :github:`19473` - Missing NULL parameter check in k_pipe_get
* :github:`19361` - BLE Scan fails to start when running in parallel with BLE mesh
* :github:`19342` - Bluetooth: Mesh: Persistent storage of Virtual Addresses
* :github:`19245` - Logging: Assert with LOG_IMMEDIATE
* :github:`19100` - LwM2M sample with DTLS: does not connect
* :github:`19053` - 2.1 Release Checklist
* :github:`18962` - [Coverity CID :203909]Memory - corruptions in /subsys/mgmt/smp_shell.c
* :github:`18867` - zsock_poll() unnecessarily wait when querying for ZSOCK_POLLOUT
* :github:`18852` - west flash fails for cc1352r_launchxl
* :github:`18635` - isr4 repeatedly gets triggered after test passes in tests/kernel/gen_isr_table
* :github:`18583` - hci_usb: NRF52840 connecting addtional peripheral fails
* :github:`18551` - address-of-temporary idiom not allowed in C++
* :github:`18530` - Convert GPIO drivers to new GPIO API
* :github:`18483` - Bluetooth: length variable inconsistency in keys.c
* :github:`18452` - [Coverity CID :203463]Memory - corruptions in /tests/lib/ringbuffer/src/main.c
* :github:`18447` - [Coverity CID :203400]Integer handling issues in /tests/lib/fdtable/src/main.c
* :github:`18410` - [Coverity CID :203448]Memory - corruptions in /subsys/net/lib/lwm2m/ipso_onoff_switch.c
* :github:`18378` - [Coverity CID :203537]Error handling issues in /samples/subsys/nvs/src/main.c
* :github:`18280` - tests/drivers/adc/adc_api fails on frdmkl25z
* :github:`18173` - ARM: Core Stack Improvements/Bug fixes for 2.1 release
* :github:`18169` - dts: bindings: inconsistent file names and base.yaml include of general device controllers
* :github:`18137` - Add section on IRQ generation to doc/guides/dts/index.rst
* :github:`17852` - Cmsis_rtos_v2_apis test failed on iotdk board.
* :github:`17838` - state DEVICE_PM_LOW_POWER_STATE of Device Power Management
* :github:`17787` - openocd unable to flash hello_world to cc26x2r1_launchxl
* :github:`17731` - Dynamically set TX power of BLE Radio
* :github:`17689` - On missing sensor, Init hangs
* :github:`17543` - dtc version 1.4.5 with ubuntu 18.04 and zephyr sdk-0.10.1
* :github:`17310` - boards: shields: use Kconfig.defconfig system for shields
* :github:`17309` - enhancements to device tree generation
* :github:`17102` - RFC: rework GPIO interrupt configuration
* :github:`16935` - Zephyr doc website: Delay search in /boards to the end of the search.
* :github:`16851` - west flash error on zephyr v1.14.99
* :github:`16735` - smp_svr sample does not discover services
* :github:`16545` - west: diagnose dependency version failures
* :github:`16482` - mcumgr seems to compromise BT security
* :github:`16472` - tinycrypt ecc-dh and ecc-dsa should not select entropy generator
* :github:`16329` - ztest teardown function not called if test function is interrupted
* :github:`16239` - Build: C++ compiler warning '-Wold-style-definition'
* :github:`16235` - STM32: Move STM32 Flash driver to CMSIS STM32Cube definitions
* :github:`16232` - STM32: implement pinmux api
* :github:`16202` - Improve help for west build target
* :github:`16034` - Net packet size of 64 bytes doesn't work.
* :github:`16023` - mcuboot: enabling USB functionality in MCUboot crashes zephyr application in slot0
* :github:`16011` - Increase coverage of tests
* :github:`15906` - WEST ERROR: extension command build was improperly defined
* :github:`15841` - Support AT86RF233
* :github:`15729` - flash: should write_protection be emulated?
* :github:`15657` - properly define kernel <--> arch APIs
* :github:`15611` - gpio/pinctrl: GPIO and introduce PINCTRL API to support gpio, pinctrl DTS nodes
* :github:`15593` - How to use gdb to view the stack of a thread
* :github:`15580` - SAMD21 Adafruit examples no longer run on boards
* :github:`15435` - device fails to boot when spi max frequency set above 1000000
* :github:`15278` - CANopen Support
* :github:`15229` - network tests have extremely restrictive whitelist
* :github:`15171` - BLE Throughput
* :github:`14927` - checkpatch: not expected behavior for multiple git commit check.
* :github:`14922` - samples/boards/altera_max10/pio: Error configuring GPIO PORT
* :github:`14753` - nrf52840_pca10056: Leading spurious 0x00 byte in UART output
* :github:`14668` - net: icmp4: Zephyr strips record route and time stamp options
* :github:`14650` - missing system calls in Counter driver APIs
* :github:`14639` - All tests should be SMP-safe
* :github:`14632` - Default for TLS_PEER_VERIFY socket option are set to required, may lead to confusion when running samples against self-signed certs
* :github:`14621` - BLE controller: Add support for Controller(SW deferred)-based Privacy
* :github:`14287` - USB HID Get_Report and Set_Report
* :github:`14206` - user mode documentation enhancements
* :github:`13991` - net: Spurious driver errors due to feeding packets into IP stack when it's not fully initialized (assumed reason)
* :github:`13943` - net: QEMU Ethernet drivers are flaky (seemingly after "net_buf" refactor)
* :github:`13941` - Alternatives for OpenThread settings
* :github:`13894` - stm32f429i_disc1: Add DTS for USB controller
* :github:`13403` - USBD event and composite-device handling
* :github:`13232` - native_posix doc: Add mention of virtual USB
* :github:`13151` - Update documentation on linking Zephyr within a flash partition
* :github:`12968` - dfu/mcuboot: solution for Set pending: don't crash when image slot corrupt
* :github:`12860` - No test builds these files
* :github:`12814` - TCP connet Net Shell function seems to not working when using NET_SOCKETS_OFFLOAD
* :github:`12635` - tests/subsys/fs/nffs_fs_api/common/nffs_test_utils.c fail with Assertion failure on nrf52840
* :github:`12553` - List of tests that keep failing sporadically
* :github:`12537` - potential over-use of k_spinlock
* :github:`12490` - Produced ELF does not follow the linux ELF spec
* :github:`12359` - Default address selection for IPv6 should follow RFC 6724
* :github:`12331` - Proposal to improve the settings subsystem
* :github:`12134` - I cannot see a Zephyr way to change the clock frequency at runtime
* :github:`12130` - Is zephyr targeting high-end phone or pc doing open ended computation on the roadmap?
* :github:`12027` - Make icount work for real on x86_64
* :github:`11751` - Rework exception & fatal error handling framework
* :github:`11519` - Add at least build test for cc1200
* :github:`11490` - setup_ipv6() treats event enums as bitmasks
* :github:`11296` - Possible ways to implement clock synchronisation over BLE
* :github:`11213` - NFFS: Handle unexpected Power Off
* :github:`11172` - ARM Cortex A Architecture support - ARMv8-A
* :github:`10996` - Add device tree support for usb controllers on x86
* :github:`10821` - ELCE: DT, Kconfig, EDTS path forward
* :github:`10534` - Can we get rid of zephyr-env.sh?
* :github:`10423` - log_core.h error on pointer-to-int-cast on 64bit system
* :github:`10339` - gpio: Cleanup flags
* :github:`10305` - RFC: Add pin mask for gpio_port_xxx
* :github:`9947` - CMake build architecture documentation
* :github:`9904` - System timer handling with low-frequency timers
* :github:`9873` - External flash driver for the MX25Rxx
* :github:`9748` - NFFS issue after many writes by btsettings
* :github:`9506` - Ztest becomes unresponsive while running SMP tests
* :github:`9349` - Support IPv6 privacy extension RFC 4941
* :github:`9333` - Support for STM32 L1-series
* :github:`9330` - network: clean up / implement supervisor to manage net services
* :github:`9194` - generated syscall header files don't have ifndef protection
* :github:`8833` - OpenThread: Minimal Thread Device (MTD) option is not building
* :github:`8539` - Categorize Kconfig options in documentation
* :github:`8262` - [Bluetooth] MPU FAULT on sdu_recv
* :github:`8242` - File system (littlefs & FAT) examples
* :github:`8236` - DTS Debugging is difficult
* :github:`7305` - CMake improvements to modularize gperf targets
* :github:`6866` - build: requirements: No module named yaml and elftools
* :github:`6562` - Question: Is QP™ Real-Time Frameworks/RTOS or libev supported in Zephyr? Or any plan?
* :github:`6521` - Scheduler needs spinlock-based synchronization
* :github:`6496` - Question: Is dynamical module loader supported in Zephyr? Or any plan?
* :github:`6389` - OpenThread: otPlatRandomGetTrue() implementation is not up to spec, may lead to security issues
* :github:`6327` - doc: GPIO_INT config option dependencies aren't clear
* :github:`6293` - Refining Zephyr's Device Driver Model
* :github:`6157` - SMP lacks low-power idle
* :github:`6084` - api: pinmux/gpio: It isn't possible to set pins as input and output simultaneously
* :github:`5943` - OT: utilsFlashWrite does not take into account the write-block-size
* :github:`5695` - C++ Support doesn't work
* :github:`5436` - Add LoRa Radio Support
* :github:`5027` - Enhance Testing and Test Coverage
* :github:`4973` - Provide Linux-style ERR_PTR/PTR_ERR/IS_ERR macros
* :github:`4951` - Prevent full rebuilds on Kconfig changes
* :github:`4917` - Reintroduce generic "outputexports" target after CMake migration
* :github:`4830` - device tree: generate pinmux
* :github:`3943` - x86: scope SMAP support in Zephyr
* :github:`3866` - To optimize the layout of the meta data of mem_slab & mem_pool
* :github:`3810` - application/kernel rodata split
* :github:`3717` - purge linker scripts of macro-based meta-language
* :github:`3701` - xtensa: scope MPU enabling
* :github:`3636` - Define region data structures exposed by linker script
* :github:`3490` - Move stm32 boards dts file to linux dts naming rules
* :github:`3488` - Dissociate board names from device tree file names
* :github:`3469` - Unify flash and code configuration across targets
* :github:`3429` - Add TSL2560 ambient light sensor driver
* :github:`3428` - Add HTU21D humidity sensor driver
* :github:`3427` - Add MPL3115A2 pressure sensor driver
* :github:`3397` - LLDP: Implement local MIB support for optional TLVs
* :github:`3276` - Dynamic Frequency Scaling
* :github:`3156` - xtensa: Support C++
* :github:`3098` - extend tests/kernel/arm_irq_vector_table to other platforms
* :github:`3044` - How to create a Zephyr ROM library
* :github:`2925` - cross-platform support for interrupt tables/code in RAM or ROM
* :github:`2814` - Add proper support for running Zephyr without a system clock
* :github:`2807` - remove sprintf() and it's brethen
* :github:`2664` - Running SanityCheck in Windows
* :github:`2338` - ICMPv6 "Packet Too Big" support
* :github:`2307` - DHCPv6
* :github:`1903` - Wi-Fi Host Stack
* :github:`1897` - Thread over BLE
* :github:`1583` - NFFS requires 1-byte unaligned accesses to flash
* :github:`1511` - qemu_nios2 should use the GHRD design
* :github:`1468` - Move NATS support from sample to a library + API
* :github:`1205` - C++ usage
