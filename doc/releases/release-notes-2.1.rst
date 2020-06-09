:orphan:

.. _zephyr_2.1:

Zephyr 2.1.0
############

We are pleased to announce the release of Zephyr kernel version 2.1.0.

Major enhancements with this release include:

* Normalized APIs across all architectures.
* Expanded support for ARMv6-M architecture.
* Added support for numerous new boards and shields.
* Added numerous new drivers and sensors.
* Added new TCP stack implementation (experimental).
* Added BLE support on Vega platform (experimental).
* Memory size improvements to Bluetooth host stack.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

No security vulnerabilities received.

Kernel
******

* Add arch abstraction for irq_offload()
* Add architecture interface headers and normalized APIs across all arches
* Show faulting CPU on fatal error message
* Improve C++ compatibility
* Modified arch API namespace to allow automatic arch API documentation
  generation
* Use logging for userspace errors

Architectures
*************

* ARC:

  * Increased the exception handling stack size
  * Implement DIRECT IRQ support
  * Implement z_arch_system_halt()

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

* RISC-V:

  * Add support to boot multicore system

* x86:

  * Add basic ACPI and non-trivial memory map support
  * Add SMP support (64-bit mode only)
  * Inline direct ISR functions

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

* Host:

  * Reworked the Host transmission path to improve memory footprint and remove potential deadlocks
  * Document HCI errors for connected callback
  * GATT: Added a ``bt_gatt_is_subscribed()`` function to check if attribute has been subscribed
  * GATT: Added an initializer for GATT CCC structures
  * HCI: Added a function to get the connection handle of a connection
  * Added ability to load CCC settings on demand to reduce memory usage
  * Made the time to run slave connection parameters update procedure configurable
  * Folded consecutive calls to bt_rand into one to reduce overhead
  * Added key displacement feature for key storage
  * Reduced severity of unavoidable warnings
  * Added support C++20 designated initializers
  * Mesh: Add the model extension concept as described in the Mesh Profile Specification
  * Mesh: Added support for acting as a Provisioner

* BLE split software Controller:

  * Numerous bug fixes
  * Fixed several control procedure (LLCP) handling issues
  * Added experimental BLE support on Vega platform.
  * Added a hook for flushing in LLL
  * Implemented the LLL reset functions in a call from ll_reset
  * Made the number of TX ctrl buffers configurable
  * Added support for Zero Latency IRQs

* BLE legacy software Controller:

  * Multiple bug fixes

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

Libraries / Subsystems
***********************

* Random

  * Add cryptographically secure random functions
  * Add bulk fill random functions

HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.

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

* We have implemented additional tests and significantly expanded the amount
  of test cases in existing tests to increase code coverage.

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.0.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`21177` - Long ATT MTU reports wrong length field in write callback.
* :github:`21148` - nrf51: uart\_1 does not compile
* :github:`21131` - Bluetooth: host: Subscriptions not removed upon unpair
* :github:`21139` - west: runners: blackmagicprobe: Keyboard Interrupt shouldn't kill the process
* :github:`21126` - drivers: spi\_nrfx\_spim: Incorrect handling of extended SPIM configuration
* :github:`21115` - Request a new repository for the Xtensa HAL
* :github:`21113` - k\_sem\_give reschedules cooperative threads unexpectedly
* :github:`21102` - Slack link at https://www.zephyrproject.org/ is expired
* :github:`21077` - Help: Pull request "Identity/Emails issues"
* :github:`21059` - Bluetooth sent callback delayed more than ATT
* :github:`21049` - Bluetooth: Multiple issues with net\_buf usage
* :github:`21048` - timer case fail on qemu\_xtensa and mps2\_an385
* :github:`21004` - cmd\_data buffer corruption
* :github:`20970` - Bluetooth: Mesh: seg\_tx\_reset in the transport layer
* :github:`20969` - No SOURCES given to target: drivers\_\_gpio
* :github:`20968` - [Coverity CID :206016] Side effect in assertion in tests/kernel/sched/metairq/src/main.c
* :github:`20967` - [Coverity CID :206017] Out-of-bounds read in drivers/ipm/ipm\_nrfx\_ipc.c
* :github:`20966` - [Coverity CID :206018] Side effect in assertion in tests/kernel/sched/metairq/src/main.c
* :github:`20965` - [Coverity CID :206019] Side effect in assertion in tests/kernel/sched/metairq/src/main.c
* :github:`20964` - [Coverity CID :206020] Bad bit shift operation in drivers/ipm/ipm\_nrfx\_ipc.c
* :github:`20963` - [Coverity CID :206021] Side effect in assertion in tests/kernel/sched/metairq/src/main.c
* :github:`20962` - [Coverity CID :206022] Out-of-bounds read in drivers/ipm/ipm\_nrfx\_ipc.c
* :github:`20939` - long duration timeouts can cause loss of time
* :github:`20938` - ATT/L2CAP "deadlock"
* :github:`20936` - tests/kernel/mem\_protect/protection fails on ARMv8-M
* :github:`20933` - x\_nucleo\_iks01a3 shield: STM LSM6DSO sensor does not work after h/w or s/w reset
* :github:`20931` - intel\_s1000\_crb samples can't be built with latest master
* :github:`20926` - ztest\_1cpu\_user\_unit\_test() doesn
* :github:`20892` - our nRF52840 board power management sleep duration
* :github:`20883` - [Coverity CID :205808] Integer handling issues in tests/net/lib/coap/src/main.c
* :github:`20882` - [Coverity CID :205806] Integer handling issues in tests/net/lib/coap/src/main.c
* :github:`20881` - [Coverity CID :205786] Integer handling issues in tests/net/lib/coap/src/main.c
* :github:`20880` - [Coverity CID :205780] Integer handling issues in tests/net/lib/coap/src/main.c
* :github:`20879` - [Coverity CID :205812] Incorrect expression in tests/kernel/spinlock/src/main.c
* :github:`20878` - [Coverity CID :205801] Incorrect expression in tests/kernel/mp/src/main.c
* :github:`20872` - [Coverity CID :205779] Parse warnings in subsys/usb/class/hid/core.c
* :github:`20871` - [Coverity CID :205815] Memory - illegal accesses in subsys/shell/shell.c
* :github:`20868` - [Coverity CID :205814] Null pointer dereferences in subsys/net/ip/6lo.c
* :github:`20867` - [Coverity CID :205803] Integer handling issues in subsys/fs/nvs/nvs.c
* :github:`20866` - [Coverity CID :205795] Integer handling issues in subsys/fs/nvs/nvs.c
* :github:`20846` - [Coverity CID :205775] Memory - corruptions in samples/net/sockets/big\_http\_download/src/big\_http\_download.c
* :github:`20845` - [Coverity CID :205824] Memory - corruptions in samples/net/mqtt\_publisher/src/main.c
* :github:`20842` - [Coverity CID :205787] Memory - corruptions in drivers/usb/device/usb\_dc\_native\_posix\_adapt.c
* :github:`20841` - [Coverity CID :205839] Error handling issues in drivers/usb/device/usb\_dc\_native\_posix.c
* :github:`20840` - [Coverity CID :205821] Error handling issues in drivers/usb/device/usb\_dc\_native\_posix.c
* :github:`20839` - [Coverity CID :205813] Error handling issues in drivers/usb/device/usb\_dc\_native\_posix.c
* :github:`20838` - [Coverity CID :205790] Null pointer dereferences in drivers/usb/device/usb\_dc\_native\_posix.c
* :github:`20837` - [Coverity CID :205777] Error handling issues in drivers/usb/device/usb\_dc\_native\_posix.c
* :github:`20836` - [Coverity CID :205776] Error handling issues in drivers/usb/device/usb\_dc\_native\_posix.c
* :github:`20834` - [Coverity CID :205825] API usage errors in boards/posix/native\_posix/hw\_models\_top.c
* :github:`20833` - Bluetooth: Deadlock in Host API from SMP callbacks.
* :github:`20826` - [Coverity CID :205798] API usage errors in boards/posix/native\_posix/hw\_models\_top.c
* :github:`20811` - spi driver
* :github:`20804` - sanitycheck: unimplemented documented option
* :github:`20800` - Ready thread is not swapped in after being woken up in IRQ
* :github:`20797` - echo server qemu\_x86 e1000 crash when coverage is enabled
* :github:`20781` - peripheral\_hr on VEGABoard disconnects from central\_hr after BT\_CONN\_PARAM\_UPDATE\_TIMEOUT
* :github:`20771` - onoff\_level\_lighting\_vnd\_app mcumgr unable to connect to provisioned node
* :github:`20769` - nucleo\_g431rb: Settings subsystem fails to initialise
* :github:`20743` - doc: settings.rst has references to mynewt structures
* :github:`20741` - Reel board Ethernet Support using the Link board ETH
* :github:`20735` - Cannot flash with jlink  on windows.
* :github:`20726` - arm: Specifying sp register in asm's clobber list is deprecated in GCC 9
* :github:`20715` - rtc driver may interrupt in a short time for large timeouts on cc13x2/cc26x2
* :github:`20707` - Define GATT service at run-time
* :github:`20695` - nRF5340: misc fixes for nRF53 porting
* :github:`20692` - samples: CAN: kconfig: CONFIG\_CAN\_AUTO\_BOFF\_RECOVERY does not exist
* :github:`20681` - samples: sensor: lps22hb: Reference to undefined CONFIG\_LPS22HB\_TRIGGER symbol
* :github:`20666` - Unexpected UART Kconfig warnings during build
* :github:`20660` - Bluetooth: host: bt\_conn\_create\_le sometimes fails to stop pre-scan before connecting
* :github:`20658` - The misc-flasher runner is not usable
* :github:`20651` - Bluetooth: disable and restart BT functionality
* :github:`20639` - x\_nucleo\_iks01a3 sample is not working anymore after #20560 has been merged
* :github:`20621` - Invalid baudrate on stm32 usart
* :github:`20620` - Advertiser seen alternating between RPA an ID address with privacy enabled
* :github:`20613` - HCI reset command complete before LL reset done
* :github:`20603` - tests/kernel/critical failed on sam\_e70\_xplained board in v.1.14-branch
* :github:`20598` - tests/lib/mem\_alloc newlibnano target run time error
* :github:`20587` - undefined reference when enabling CONFIG\_STACK\_CANARIES
* :github:`20582` - samples/subsys/logging/syst is broken when building with gcc-arm-none-eabi-7-2018-q2-update
* :github:`20571` - devicetree: fix non-deterministic multi-level interrupt encodings
* :github:`20558` - Build failure for samples/bluetooth/peripheral\_hr/sample.bluetooth.peripheral\_hr\_rv32m1\_vega\_ri5cy on rv32m1\_vega\_ri5cy
* :github:`20545` - imgtool: signing image fails: missing DT\_FLASH\_WRITE\_BLOCK\_SIZE
* :github:`20540` - [Coverity CID :205656]Error handling issues in /tests/net/tcp/src/main.c
* :github:`20539` - [Coverity CID :205637]Resource leaks in /tests/net/socket/tcp/src/main.c
* :github:`20538` - [Coverity CID :205673]Memory - corruptions in /tests/net/ppp/driver/src/main.c
* :github:`20536` - [Coverity CID :205607]Memory - corruptions in /tests/net/ppp/driver/src/main.c
* :github:`20535` - [Coverity CID :205619]Null pointer dereferences in /tests/net/ieee802154/fragment/src/main.c
* :github:`20534` - [Coverity CID :205669]Incorrect expression in /tests/kernel/mem\_protect/stack\_random/src/main.c
* :github:`20533` - [Coverity CID :205667]Error handling issues in /tests/drivers/counter/counter\_basic\_api/src/test\_counter.c
* :github:`20530` - [Coverity CID :205663]Memory - corruptions in /tests/crypto/tinycrypt/src/sha256.c
* :github:`20515` - [Coverity CID :205670]Code maintainability issues in /subsys/settings/src/settings\_nvs.c
* :github:`20514` - [Coverity CID :205633]Memory - illegal accesses in /subsys/settings/src/settings.c
* :github:`20513` - [Coverity CID :205621]Memory - illegal accesses in /subsys/net/lib/websocket/websocket.c
* :github:`20512` - [Coverity CID :143683]Error handling issues in /subsys/fs/fcb/fcb.c
* :github:`20511` - [Coverity CID :205612]Control flow issues in /subsys/disk/disk\_access\_spi\_sdhc.c
* :github:`20510` - [Coverity CID :205660]Incorrect expression in /subsys/debug/tracing/ctf/ctf\_top.c
* :github:`20509` - [Coverity CID :205632]Incorrect expression in /subsys/debug/tracing/ctf/ctf\_top.c
* :github:`20508` - [Coverity CID :205634]Code maintainability issues in /samples/net/sockets/websocket\_client/src/main.c
* :github:`20507` - [Coverity CID :205662]Memory - illegal accesses in /samples/net/sockets/dumb\_http\_server\_mt/src/main.c
* :github:`20506` - [Coverity CID :205672]Null pointer dereferences in /samples/drivers/espi/src/main.c
* :github:`20505` - [Coverity CID :205613]Null pointer dereferences in /samples/drivers/espi/src/main.c
* :github:`20504` - [Coverity CID :205661]Incorrect expression in /drivers/watchdog/wdt\_wwdg\_stm32.c
* :github:`20503` - [Coverity CID :205655]Error handling issues in /drivers/watchdog/wdt\_wwdg\_stm32.c
* :github:`20502` - [Coverity CID :205665]Integer handling issues in /drivers/video/mt9m114.c
* :github:`20501` - [Coverity CID :205643]Integer handling issues in /drivers/video/mt9m114.c
* :github:`20499` - [Coverity CID :205625]Error handling issues in /drivers/sensor/lsm6dso/lsm6dso\_shub.c
* :github:`20498` - [Coverity CID :205628]Error handling issues in /drivers/sensor/amg88xx/amg88xx\_trigger.c
* :github:`20496` - [Coverity CID :205630]Memory - illegal accesses in /drivers/pwm/pwm\_mchp\_xec.c
* :github:`20495` - [Coverity CID :205622]Memory - illegal accesses in /drivers/pwm/pwm\_mchp\_xec.c
* :github:`20494` - [Coverity CID :205617]Memory - corruptions in /drivers/kscan/kscan\_mchp\_xec.c
* :github:`20493` - [Coverity CID :205668]Insecure data handling in /drivers/ethernet/eth\_enc424j600.c
* :github:`20489` - [Coverity CID :205645]Integer handling issues in /drivers/counter/counter\_mchp\_xec.c
* :github:`20488` - [Coverity CID :205614]Integer handling issues in /drivers/clock\_control/nrf\_clock\_calibration.c
* :github:`20487` - [Coverity CID :205648]Memory - corruptions in /arch/arc/core/mpu/arc\_mpu\_v3\_internal.h
* :github:`20480` - i2c driver for cc13xx/cc26xx is configured with incorrect frequency
* :github:`20472` - drivers/flash: nRF flash driver uses absolute addressing instead of relative
* :github:`20450` - Bluetooth: hci\_uart: conn param update request from peripheral ignored
* :github:`20449` - 'west flash' command failed on sam\_e70\_xplained board.
* :github:`20445` - tests/kernel/critical failed on mimxrt1050\_evk board.
* :github:`20444` - sanitycheck error with tests/arch/x86/info.
* :github:`20438` - Kernel timeout API does not document well accepted values
* :github:`20431` - sockets\_tls: missing sendmsg
* :github:`20425` - storage/flash\_map: flash\_area\_get\_sectors can't fetch sectors on devices with non-zero flash base address
* :github:`20423` - drivers/flash: flash\_get\_page\_info\_by\_off uses relative addresses
* :github:`20422` - Device with bonds should not accept new keys without user awareness
* :github:`20417` - BME280 wrong pressure unit?
* :github:`20416` - sample: sensor: fxos8700 issues
* :github:`20406` - misc.app\_dev.libcxx test fails to build for qemu\_x86\_64
* :github:`20371` - Sanitycheck filtering broken
* :github:`20351` - sample vl53l0x fails on disco\_l475\_iot1
* :github:`20332` - Nordic: DocLib links are obsolete
* :github:`20325` - samples/drivers/i2c\_scanner does not work on STM32 NUCLEO and DISCOVERY boards
* :github:`20313` - Zperf documentation points to wrong iPerf varsion
* :github:`20310` - SDHC : Could not enable SPI clock on nucleo\_f091rc
* :github:`20299` - bluetooth: host: Connection not being unreferenced when using CCC match callback
* :github:`20297` - Bluetooth: can't close bt\_driver log output
* :github:`20285` - ST lis2dh sample with motion callback
* :github:`20284` - zephyr-env.sh  Is this supposed to be unsetopt posixargzero ?
* :github:`20274` - Kconfig new libc changes cause echo server cmake error
* :github:`20260` - logging system call
* :github:`20255` - Meta-IRQs making cooperative threads preemptive
* :github:`20250` - hci\_usb: scanning crashes controller if a lot of devices are nearby
* :github:`20246` - Module Request: hal\_unisoc
* :github:`20245` - HTTP parser error with chunked transfer encoding
* :github:`20244` - mesh: demo: BT fails it init
* :github:`20232` - Bluetooth: Kernel panic on gatt discover in shell app
* :github:`20225` - [TOPIC-GPIO] sam\_e70\_xplained fails 2-pin active-low pull test
* :github:`20224` - [TOPIC-GPIO] rv32m1\_vega\_ri5cy fails 2-pin double-edge detection test
* :github:`20223` - [TOPIC-GPIO] efr32mg\_sltb004a fails 2-pin double-edge detection test
* :github:`20205` - ztest testing.ztest does not have a prj.conf with CONFIG\_ZTEST=y
* :github:`20202` - tests/arch/arm/arm\_interrupt failed on sam\_e70\_xplained board.
* :github:`20177` - sanitycheck error with tests/benchmarks/timing\_info.
* :github:`20176` - tests/drivers/pwm/pwm\_api failed on reel\_board.
* :github:`20167` - posix clock: unexpected value for CLOCK\_REALTIME when used with newlib
* :github:`20163` - doc: storage settings not clear
* :github:`20135` - Bluetooth: controller: split: Missing initialization of master terminate\_ack flag
* :github:`20122` - Deadlock in ASAN leak detection on exit
* :github:`20110` - Crash in hci\_driver.c when create\_connection\_cancel is issued after create connection
* :github:`20109` - altera\_nios2 support decision required
* :github:`20105` - tests/subsys/fs/fcb/ Using uninitialised memory/variables
* :github:`20104` - Kconfig is too slow
* :github:`20100` - Slave PTP clock time is updated with large value when Master PTP Clock time has changed
* :github:`20088` - tests/net/icmpv6/ failed on mimxrt1050\_evk board.
* :github:`20086` - Broken-looking duplicated ESPI\_XEC symbol
* :github:`20072` - Incompatible pointer types in Nordic Driver nrfx\_usbd.h
* :github:`20071` - Incompatible pointer types in Nordic Driver
* :github:`20049` - Build warnings in several unit tests
* :github:`20045` - z\_sched\_abort: sched\_spinlock should be released before k\_busy\_wait
* :github:`20042` - Telnet can connect only once
* :github:`20033` - Thread suspend only works if followed by k\_sleep in thread that is performing the suspension
* :github:`20032` - Make it clear in HTML docs what monospaced text is a link
* :github:`20030` - stm32 can: zcan\_frame from fifo uninitialized
* :github:`20022` - sanitycheck is not failing on build warnings
* :github:`20021` - Add a module to Zephyr to include TF-M project and it's related repos
* :github:`20016` - STM32F4: cannot erase sectors from bank2
* :github:`20010` - Cannot flash mimxrt1050\_evk board
* :github:`20007` - tests/net/mld failed on mimxrt1050\_evk board.
* :github:`20000` - Invalid callback parameters in drivers/serial/uart\_nrfx\_uarte.c (using async API)
* :github:`19969` - [TOPIC-GPIO] mcux driver problems with pull configuration
* :github:`19963` - settings test tests/subsys/settings/fcb/raw failing
* :github:`19918` - Incremental builds broken for OpenAMP sample
* :github:`19917` - Bluetooth: Controller: Missing LL\_ENC\_RSP after HCI LTK Negative Reply
* :github:`19915` - tests/net/icmpv6 failed on sam\_e70 board.
* :github:`19914` - tests/net/shell failed on sam\_e70 board.
* :github:`19910` - Bluetooth: Mesh: Thread stack can reduce by use malloc&free function
* :github:`19898` - CONFIG\_NET\_ROUTE\_MCAST and CONFIG\_NET\_ROUTING can't be enabled
* :github:`19889` - Buffer leak in GATT for Write Without Response and Notifications
* :github:`19885` - SMP doesn't work on ARC any longer
* :github:`19877` - Broken partition size
* :github:`19872` - sensor/lis2dh: using runtime scale other than 2g generates strange values
* :github:`19871` - display/ssd1306: allow "reverse display" in kconfig or dts
* :github:`19867` - modem: ublox-sara-r4/u2 build error
* :github:`19848` - stm32wb MPU failure
* :github:`19841` - MIPI Sys-T logging/tracing support
* :github:`19837` - SS register is 0 when taking exceptions on qemu\_x86\_long
* :github:`19833` - missing or empty reg/ranges property when trying to build blink\_led example
* :github:`19820` - Bluetooth: Host: Unable to use whitelist in peripheral only build
* :github:`19818` - Compiler error for counter example (nRF52\_pca10040)
* :github:`19811` - native\_posix stack smashing
* :github:`19802` - Zephyr was unable to find the toolchain after update to zephyr version 1.13.0
* :github:`19795` - bt\_gatt\_attr\_next returns first attribute in table for attributes with static storage.
* :github:`19791` - How to use CMSIS DSP Library on nRF52832 running zephyr LTS Version(V1.14) ?
* :github:`19783` - floating point in C++ on x86\_64 uses SSE
* :github:`19775` - net\_calc\_chksum: Use of un-initialized memory on 64 bit targets
* :github:`19769` - CONFIG\_FLASH\_SIZE should be CONFIG\_FLASH\_END and specified in hex
* :github:`19767` - Bluetooth: Mesh: Provision Random buffer has too small size
* :github:`19762` - tests/net/lib/tls\_credentials failed on sam\_e70\_xplained board.
* :github:`19759` - z\_arch\_switch() passed pointer to NULL outgoing switch handle on dummy thread context switch
* :github:`19748` - k\_sleep(K\_FOREVER) behavior unexpected
* :github:`19734` - "make gdbserver" doesn't work properly for qemu\_x86\_long
* :github:`19724` - Bluetooth: Mesh: Receiving an access message
* :github:`19722` - Settings: settings\_file\_save\_priv() use of uninitialized variable
* :github:`19721` - samples/bluetooth/ipsp does not respond to pings from Linux
* :github:`19717` - Add provisions for supporting multiple CMSIS variants
* :github:`19701` - mem\_pool\_threadsafe sporadic failures impacting CI
* :github:`19700` - nrfx\_uart RX hang on errors
* :github:`19697` - tests/subsys/fs/fat\_fs\_api uses unitialized variables
* :github:`19692` - [TOPIC-GPIO] gpi\_api\_1pin test failures
* :github:`19685` - Samples: BluetoothMesh: not able to connect with device over GATT to provision it
* :github:`19683` - nrf: clock reimplementation breaks test
* :github:`19678` - Noticeable delay between processing multiple client connection requests (200ms+)
* :github:`19660` - missing file reference in samples/sensor/ti\_hdc doc
* :github:`19649` - [TOPIC-GPIO]: Replace GPIO\_INT\_DEBOUNCE with GPIO\_DEBOUNCE
* :github:`19638` - Bluetooth: Mesh: Provisioning Over PB-ADV
* :github:`19629` - tinycbor buffer overflow causing mcumgr image upload failure
* :github:`19612` - ICMPv6 packet is routed to wrong interface when peer is not found in neighbor cache
* :github:`19604` - Bluetooth: ATT does not release all buffers on disconnect
* :github:`19603` - addition to winbond,w25q16.yaml required for SPI CS to be controlled by driver
* :github:`19599` - ARC builds missing z\_arch\_start\_cpu() when !SMP
* :github:`19592` - Request new repository to host the Eclipse plugin for building Zephyr applications
* :github:`19569` - nRF RTC Counter with compile time decision about support of custom top value
* :github:`19560` - Console on CDC USB  crashes when CONFIG\_USB\_COMPOSITE\_DEVICE=y
* :github:`19552` - [TOPIC-GPIO]: Support for legacy interrupt configuration breaks new API contract
* :github:`19550` - drivers/pcie: \`pcie\_get\_mbar()\` should return a \`void \*\` not \`u32\_t\`
* :github:`19549` - kernel/mem\_protection/stackprot fails on NXP RT series platforms on v1.14.1-rc3 release
* :github:`19544` - make usb power settings in "Configuration Descriptor" setable
* :github:`19543` - net: tcp: echo server stops if CONFIG\_POSIX\_MAX\_FDS is not set
* :github:`19539` - Support MQTT over Websocket
* :github:`19537` - debug:object\_tracing: The trace list is not complete once we initialize the object on the trace list
* :github:`19536` - devicetree bindings path misinterpreted
* :github:`19535` - Doubly freed memory in the pipe\_api test
* :github:`19525` -  Can't change the slave latency on a connection.
* :github:`19515` - Bluetooth: Controller: assertion failed
* :github:`19509` - Bluetooth: stm32wb55: Unable to pair with privacy-enabled peer
* :github:`19490` - Bluetooth: split: 'e' assert during disconnect
* :github:`19484` - Bluetooth: split:  bt\_set\_name() asserts due to flash and radio coex
* :github:`19472` - drivers: usb\_dc\_stm32:  shows after some time errors and warnings
* :github:`19459` - Bluetooth: Mesh:  Mesh Model State Binding.
* :github:`19456` - arch/x86: make use of z\_bss\_zero() and z\_data\_copy()
* :github:`19452` - Bluetooth: Mesh: Mesh model implementation?
* :github:`19447` - SEGGER\_RTT.h: No such file or directory
* :github:`19438` - boot flags incorrect after image swapping
* :github:`19437` - tests/kernel/sched/schedule\_api tests fail to build
* :github:`19432` - nrfx: nrf52840\_pca10056 SPIM1 cannot be selected without SPIM3
* :github:`19420` - power: system power management sleep duration
* :github:`19419` - Build automation and testing tools
* :github:`19415` - typo in nucleo\_l496zg.dts
* :github:`19413` - Not able to scan and connect to other ble devices with HCI commands
* :github:`19398` - net: ENC28J60 driver does not respond to ping
* :github:`19385` - compilation error
* :github:`19381` - \`k\_yield()\` exhibits different behavior with \`CONFIG\_SMP\`
* :github:`19376` - Build on a ARM host
* :github:`19374` - net: echo server: TCP add support for multiple connections
* :github:`19370` - bugs in kernel/atomic\_c
* :github:`19367` - net: TCP/IPv4: TCP stops working after dropping segment with incorrect checksum
* :github:`19363` - arc: bug in \_firq\_enter
* :github:`19353` - arch/x86: QEMU doesn't appear to support x2APIC
* :github:`19347` - Bluetooth: BL654 USB dongle not found after flashing
* :github:`19342` - Bluetooth: Mesh: Persistent storage of Virtual Addresses
* :github:`19320` - build error using logger in test case
* :github:`19319` - tests/kernel/spinlock only runs on ESP32
* :github:`19317` - need a minimal log implementation that maps to printk()
* :github:`19307` - \_interrupt\_stack is defined in the kernel, but declared in arch headers
* :github:`19299` - kernel/spinlock: A SMP race condition in SPIN\_VALIDATE
* :github:`19284` - Service Changed indication not being sent in some cases
* :github:`19270` - GPIO: STM32: Migration to new API
* :github:`19267` - Service changed not notified upon reconnection.
* :github:`19265` - Bluetooth: Mesh: Friend Send model message to LPN
* :github:`19263` - Bluetooth: Mesh: Friend Clear Procedure Timeout
* :github:`19250` - NVS: Overwriting an item with a shorter matching item fails
* :github:`19239` - tests/kernel/common failed on iotdk board.
* :github:`19238` - tests/subsys/usb/device failed on reel\_board.
* :github:`19235` - move drivers/timer/apic\_timer.c to devicetree
* :github:`19231` - native\_posix\_64/tests/subsys/fs/fat\_fs\_api/filesystem.fat fails
* :github:`19227` - IOTDK uses QMSI DT binding
* :github:`19226` - Device Tree Enhancements in 2.1
* :github:`19219` - drivers/i2c/i2c\_dw.c is not 64-bit clean
* :github:`19216` - Ext library for WIN1500: different values of AF\_INET
* :github:`19198` - Bluetooth: LL split assert on connect
* :github:`19191` - problem with implementation of sock\_set\_flag
* :github:`19186` - BLE: Mesh: IVI Initiator When ivi in progress timeout
* :github:`19181` - sock\_set\_flag implementation in sock\_internal.h does not work for 64 bit pointers
* :github:`19178` - Segmentation fault when running echo server
* :github:`19177` - re-valuate commit 0951ce2
* :github:`19176` - NET: LLMNR: zephyr drops IPV4 LLMNR packets
* :github:`19167` - Message queues bug when using C++
* :github:`19165` - zephyr\_file generates bad links on branches
* :github:`19164` - compiling native\_posix64 with unistd.h & net/net\_ip.h fail
* :github:`19144` - arch/x86: CONFIG\_BOOT\_TIME\_MEASUREMENT broken
* :github:`19135` - net: ipv4: udp: echo server sends malformed data bytes in reply to broadcast packet
* :github:`19133` - Scheduler change in #17369 introduces crashes
* :github:`19103` - zsock\_accept\_ctx blocks even when O\_NONBLOCK is specified
* :github:`19098` - Failed to flash on ESP32
* :github:`19096` - No error thrown for device tree node with missing required property of type compound
* :github:`19079` - Enable shield sample on stm32mp157c\_dk2
* :github:`19078` - search for board specific shield overlays doesn't always work
* :github:`19066` - Build error with qemu\_x86\_64
* :github:`19065` - Build error with stm32h747i\_disco\_m4
* :github:`19064` - Correct docs for K\_THREAD\_DEFINE
* :github:`19059` - i2c\_ll\_stm32\_v2: nack on write is not handled correctly
* :github:`19051` - [Zephyr v2.0.0 nrf52840] Unable to reconnect to recently bonded peripheral
* :github:`19039` - Bluetooth: Qualification test case GATT/SR/UNS/BI-02-C fails
* :github:`19038` - [zephyr branch 1.14 and master -stm32-netusb]:errors when i view RNDIS Device‘s properties on Windows 10
* :github:`19034` - sanitycheck fail with ninja option with single-core machine
* :github:`19031` - nrfx\_clock.c functions are not available with CONFIG\_NRFX\_CLOCK
* :github:`19015` - Bluetooth: Mesh: Node doesn't respond to "All Proxies" address
* :github:`19013` - [Zephyr 1.14]: NetUsb and Ethernet work together
* :github:`19004` - problems in sanitycheck/CI infrastructure revealed by post-release change
* :github:`18999` - assignment in assert in test of arm\_thread\_arch causes build failures
* :github:`18990` - C++ New allocates memory from kernel heap
* :github:`18988` - BLE Central auto enables indications and notifies
* :github:`18986` - DTS: transition from alias to node label as the standard prefix
* :github:`18973` - z\_arch\_system\_halt() does not block interrupts
* :github:`18961` - [Coverity CID :203912]Error handling issues in /samples/net/sockets/coap\_client/src/coap-client.c
* :github:`18957` - NET\_L2: modem drivers (offloaded) aren't assigned a net\_l2 which causes a crash in net\_if\_up()/net\_if\_down()
* :github:`18956` - memory protection for x86 dependent on XIP
* :github:`18935` - [Zephyr 1.14] drivers: flash: spi\_nor: Problematic write with page boundaries
* :github:`18880` - boards: mec15xxevb\_assy6853: consider moving ARCH\_HAS\_CUSTOM\_BUSY\_WAIT to SoC definition
* :github:`18873` - zsock\_socket() should support proto==0
* :github:`18870` - zsock\_getaddrinfo() returns garbage values if IPv4 address is passed and hints->ai\_family == AF\_INET6
* :github:`18858` - Runner support for stm32flash utility
* :github:`18832` - Doc: contact-us page should use slack invite (not zephyrproject.slack.com)
* :github:`18824` - tests/subsys/usb/device/ failed on sam\_e70 board.
* :github:`18816` - ssd1306 driver can't work with lvgl
* :github:`18807` - Support the Ubuntu Cross Toolchain
* :github:`18803` - LTS - support time
* :github:`18787` - arch/x86: retire loapic\_timer.c driver in favor of new apic\_timer.c
* :github:`18749` - Avenger96 regressed in mainline for U-Boot M4 boot
* :github:`18695` - Watchdog: stm32: Wrong timeout value when watchdog started at boot
* :github:`18657` - drivers/timer/hpet.c should use devicetree, not CONFIG\_\* for MMIO/IRQ data
* :github:`18652` - Optimization flags from CMAKE\_BUILD\_TYPE are not taken into account
* :github:`18592` - (nRF51) The RSSI signal does not rise above -44 dBm
* :github:`18591` - tests/kernel/fifo/fifo\_timeout/kernel.fifo.timeout.poll fails to run on multiple ARM platforms
* :github:`18585` - STM32G4 support
* :github:`18583` - hci\_usb: NRF52840 connecting addtional peripheral fails
* :github:`18540` - MEC1501 ADC is missing in HAL
* :github:`18539` - MEC1501 PWM is missing in HAL
* :github:`18488` - Bluetooth: Mesh: Friend queue message seqnum order
* :github:`18480` - Microchip's MEC1501 HAL is broken (watchdog part)
* :github:`18465` - timeutil\_timegm() has undefined behavior
* :github:`18451` - [Coverity CID :203528]Integer handling issues in /tests/lib/fdtable/src/main.c
* :github:`18449` - [Coverity CID :203458]Integer handling issues in /tests/lib/fdtable/src/main.c
* :github:`18450` - [Coverity CID :203505]Integer handling issues in /tests/lib/fdtable/src/main.c
* :github:`18448` - [Coverity CID :203429]Integer handling issues in /tests/lib/fdtable/src/main.c
* :github:`18440` - [Coverity CID :203439]Memory - corruptions in /tests/kernel/mem\_protect/protection/src/main.c
* :github:`18441` - [Coverity CID :203460]Memory - corruptions in /tests/kernel/mem\_protect/protection/src/main.c
* :github:`18373` - [Coverity CID :203399]API usage errors in /samples/boards/olimex\_stm32\_e407/ccm/src/main.c
* :github:`18341` - settings: test setting FS back-end using littlefs
* :github:`18340` - settings: make NVS the default backend
* :github:`18308` - net: TCP/IPv6 set of fragmented packets causes Zephyr to quit
* :github:`18305` - Native Posix target can not use features with newlib dependencies
* :github:`18297` - Bluetooth: SMP: Pairing issues
* :github:`18282` - tests/kernel/sched/schedule\_api/ fails on LPC54114\_m4
* :github:`18160` - Cleanup dts compatible for "nxp,kinetis-sim" on nxp\_ke1xf
* :github:`18143` - stm32f SPI Slave TX does not work correctly, but occurs OVERRUN err
* :github:`18138` - xtensa arch has two different implementations
* :github:`18105` - BSD socket offload with IPv4 and IPv6 disabled breaks many client-based net samples
* :github:`18031` - samples/shields/x\_nucleo\_iks01a3 test is stucking due to dca45cb commit
* :github:`17998` - STM32 (Nucleo L476RG) SPI pins floating
* :github:`17983` - Bluetooth: Re-establish security before notifications/indications can be sent
* :github:`17949` - stm32 i2c driver has problems with AHB\_PRESCALER, APB1\_PRESCALER, APB2\_PRESCALER
* :github:`17892` - arch/x86: clean up segmentation.h
* :github:`17888` - arch/x86: remove IAMCU ABI support
* :github:`17832` - x86: update mmustructs.h and x86\_mmu.c to support long mode
* :github:`17829` - support default property values in devicetree bindings
* :github:`17805` - [Zepyhr v1.14.0 and master] Unable to run commands of mcumgr tool over UART like reset
* :github:`17781` - Question:Is it possible to connect the device on internet using bluetooth connection?
* :github:`17645` - VSCode debugging Zephyr application
* :github:`17626` - Change sanitycheck to use 'gcovr' instead of 'lcov'
* :github:`17625` - driver: gpio: PCAL9535A: can't write to register (read is possible)
* :github:`17548` - Can't set thread name with k\_thread\_create prevents useful tracing information
* :github:`17546` - Bluetooth: Central Scan fails continuously if last connect attempt failed to complete
* :github:`17454` - Bluetooth: Mesh: Add provisioner support
* :github:`17443` - Kconfig: move arch-specific stack sizes to arch trees?
* :github:`17430` - arch/x86: drivers/interrupt\_controller/system\_apic.c improperly classifies IRQs
* :github:`17361` - \_THREAD\_QUEUED overlaps with x86 \_EXC\_ACTIVE in k\_thread.thread\_state
* :github:`17337` - ArmV7-M mpu sub region alignment
* :github:`17239` - Too many open files crash when running "sanitycheck" with no arguments
* :github:`17234` - CONFIG\_KERNEL\_ENTRY appears to be superfluous
* :github:`17133` - arch/x86: x2APIC EOI should be inline
* :github:`17104` - arch/x86: fix -march flag for Apollo Lake
* :github:`17064` - drivers/serial/uart\_ns16550: CMD\_SET\_DLF should be removed
* :github:`17004` - arch/x86: build errors with newest build-grub.sh scripts
* :github:`16900` - Inline assembly in Arm z\_arch\_switch\_to\_main\_thread missing clobber list
* :github:`16880` - Systematic \*-zephyr-eabi/bin/ld: warning: toolchain\_is\_ok cannot find entry symbol \_start; defaulting to 000::00XXXXX
* :github:`16791` - build system does not see changes in DTS dependencies
* :github:`16723` - nrfx: uart: power management does not include CTS/RTS pins
* :github:`16721` - PCIe build warnings from devicetree
* :github:`16673` - usb\_dc\_stm32: If i remove the cable while writing, the program will freeze.
* :github:`16599` - drivers: usb\_dc\_nrfx: unstable handling of hosts suspend/resume
* :github:`16529` - LTS 1.14.0: sanitycheck: Cannot identify OOT boards and shields
* :github:`16452` - drivers: ethernet: stm32, sam, mcux: LAA bit not set
* :github:`16421` - drivers: rtc: stm32: correct tm\_mon conversion
* :github:`16376` - posix ext: Implement eventfd()
* :github:`16320` - The routing option CONFIG\_NET\_ROUTING needs clarification
* :github:`16223` - stm32: Unable to send 64 byte packet over control endpoint
* :github:`16167` - Implement interrupt driven GPIO on LPC families
* :github:`16097` - STM32 Ethernet driver should be able to detect the carrier state
* :github:`16041` - stm32f407 flash erase error sometimes
* :github:`16035` - facing problem with SDHC driver disk mount, need help to debug better
* :github:`16032` - Socket UDP: Low transmission efficiency
* :github:`16031` - Toolchain abstraction
* :github:`15912` - add Reject as an option to pull request reviews
* :github:`15881` - tests/net/buf fails on qemu\_x86\_64
* :github:`15841` - Support AT86RF233
* :github:`15604` - Suspicious PCI and build\_on\_all default test coverage
* :github:`15603` - Unable to use C++ Standard Library
* :github:`15598` - Standard devicetree connectors for boards
* :github:`15494` - 2.0 Release Checklist
* :github:`15359` - The docs incorrectly state that common.dts integrates with mcuboot
* :github:`15323` - blink\_led sample does not work on most of the nRF boards
* :github:`15196` - logging: Support for blocking deferred logging
* :github:`15027` - doc: PDF generation broken
* :github:`14906` - USB: NXP Device controller does not pass testusb tests
* :github:`14683` - need end-to-end memory protection samples
* :github:`13725` - drivers: ssd1306: When 128x32 is used, only half of the screen is output.
* :github:`13708` - No Arduino interface definition for Nordic dev. kits
* :github:`13417` - tests/drivers/watchdog/wdt\_basic\_api/testcase.yaml: test\_wdt\_no\_callback() failed at "Waiting to restart MCU"
* :github:`13000` - sanitycheck serializes running tests on ARC simulator
* :github:`12969` - settings: loading key-value pairs for given subtree
* :github:`12965` - POSIX subsys: Need more fine-grained enable options
* :github:`12961` - ARM Memory Protection functions not invoked in SWAP for ARMv6/ARMv8-M Baseline
* :github:`12703` - how to configure interrupt signals on shields via device tree?
* :github:`12677` - USB: There are some limitations for users to process descriptors
* :github:`12653` - Sanitycheck should not write results into scripts/sanity\_chk
* :github:`12535` - Bluetooth: suspend private address (RPA) rotating
* :github:`12509` - Fix rounding in \_ms\_to\_ticks()
* :github:`12504` - STM32: add USB\_OTG\_HS example
* :github:`12206` - OpenThread apps want to download and build OpenThread every time!
* :github:`12114` - assertion using nRF5 power clock with BLE and nRF5 temp sensor
* :github:`11743` - logging: add user mode access
* :github:`11717` - qemu\_x86 's SeaBIOS clears the screen every time it runs
* :github:`11655` - Alleged multiple design and implementation issues with logging
* :github:`11501` - RFC: Improve CI and add more status items
* :github:`10748` - Work waiting on pollable objects
* :github:`10701` - API: Prefix (aio\_) conflict between POSIX AsyncIO and Designware AnalogIO Comparator
* :github:`10503` - User defined USB function & usb\_get\_device\_descriptor()
* :github:`10338` - Add PyLint checking of all python scripts in CI
* :github:`10256` - Add support for shield x-nucleo-idb05a1
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
* :github:`7909` - tests/kernel/common.test\_bitfield fails on max10
* :github:`7375` - Codecov does not report coverage of code that is not covered by the native\_posix test suite
* :github:`7213` - DTS should use (one or more) prefixes on all defines
* :github:`6991` - Enhance test reporting and maintain one source for testcase meta data
* :github:`6858` - Default board configuration guidelines
* :github:`6446` - sockets: Accept on non-blocking socket is currently blocking
* :github:`6152` - Inter-applications flash layout exchange mechanism
* :github:`5138` - dts: boards: provide generic dtsi file for 'generic' boards
* :github:`4028` - C++ 11 Support
* :github:`3981` - ESP32 uart driver does not support Interrupt/fifo mode
* :github:`3877` - Use mbedtls from Zephyr instead of openthread
* :github:`652` - Provide a mean to find tests with 0 platforms due to bad filtering
* :github:`3497` - refactor \_NanoFatalErrorHandler
* :github:`3181` - scalable solution for test case stack sizes
* :github:`3124` - Atmel SAM RTC driver
* :github:`3056` - arch-specific inline functions cannot manipulate \_kernel
* :github:`2686` - Add qemu\_cortex\_m0/m0+ board.
* :github:`2490` - Provide sanity test cases for NANO\_ESF/NANO\_ISF structures
* :github:`2144` - clearly document internal kernel interfaces
