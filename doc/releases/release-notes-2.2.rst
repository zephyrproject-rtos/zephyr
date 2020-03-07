:orphan:

.. _zephyr_2.2:

Zephyr 2.2.0 (Working Draft)
############################

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
  * Fix CVE-2020-10020
  * Fix CVE-2020-10021

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
* Fix an issue with the sytsem call stack frame if the system call is
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
  * Added a Kconfig option, BT_CTLR_PARAM_CHECK, to enable addtional parameter
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
