:orphan:

.. _zephyr_3.6:

Zephyr 3.6.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.6.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v3.5.0 to Zephyr v3.6.0 can be found in the separate :ref:`migration guide<migration_3.6>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Kernel
******

Architectures
*************

* ARC

* ARM

* ARM64

* RISC-V

* Xtensa

  * Removed the unused Kconfig option ``CONFIG_XTENSA_NO_IPC``.

  * Added userspace support via MMU.

* x86

* POSIX

Bluetooth
*********

* Audio

* Direction Finding

* Host

  * Added ``recycled()`` callback to :c:struct:`bt_conn_cb`, which notifies listeners when a
    connection object has been freed, so it can be utilized for different purposes. No guarantees
    are made to what listener will be granted the object, as only the first claim is served.

* Mesh

  * Added the delayable messages functionality to apply random delays for
    the transmitted responses on the Access layer.
    The functionality is enabled by the :kconfig:option:`CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG`
    Kconfig option.
  * The Bluetooth Mesh Protocol 1.1 is now supported by default.

* Controller

Boards & SoC Support
********************

* Added support for these SoC series:

  * Added support for Renesas R-Car Gen4 series

* Removed support for these SoC series:

* Made these changes in other SoC series:

  * Nordic SoCs now imply :kconfig:option:`CONFIG_XIP` instead of selecting it, this allows for
    creating RAM-based applications by disabling it.

* Added support for these ARC boards:

* Added support for these ARM boards:

  * Added support for Renesas R-Car Spider board CR52: ``rcar_spider_cr52``

  * Added support for Adafruit QTPy RP2040 board: ``adafruit_qt_py_rp2040``

  * Added support for Wiznet W5500 Evaluation Pico board: ``w5500_evb_pico``

* Added support for these ARM64 boards:

* Added support for these RISC-V boards:

* Added support for these X86 boards:

* Added support for these Xtensa boards:

* Added support for these POSIX boards:

* Made these changes for ARC boards:

* Made these changes for ARM boards:

* Made these changes for ARM64 boards:

* Made these changes for RISC-V boards:

  * ``longan_nano``: Enabled ADC support.

* Made these changes for X86 boards:

* Made these changes for Xtensa boards:

* Made these changes for native/POSIX boards:

  * The :ref:`simulated nrf5340 targets<nrf5340bsim>` now include the IPC and MUTEX peripherals,
    and support OpenAMP to communicate between the cores.
    It is now possible to run the BLE controller or 802.15.4 driver in the net core, and application
    and BT host in the app core.

  * The nrf*_bsim simulated targets now include models of the UART peripheral. It is now possible
    to connect a :ref:`nrf52_bsim<nrf52_bsim>` UART to another, or a UART in loopback, utilizing
    both the new and legacy nRFx UART drivers, in any mode.

  * For the native simulator based targets it is now possible to set via Kconfig command line
    options which will be handled by the executable as if they were provided from the invoking
    shell.

  * For all native boards boards, the native logger backend will also be used even if the UART is
    enabled.

  * Several bugfixes and other minor additions to the nRF5x HW models.

  * Multiple documentation updates and fixes for all native boards.

* Removed support for these ARC boards:

* Removed support for these ARM boards:

* Removed support for these ARM64 boards:

* Removed support for these RISC-V boards:

* Removed support for these X86 boards:

* Removed support for these Xtensa boards:

* Made these changes in other boards:

* Added support for these following shields:

Build system and infrastructure
*******************************

* Added functionality for Link Time Optimization.
  This change includes interrupt script generator rebuilding and adding following options:

  - :kconfig:option:`CONFIG_ISR_TABLES_LOCAL_DECLARATION` Kconfig option:
    LTO compatible interrupt tables parser,
  - :kconfig:option:`CONFIG_LTO` Kconfig option: Enable Link Time Optimization.

  Currently the LTO compatible interrupt tables parser is only supported by ARM architectures and
  GCC compiler/linker.
  See `pull request :github:`66392` for details.

* Dropped the ``COMPAT_INCLUDES`` option, it was unused since 3.0.

* Fixed an issue whereby board revision ``0`` did not include overlay files for that revision.

* Added ``PRE_IMAGE_CMAKE`` and ``POST_IMAGE_CMAKE`` hooks to sysbuild modules, which allows for
  modules to run code after and before each image's cmake invocation.

* Added :kconfig:option:`CONFIG_ROM_END_OFFSET` option which allows reducing the size of an image,
  this is intended for use with firmware signing scripts which add additional data to the end of
  images outside of the build itself.

* Added MCUboot image size reduction to sysbuild images which include MCUboot which prevents
  issues with building firmware images that are too large for MCUboot to swap.

* Deprecated :kconfig:option:`CONFIG_BOOTLOADER_SRAM_SIZE`, users of this should transition to
  having RAM set up properly in their board devicetree files.

* Fixed an issue whereby shields were processed in order of the root they resided in rather than
  the order they were supplied to cmake in.

* Fixed an issue whereby using some shields with sysbuild would cause a cmake Kconfig error.

* Fixed an issue where the macros ``_POSIX_C_SOURCE`` and ``_XOPEN_SOURCE`` would be defined
  globally when building with Picolibc or for the native (``ARCH_POSIX``) targets.
  After this change users may need to define them for their own applications or libraries if they
  require them.

* Added support for sysbuild setting a signing script (``SIGNING_SCRIPT``), see
  :ref:`west-extending-signing` for details.

* Added support for ``FILE_SUFFIX`` in the build system which allows for adding suffixes to
  application Kconfig fragment file names and devicetree overlay file names, see
  :ref:`application-file-suffixes` and :ref:`sysbuild_file_suffixes` for details.

* Deprecated ``CONF_FILE`` ``prj_<build>.conf`` build type.

Drivers and Sensors
*******************

* ADC

* CAN

  * Added system call :c:func:`can_get_mode()` for getting the current operation mode of a CAN
    controller.

  * Add system call :c:func:`can_get_transceiver()` for getting the CAN transceiver associated with
    a CAN controller.

  * Added accessor functions for the CAN statistics.

  * Added common bit error counter to the CAN statistics.

  * Added CAN statistics support to the following drivers:

    * :dtcompatible:`microchip,mcp2515`
    * :dtcompatible:`espressif,esp32-twai`
    * :dtcompatible:`kvaser,pcican`

  * Added CAN controller driver for the Nuvoton NuMaker series
    (:dtcompatible:`nuvoton,numaker-canfd`).

  * Added CAN controller driver for the Infineon XMC4xxx family
    (:dtcompatible:`infineon,xmc4xxx-can` and :dtcompatible:`infineon,xmc4xxx-can-node`).

  * Added support for the NXP S32K1xx family to the :dtcompatible:`nxp,flexcan` driver.

  * Use named IRQs "int0" and "int1" in all Bosch M_CAN-based front-end drivers.

  * The :dtcompatible:`zephyr,native-linux-can` driver now supports being built with embedded C
    libraries.

  * Added support for setting "raw" timing values from the :ref:`CAN shell <can_shell>`.

* Clock control

  * Renesas R-Car clock control driver now supports Gen4 SoCs
  * Renamed ``CONFIG_CLOCK_CONTROL_RA`` to :kconfig:option:`CONFIG_CLOCK_CONTROL_RENESAS_RA`

* Counter

  * The nRFx counter driver now works with simulated nrf*_bsim targets.

  * counter_native_posix driver: Added support for top value configuration, and a bugfix.

* DAC

* Disk

* Display

* DMA

* Entropy

  * The "native_posix" entropy driver now accepts a new command line option ``seed-random``.
    When used, the random generator will be seeded from ``/dev/urandom``

* Ethernet

  * The "native_posix" ethernet driver now supports being built with embedded C libraries.
  * Enabled HW checksum offloading for STM32H7.
  * Added implementation of Open Alliance's TC6 T1S driver.
  * Added xmc4xxx driver.
  * Added NXP enet driver with PTP support.
  * Added KSZ8081 PHY driver.
  * Added proper IPv4 multicast support to NXP mcux driver.
  * Added LAN8651 T1S support.
  * Added DSA support to STM32.
  * Added tja1103 PHY support.
  * Added Nuvoton numaker support.
  * Fixed lan865x driver. Transmission speed improvements, IRQ handling fixes.
  * Fixed s32_gmac driver. Link up/down handling fixes.
  * Fixed phy_mii driver. The invalid phy id was incorrectly checked.
  * Fixed sam_gmac driver. PTP clock adjustment was wrong for negative values.
  * Fixed adin2111 driver. Initialization was done incorrectly when working with adin2110.
  * Fixed ksz8081 driver. Logging changes, RMII clock fixes, GPIO pin fixes.

* Flash

  * Atmel SAM: Redesign controller to fully utilize flash page layout.
  * ``spi_nor`` driver now sleeps between polls in ``spi_nor_wait_until_ready``. If this is not
    desired (For example due to ROM constraints in a bootloader),
    :kconfig:option:`CONFIG_SPI_NOR_SLEEP_WHILE_WAITING_UNTIL_READY` can be disabled.

  * ``nordic_qspi_nor`` driver now supports user-configurable QSPI timeout with
    :kconfig:option:`CONFIG_NORDIC_QSPI_NOR_TIMEOUT_MS`.

* GPIO

  * Renesas R-Car GPIO driver now supports Gen4 SoCs
  * Renamed ``CONFIG_GPIO_RA`` to :kconfig:option:`CONFIG_GPIO_RENESAS_RA`

* I2C

* I2S

* I3C

  * The Legacy Virtual Register defines have been renamed from ``I3C_DCR_I2C_*``
    to ``I3C_LVR_I2C_*``.

  * Added the ability to specify a start address when searching for a free I3C
    address to be reserved. This requires a new function argument to
    :c:func:`i3c_addr_slots_next_free_find`.

  * Added a field named ``num_xfer`` in :c:struct:`i3c_msg` and
    :c:struct:`i3c_ccc_taget_payload` as an output to indicate the actual
    number of bytes transferred.

  * Cadence I3C driver (:file:`drivers/i3c/i3c_cdns.c`):

    * Added support to handle controller abort where target does not emit
      end of data for register read but continues sending data.

    * Updated the timeout calculation to be coupled with CPU speed instead of
      a fixed number of retries.

  * NXP MCUX I3C driver (:file:`drivers/i3c/i3c_mcux.c`):

    * Fixed ``mcux_i3c_config_get()`` of not returning the configuration to caller.

    * Sped up the FIFO read routine to support higher transfer rate.

    * Removed the infinite wait for MCTRLDONE in auto IBI.

    * Added ``disable-open-drain-high-pp`` property to
      :dtcompatible:`nxp,mcux-i3c`, which allows alternative high time for
      open-drain clock.

* IEEE 802.15.4

  * Removed :kconfig:option:`CONFIG_IEEE802154_SELECTIVE_TXPOWER` Kconfig option.

* Interrupt Controller

* Input

  * The ``short-codes`` property of :dtcompatible:`zephyr,input-longpress` is
    now optional, the node can be used by specifying only input and long codes.
  * Added support for keyboard matrix drivers, including a new
    :dtcompatible:`gpio-kbd-matrix` and :dtcompatible:`input-keymap` drivers,
    see :ref:`gpio-kbd` for more details.
  * Added a pair of input codes to HID codes translation functions, see
    :c:func:`input_to_hid_code` and :c:func:`input_to_hid_modifier`.
  * Added power management support to :dtcompatible:`gpio-keys`
    :dtcompatible:`focaltech,ft5336`.
  * Added a :dtcompatible:`zephyr,native-linux-evdev` device node for getting
    input events from a Linux evdev device node.
  * Added support for optical encoders and power management to :dtcompatible:`gpio-qdec`.
  * New drivers :dtcompatible:`espressif,esp32-touch`, :dtcompatible:`analog-axis`.

* PCIE

  * Fixed MMIO size calculation by disabling IO/memory decoding beforehand.

  * Modified to use PNP ID for PRT retrieval.

* ACPI

* Pin control

  * Renesas R-Car pinctrl driver now supports Gen4 SoCs
  * Renamed ``CONFIG_PINCTRL_RA`` to :kconfig:option:`CONFIG_PINCTRL_RENESAS_RA`
  * Renesas R-Car pinctrl driver now supports voltage control for R8A77951 and
    R8A77961 SoCs
  * Added driver for ZynqMP / Mercury XU
  * Added driver for i.MX8QM/QXP
  * Added driver for Renesas RZ/T2M

* PWM

* Regulators

* Reset

* Retained memory

  * Retained memory driver backend for registers has been added.

  * Retained memory API status changed from experimental to unstable.

* RTC

  * Atmel SAM: Added RTC driver.

* SDHC

* Sensor

* Serial

  * Added drivers to support UART on Renesas RA and RZ/T2M.

  * Added support for higher baud rate for ITE IT8xxx2.

  * Added driver to support Intel Lightweight UART.

  * Added UART asynchronous RX helper.

  * Added support for async API on NS16550 driver.

  * Updated ``uart_esp32`` to use serial port configuration from devicetree.

  * Added an adaptation API to provide interrupt driven API for drivers
    which have only implemented async API.

  * Emulated UART driver (:file:`drivers/serial/uart_emul.c`):

    * Added emulated interrupt based TX.

    * Added emulated error for testing.

    * Modified to use local work queue for data transfer.

    * Modified FIFO size and its handling to be more aligned with real hardware.

* SPI

* Timer

* USB

* Wi-Fi

  * Added Infineon airoc driver.
  * Fixed esp32 driver. Decreased minimum heap size, disabled automatic reconnection on leaving.
  * Fixed esp_at driver. Allow building without IPv4 support. Passive Receive mode fixes. Depend on UART runtime configuration.
  * Fixed winc1500 driver. Disconnect result event was not returned when disconnecting.

Networking
**********

* CoAP:

  * Emit observer/service network events using the Network Event subsystem.

  * Added new API functions:

    * :c:func:`coap_get_transmission_parameters`
    * :c:func:`coap_set_transmission_parameters`

* Connection Manager:

* DHCP:

* Ethernet:

  * Allow manual registration of ARP entries.
  * Added PHY mode selection to device tree.
  * Added TX-Injection mode support.

* gPTP:

  * Use local port identity when forwarding sync messages.
  * Fix double converted byte order of BMCA info.
  * Always use GM PRIO root system id for announce messages.
  * Create gPTP handler thread stack size Kconfig option.
  * Invert the priority of outgoing packets.

* ICMP:

* IPv6:

* LwM2M:

* Misc:

  * It is now possible to have separate IPv4 TTL value and IPv6 hop limit value for
    unicast and multicast packets. This can be controlled in each socket via
    :c:func:`setsockopt` API.

  * Added support for compile time network event handlers using the macro
    :c:macro:`NET_MGMT_REGISTER_EVENT_HANDLER`.

  * The :kconfig:option:`CONFIG_NET_MGMT_EVENT_WORKER` choice is added to
    allow emitting network events using the system work queue or synchronously.

* MQTT-SN:

* OpenThread:

* PPP:

* Sockets:

  * Added support for IPv4 multicast ``IP_ADD_MEMBERSHIP`` and ``IP_DROP_MEMBERSHIP`` socket options.
  * Added support for IPv6 multicast ``IPV6_ADD_MEMBERSHIP`` and ``IPV6_DROP_MEMBERSHIP`` socket options.

* TCP:

* TFTP:

* WebSocket

* Wi-Fi:

  * Added Wi-Fi driver version information to Wi-Fi shell.
  * Added AP (Access Point) mode support to Wi-Fi shell.
  * Added Regulatory channel information.
  * Added Wi-Fi bindings to connection manager.
  * Fixed Wi-Fi shell. SSID print fixes. Help text fixes. Channel validation fixes.
  * Fixed TWT functionality. Teardown status was not updated. Powersave fixes.

USB
***

Devicetree
**********

API
===

Bindings
========

Libraries / Subsystems
**********************

* Management

  * Fixed an issue in MCUmgr image management whereby erasing an already erased slot would return
    an unknown error, it now returns success.

  * Fixed MCUmgr UDP transport structs being statically initialised, this results in about a
    ~5KiB flash saving.

  * Fixed an issue in MCUmgr which would cause a user data buffer overflow if the UDP transport was
    enabled on IPv4 only but IPv6 support was enabled in the kernel.

  * Implemented datetime functionality in MCUmgr OS management group, this makes use of the RTC
    driver API.

  * Fixed an issue in MCUmgr console UART input whereby the FIFO would be read outside of an ISR,
    which is not supported in the next USB stack.

  * Fixed an issue whereby the ``mcuboot erase`` DFU shell command could be used to erase the
    MCUboot or currently running application slot.

  * Fixed an issue whereby messages that were too large to be sent over the UDP transport would
    wrongly return :c:enum:`MGMT_ERR_EINVAL` instead of :c:enum:`MGMT_ERR_EMSGSIZE`.

  * Fixed an issue where confirming an image in Direct XIP mode would always confirm the image in
    the primary slot even when executing from the secondary slot, now the currently active image is
    always confirmed.

  * Added support for retrieving registered command groups, to support registering and deregistering
    default command groups at runtime, allowing an application to support multiple implementations
    for the same command group.

  * Fixed an issue in MCUmgr FS management whereby the semaphore lock would not be given if an
    error was returned, leading to a possible deadlock.

  * Added support for custom payload MCUmgr handlers, this can be enabled with
    :kconfig:option:`CONFIG_MCUMGR_MGMT_CUSTOM_PAYLOAD`.

  * Fixed an issue in MCUmgr image management whereby an error would be returned if a command was
    sent to erase the slot which was already erased.

  * Added support for image slot size checking to ensure an update can be utilised by MCUboot,
    this can be performed by using sysbuild when building both application and MCUboot by enabling
    :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_SYSBUILD` or by use of bootloader information
    sharing from MCUboot by enabling
    :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_BOOTLOADER_INFO`.

* File systems

* Modem modules

* Power management

  * Atmel SAM: introduced SUPC functions to allow wakeup sources and poweroff.

* Random

* Retention

  * Fixed issue whereby :kconfig:option:`CONFIG_RETENTION_BUFFER_SIZE` values over 256 would cause
    an infinite loop due to use of 8-bit variables.

* Storage

  * File systems: LittleFS module has been updated to version 2.8.1.

  * Following Flash Map API macros, marked in 3.2 as deprecated, have been removed:
    ``FLASH_AREA_ID``, ``FLASH_AREA_OFFSET``, ``FLASH_AREA_SIZE``,
    ``FLASH_AREA_LABEL_EXISTS`` and ``FLASH_AREA_DEVICE``.

* Binary descriptors

* POSIX API

  * conformance: complete support for ``POSIX_THREADS_EXT``, ``XSI_THREADS_EXT``,
    ``POSIX_CLOCK_SELECTION``, and ``POSIX_SEMAPHORES`` Option Groups.

  * conformance: complete support for ``_POSIX_MESSAGE_PASSING`` and
    ``_POSIX_PRIORITY_SCHEDULING`` Options.

  * coverity: fix CID 211585, 334906, 334909, and 340851

  * documentation: improve structure and accuracy of POSIX docs

  * menuconfig: improved navigation and organization of POSIX options

  * pthread: allocate and free stacks with pthread_attr_t, embed attr in thread structure

  * pthread: support deferred and asynchronous thread cancellation

  * pthread: support stack sizes up to 8MB

  * samples: add dining philosophers sample app

  * semaphores: add support for named semaphores

  * shell: add a top-level ``posix`` command in the Zephyr shell. Zephyr shell utilities for
    the POSIX API can be added as subcommands (e.g. ``posix uname -a``)

  * timers: use async thread cancellation, add support for ``SIGEV_THREAD``, ``CLOCK_REALTIME``

  * unistd: add compile-time-constant sysconf() implementation

* LoRa/LoRaWAN

* RTIO

* ZBus

  * Renamed :kconfig:option:`ZBUS_MSG_SUBSCRIBER_NET_BUF_DYNAMIC` and
    :kconfig:option:`ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC`
    with :kconfig:option:`ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC` and
    :kconfig:option:`ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_STATIC`

HALs
****

MCUboot
*******

  * Fixed compatible sector checking in bootutil.

  * Fixed Kconfig issue with saving encrypted TLVs not depending on encryption being enabled.

  * Fixed issue with missing condition check for applications in sysflash include file.

  * Fixed issue with single slot encrypted image listing support in boot_serial.

  * Fixed issue with allowing MBEDTLS Kconfig selection when tinycrypt is used.

  * Fixed missing response if echo command was disabled in boot_serial.

  * Fixed issue with USB configurations not generating usable images.

  * Added debug logging for boot status write in bootutil.

  * Added estimated image overhead size to cache in sysbuild.

  * Added firmware loader operating mode which allows for a dedicated secondary slot image that
    is used to update the primary image.

  * Added error if main thread is not pre-emptible when USB CDC serial recovery is enabled.

  * Added error if USB CDC and console are both enabled and set to the same device.

  * Removed the deprecated ``CONFIG_ZEPHYR_TRY_MASS_ERASE`` Kconfig option.

  * Updated zcbor to version 0.8.1 and re-generated boot_serial files.

  * Moved IO functions out of main to separate file.

  * Made ``align`` parameter of imgtool optional.

  * The MCUboot version in this release is version ``2.1.0+0-dev``.

Nanopb
******

zcbor
*****

zcbor has been updated from 0.7.0 to 0.8.1.
Full release notes can be found at:
https://github.com/zephyrproject-rtos/zcbor/blob/0.8.0/RELEASE_NOTES.md and
https://github.com/zephyrproject-rtos/zcbor/blob/0.8.1/RELEASE_NOTES.md

Highlights:

* Add support for unordered maps
* Performance improvements
* Naming improvements for generated code
* Bugfixes

LVGL
****

LVGL has been updated from 8.3.7 to 8.3.11.
Detailed release notes can be found at:
https://github.com/zephyrproject-rtos/lvgl/blob/zephyr/docs/CHANGELOG.md

Additionally the following changes in Zephyr were done:

  * Added the :dtcompatible:`zephyr,lvgl-keypad-input` compatible for keypad input.

  * Fixed issue with the Zephyr log levels not mapping properly to LVGL log levels.

  * Fixed issue where setting :kconfig:option:`CONFIG_LV_Z_FULL_REFRESH` did not
    set :kconfig:option:`CONFIG_LV_Z_VDB_SIZE` to 100 percent.

Trusted Firmware-A
******************

Documentation
*************

Tests and Samples
*****************

* :ref:`native_sim<native_sim>` has replaced :ref:`native_posix<native_posix>` as the default
  test platform.
  :ref:`native_posix<native_posix>` remains supported and used in testing but will be deprecated
  in a future release.

* Bluetooth split stacks tests, where the BT host and controller are run in separate MCUs, are
  now run in CI based on the :ref:`nrf5340_bsim<nrf5340bsim>` targets.
  Several other runtime AMP tests based on these targets have been added to CI, including tests
  of OpenAMP, the mbox and IPC drivers/subsystem, and the logger multidomain functionality.

* Runtime UART tests have been added to CI based on the :ref:`nrf52_bsim<nrf52_bsim>` target.
  These include tests of the nRFx UART driver and networked BT stack tests with the host and
  controller in separate devices communicating over the HCI UART driver.

* Fixed an issue in :zephyr:code-sample:`smp-svr` sample whereby if USB was already initialised,
  application would fail to boot properly.

* Added a LVGL sample :zephyr:code-sample:`lvgl-accelerometer-chart` showcasing displaying of live
  sensor data in a chart widget.
