:orphan:

.. _zephyr_2.2:

Zephyr 2.2.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr RTOS version 2.2.0.

Major enhancements with this release include:

* We added initial support for 64-bit ARMv8-A architecture (Experimental).
* CANopen protocol support through 3rd party CANopenNode stack

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

No security vulnerabilities received.

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

* <TBD>

Architectures
*************

* ARC:

  * <TBD>

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

  * <TBD>

* RISC-V:

  * Added GPIO driver for LiteX VexRiscv
  * Fixed Ethernet networking for LiteX VexRiscv
  * Added Programmable Interrupt Controller support for SweRV
  * Fixed invalid channel bug for RV32M1 interrupt controller
  * Added PWM support for RV32M1
  * Optimized reads of MTIME/MTIMECMP on 64-bit RISC-V

* x86:

  * <TBD>

Boards & SoC Support
********************

* Added support for these SoC series:

.. rst-class:: rst-columns

   * <TBD>

* Added support for these ARM boards:

  .. rst-class:: rst-columns

     * <TBD>

* Removed support for these ARM boards:

  .. rst-class:: rst-columns

     * TI CC2650


* Added support for these following shields:

  .. rst-class:: rst-columns

     * <TBD>

Drivers and Sensors
*******************

* ADC

  * <TBD>

* Bluetooth

  * <TBD>

* CAN

  * Support for CAN_2 on STM32, but no simultaneous use of CAN_1 and CAN_2.
  * Support for STM32F3 and STM32F4 series

* Clock Control

  * <TBD>

* Console

  * <TBD>

* Counter

  * The counter_read() API function is deprecated in favor of
    counter_get_value(). The new API function adds a return value for
    indicating whether the counter was read successfully.

* Display

  * <TBD>

* DMA

  * <TBD>

* EEPROM

  * Added EEPROM driver for STM32L0 and STM32L1 SoC series
  * Added EEPROM simulator (replacing native_posix EEPROM driver)

* Entropy

  * <TBD>

* Ethernet

  * Support for SiLabs Giant Gecko GG11 Ethernet driver

* Flash

  * <TBD>

* GPIO

  * <TBD>

* Hardware Info

  * <TBD>

* I2C

  * <TBD>

* I2S

  * <TBD>

* IEEE 802.15.4

  * Add support for IEEE 802.15.4 rf2xxx driver

* Interrupt Controller

  * <TBD>

* IPM

  * <TBD>

* Keyboard Scan

  * <TBD>

* LED

  * <TBD>

* LoRa

  * Added APIs and drivers needed to support LoRa technology by reusing the
    LoRaMac-node library.

* Modem

  * <TBD>

* Pinmux

  * <TBD>

* PS/2

  * <TBD>

* PWM

  * <TBD>

* Sensor

  * <TBD>

* Serial

  * <TBD>

* SPI

  * <TBD>

* Timer

  * <TBD>

* USB

  * <TBD>

* Video

  * <TBD>

* Watchdog

  * <TBD>

* WiFi

  * <TBD>

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

  * <TBD>

* BLE split software Controller:

  * <TBD>

* BLE legacy software Controller:

  * <TBD>

Build and Infrastructure
************************

* The minimum Python version supported by Zephyr's build system and tools is
  now 3.6.
* Renamed :file:`generated_dts_board.h` and :file:`generated_dts_board.conf` to
  :file:`devicetree.h` and :file:`devicetree.conf`, along with various related
  identifiers. Including :file:`generated_dts_board.h` now generates a warning
  saying to include :file:`devicetree.h` instead.
* <Other items TBD>

Libraries / Subsystems
***********************

* LoRa

  * LoRa support was added through official LoRaMac-node reference
    implementation.

HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.

Documentation
*************

* <TBD>

Tests and Samples
*****************

* <TBD>

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.1.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title
