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

* ARM:

  * Added support for memory protection features (user mode and
    hardware-based stack overflow detection) in ARMv6-M architecture
  * Added QEMU support for ARMv6-M architecture
  * Extended test coverage for ARM-specific kernel features in ARMv6-M
    architecture
  * Enhanced runtime MPU programming in ARMv8-M architecture, making
    the full partitioning of kernel SRAM memory a user-configurable
    feature.
  * Added CMSIS support for Cortex-R architectures.
  * Updated CMSIS headers to version 5.6
  * Added missing Cortex-R CPU device tree bindings.
  * Fixed incorrect Cortex-R device tree specification.
  * Fixed several bugs in ARM architecture implementation

* POSIX:

  * Added support for CONFIG_DYNAMIC_INTERRUPTS (native_posix
    & nrf52_bsim)

Boards & SoC Support
********************

* Added support for these SoC series:

.. rst-class:: rst-columns

   * Atmel SAMD51, SAME51, SAME53, SAME54
   * Nordic Semiconductor nRF53
   * NXP Kinetis KV5x
   * STMicroelectronics STM32G4

* Added support for these ARM boards:

  .. rst-class:: rst-columns

     * actinius_icarus
     * cc3235sf_launchxl
     * decawave_dwm1001_dev
     * degu_evk
     * frdm_k22f
     * frdm_k82f
     * mec1501modular_assy6885
     * nrf52833_pca10100
     * nrf5340_dk_nrf5340
     * nucleo_g431rb
     * pico_pi_m4
     * qemu_cortex_r0
     * sensortile_box
     * steval_fcu001v1
     * stm32f030_demo
     * stm32l1_disco
     * twr_kv58f220m

* Added support for these following shields:

  .. rst-class:: rst-columns

     * adafruit_2_8_tft_touch_v2
     * dfrobot_can_bus_v2_0
     * link_board_eth
     * ssd1306_128x32
     * ssd1306_128x64
     * waveshare_epaper
     * x_nucleo_idb05a1

* Added CAN support for Olimexino STM32 board

Drivers and Sensors
*******************

* ADC

  * Added support for STM32G4X in STM32 driver
  * Added Microchip XEC ADC driver

* Bluetooth

  * Added RPMsg transport HCI driver

* CAN

  * Added API to read the bus-state and error counters
  * Added API for bus-off recovery
  * Optimizations for the MCP2515 driver
  * Bug fixes

* Clock Control

  * Added support for nRF52833 in nRF driver
  * Added support for STM32G4X in STM32 driver

* Console

  * Removed deprecated function console_register_line_input

* Counter

  * Added support for STM32L1 and STM32G4X in STM32 driver
  * Removed QMSI driver
  * Added Microchip XEC driver

* Display

  * Enhanced SSD1306 driver to support build time selection
  * Enhanced SSD16XX driver to use bytestring property for LUT and parameters

* DMA

  * Added generic STM32 driver
  * Removed QMSI driver

* EEPROM

  * Added EEPROM device driver API
  * Added Atmel AT24 (and compatible) I2C EEPROM driver
  * Added Atmel AT25 (and compatible) SPI EEPROM driver
  * Added native_posix EEPROM emulation driver

* Entropy

  * Added RV32M1 driver
  * Added support for STM32G4X in STM32 driver

* Ethernet

  * Added MAC address configuration and carrier state detection to STM32 driver
  * Added ENC424J600 driver
  * Removed DesignWare driver

* Flash

  * Added deep-power-down mode support in SPI NOR driver
  * Fixed STM32 driver for 2MB parts
  * Added support for STM32G4X in STM32 driver
  * Removed QMSI driver

* GPIO

  * Added support for STM32G4X in STM32 driver
  * Removed QMSI, SCH, and SAM3 drivers

* Hardware Info

  * Added LiteX DNA driver

* I2C

  * Converted remaining drivers to device tree
  * Added support for STM32G4X in STM32 driver
  * Fixed DesignWare driver for 64-bit
  * Removed QMSI driver
  * Added proper error handling in XEC driver

* I2S

  * Refactored STM32 driver

* IEEE 802.15.4

  * Added CC13xx / CC26xx driver

* Interrupt Controller

  * Added support for SAME54 to SAM0 EIC driver
  * Added support for STM32G4X in STM32 driver
  * Converted RISC-V plic to use multi-level irq support

* IPM

  * Added nRFx driver

* Keyboard Scan

  * Added Microchip XEC driver

* LED

  * Removed non-DTS support from LP5562, PCA9633, and LP3943 drivers

* Modem

  * Added simple power management to modem receiver

* Pinmux

  * Added support for STM32G4X in STM32 driver
  * Removed QMSI driver

* PS/2

  * Added Microchip XEC driver

* PWM

  * Added PWM shell
  * Added Microchip XEC driver
  * Removed QMSI driver

* Sensor

  * Fixed raw value scaling and SPI burst transfers in LIS2DH driver
  * Converted various drivers to device tree
  * Fixed fractional part calculation in ENS210 driver
  * Added OPT3001 light sensor driver
  * Added SI7060 temperature sensor driver
  * Added TMP116 driver
  * Implemented single shot mode in SHT3XD driver
  * Added single/double tap trigger support in LIS2DW12 driver

* Serial

  * Added support for SAME54 to SAM0 driver
  * Added support for STM32G4X in STM32 driver
  * Added support for 2 stop bits in nRF UARTE and UART drivers
  * Removed QMSI driver
  * Added ESP32 driver with FIFO/interrupt support

* SPI

  * Added support for nRF52833 in nRFx driver
  * Added support for STM32G4X in STM32 driver
  * Added RV32M1 driver
  * Added Microchip XEC driver
  * Added LiteX driver
  * Removed Intel Quark driver

* Timer

  * Fixed starving clock announcements in SYSTICK and nRF drivers
  * Fixed clamp tick adjustment in tickless mode in various drivers
  * Fixed calculation of absolute cycles in SYSTICK driver
  * Fixed lost ticks from unannounced elapsed in nRF driver
  * Fixed SMP bug in ARC driver
  * Added STM32 LPTIM driver
  * Changed CC13X2/CC26X2 to use RTC instead of SYSTICK for system clock

* USB

  * Added support for nRF52833 in nRFx driver
  * Added support for STM32G4X in STM32 driver
  * Enabled ZLP hardware handling for variable-length data storage

* Video

  * Added MCUX CSI and Aptina MT9M114 drivers
  * Added software video pattern generator driver

* Watchdog

  * Added support for SAME54 to SAM0 driver
  * Converted drivers to use device tree
  * Removed QMSI driver
  * Added STM32 WWDG driver
  * Added Microchip XEC driver

* WiFi

  * Implemented TCP/UDP socket offload with TLS in Inventek eS-WiFi driver

Networking
**********

* Added new TCP stack implementation. The new TCP stack is still experimental
  and is turned off by default. Users wanting to experiment with it can set
  :option:`CONFIG_NET_TCP2` Kconfig option.
* Added support for running MQTT protocol on top of a Websocket connection.
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

* A new Getting Started Guide simplifies and streamlines the "out of
  box" experience for developers, from setting up their development
  environment through running the blinky sample.
* Many additions and updates to architecture, build, and process docs including
  sanity check, board porting, Bluetooth, scheduling, timing,
  peripherals, configuration, and user mode.
* Documentation for new boards and samples.
* Improvements and clarity of API documentation.

Tests and Samples
*****************

* TBD

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.0.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`99999` - issue title
