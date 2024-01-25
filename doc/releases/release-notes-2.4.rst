:orphan:

.. _zephyr_2.4:

Zephyr 2.4.0
############

We are pleased to announce the release of Zephyr RTOS version 2.4.0.

Major enhancements with this release include:

* Introduced initial support for virtual memory management.

* Added Bluetooth host support for periodic advertisement and isochronous
  channels.

* Enabled the new TCP stack, TCP2, by default. This stack was introduced in
  Zephyr v2.1.0 to improve network protocol testability with open source tools.

* Introduced a new toolchain abstraction with initial implementations for GCC
  and LLVM/Clang, and groundwork for future support of commercial toolchains.

* Moved to using C99 integer types and deprecate Zephyr integer types.  The
  Zephyr types can be enabled by Kconfig DEPRECATED_ZEPHYR_INT_TYPES option.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* CVE-2020-10060: UpdateHub Might Dereference An Uninitialized Pointer
* CVE-2020-10064: Improper Input Frame Validation in ieee802154 Processing
* CVE-2020-10066: Incorrect Error Handling in Bluetooth HCI core
* CVE-2020-10072: all threads can access all socket file descriptors
* CVE-2020-13598: FS: Buffer Overflow when enabling Long File Names in FAT_FS and calling fs_stat
* CVE-2020-13599: Security problem with settings and littlefs
* CVE-2020-13601: Under embargo until 2020/11/18
* CVE-2020-13602: Remote Denial of Service in LwM2M do_write_op_tlv

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Known issues
************

You can check all currently known issues by listing them using the GitHub
interface and listing all issues with the `bug label
<https://github.com/zephyrproject-rtos/zephyr/issues?q=is%3Aissue+is%3Aopen+label%3Abug>`_.

API Changes
***********

* Moved to using C99 integer types and deprecate Zephyr integer types.  The
  Zephyr types can be enabled by Kconfig DEPRECATED_ZEPHYR_INT_TYPES option.

* The ``<sys/util.h>`` header has been promoted to a documented API with
  :ref:`experimental stability <api_lifecycle>`. See :ref:`util_api` for an API
  reference.

* The :c:func:`wdt_feed` function will now return ``-EAGAIN`` if
  issuing a feed would stall the caller.  Application code may need to
  ignore this diagnostic result or initiate another feed operation
  later.

* ``<drivers/uart.h>`` has seen its callbacks normalized.
  :c:type:`uart_callback_t` and :c:type:`uart_irq_callback_user_data_t`
  had their signature changed to add a struct device pointer as first parameter.
  :c:type:`uart_irq_callback_t` has been removed. :c:func:`uart_callback_set`,
  :c:func:`uart_irq_callback_user_data_set` and :c:func:`uart_irq_callback_set`
  user code have been modified accordingly.

* ``<drivers/dma.h>`` has seen its callback normalized. It had its signature
  changed to add a struct device pointer as first parameter. Such callback
  signature has been generalized through the addition of dma_callback_t.
  'callback_arg' argument has been renamed to 'user_data. All user code have
  been modified accordingly.

* ``<drivers/ipm.h>`` has seen its callback normalized.
  :c:type:`ipm_callback_t` had its signature changed to add a struct device
  pointer as first parameter. :c:func:`ipm_register_callback` user code have
  been modified accordingly. The context argument has been renamed to user_data
  and all drivers have been modified against it as well.

* The :c:func:`fs_open` function now accepts open flags that are passed as
  a third parameter.
  All custom file system front-ends require change to the implementation
  of ``open`` callback to accept the new parameter.
  To maintain original behaviour within user code, two argument invocations
  should be converted to pass a third argument ``FS_O_CREATE | FS_O_RDWR``.

* The struct device got 3 attributes renamed: ``config_info`` to ``config``,
  ``driver_api`` to ``api`` and finally ``driver_data`` to ``data``.
  This renaming was done to get rid of legacy names, for which the reasons
  do no longer apply.

* All device instances got a const qualifier. So this applies to all APIs
  manipulating ``struct device *`` (ADC, GPIO, I2C, ...). In order to avoid
  const qualifier loss on ISRs, all ISRs now take a ``const *void`` as a
  parameter as well.

* The ``_gatt_`` and ``_GATT_`` infixes have been removed for the HRS, DIS
  and BAS APIs and the Kconfig options.

* ``<include/bluetooth/gatt.h>`` callback :c:func:`bt_gatt_attr_func_t` used by
  :c:func:`bt_gatt_foreach_attr` and :c:func:`bt_gatt_foreach_attr_type` has
  been changed to always pass the original pointer of attributes along with its
  resolved handle.

* Established the unrestricted alignment of flash reads for all drivers.

Deprecated in this release
==========================

* The full set of ``k_mem_pool`` and ``sys_mem_pool`` APIs
  are considered deprecated as of this release. The replacements are
  the ``k_heap`` and ``sys_heap`` APIs. These APIs are not tagged with
  ``__deprecated`` in the 2.4 release, but will be in 2.5. They will be
  removed completely in Zephyr 2.6 LTS. The set of APIs now deprecated is as
  follows:

  * ``k_mbox_data_block_get()``
  * ``k_pipe_block_put()``
  * ``K_MEM_POOL_DEFINE()``
  * ``k_mem_pool_alloc()``
  * ``k_mem_pool_free()``
  * ``k_mem_pool_free_id()``
  * ``SYS_MEM_POOL_DEFINE()``
  * ``sys_mem_pool_init()``
  * ``sys_mem_pool_alloc()``
  * ``sys_mem_pool_free()``
  * ``sys_mem_pool_try_expand_inplace()``

* The Kconfig option ``CONFIG_MULTITHREADING`` to disable multi-threading was
  deprecated due to lack of maintainership. This means that single-threaded
  mode with the scheduler disabled is deprecated; normal multi-threaded mode is
  still fully supported.

Removed APIs in this release
============================

* Other

  * The deprecated ``MACRO_MAP`` macro has been removed from the
    :ref:`util_api`. Use ``FOR_EACH`` instead.
  * The CONFIG_NET_IF_USERSPACE_ACCESS is removed as it is no longer needed.

* Build system

  * The set of ``*_if_kconfig()`` CMake functions have been removed. Use
    ``_ifdef(CONFIG_ ...)`` instead.

Stable API changes in this release
==================================

* USB

  * HID class callbacks now takes a parameter ``const struct device*`` which
    is the HID device for which callback was called.

* Bluetooth

  * The ``_gatt_`` infix has been removed from all GATT service APIs.

* Bluetooth HCI Driver

  * bt_hci_evt_is_prio() removed, use bt_hci_evt_get_flags() instead when
    CONFIG_BT_RECV_IS_RX_THREAD is defined and call bt_recv and bt_recv_prio
    when their flag is set, otherwise always call bt_recv().

Kernel
******

* Initial support for virtual memory management

  * API definitions in ``include/sys/mem_manage.h``.
  * Supporting architectures will implement ``arch_mem_map()`` and enable
    ``CONFIG_MMU``.
  * The kernel is linked at its physical memory location in RAM.
  * The size of the address space is controlled via ``CONFIG_KERNEL_VM_SIZE``
    with memory mapping calls allocating virtual memory growing downward
    from the address space limit towards the system RAM mappings.
  * This infrastructure is still under heavy development.

* Device memory mapped I/O APIs

  * Namedspaced as DEVICE_MMIO and specified in a new
    ``include/sys/device_mmio.h`` header.
  * This is added to facilitate the specification and the storage location of
    device driver memory-mapped I/O regions based on system configuration.

    * Maintained entirely in ROM for most systems.
    * Maintained in RAM with hooks to memory-mapping APIs for MMU or PCI-E
      systems.

* Updates for Memory Domain APIs

  * All threads now are always a member of a memory domain. A new
    memory domain ``k_mem_domain_default`` introduced for initial threads
    like the main thread.
  * The ``k_mem_domain_destroy()`` and ``k_mem_domain_remove_thread()`` APIs
    are now deprecated and will be removed in a future release.
  * Header definitions moved to ``include/app_memory/mem_domain.h``.

* Thread stack specification improvements

  * Introduced a parallel set of ``K_KERNEL_STACK_*`` APIs for specifying
    thread stacks that will never host user threads. This will conserve memory
    as ancillary data structures (such as privilege mode elevation stacks) will
    not need to be created, and certain alignment requirements are less strict.

  * Internal interfaces to the architecture code have been simplified. All
    thread stack macros are now centrally defined, with arches declaring
    support macros to indicate the alignment of the stack pointer, the
    stack buffer base address, and the stack buffer size.

Architectures
*************

* ARC

  * Added ARC MetaWare toolchain support
  * General arch improvements for stacks & memory domains
  * API improvements for cache flush and cache invalidate
  * Debugging help: show all registers on exception
  * Fix for fast irq (one register bank configuration)
  * Fix for undefined shift behavior (CID 211523)

* ARM

  * AARCH32

    * Added support for ARM Cortex-M1 architecture.
    * Implemented the timing API in Cortex-M architecture using the Data
      Watchpoint and Trace (DWT) unit.
    * The interrupt vector relaying feature support was extended to Cortex-M
      Mainline architecture variants.
    * Cortex-M fault handling implementation was enhanced by adding an option to
      generate and supply the full register state to the kernel fatal error
      handling mechanism.
    * Fixed Cortex-M boot sequence for single-threaded applications
      (CONFIG_MULTITHREADING=n).
    * Added thread safety to Non-Secure entry function calls in ARMv8-M
      architecture.
    * Fixed stack randomization for main thread.
    * Fixed exception vector table alignment in Cortex-M architecture
    * Increased test coverage in QEMU for ARMv6-M architecture variant.
    * Removed the implementation of arch_mem_domain_* APIs for Cortex-M

  * AARCH64

    * Re-implemented thread context-switch to use the _arch_switch() API

* POSIX

* RISC-V

* x86

  * x86 MMU paging support has been overhauled to meet CONFIG_MMU requirements.

    * ``arch_mem_map()`` is implemented.
    * Restored support for 32-bit non-PAE paging. PAE use is now controlled
      via the ``CONFIG_X86_PAE`` option
    * Initial kernel page tables are now created at build time.
    * Page tables are no longer strictly identity-mapped

  * Added ``zefi`` infrastructure for packaging the 64-bit Zephyr kernel into
    an EFI application.

  * Added a GDB stub implementation that works over serial for x86 32-bit.

Boards & SoC Support
********************

* Added support for these SoC series:

  * ARM Cortex-M1/M3 DesignStart FPGA
  * Atmel SAM4L
  * Nordic nRF52805
  * NXP i.MX RT685, i.MX8M Mini, and LPC11U6x
  * ARC QEMU support for EM and HS family

* Made these changes in other SoC series:

  * STM32L4/STM32WB: Added support for Low Power Mode.
  * STM32H7/STM32WB/STM32MP1: Added Dual Core concurrent register access
    protection using HSEM.
  * Increased cpu frequency for ARC nsim_hs_smp.

* Changes for ARC boards:

  * ARC QEMU boards for ARC EM and HS
  * ARC MetaWare toolchain support, including mdb runner for various ARC boards
  * gcov coverage support for ARC QEMU
  * New nSIM configuration, corresponding to em7d_v22 EMSK board
  * Enabled SMP on HSDK board, including dual core and quad core configurations.
  * Switched from legacy ARC-nSIM UART to ns16550 UART model and driver.
  * Fixed EMSDP secure config for emsdp_em7d_esp.

* Added support for these ARM boards:

  * Adafruit ItsyBitsy M4 Express
  * Arduino Nano 33 IOT
  * ARM Cortex-M1/M3 DesignStart FPGA reference designs running on the Digilent
    Arty A7 development board
  * Atmel SAM4L-EK board
  * Circuit Dojo nRF9160 Feather
  * EOS S3 Quick Feather
  * Laird Connectivity Pinnacle 100 Modem Development board (pinnacle_100_dvk)
  * nRF21540 DK (nrf21540dk_nrf52840)
  * nRF52805 emulation on nRF52 DK (nrf52dk_nrf52805)
  * nRF5340 DK
  * Nuvoton npcx7m6fb and pfm m487 boards
  * NXP i.MX RT685 EVK, i.MX8M Mini EVK, LPCXpresso LPC11U68
  * OLIMEX-STM32-H103
  * Ruuvitag board
  * Seagate FaZe board
  * Seeeduino XIAO
  * Serpente board
  * Silicon Labs BRD4180A (a.k.a. SLWRB4180A) Mighty Gecko Radio Board
  * ST B_L4S5I_IOT01A Discovery kit
  * ST NUCLEO-H745ZI-Q
  * Waveshare Open103Z
  * WeAct Studio Black Pill V2.0

* Made these changes in other boards:

  * b_l072z_lrwan1: Added flash, LoRa, USB, EEPROM, RNG
  * nRF boards: enabled HW Stack Protection by default on boards maintained by Nordic
  * nucleo_l552ze_q: Added non secure target and TFM support
  * STM32 boards: Enabled MPU on all boards with at least 64K flash
  * lpcxpresso55s69: Added TFM support

* Added support for these following shields:

  * Adafruit WINC1500 Wifi
  * ARM Ltd. V2C-DAPLink for DesignStart FPGA
  * Atmel AT86RF2XX Transceivers
  * Buydisplay 2.8" TFT Touch Shield with Arduino adapter
  * DAC80508 Evaluation Module

Drivers and Sensors
*******************

* ADC

  * Added chip select flags to SPI ADC drivers.

* Audio

  * N/A

* Bluetooth

  * L2CAP RX MTU is now controlled by CONFIG_BT_L2CAP_RX_MTU when
    CONFIG_BT_ACL_FLOW_CONTROL is disabled, previously this was controlled
    by CONFIG_BT_RX_BUF_LEN. If CONFIG_BT_RX_BUF_LEN has been changed from its
    default value then CONFIG_BT_L2CAP_RX_MTU should be set to
    CONFIG_BT_RX_BUF_LEN - 8.

* CAN

  * Added chip select flags to SPI CAN drivers.
  * Fixed MCP2515 driver to wait to reset.

* Clock Control

  * STM32: Various changes including Flash latency wait states computation,
    configuration option additions for H7 series, and fixes on F0/F3 PREDIV1
    support
  * Added LPC11U6X driver.

* Console

  * Added IPM driver.

* Counter

  * STM32: Added support on F0/F2 series.
  * Added MCUX PIT counter driver for Kinetis K6x and K8x SoCs.

* Crypto

  * N/A

* DAC

  * STM32: Added support for F0/F2/G4/L1 series.

* Debug

  * N/A

* Display

  * Enhanced SSD16XX driver to support loading WS from OTP.
  * Added chip select flags to SPI display drivers.

* DMA

  * STM32: Number of changes including k_malloc removal, driver priority init
    increase, get_status API addition and various cleanups.
  * Added MCUX EDMA driver for i.MX RT and Kinetis K6x SoCs.
  * Added MCUX LPC driver for LPC and i.MX RT6xx SoCs.

* EEPROM

  * Added driver supporting the on-chip EEPROM found on NXP LPC11U6X MCUs.
  * Fixed at2x cs gpio flags extraction from DT.

* Entropy

  * STM32: Added support for ISR mode. Added support on F7/H7/L0 series.

* ESPI

  * Enhanced XEC driver to support KBC status operations, ACPI_EC1 interface,
    and slaves with long initializations.
  * Fixed XEC driver frequency override during IO selection.

* Ethernet

  * Added VLAN support to Intel e1000 driver.
  * Added Ethernet support to stm32h7 based boards (with IT based TX).
  * Moved stm32 driver to device tree configuration.
  * Added support for setting fixed configuration and read from device tree
    for ENET ETH interface and PHY in mcux driver.
  * Added support for device that do not use SMI for PHY setup in mcux driver.
  * Added support for multiport gPTP in native_posix driver. This allows gPTP
    bridging testing.
  * Fixed MAC registers in enc28j60 driver to the latest Microchip reference manual.

* Flash

  * The driver selected by ``CONFIG_SPI_FLASH_W25QXXDV`` has been
    removed as it is unmaintained and all its functionality is available
    through ``CONFIG_SPI_NOR``.  Out of tree uses should convert to the
    supported driver using the ``jedec,spi-nor`` compatible.
  * Enhanced nRF QSPI NOR flash driver (nrf_qspi_nor) so it supports unaligned read offset, read length and buffer offset.
  * Added SFDP support in spi_nor driver.
  * Fixed regression in nRF flash driver (soc_flash_nrf) with :kconfig:option:`CONFIG_BT_CTLR_LOW_LAT` option.
  * Introduced NRF radio scheduler interface in nRF flash driver (soc_flash_nrf).
  * STM32: Factorized support for F0/F1/F3. Added L0 support. Various fixes.

* GPIO

  * Added driver for the Xilinx AXI GPIO IP.
  * Added LPC11U6X driver.

* Hardware Info

  * Added Atmel SAM4L driver.

* I2C

  * Introduced new driver for NXP LPC11U6x SoCs.  See
    :kconfig:option:`CONFIG_I2C_LPC11U6X`.

  * Introduced new driver for emulated I2C devices, where I2C operations
    are forwarded to a module that emulates responses from hardware.
    This enables testing without hardware and allows unusual conditions
    to be synthesized to test driver behavior.  See
    :kconfig:option:`CONFIG_I2C_EMUL`.

  * STM32: V1: Reset i2c device on read/write error.
  * STM32: V2: Added dts configurable Timing option.
  * Fixed MCUX LPI2C driver transfer status after NACK.

* I2S

  * Added LiteX controller driver.

* IEEE 802.15.4

  * Allow user to disable auto-start of IEEE 802.15.4 network interface.
    By default the IEEE 802.15.4 network interface is automatically started.
  * Added support for setting TX power in rf2xx driver.
  * Added Nordic 802.15.4 multiprotocol support, see :kconfig:option:`CONFIG_NRF_802154_MULTIPROTOCOL_SUPPORT`.
  * Added Kconfig :kconfig:option:`CONFIG_IEEE802154_VENDOR_OUI_ENABLE` option for defining OUI.

* Interrupt Controller

  * Enhanced GICV3 driver to support SGI API.
  * Added NPCX MIWU driver.

* IPM

  * Added Intel ADSP driver.

* Keyboard Scan

  * Enhanced FT5336 driver to support additional part number variants.

* LED

  * Added TI LP503X controller driver.
  * Introduced led_set_color, let_get_info, and channel-dedicated syscalls
  * Added shell support.

* LED Strip

  * Enhanced APA102 driver to support SPI chip select.

* LoRa

  * Made various enhancements and fixes in SX1276 driver.

* Modem

  * Added option to query the IMSI and ICCID from the SIM.
  * Added support for offloaded Sierra Wireless HL7800 modem.

* PECI

  * N/A

* Pinmux

  * Added LPC11U6X driver.
  * Added NPCX driver.

* PS/2

  * N/A

* PWM

  * STM32: Refactored using Cube LL API.
  * Added SAM9 TCC based driver.

* Sensor

  * Added API function ``sensor_attr_get()`` for getting a sensor's
    attribute.
  * Added support for wsen-itds accelerometer sensor.
  * Added chip select flags to SPI sensor drivers.
  * Added IIS2DH accelerometer driver.
  * Added MAX17055 fuel-gauge sensor driver.
  * Added SI7055 temperature sensor driver.
  * Enhanced FXOS8700 driver to support magnetic vector magnitude function.
  * Added SM351LT magnetoresistive sensor driver.
  * Added VCNL4040 proximity and light sensor driver.
  * Refactored LIS2DH and LSM6DSL drivers to support multiple instances.

* Serial

  * Added driver for the Xilinx UART Lite IP.
  * Added NXP IUART driver for i.MX8M Mini.
  * Implemented uart_config_get API in MCUX UART driver.
  * Added LPC11U6X driver.

* SPI

  * The SPI driver subsystem has been updated to use the flags specified
    in the cs-gpios devicetree properties rather than the
    SPI_CS_ACTIVE_LOW/HIGH configuration options.  Devicetree files that
    specify 0 for this field will probably need to be updated to specify
    GPIO_ACTIVE_LOW.  SPI_CS_ACTIVE_LOW/HIGH are still used for chip
    selects that are not specified by a cs-gpios property.
  * Added driver for the Xilinx AXI Quad SPI IP.
  * STM32: Various fixes around DMA mode.
  * Extended MCUX Flexcomm driver to support slave mode.
  * Added optional delays to MCUX DSPI and LPSPI drivers.

* Timer

  * N/A

* USB

  * The usb_enable() function, which, for some samples, was invoked
    automatically on system boot up, now needs to be explicitly called
    by the application in order to enable the USB subsystem. If your
    application relies on any of the following Kconfig options, then
    it shall also enable the USB subsystem:

    * :kconfig:option:`CONFIG_USB_DEVICE_NETWORK_ECM`
    * :kconfig:option:`CONFIG_USB_DEVICE_NETWORK_EEM`
    * :kconfig:option:`CONFIG_USB_DEVICE_NETWORK_RNDIS`
    * :kconfig:option:`CONFIG_TRACING_BACKEND_USB`
    * ``CONFIG_USB_UART_CONSOLE``

  * USB device support has got its own work queue
    which is used by CDC ACM class by default.

  * CDC ACM Class was slightly reworked.

  * Suspend and resume support in CDC ACM and HID classes
    has been corrected.

  * Atmel SAM0 USB device driver (usb_dc_sam0) was revised.
    All drivers now use common macros for getting indexes and direction
    from an endpoint.

* Video

  * N/A

* Watchdog

  * Added MCUX WWDT driver for LPC SoCs.
  * Enhanced Gecko driver to support Gecko Series 2 SoC.

* WiFi

  * Added IPv6 support to Simplelink driver.
  * Added DNS offloading support to eswifi driver.
  * Fixed esp driver offload protocol parsing.
  * Fixed esp driver GPIO reset control logic.
  * Fixed eswifi driver offloading packet parsing.

Networking
**********

* The new TCP stack is enabled by default. The legacy TCP stack is not yet
  removed and can be used if needed.
* The network interface is made a kernel object. This allows better access
  control handling when usermode is enabled.
* The kernel stacks are used in network related threads to save memory when
  usermode is enabled.
* Network statistics collection can be enabled in key points of the network
  stack. This can be used to get information where time is spent in RX or TX.
* The BSD socket sendmsg() can now be used with AF_PACKET type sockets.
* Added support for enabling OpenThread reference device.
* Added support for enabling MQTT persistent sessions.
* Added "net tcp recv" command to net shell to enable TCP RX in manual testing.
* Added ObjLnk resource type support to LWM2M.
* Added userspace support to MQTT publisher, echo-server and echo-client
  sample applications.
* Added support to rejecting received and unsupported PPP options.
* Added support for select() when using socket offloading.
* Added support for IPv6 multicast packet routing.
* Added support to SOCK_DGRAM type sockets for AF_PACKET family.
* Added support for using TLS sockets when using socket offloading.
* Added additional checks in IPv6 to ensure that multicasts are only passed to the
  upper layer if the originating interface actually joined the destination
  multicast group.
* Allow user to specify TCP port number in HTTP request.
* Allow application to initialize the network config library instead of network
  stack calling initialization at startup. This enables better control of
  network resources but requires application to call net_config_init_app()
  manually.
* Allow using wildcards in CoAP resource path description.
* Allow user to specify used network interface in net-shell ping command.
* Allow user to select a custom mbedtls library in OpenThread.
* Removed dependency to :kconfig:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` from offloaded
  WiFi device drivers.
* Print more gPTP status information in gptp net shell.
* Fixed the network traffic class statistics collection.
* Fixed WiFi shell when doing a scan.
* Fixed IPv6 routes when nexthop is link local address of the connected peer.
* Fixed IPv6 Router Solicitation message handling.
* Fixed BSD socket lib and set errno to EBADF if socket descriptor is invalid.
* Fixed received DNS packet parsing.
* Fixed DNS resolving by ignoring incoming queries while we are resolving a name.
* Fixed CoAP zero length option parsing.
* Fixed gPTP port numbering to start from 1.
* Fixed gPTP BMCA priority vector calculation.
* Fixed multiple interface bound socket recv() for AF_PACKET sockets.
* Fixed PPP Term-Req and Term-Ack packet length when sending them.
* Fixed PPP ipv6cp and ipcp Configure-Rej handling.
* Fixed PPP option parsing and negotiation handling.
* Fixed PPP ipcp option handling when the protocol goes down.
* Fixed PPP ipv6cp and ipcp network address removal when connection goes down.
* Added support to rejecting received and unsupported PPP options.
* Added initial support for PAP authentication in PPP.
* Fixed a race PPP when ppp_fsm_open() was called in CLOSED state.
* Fixed LWM2M FOTA socket closing.
* Fixed LWM2M block transfer retransmissions.
* Fixed LWM2M opaque data transfer in block mode.
* Fixed LWM2M Security and Server object instance matching.
* Fixed LWM2M updating lifetime on Register Update event.
* Fixed MQTT double CONNACK event notification on server reject.

Bluetooth
*********

* Host

  * Added basic support for Isochronous Channels (also known as LE Audio).
  * Added support for Periodic Advertising (both Advertising and Scanning
    procedures).
  * The application can now specify preferences for the PHY update procedure PHY
    choices.
  * A new "bond_deleted" callback has been introduced.
  * Added a new callback for GATT (un)subscription.
  * Added support for the application to provide subscription information to the
    stack prior to reconnection (``bt_gatt_resubscribe``).
  * The application can now request for the CCC descriptor to be discovered
    automatically by the stack when subscribing to a characteristic.
  * Fixed a regression introduced in 2.3 along the EATT feature, where the ATT
    throughput could not reach the expected values.
  * Fixed a deadlock in the RX thread that was observed multiple times in
    scenarios involving high throughput and a sudden disconnection.
  * Fixed a race condition upon advertising resume.
  * The GATT notify multiple feature is now disabled by default.
  * The advertiser can now be requested to restart even when a connection
    object is not available.
  * The L2CAP security level will now be elevated automatically when a
    connection is rejected for security reasons.
  * When LE Secure Connections are the only option enabled, the security level
    will now be elevated to Level 4 automatically.
  * Fixed CCC restoring when using settings lazy loading.
  * Fixed recombination of ACL L2CAP PDUs when the header itself is split across
    multiple HCI ACL packets.
  * GATT no longer assumes the position of the CCC descriptor and instead
    discovers it.
  * Multiple additional fixes.

* Mesh

  * Added support for storage of model data in a key-value fashion.
  * Added support for a network loopback.
  * Multiple qualification-related fixes.

* BLE split software Controller

  * The advanced scheduling algorithms that were supported in the legacy
    Controller have been ported to the split one.
  * Preliminary support for Advertising Extensions, restricted to
    non-connectable advertising for now.
  * Very early support for Periodic Advertising. This should be considered an
    early experimental draft at this stage.
  * Added full support for the Nordic nRF5340 IC, not just the engineering
    sample.
  * Added support for the Nordic nRF52805 IC.
  * Several fixes to scheduling and window calculation, some of which had an
    impact in the cooperation between the flash driver and the Controller.
  * Fixed an null pointer dereference in the ticker code.

* HCI Driver

  * A new BT_QUIRK_NO_AUTO_DLE has been added for Controllers that do not follow
    the recommendation of auto-initating the data length update procedure. This
    is in fact the case of the split software Controller.

Build and Infrastructure
************************

* Improved support for additional toolchains:

  * Better toolchain abstractions.
  * Support for the ARC MetaWare toolchain.

* Devicetree

  * Added new devicetree macros that provide a default value if the property
    or cell accessor doesn't contain the requested data.

  * Added support for inferring bindings for ``/zephyr,user`` devicetree node
    to allow applications an easy way to specify application specific
    devicetree properties without having a binding.

* Support for multiple SOC and ARCH roots.
  The :ref:`SOC_ROOT <application>` and ``ARCH_ROOT`` variables used to specify
  support files for out of tree SoCs and architectures now accept multiple
  paths, separated by semicolons. As a result, the ``SOC_DIR`` Kconfig variable
  is no longer supported.

  Uses like ``source $(SOC_DIR)/<path>`` must be changed to
  ``rsource <relative>/<path>`` or similar.

* BOARD, SOC, DTS, and ARCH roots can now be specified in each module's
  :file:`zephyr/module.yml` file; see :ref:`modules_build_settings`.

Libraries / Subsystems
**********************

* Disk

* Management

  * MCUmgr

    * Moved mcumgr into its own directory.
    * UDP port switched to using kernel stack.
    * smp: added missing socket close in error path.

  * Added support for Open Supervised Device Protocol (OSDP), see :kconfig:option:`CONFIG_OSDP`.

  * updatehub

    * Added download block check.
    * Added support to flash integrity check using SHA-256 algorithm.
    * Moved updatehub from lib to subsys/mgmt directory.
    * Fixed out-of-bounds access and add flash_img_init return value check.
    * Fixed getaddrinfo resource leak.

* Settings

  * If a setting read is attempted from a channel that doesn't support reading return an error rather than faulting.
  * Disallow modifying the content of a static subtree name.

* Random

* POSIX subsystem

* Power management

* Logging

  * Fixed immediate logging with multiple backends.
  * Switched logging thread to use kernel stack.
  * Allow users to disable all shell backends at one using :kconfig:option:`CONFIG_SHELL_LOG_BACKEND`.
  * Added Spinel protocol logging backend.
  * Fixed timestamp calculation when using NEWLIB.

* LVGL

  * Library has been updated to the new major release v7.0.2.

  * It is important to note that v7 introduces multiple API changes and new
    configuration settings, so applications developed on v6 or previous versions
    will likely require some porting work. Refer to `LVGL 7 Release notes
    <https://github.com/lvgl/lvgl/releases/tag/v7.0.0>`_ for more information.

  * LVGL Kconfig option names have been aligned with LVGL. All LVGL
    configuration options ``LV_[A-Z0-9_]`` have a matching Zephyr Kconfig
    option named as ``CONFIG_LVGL_[A-Z0-9_]``.

  * LVGL Kconfig constants have been aligned with upstream suggested defaults.
    If your application relies on any of the following Kconfig defaults consider
    checking if the new values are good or they need to be adjusted:

    * :kconfig:option:`CONFIG_LVGL_HOR_RES_MAX`
    * :kconfig:option:`CONFIG_LVGL_VER_RES_MAX`
    * :kconfig:option:`CONFIG_LVGL_DPI`
    * :kconfig:option:`CONFIG_LVGL_DISP_DEF_REFR_PERIOD`
    * :kconfig:option:`CONFIG_LVGL_INDEV_DEF_READ_PERIOD`
    * :kconfig:option:`CONFIG_LVGL_INDEV_DEF_DRAG_THROW`
    * :kconfig:option:`CONFIG_LVGL_TXT_LINE_BREAK_LONG_LEN`
    * :kconfig:option:`CONFIG_LVGL_CHART_AXIS_TICK_LABEL_MAX_LEN`

  * Note that ROM usage is significantly higher on v7 for minimal
    configurations. This is in part due to new features such as the new drawing
    system. LVGL maintainers are currently investigating ways for reducing the
    library footprint when some options are not enabled, so you should wait for
    future releases if higher ROM usage is a concern for your application.

* Shell

  * Switched to use kernel stacks.
  * Fixed select command.
  * Fixed prompting dynamic commands.
  * Change behavior when more than ``CONFIG_SHELL_ARGC_MAX`` arguments are
    passed.  Before 2.3 extra arguments were joined to the last argument.
    In 2.3 extra arguments caused a fault.  Now the shell will report that
    the command cannot be processed.

* Storage

  * Added flash SHA-256 integrity check.

* Tracing

  * Tracing backed API now checks if init function exists prio to calling it.

* Debug

  * Core Dump

    * Added the ability to do core dump when fatal error is encountered.
      This allows dumping the CPU registers and memory content for offline
      debugging.
    * Cortex-M, x86, and x86-64 are supported in this release.
    * A data output backend utilizing the logging subsystem is introduced
      in this release.

HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.

Documentation
*************

Tests and Samples
*****************

  * nvs: Do full chip erase when flashing.
  * nrf: onoff_level_lighting_vnd_app: Fixed build with mcumgr.
  * drivers: flash_shell: new commands write_unaligned and write_pattern.
  * bluetooth: hci_spi: Fixed cmd_hdr and acl_hdr usage.
  * Removed zephyr nfc sample.
  * drivers: Fixed uninitialized spi_cfg in spi_fujitsu_fram sample.
  * Updated configuration for extended advertising in Bluetooth hci_uart and hci_rpmsg examples.

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.3.0 tagged
release:

* :github:`28665` - boards b_l4s5i_iot01a: invertion of user LEDS polarity
* :github:`28659` - [Coverity CID :214346] Out-of-bounds access in subsys/net/ip/tcp2.c
* :github:`28654` - [lwm2m stm32F429] No registration with server possible
* :github:`28653` - Bluetooth: Mesh: TX Power Dynamic Control
* :github:`28639` - tests: kernel: sleep: is failing for nRF51
* :github:`28638` - bq274xx sample unable to build
* :github:`28635` - nrf: qspi: devicetree opcode properties are ignored
* :github:`28628` - samples/tfm_integration/tfm_ipc: regression on nucleo_l552_ze
* :github:`28627` - tests: kernel: fatal: exception: stack_sentinel test is failing for nRF platforms
* :github:`28625` - tests: net: tcp2: llegal use of the EPSR
* :github:`28621` - tests: kernel: mem_protect: syscalls: wrong FAULTY_ADDRESS for nucleo_l073rz
* :github:`28605` - Build failure - (64-bit platforms) acrn/bcm958402m2_a72/native_posix_64/... on a number of sanitycheck tests w/TCP2
* :github:`28604` - mcumgr smp_svr sample not working over shell or serial transport
* :github:`28603` - tests: kernel: timer: timer_api: Failed on nucleo_l073rz
* :github:`28602` - TCP2:frdm_k64f/mimxrt1064_evk  tests/net/tcp2 regression failure in RC2
* :github:`28577` - possible bug / regression in new TCP stack
* :github:`28571` - Erroneous call to ull_disable_mark in ull_adv::disable()
* :github:`28565` - sensor: lsm6dsl: incompatible pointer type (warning)
* :github:`28559` - Unable to extend the flash sync API part of the BLE Controller
* :github:`28552` - up_squared: samples/portability/cmsis_rtos_v1/philosophers/ failed.
* :github:`28549` - up_squared: tests/kernel/threads/thread_apis/ failed
* :github:`28548` - up_squared:  tests/arch/x86/pagetables/ failed.
* :github:`28547` - up_squared: tests/subsys/debug/coredump failed.
* :github:`28540` - littlefs: MPU FAULT and failed to run
* :github:`28538` - Atmel SAM4L have two pinctrl with wrong map
* :github:`28492` - Could not build Zephyr application for swervolf_nexys board in simulation
* :github:`28480` - ``tests/lib/devicetree/legacy_api/libraries.devicetree.legacy`` fails to build on pinnacle_100_dvk
* :github:`28471` - Central not working properly on nRF5340-DK
* :github:`28465` - Building OpenThread NCP: build system has concurrency issue
* :github:`28460` - Generated ExternalProject include directories
* :github:`28453` - qemu 5.1 hangs on a number tests on x86_64
* :github:`28443` - drivers: sensor: hts221 compilation issue linked to DT property drdy_gpios
* :github:`28434` - Shell Tab Completion Candidates results in segmentation fault
* :github:`28414` - kernel/timeout: next_timeout() is returning negative number of ticks
* :github:`28413` - [Coverity CID :214280] Unintentional integer overflow in tests/posix/common/src/nanosleep.c
* :github:`28412` - [Coverity CID :214279] 'Constant' variable guards dead code in tests/drivers/clock_control/nrf_lf_clock_start/src/main.c
* :github:`28411` - [Coverity CID :214281] Unchecked return value in subsys/mgmt/osdp/src/osdp.c
* :github:`28397` - gcc 10.x compile warning/error for array subscript is outside the bounds in cmsis_rtos_v2/thread.c
* :github:`28394` - nanosleep test failed on ARC series targets
* :github:`28390` - drivers: sensor: lsm6dsl compilation issue when sensor defined in board (I2C) and in test (SPI)
* :github:`28385` - drivers.clock.nrf_lf_clock_start_xtal_no_wait.wait_in_thread fails on nrf9160dk_nrf9160
* :github:`28384` - Bluetooth: L2CAP: Bad CoC SDU segment handling
* :github:`28380` - drivers: peci: xec: Cannot recover PECI bus after PECI transfer fails
* :github:`28375` - gcc 10.x compile warning/error for array subscript 0 is outside the bounds in tests/bluetooth/tester/src/gap.c
* :github:`28371` - gcc 10.x compile warning/error for array subscript 0 is outside the bounds in subsys/bluetooth/mesh/prov.c
* :github:`28361` - USB audio samples fails if ASSERT=y
* :github:`28360` - drivers: nrf_802154: SWI IRQ priority is not read correctly
* :github:`28347` - Possible use-after-free of rx_msg->tx_block in kernel/mailbox.c
* :github:`28344` - cdc_acm sample with CONFIG_NO_OPTIMIZATIONS=y crashes on nrf52840 dev board
* :github:`28343` - Bluetooth peripheral sample auto disconnects "ST B_L4S5I_IOT01A Discovery kit"
* :github:`28341` - No SRAM available to link echo_server for atsamr21 with ieee802154.overlay
* :github:`28337` - Cannot flash Atmel boards using west
* :github:`28332` - What is the airspeed velocity of an unladen swallow running Zephyr?
* :github:`28331` - Shell on CDC ACM UART stopped working after PR #24873
* :github:`28326` - Sample boards nrf mesh onoff not working
* :github:`28325` - bluetooth: null pointer dereference for non-connectable extended advertising
* :github:`28324` - GATT notifications aren't working for CUD characteristics
* :github:`28319` - tests: kernel: context: fails because timer expiration is shorter than excepted
* :github:`28317` - Asymmetric nrfx spi_transceive tx/rx lengths outputs error
* :github:`28307` - Can't build bootloader/mcuboot while ``CONF_FILE`` contains multiple files.
* :github:`28305` - Device not found (SX1276 with nRF52840)
* :github:`28303` - nucleo_l4r5zi uses wrong pinmux setting
* :github:`28295` - kernel.common: lpcxpresso55s16_ns test failure
* :github:`28294` - arch.interrupt.gen_isr_table.arm_mainline: lpcxpresso55s16_ns failed
* :github:`28289` - tests: arch: arm: arm_sw_vector_relay: fails on nucleo_f091rc
* :github:`28283` - LWM2M: Invalid ACK when server is using message ID 0
* :github:`28282` - Slave host auto-initiate stalls if master does not support extended reject indications, and procedure collision occurs
* :github:`28280` - tests/kernel/tickless/tickless_concept: disco_l475_iot1 build issue
* :github:`28275` - drivers: bluetooth: hci_spi: hci driver is init before spi causing an error on device_get_binding
* :github:`28270` - Errors in the HL7800.c file
* :github:`28267` - up_squared(acrn):running tests/kernel/workq/work_queue_api/ failed
* :github:`28266` - up_squared(acrn):running tests/kernel/sched/schedule_api/ failed
* :github:`28265` - up_squared(acrn):running tests/kernel/timer/timer_api/ failed
* :github:`28264` - up_squared(acrn):running tests/kernel/timer/timer_monotonic/ failed
* :github:`28262` - up_squared(acrn):running tests/kernel/tickless/tickless_concept/ failed
* :github:`28261` - up_squared(acrn):running tests/kernel/common/ failed
* :github:`28260` - up_squared(acrn):running tests/portability/cmsis_rtos_v2/ failed
* :github:`28259` - up_squared(acrn):running tests/subsys/debug/coredump/ failed
* :github:`28258` - up_squared(acrn):running tests/drivers/counter/counter_cmos/ failed
* :github:`28256` - mimxrt1050_evk: running samples/subsys/fs/fat_fs/ failed
* :github:`28255` - mimxrt1050_evk:running samples/drivers/display/ failed
* :github:`28251` - Tests of the cmsis_dsp library fails on nrf52840dk_nrf52840 platform
* :github:`28248` - bt_gatt_notify() causes "unable to alllocate TX buffer"
* :github:`28240` - nordic spim: does not work with SPI-SDHC infrastructure
* :github:`28234` - ipv6: multicast group: wrong filtering
* :github:`28230` - "make zephyr_generated_headers" produces incorrect result (SHELL:") after recent cmake refactor
* :github:`28229` - Possible NULL dereference in subsys/net/ip/net_context.c.
* :github:`28223` - LEDs in the board nRF52840dk_nRF52840 dont work with Lora
* :github:`28218` - Possible NULL dereference in subsys/logging/log_msg.c.
* :github:`28216` - socket: send fails instead of blocking when there are no more net buffers
* :github:`28211` - "High" current drawn when ussing RTT log back-end with CONFIG_LOG_IMMEDIATE and CONFIG_LOG_BACKEND_RTT_MODE_DROP
* :github:`28206` - mimxrt685_cm33: many cases has no console output seems hangs in kernel init.
* :github:`28205` - kernel.timer.tickless: frdmk64f failure
* :github:`28203` - Cannot flash TI boards using west
* :github:`28202` - Adafruit TFT touch shield cap touch flipped sides left-to-right
* :github:`28197` - samples/net/sockets/echo_client/sample.net.sockets.echo_client.nrf_openthread fails to build
* :github:`28196` - samples/boards/intel_s1000_crb/audio/sample.board.intel_s1000_crb.audio Fails to build
* :github:`28193` - include/drivers/flash: API stands mistakenly unrestricted alignment of writes.
* :github:`28185` - Problem using SX1276 with nRF52840dk
* :github:`28184` - tests: drivers: spi: spi_loopback: fails on board nucleo_wb55rg
* :github:`28181` - MQTT not working with MOSQUITTO broker:
* :github:`28174` - [Coverity CID :214213] Improper use of negative value in tests/net/socket/af_packet/src/main.c
* :github:`28173` - [Coverity CID :214210] Side effect in assertion in tests/arch/arm/arm_interrupt/src/arm_interrupt.c
* :github:`28172` - [Coverity CID :214227] Resource leak in subsys/mgmt/hawkbit/hawkbit.c
* :github:`28171` - [Coverity CID :214224] Unsigned compared against 0 in subsys/storage/flash_map/flash_map.c
* :github:`28169` - [Coverity CID :214220] Explicit null dereferenced in subsys/mgmt/hawkbit/hawkbit.c
* :github:`28167` - [Coverity CID :214209] Dereference after null check in subsys/mgmt/osdp/src/osdp.c
* :github:`28166` - [Coverity CID :214211] Unused value in drivers/entropy/entropy_stm32.c
* :github:`28165` - [Coverity CID :214215] Out-of-bounds access in subsys/mgmt/mcumgr/smp_shell.c
* :github:`28164` - [Coverity CID :214225] Buffer not null terminated in subsys/net/lib/lwm2m/ipso_generic_sensor.c
* :github:`28163` - [Coverity CID :214223] Untrusted value as argument in subsys/net/lib/sockets/sockets_tls.c
* :github:`28162` - [Coverity CID :214221] Untrusted value as argument in subsys/net/lib/sockets/sockets_tls.c
* :github:`28161` - [Coverity CID :214219] Uninitialized scalar variable in subsys/net/lib/sockets/sockets_tls.c
* :github:`28160` - [Coverity CID :214212] Negative array index read in subsys/net/lib/dns/resolve.c
* :github:`28157` - benchmark.data_structures fails(bus error) on mimxrt1020/60/64/frdmk64f  platform
* :github:`28156` - twr_kv58f220m: libraries.cmsis_dsp.transform.cf64 test fails
* :github:`28154` - reel_board:running samples/subsys/usb/console/ failed
* :github:`28153` - reel_board: running samples/subsys/shell/fs/ failed
* :github:`28152` - frdm_k64f: running samples/subsys/canbus/canopen/ failed
* :github:`28151` - gPTP should allow user setting of priority1 and priority2 fields used in BMCA
* :github:`28150` - mec15xxevb_assy6853:running samples/boards/mec15xxevb_assy6853/power_management/ failed
* :github:`28149` - mec15xxevb_assy6853:running samples/drivers/ps2/ failed
* :github:`28148` - mec15xxevb_assy6853:running samples/drivers/espi/ failed
* :github:`28146` - mec15xxevb_assy6853:running samples/drivers/kscan/ failed
* :github:`28145` - nRF52840 Dongle cannot scan LE Coded PHY devices
* :github:`28139` - tests: benchmarks: data_structure_perf: rbtree_perf: uninitialized root struct
* :github:`28138` - No more able to flash board on windows
* :github:`28134` - mcuboot: specifying -DCONF_FILE results in failure
* :github:`28133` - using nrf52dk_nrf52832 with serial disabled
* :github:`28131` - Crash while serving large files via HTTP with TCP2
* :github:`28118` - timers strange rounding errors
* :github:`28114` - subsys: OSDP forces SERIAL=y
* :github:`28112` - timer/scheduler problem (STM32F407)
* :github:`28108` - EEPROM shell MPU Fault when performing a write command with more than 9 bytes
* :github:`28104` - sanitycheck overloaded by tests/subsys/logging/log_immediate with large -j values
* :github:`28099` - subsys: power: device implicit depends on CONFIG_SYS_POWER_MANAGEMENT
* :github:`28097` - cmake: fails to filter options for target language
* :github:`28095` - Doc: Getting Started Guide: reel board blinky gif is outdated
* :github:`28092` - Make SPI speed of SDHC card configurable
* :github:`28090` - bluetooth: build error with extended advertising
* :github:`28083` - Align MWDT and LD linker scripts
* :github:`28069` - eswifi: build failure
* :github:`28068` - Crash in USB device when turning HFXO off
* :github:`28061` - nrf52840 can't boot up after power plug in,unless it was connected to JLINKRTTVIEWER with a JTAG
* :github:`28059` - sample for sensor lps22hh is not filtered out for bare nrf52dk_nrf52832
* :github:`28057` - TCP2: client side receives EOF before all pending data is fed into it
* :github:`28053` - Eclipse broken build ability
* :github:`28052` - metairq_dispatch sample fails on nrf platforms
* :github:`28049` - nucleo_wb55rg: test/spi/spi_loopback build failure
* :github:`28045` - [mimxrt1050_evk] uart_fifo_fill only  send 1 byte
* :github:`28040` - sanitycheck reports test timeouts as "exited with 2"
* :github:`28036` - samples/drivers/flash_shell/sample.drivers.flash.shell fails to build on nucleo_wb55rg
* :github:`28033` - rand32_ctr_drbg.c fails to build
* :github:`28032` - eth_enc424j600 fails to build
* :github:`28031` - samples/subsys/mgmt/mcumgr/smp_svr/sample.mcumg.smp_svr.bt fails to build
* :github:`28020` - call k_malloc or k_mem_slab_alloc allowed or not
* :github:`28017` - tests/bluetooth/init/bluetooth.init.test_controller_dbg_ll_sw_split fails to build on a few boards
* :github:`28016` - tests/boards/intel_s1000_crb/main/boards.s1000_crb.main fails to build
* :github:`28013` - tests/misc/test_build/buildsystem.kconfig.utf8_in_values fails on faze
* :github:`28012` - tests/net/lib/mqtt_subscriber/net.mqtt.subscriber fails to build on cc3220sf_launchxl
* :github:`28006` - Module: mbedtls broken following driver instances const-ification
* :github:`28003` - Module: segger broken following driver instances const-ification
* :github:`28000` - sam_e70_xplained:Test cases run failed at tests/net/lib/dns_packet/.
* :github:`27985` - change in device initialization behavior
* :github:`27982` - TCP2: Apparent issues with client-side connections (hangs when server (apparently) closes connection).
* :github:`27964` - usb: Standard requests are not filtered.
* :github:`27963` - tests: net: socket: af_packet: failed on nucleo_f746zg
* :github:`27958` - USB: GET_STATUS(Device) is improperly handled
* :github:`27943` - tests/kernel/sched/schedule_api fails on nsim_hs_smp
* :github:`27935` - hci_uart not acknowledging data correctly / losing packets
* :github:`27934` - Tests ignore custom board config overlays
* :github:`27931` - Address resolving when eswifi is used causes MPU FAULT
* :github:`27929` - Address resolving when eswifi is used causes MPU FAULT
* :github:`27928` - Settings api hangs
* :github:`27921` - Bluetooth: Dynamic TX power is overwritten every procedure
* :github:`27915` - Samples:LoRa  send;sx126x with NRF52840dk ,no data from SPI miso
* :github:`27887` - Event counter may get out of sync when multiple events collide in ticker
* :github:`27880` - build errors for some samples/ on lpcxpresso55s69_cpu1
* :github:`27876` - TCP2: Apparent issues with server-side connections (>1 connection doesn't work properly)
* :github:`27874` - Nordic timer failures with synchronized periodic timers
* :github:`27867` - up_squared: couldn't get test result from serial of each test.
* :github:`27855` - i2c bitbanging on nrf52840
* :github:`27849` - tests: lib: cmsis_dsp: transform: malloc out of memory
* :github:`27847` - tests/lib/sprintf fails on native_posix_64
* :github:`27843` - spi_nor.c: Wrong buffers for rx_set
* :github:`27838` - [Coverity CID :212961] Side effect in assertion in tests/kernel/threads/thread_apis/src/test_threads_cancel_abort.c
* :github:`27837` - [Coverity CID :212956] Out-of-bounds access in tests/kernel/mem_protect/mem_map/src/main.c
* :github:`27836` - [Coverity CID :212960] Logically dead code in samples/net/sockets/echo_client/src/echo-client.c
* :github:`27835` - [Coverity CID :212962] Macro compares unsigned to 0 in include/sys/mem_manage.h
* :github:`27834` - [Coverity CID :212959] Macro compares unsigned to 0 in include/sys/mem_manage.h
* :github:`27833` - [Coverity CID :212958] Out-of-bounds access in arch/x86/core/x86_mmu.c
* :github:`27832` - [Coverity CID :212957] Out-of-bounds access in arch/x86/core/x86_mmu.c
* :github:`27821` - frdm_k64f:running test cases /tests/subsys/power/power_mgmt/ error
* :github:`27820` - reel_board:running failed in tests/drivers/gpio/gpio_api_1pin/
* :github:`27813` - samples without sample.yaml
* :github:`27811` - intermittent failure of tests/net/socket/select on qemu_x86
* :github:`27803` - samples: update to support new devicetree flag defaults
* :github:`27792` - Default clock settings for STM32F7 violates operating conditions
* :github:`27791` - DT_DRV_COMPAT in spi_flash_w25qxxdv.c named incorrectly
* :github:`27785` - memory domain arch implementation not correct with respect to SMP on ARC
* :github:`27783` - Add support for mbedTLS Server Name Indication (SNI) at configuration
* :github:`27771` - iotdk: cpu_stats function doesn't work as expected
* :github:`27768` - Usage fault when running with CONFIG_NO_OPTIMIZATIONS=y
* :github:`27765` - Sanitycheck: non-existing test case shows up in .xml file.
* :github:`27753` - drivers: sensor: lis2dh: compilation issue struct lis2dh_config' has no member named 'spi_conf'
* :github:`27745` - Zephyr with host stack and hci driver only ?
* :github:`27738` - em_starterkit_7d sanitycheck test failure on tests\kernel\mem_protect\syscalls test
* :github:`27734` - vl53l0x driver gives wrong offset calibration value
* :github:`27727` - mcumgr serial interface does not work with CDC_ACM UART
* :github:`27721` - Concurrent file descriptor allocations may return the same descriptor
* :github:`27718` - Updatehub might dereference an uninitialized pointer
* :github:`27712` - warnings when compiling smp_svr with newlibc on 2.3.0
* :github:`27706` - Cannot debug specific files
* :github:`27693` - Crash on ARM when BT LE scan response packet too big
* :github:`27648` - [Coverity CID :212430] Unchecked return value in tests/kernel/msgq/msgq_api/src/test_msgq_contexts.c
* :github:`27647` - [Coverity CID :212429] Negative array index write in tests/subsys/fs/fs_api/src/test_fs_dir_file.c
* :github:`27646` - [Coverity CID :212428] Unchecked return value in tests/kernel/msgq/msgq_api/src/test_msgq_contexts.c
* :github:`27645` - [Coverity CID :212424] Unchecked return value in tests/kernel/msgq/msgq_api/src/test_msgq_contexts.c
* :github:`27644` - [Coverity CID :212141] Improper use of negative value in tests/lib/fdtable/src/main.c
* :github:`27643` - [Coverity CID :212427] Invalid type in argument to printf format specifier in samples/drivers/jesd216/src/main.c
* :github:`27642` - [Coverity CID :212143] Unused value in samples/drivers/flash_shell/src/main.c
* :github:`27641` - [Coverity CID :212142] Unused value in samples/drivers/flash_shell/src/main.c
* :github:`27640` - [Coverity CID :212426] Unrecoverable parse warning in drivers/wifi/eswifi/eswifi_socket_offload.c
* :github:`27639` - [Coverity CID :212425] Out-of-bounds access in drivers/ethernet/eth_mcux.c
* :github:`27637` - Bluetooth: controller: Possible corruption in AD data
* :github:`27636` - sensor: shell float output broken w/ CONFIG_NEWLIB_LIBC=y
* :github:`27634` - wifi simple_link driver build error
* :github:`27613` - CONFIG_ASSERT not working on nrf5340dk_nrf5340_cpunet in hci_rpmsg sample
* :github:`27612` - RFC: API Change: usb: Device argument to USB HID ops
* :github:`27610` - UART_ERROR_FRAMING
* :github:`27600` - JSON Api refuse to decode null value
* :github:`27599` - bluetooth shell deadlock on USB shell UART
* :github:`27597` - build system fails to propagate devicetree change to Kconfig
* :github:`27592` - threads without name show up as junk names in SystemView
* :github:`27587` - New socket close() implementation broke build of platforms using socket offloading
* :github:`27582` - BT Identity address is overwritten when using extended advertising
* :github:`27580` - west install error
* :github:`27576` - sample.drivers.sample.drivers.peci failed to run
* :github:`27574` - mec15xxevb_assy6853:arch.arm.arch.arm.no.multithreading failed to run
* :github:`27572` - mec15xxevb_assy6853:crypto.tinycrypt.hmac_prng.hmac_prng failed to build,
* :github:`27571` - up_squared:tests/portability/cmsis_rtos_v2/thread_api failed
* :github:`27569` - mimxrt1050_evk:samples.usb.cdc-acm-composite failed
* :github:`27566` - nRF52832: MCUBoot cannot read signed SMP Server Sample binary
* :github:`27560` - APIs for dynamically creating thread stacks
* :github:`27558` - "west update" only certain vendor
* :github:`27548` - CMake and west doesn't accept multiple overlay files during build
* :github:`27547` - samples/boards/reel_board/mesh_badge fails booting with error in i2c_nrfx_twim
* :github:`27544` - TrustZone: NSC_ALIGN gets redefined
* :github:`27533` - kernel crashes with small CONFIG_TIMESLICE_SIZE
* :github:`27531` - Zephyr testing via emulators
* :github:`27529` - sanitycheck: incorrect correct calculation of total_skipped when --subset is set:
* :github:`27526` - poll(2) returning -1 errno ENOMEM
* :github:`27523` - [RFC] drivers: display: Implementing driver for sharp memory display
* :github:`27522` - shell: Output can get corrupted when printing from thread before command completes
* :github:`27511` - coverage: qemu platforms: sanitycheck generates many ``unexpected eof`` failures when enable coverage
* :github:`27505` - spi: mchp: Unintended data is transmitted when tx and rx operations are performed simultaneously
* :github:`27503` - testcases under zephyr/tests/application_development take a very long time to dump coverage data
* :github:`27495` -  Include full register state in ARM Cortex M Exception Stack Frame (ESF)
* :github:`27488` - Bluetooth Mesh samples don't build
* :github:`27482` - Bluetooth stops responding when calling k_delayed_work_submit. v2.3.0
* :github:`27473` - RT1050/60/64-evk board user LED does not work
* :github:`27465` - How recursively build boards on Zephyr?
* :github:`27464` - LOG_BACKEND_NET does not work for certain application/ip configurations
* :github:`27463` - Cannot build samples/net/sockets/echo for cc3220sf_launchxl
* :github:`27448` - fatal error: device_imx.h: No such file or directory
* :github:`27446` - Unable to flash cc1352r (no xds) with openocd in Zephyr SDK
* :github:`27444` - spi sdhc CS signal not working
* :github:`27434` - Bluetooth: L2CAP: buffer use after free
* :github:`27428` - Cannot compile network logging backend with IPv6-only
* :github:`27421` - libraries.cmsis_dsp.matrix.binary_q15: buffer allocation failure on twr_kv58f220m
* :github:`27420` - drivers.uart: config test failure on uart_mcux.c (was twr_kv58f220m platform)
* :github:`27414` - Bluetooth: Controller: First advertisement does not preempt continuous scanner
* :github:`27404` - IS_ENABLED not working with C++ (was: Is DT_INST_FOREACH_STATUS_OKAY broken on v2.3?)
* :github:`27403` - uart_fifo_read can only read one character
* :github:`27399` - [RFC] API change - Switch all struct device to constant
* :github:`27397` - [RFC] API change - Device structure attribute renaming
* :github:`27396` - samples/subsys/logging/logger timeout when sanitycheck enable coverage, it needs a filter
* :github:`27392` - tests/kernel/device/kernel.device.pm fails to build on cc1352r1_launchxl
* :github:`27380` - Cannot use mcuboot with i.MXRT1060 due to a problem with the vector table address
* :github:`27379` - Macro Z_ARC_MPU_SIZE_ALIGN seems to be missing
* :github:`27377` - up_squared(acrn):samples/philosophers/ caused the acrn platform crashed.
* :github:`27375` - "west zephyr-export" dumps stack if cmake is not installed
* :github:`27373` - CivetWeb Support for STM32H7 Series
* :github:`27370` - Constant asserts from nrf5 clock calibration
* :github:`27366` - tests: net: regression on many tests
* :github:`27363` - mec15xxevb_assy6853:kernel.device.pm failed
* :github:`27362` - cannot move to 1M baud rate in bt_shell
* :github:`27353` - west flash ignores --bin-file parameter and uses hex file when nrfjprog is used internally
* :github:`27348` - When using CONFIG_NVS it triggers BUS FAULT during startup on "nucleo_wb55rg" board
* :github:`27340` - <wrn> bt_driver: Discarding event 0x3e
* :github:`27339` - up_squared: Zephyr does not boot via grub anymore
* :github:`27338` - Bluetooth: host: GATT service request is not able to trigger the authentication procedure while in SC only mode
* :github:`27331` - Fails to upload over BLE using Zephyr with SMP Server Sample
* :github:`27330` - include in prj.conf
* :github:`27329` - [Coverity CID :211587] Unchecked return value in tests/drivers/clock_control/clock_control_api/src/test_clock_control.c
* :github:`27328` - [Coverity CID :211586] Resource leak in tests/posix/fs/src/test_fs_open_flags.c
* :github:`27327` - [Coverity CID :211585] Argument cannot be negative in tests/posix/fs/src/test_fs_open_flags.c
* :github:`27326` - [Coverity CID :211584] Logically dead code in drivers/wifi/eswifi/eswifi_core.c
* :github:`27325` - [Coverity CID :211583] Unchecked return value in drivers/wifi/eswifi/eswifi_socket.c
* :github:`27324` - [Coverity CID :211572] Out-of-bounds read in soc/xtensa/sample_controller/include/_soc_inthandlers.h
* :github:`27323` - [Coverity CID :211551] Out-of-bounds read in soc/xtensa/sample_controller/include/_soc_inthandlers.h
* :github:`27322` - [Coverity CID :211546] Out-of-bounds read in soc/xtensa/sample_controller/include/_soc_inthandlers.h
* :github:`27321` - [Coverity CID :211539] Out-of-bounds read in soc/xtensa/sample_controller/include/_soc_inthandlers.h
* :github:`27320` - [Coverity CID :211537] Out-of-bounds read in soc/xtensa/sample_controller/include/_soc_inthandlers.h
* :github:`27319` - [Coverity CID :211523] Bad bit shift operation in arch/arc/core/mpu/arc_mpu_v2_internal.h
* :github:`27318` - Decouple TLS socket from net_context
* :github:`27303` - RFC: downgrade i2c eeprom_slave driver to test
* :github:`27293` - Test nrf52840dk_nrf52840 tests/net/socket/net_mgmt/net.socket.mgmt build failure
* :github:`27288` - linker relocation feature fails for out of tree projects
* :github:`27282` - Drivers in app folder
* :github:`27280` - drivers: bluetooth: hci: spi: CS DT config not working because CS gpio_dt_flags are not set in the spi_cs_config struct
* :github:`27268` - usb: mcux RT1060 EVK - when using on-chip memory, USB fails
* :github:`27266` - samples: bluetooth: hci_spi: Invalid cmd_hdr and acl_hdr usage
* :github:`27249` - Is there any development plan for supporting RPL stack ？
* :github:`27239` - samples/subsys/canbus/isotp/sample.subsys.canbus.isotp fails on FRDM-K64F
* :github:`27238` - tests/net/socket/af_packet fails on FRDM-K64F
* :github:`27237` - Out_of_tree example broken
* :github:`27227` - shell crashes on qemu_x86 board upon the Tab button press
* :github:`27220` - Bluetooth: L2CAP: l2cap_change_security() not considering bt_conn::sec_level when handling BT_L2CAP_LE_ERR_AUTHENTICATION
* :github:`27219` - thousands of lines of log spam in buildkite output
* :github:`27212` - drivers: clock_control: stm32h7 cannot choose system frequency higher than 400MHz
* :github:`27211` - sanitycheck: add option to only build/run on emulated targets
* :github:`27205` - tests/kernel/timer/timer_api test fails on twr_ke18f
* :github:`27202` - tests/kernel/threads/thread_apis failure on lpcxpresso55s16_ns
* :github:`27181` - New drivers out of device tree
* :github:`27177` - Unable to build samples/bluetooth/st_ble_sensor for steval_fcu001v1 board
* :github:`27173` - [v2.1] Unable to build Zephyr 2.1 for Upsquared board running ACRN
* :github:`27172` - shell: logging: CONFIG_SHELL_LOG_BACKEND is forced if CONFIG_LOG is chosen
* :github:`27166` -  tests/kernel/sched/schedule_api need add ram limitaion as some platform not support
* :github:`27164` -  tests/lib/mem_alloc failed on up_squared board.
* :github:`27162` - reel_board:tests/net/ieee802154/l2 failed.
* :github:`27161` - shell:  shell_start() and shell_stop() can cause deadlock
* :github:`27154` - bt_conn_le_param_update doesn't return an error when setting the timeout >30sec, stops device from sleeping on nrf52840
* :github:`27151` - sanitycheck: samples: net: echo_server: Doesn't run all configurations from atmel_rf2xx shield
* :github:`27150` - [Coverity CID :211513] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`27149` - [Coverity CID :211508] Unchecked return value in tests/kernel/mem_protect/futex/src/main.c
* :github:`27148` - [Coverity CID :211506] Operands don't affect result in tests/drivers/clock_control/nrf_onoff_and_bt/src/main.c
* :github:`27147` - [Coverity CID :211505] Operands don't affect result in tests/drivers/clock_control/nrf_onoff_and_bt/src/main.c
* :github:`27145` - [Coverity CID :211511] Dereference after null check in subsys/net/ip/net_if.c
* :github:`27144` - [Coverity CID :211501] Explicit null dereferenced in subsys/net/ip/tcp2.c
* :github:`27143` - [Coverity CID :211512] Out-of-bounds read in drivers/wifi/eswifi/eswifi_socket_offload.c
* :github:`27142` - [Coverity CID :211509] Out-of-bounds read in drivers/wifi/eswifi/eswifi_socket_offload.c
* :github:`27141` - [Coverity CID :211507] Out-of-bounds read in drivers/wifi/eswifi/eswifi_socket_offload.c
* :github:`27140` - [Coverity CID :211504] Out-of-bounds read in drivers/wifi/eswifi/eswifi_socket_offload.c
* :github:`27139` - [Coverity CID :211503] Out-of-bounds read in drivers/wifi/eswifi/eswifi_socket_offload.c
* :github:`27138` - [Coverity CID :211502] Out-of-bounds read in drivers/wifi/eswifi/eswifi_socket_offload.c
* :github:`27130` - samples/drivers/spi_flash has no README
* :github:`27120` - exception happened when running CI
* :github:`27118` - Bluetooth: HCI: Missing implementation of hci_driver.h functions
* :github:`27112` - [v2.3.0] mcumgr fs download crashes
* :github:`27090` - LE Coded PHY scanning on nRF9160DK fails
* :github:`27081` - missing ``python3-devel`` dependency (was python3-psutil)
* :github:`27080` - uarte_instance_init() in NRF UARTE driver does not disable UART prior to setting PSEL pin values
* :github:`27079` - espi: driver: mchp:  eSPI driver indicates flash channel is ready to eSPI host even before the channel negotiation takes place
* :github:`27078` - drivers: espi: mchp: Cannot perform multiple transactions over eSPI OOB channel
* :github:`27074` - doc: coding_guidelines: broken links to MISRA-C example suite
* :github:`27071` - USB: CDC-ACM uart console hijacks usb_enable call preventing user applications from registering callbacks
* :github:`27057` - NUCLEO-H745ZI-Q add cortex-m4 ethernet support
* :github:`27056` - Local header found before system header of same name
* :github:`27055` - BlueZ with ESP32 boards supported or not?
* :github:`27037` - No network interface found when running wifi sample
* :github:`27010` - net: ieee802154: wrong header generation
* :github:`27003` - CMakeLists.txt newline check is too strict
* :github:`27002` - checkpatch.pl incorrect ERROR:POINTER_LOCATION
* :github:`26998` - [Coverity CID :211479] Unchecked return value in tests/kernel/mutex/mutex_api/src/test_mutex_apis.c
* :github:`26997` - [Coverity CID :211474] Unchecked return value in tests/kernel/mutex/mutex_api/src/test_mutex_apis.c
* :github:`26996` - [Coverity CID :211340] Side effect in assertion in tests/kernel/smp/src/main.c
* :github:`26995` - [Coverity CID :211478] Logically dead code in samples/net/sockets/big_http_download/src/big_http_download.c
* :github:`26994` - [Coverity CID :210616] Resource leak in lib/updatehub/updatehub.c
* :github:`26993` - [Coverity CID :210593] Out-of-bounds access in lib/updatehub/updatehub.c
* :github:`26992` - [Coverity CID :210547] Unchecked return value in lib/updatehub/updatehub.c
* :github:`26991` - [Coverity CID :210072] Resource leak in subsys/mgmt/smp_udp.c
* :github:`26990` - i2c transfers are timing out with SSD1306 display
* :github:`26989` - [Coverity CID :211477] Unchecked return value in subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`26988` - [Coverity CID :211473] Unchecked return value in subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`26986` - [Coverity CID :211480] Printf arg count mismatch in arch/x86/zefi/zefi.c
* :github:`26985` - [Coverity CID :211476] Extra argument to printf format specifier in arch/x86/zefi/zefi.c
* :github:`26984` - sys/device_mmio.h API design should accept generic DT node identifiers
* :github:`26983` - MPU FAULT in nRF52840-DK
* :github:`26981` - Problem with PPP + GSM MUX with SIMCOM7600E
* :github:`26970` - usb: overflow of USB transfers leads to clogging
* :github:`26966` - Example OTA-DFU for Android/IOS app
* :github:`26961` - occasional sanitycheck failures in samples/subsys/settings
* :github:`26954` - devicetree: warning: braces around scalar initializer
* :github:`26953` - settings: ISO C++ forbids converting a string constant to 'char*'
* :github:`26948` - cmake failure when using ZEPHYR_MODULES without west
* :github:`26941` - Meta-IRQ documentation references
* :github:`26939` - MCUMGR - smp shell server sends responses to wrong port
* :github:`26937` - Kconfig choice Warning
* :github:`26924` - Bluetooth: Mesh: no space to store ccc cfg
* :github:`26923` - [RFC] API change - Normalize DMA, IPM and UART callbacks signatures including the caller's device pointer.
* :github:`26919` - ipv6: promiscuous mode: packet flood over 802.15.4 adapter
* :github:`26914` - gen_kobject_list.py dosn't generate correct gperf info for ARC MetaWare toolchain
* :github:`26910` - sanitycheck always treats warnings as errors
* :github:`26900` - Bluetooth: host: bt_conn_recv() assumes ACL data is >= 2 bytes
* :github:`26896` - STM32: mcu goes to sleep inadvertently when using PM.
* :github:`26868` - qemu_x86_64 icount support with SMP
* :github:`26862` - Bluetooth: GATT: CCC is not properly stored
* :github:`26848` - kernel: undefined reference with --no-gc-sections
* :github:`26833` - RFC: subsys: fs: Support file open flags to fs and POSIX API
* :github:`26832` - [mcux_counter_rtc][frdm_k82f] counter_basic_api hangs
* :github:`26828` - Build Error - Network communication between Zephyr app on QEMU and Host OS
* :github:`26826` - i2c_nrfx_twi_transfer hangs when SDA/SCL are set to pins 0,1
* :github:`26818` - drivers: uart_console.c: usb_enable() broken
* :github:`26814` - net_ipv6_send_rs behaviour doesn't comply with RFC4291
* :github:`26812` - NXP: tests/drivers/dma/loop_transfer fails on FRDM-K64F
* :github:`26807` - Bluetooth HCI USB sample is not working
* :github:`26805` - test: drivers: i2c: i2c_slave_api:
* :github:`26804` - Bluetooth mesh repeated provision/gatt bearer connection crash
* :github:`26803` - Cortex-M7 Thumb-2 Instructions Alignment
* :github:`26801` - UART API has ifdefs around API functions
* :github:`26796` - Interrupts on Cortex-M do not work with CONFIG_MULTITHREADING=n
* :github:`26793` - kernel: work: triggers immediately with longer timeouts
* :github:`26788` - cmake build system works wrong with cmake version 3.15.5
* :github:`26782` - boards: mchp: mec15xxevb_assy6853:  Cannot set gpios as alternate function when enabling multiple instances of a driver
* :github:`26769` - "west flash -r openocd --serial <serial_num>" ignores serial_num and flashes wrong board
* :github:`26766` - Build failure on nucleo_wb55rg for tests/kernel/profiling/profiling_api/kernel.common.profiling
* :github:`26764` - Build failure on intel_s1000_crb for samples/drivers/flash_shell/sample.drivers.flash.shell
* :github:`26759` - Build error -  Nothing found at GNUARMEMB_TOOLCHAIN_PATH
* :github:`26758` - Missing documentation of report targets (ram/rom report, puncover)
* :github:`26746` - Change sanitycheck to used pickled EDT
* :github:`26731` - Single channel selection - Bluetooth - Zephyr
* :github:`26729` - FCB flash_area_write fails on nRF52840DK when using mx25r64 storage
* :github:`26725` - USB suspend-resume process is not properly handled
* :github:`26723` - NULL handler in work queue entry can be called resulting in silent reboot
* :github:`26720` - lib: sockets: getaddrinfo don't work without newlib C on ARM
* :github:`26717` - Big HTTP Download - Upgrade
* :github:`26708` - RFC: API Change: watchdog: wdt_feed error codes
* :github:`26701` - Invalid handling of large cycle count in rtc timer
* :github:`26700` - waveshare_open103z board can't build tests/mem_protect
* :github:`26695` - net: TCP2: connect() returns 0 without waiting for handshake completion
* :github:`26689` - Couldn't get test result from serial on up_squared board.
* :github:`26685` - sanitycheck "--only-failed" is broken
* :github:`26683` - Transition from non-secure to kernel causes "Stacking error"
* :github:`26679` - sanitycheck passes tests if the emulator exits unexpectedly
* :github:`26676` - MDB runner is not capturing real board's output
* :github:`26665` - Implement reset for ARC development boards
* :github:`26664` - frdm_kw41z: tests/drivers/pwm/pwm_api fails in test_pwm_cycle()
* :github:`26663` - sanitycheck reports failing tests with em_starterkit_em7d_v22 board
* :github:`26651` - Updatehub: frdm_k64f resets in a loop
* :github:`26647` - build generates unaligned function reference in v2.3
* :github:`26643` - Nucleo board Slow Code execution at power up - need to always reset
* :github:`26628` - Couldn't find Definition for CTE transmit and enable command for Connectionless AoA/AoD implementation in Zephyr
* :github:`26627` - tests/benchmarks/sys_kernel failed on up_squared.
* :github:`26626` - tests/portability/cmsis_rtos_v1 failed on reel_board.
* :github:`26625` - tests/net/utils failed on multiple arm platforms.
* :github:`26624` - Noridc52840 hci_usb bug on linux when " discoverable on " by bluetoothctl
* :github:`26621` - System can't recover after assertion failed in kernel/mem_domain.c
* :github:`26619` - tests/unit/rbtree fails
* :github:`26617` - devicetree: sam0 gclk
* :github:`26607` - STM32F0 nucleo PWM output not working
* :github:`26602` - GH Action: Automate removal of tag "Waiting for response"
* :github:`26600` - net.util test is broken on MPU-enabled ARM platforms
* :github:`26596` - west: rimage support in ``west sign`` poorly documented
* :github:`26595` - tests/kernel/obj_tracing thread counting issue with 1.14 branch.
* :github:`26587` - DT_CALL_WITH_ARG macro missing
* :github:`26586` - K_TIMER_DEFINE macro causing build error
* :github:`26582` - What happened to DT_HAS_NODE macro?
* :github:`26575` - devicetree: need save/restore support for devicetree data
* :github:`26568` - tests: net: socket: af_packet: is ethernet cable now mandatory to run this test ?
* :github:`26555` - uart: uart_nrfx_uarte: async init does not cleanup previous sync rx
* :github:`26551` - sam0 devicetree failing to compile
* :github:`26536` - The CONFIG_BT_L2CAP_RX_MTU setting is not reflected correctly in the build
* :github:`26529` - How to support Nordic ble5.0 on Android 7.0？
* :github:`26527` - mimxrt1050_evk:Couldn't flash image by using west flash command.
* :github:`26524` - Problem with hci_uart and L2CAP CoC connections
* :github:`26519` - samples: net: sockets: dumb_http_server: instabllity on nucleo_f767zi
* :github:`26518` - NRF temperature sensor driver race condition
* :github:`26509` - net_l2_ppp.ppp_link_terminated: SARA U201 modem
* :github:`26508` - CI: simulated BT tests not run if BT tests are changed
* :github:`26506` - how does hci_usb (hci_usb fw : \ncs\v1.3.0\zephyr\samples\bluetooth\hci_usb) set mac and send/receive files ?
* :github:`26505` - An example of using the microphone in Thingy 52
* :github:`26499` - usermode: random: backport random syscall
* :github:`26476` - ARM Cortex-A: architecture timer continuously firing in tick-less mode
* :github:`26467` - Bluetooth: Race-condition on persistent connectable advertiser
* :github:`26466` - Bluetooth: host: Do auto-postponement of advertising also when application requests advertising
* :github:`26455` - bme280 connect to rt1020_evk
* :github:`26450` - Bad disconnect reason when client connects with wrong address type
* :github:`26438` - Bluetooth: Reconnection to paired/bonded peripheral fails
* :github:`26435` - Suspicious source code with subsys/random/random32_entropy_device: seg fault risk
* :github:`26434` - nrf9160 uart_tx can return -ENOTSUP, which is not documented behavior
* :github:`26428` - LPSPI support for i.MX RT106x
* :github:`26427` - Linker problems with zephyr-sdk-0.11.2: undefined reference to 'gettimeofday'
* :github:`26424` - master west.yml references pull in hal_stm32
* :github:`26419` - Cannot request update when writing to external flash
* :github:`26415` - CONFIG_FS_LOG_LEVEL_OFF option doesn't work with LittleFS
* :github:`26413` - disco_l475_iot1: flash storage corruption caused by partition overlap
* :github:`26410` - RFC: soc: Initial Nuvoton NPCX port
* :github:`26407` - fs: nvs: Incorrect handling of corrupt ate's in nvs_gc
* :github:`26406` - On x86, the main stack overflows when CONFIG_NET_IPV6 and CONFIG_DEBUG are enabled
* :github:`26403` - Compile Error when trying to build samples/synchronization
* :github:`26397` - storage: flash_map: Only works on limited compatibles
* :github:`26391` - stm32f746g: sample subsys/usb/hid-cdc does not work
* :github:`26377` - Problems getting I2C to work on NXP i.MX RT1020 EVK
* :github:`26372` - qspi driver does not work if multithreading is disabled
* :github:`26369` - C++ compilation warning for Z_TIMEOUT_TICKS
* :github:`26363` - samples: subsys: canbus: canopen: objdict: CO_OD.h is not normally made.
* :github:`26362` - arc gdb failed to load core dump file
* :github:`26361` - [Coverity CID :211051] Explicit null dereferenced in tests/lib/ringbuffer/src/main.c
* :github:`26360` - [Coverity CID :211048] Side effect in assertion in tests/drivers/uart/uart_async_api/src/test_uart_async.c
* :github:`26359` - [Coverity CID :211047] Dereference null return value in tests/net/ipv6/src/main.c
* :github:`26358` - [Coverity CID :211044] Unchecked return value in tests/subsys/settings/fcb_init/src/settings_test_fcb_init.c
* :github:`26357` - [Coverity CID :211046] Unchecked return value in boards/posix/native_posix/timer_model.c
* :github:`26356` - [Coverity CID :211043] Logical vs. bitwise operator in subsys/net/lib/lwm2m/lwm2m_rw_oma_tlv.c
* :github:`26355` - [Coverity CID :211045] Macro compares unsigned to 0 in kernel/timeout.c
* :github:`26354` - [Coverity CID :211040] Macro compares unsigned to 0 in kernel/timeout.c
* :github:`26353` - [Coverity CID :211039] Out-of-bounds access in drivers/gpio/gpio_nrfx.c
* :github:`26352` - [Coverity CID :211049] Macro compares unsigned to 0 in arch/x86/core/x86_mmu.c
* :github:`26343` - Gatt Bearer Issue
* :github:`26337` - BT scan: filter duplicates yields duplicates
* :github:`26333` - Bluetooth: Split LL: Cannot store Bluetooth keys
* :github:`26313` - nucleo_h745zi_q_m7 pwm device tree bug
* :github:`26303` - Bluetooth: Windows 10 cannot reconnect on direct advertising from Zephyr
* :github:`26302` - Test gen_isr_tables from ./tests/kernel/gen_isr_table/ fails on nrf9160dk_nrf9160
* :github:`26296` - Store logs in persistent storage (ext. flash, SD card)
* :github:`26295` - Enable persistent storage (ext flash/SD card) as logger backend
* :github:`26294` - Test suite output is hard to read
* :github:`26291` - canopen: error when CAN_MCP2515_MAX_FILTER > 8
* :github:`26290` - gfhgf
* :github:`26284` - device.h doxygen
* :github:`26281` - Question: Does NRF52840-DK support both OpenThread and BLE at the same time
* :github:`26280` - test_kernel_systicks from tests/portability/cmsis_rtos_v1 fails on nrf platforms
* :github:`26279` - littlefs: Unable to erase external flash.
* :github:`26278` - [v2.2] bt_att: Unhandled ATT code 0x1d
* :github:`26271` - k_sleep/k_msleep ends too early on UP_squared board
* :github:`26267` - drivers: SPI: CS output type not honored
* :github:`26266` - Cast and shift operator priority issue may lead to wrong memory size result in fat_fs example
* :github:`26265` - Zephyr os bluetooth peripheral example indication. When i flash code to my board custom configuration for indication will shown and after i click button for indication it device will disconnect from phone. My board is nrf52832.
* :github:`26264` - tests/benchmarks/latency_measure failed on up_squared board.
* :github:`26263` - reel_board:tests/posix/common failed.
* :github:`26259` - Add AT86RF233 REB Xplained Pro Extension shield
* :github:`26256` - NRF51822 BLE Micro module: hangs on k_msleep() (RTC counter not working)
* :github:`26255` - k_uptime_ticks() returns pointer instead of value
* :github:`26252` - bluetooth: controller: Cannot receive long packets
* :github:`26248` - A timer with 24-hour timeout fires immediately
* :github:`26242` - qemu_x86 and qemu_cortex_m3 time handling broken with CONFIG_QEMU_ICOUNT
* :github:`26235` - multi vlan support networking
* :github:`26234` - Question: how can a NRF52840-DK's clock be set to 64MHz
* :github:`26232` - Segger Embedded Studio doesn't find the right python
* :github:`26220` - OpenThread L2 does not implement ``enable`` API function
* :github:`26209` - sanitycheck tries to run random *samples*, without being asked for
* :github:`26200` - BT_LE_ADV_OPT_EXT_ADV causes bt_le_adv_start to return -22
* :github:`26197` - tracking provenance of utility code
* :github:`26185` - Sample posix:eventfd fails on all platforms
* :github:`26177` - Bluetooth: Mesh: Friend node unexpected un-reference buffer
* :github:`26174` - Add STM32H7 Series Ethernet Driver Support
* :github:`26172` - Zephyr Master/Slave not conforming with Core Spec. 5.2 connection policies
* :github:`26169` - Enable -O0 for only one \*.c file
* :github:`26168` - arch-level memory-mapping interface
* :github:`26167` - Extend the sensor API with function for getting the value of a sensor attribute
* :github:`26165` - Clock not initialized in LPC Flexcomm UART driver
* :github:`26150` - storage/stream: flash_img_bytes_written() might returns more than number of payload bytes written.
* :github:`26149` - building native_posix against musl-libc
* :github:`26139` - west: nrfjprog and jlink runner leave SW-DP registers in enabled state
* :github:`26136` - CMake Error in Windows Environment
* :github:`26131` - nrf52840_mdk: add support for nrf stock bootloader
* :github:`26119` - Compilation error when enabling MPU on STM32 L0 boards
* :github:`26112` - bug: cmake loops when passing overlay file with left slashes in file path
* :github:`26107` - driver MMIO virtual address space mapping
* :github:`26106` - mcumgr: smp_bt: wrong notify MTU calculation with CONFIG_BT_GATT_NOTIFY_MULTIPLE
* :github:`26105` - Test kernel.memory_protection.stack_random fails on nrf52dk_nrf52832
* :github:`26104` - Asynchronous input via UART
* :github:`26096` - cmake finds a DTC from Zephyr-SDK version, it tries to execute it, and it fails
* :github:`26095` - Requirements.txt pip version conflict
* :github:`26080` - gPTP time sync fails if having more than one port
* :github:`26076` - bug: cortex-m0: vector table base address is set to zero when soc has control over where to put vector table.
* :github:`26071` - Bluetooth: host: ATT sent callback lost
* :github:`26070` - Bluetooth: ATT request not processed
* :github:`26065` - sanitycheck reports failing tests with timeout as passing
* :github:`26064` - tests/kernel/timer/timer_api failed on mec15xxevb_assy6853 board.
* :github:`26059` - Potentially incorrect interrupt handling in nRF SoC .dtsi for GPIO
* :github:`26049` - False multiple define of irq with IRQ_CONNECT
* :github:`26039` - tests: kernel: timer: timer_api: regression on STM32 boards
* :github:`26038` - build zephyr with llvm fail
* :github:`26037` - RFC: API Change: Bluetooth Mesh
* :github:`26034` - menuconfig target aborts when Kconfig warnings are present
* :github:`26033` - NET_SOCKETS_OFFLOAD conflicts with POSIX_API
* :github:`26030` - RV32M1_RI5CY: tests/kernel/threads/thread_apis and thread_init fails
* :github:`26021` - Problems compiling for Measuring Time
* :github:`26017` - Build error in shell with gcc 10.1 (tests/drivers/uart/uart_basic_api)
* :github:`25991` - [net][net.socket.select][imx-rt series] test fails  (k_uptime_get_32() - tstamp <= FUZZ is false)
* :github:`25990` - tests/net/socket/select failed on sam_e70_xplained board.
* :github:`25989` - STM32_LPTIM_TIMER wrongly depends on DEVICE_POWER_MANAGEMENT
* :github:`25988` - [Coverity CID :210687] Argument cannot be negative in tests/net/socket/socketpair/src/test_socketpair_happy_path.c
* :github:`25987` - [Coverity CID :210685] Pointless string comparison in tests/lib/devicetree/legacy_api/src/main.c
* :github:`25986` - [Coverity CID :210684] Explicit null dereferenced in tests/kernel/mbox/mbox_api/src/test_mbox_api.c
* :github:`25985` - [Coverity CID :210683] Pointless string comparison in tests/lib/devicetree/legacy_api/src/main.c
* :github:`25984` - [Coverity CID :210686] Unchecked return value in lib/os/mempool.c
* :github:`25983` - [Coverity CID :210682] Unchecked return value in lib/os/mempool.c
* :github:`25982` - [Coverity CID :210020] Explicit null dereferenced in drivers/usb/device/usb_dc_mcux_ehci.c
* :github:`25981` - Support storing mcuboot images on serial flash accessed through Nordic QSPI
* :github:`25979` - Need root LICENSE files in hal_stm32 module
* :github:`25965` - hci_uart not responding at higher baudrates on NRF52810
* :github:`25964` - Bluetooth: <err> bt_att: ATT Timeout
* :github:`25958` - Concept Overview for improving support for serial flash devices via SPI and QSPI
* :github:`25956` - Including header files from modules into app
* :github:`25952` - STM32 LPTIM driver doesn't restart counter after sleeping K_TICKS_FOREVER
* :github:`25945` - devicetree: support generating symbols for -gpios properties w/o compatible
* :github:`25942` - Bluetooth: Scanning + Non-connectable advertising broken on nRF5340
* :github:`25926` - k_cycle_get_32() returns 0 in native_posix
* :github:`25920` - Compilation error when CONFIG_BOOTLOADER_MCUBOOT=y specified
* :github:`25919` - dhcpv4 or rx ethernet packets not working on nucleo_f429zi
* :github:`25892` - arc emsdp board work wrong with emsdp_em7d_esp config
* :github:`25869` - 2.3: Missing release notes
* :github:`25865` - Device Tree Memory Layout
* :github:`25859` - mesh example not working with switched off dcdc?
* :github:`25853` - modem_ublox_sara_r4: Cannot connect to UDP remote
* :github:`25833` - [lpcxpresso55s69_cpu1] no applications and build guide, hello world can not build
* :github:`25827` - Devicetree: add accessors with defaults
* :github:`25794` - [Coverity CID :210554] Uninitialized scalar variable in tests/net/iface/src/main.c
* :github:`25792` - [Coverity CID :210552] Resource leak in tests/net/pm/src/main.c
* :github:`25790` - [Coverity CID :210594] Dereference after null check in subsys/testsuite/ztest/src/ztest_mock.c
* :github:`25786` - [Coverity CID :210558] Dereference before null check in drivers/sensor/sensor_shell.c
* :github:`25784` - [Coverity CID :210546] Dereference after null check in tests/net/promiscuous/src/main.c
* :github:`25783` - [Coverity CID :210051] Dereference after null check in subsys/net/ip/tcp2.c
* :github:`25782` - [Coverity CID :210035] Dereference before null check in drivers/sensor/bq274xx/bq274xx.c
* :github:`25781` - [Coverity CID :210031] Dereference before null check in drivers/modem/gsm_ppp.c
* :github:`25778` - [Coverity CID :210604] Out-of-bounds access in tests/kernel/mem_protect/protection/src/main.c
* :github:`25777` - [Coverity CID :210589] Out-of-bounds access in tests/kernel/mem_protect/protection/src/main.c
* :github:`25776` - [Coverity CID :210573] Out-of-bounds access in tests/kernel/mem_protect/userspace/src/main.c
* :github:`25750` - [Coverity CID :210066] Unintentional integer overflow in include/sys/time_units.h
* :github:`25749` - [Coverity CID :210033] Unintentional integer overflow in drivers/sensor/mpr/mpr.c
* :github:`25748` - [Coverity CID :210606] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`25747` - [Coverity CID :210596] Assign instead of compare in subsys/logging/log_output_syst.c
* :github:`25746` - [Coverity CID :210584] Assign instead of compare in subsys/logging/log_output_syst.c
* :github:`25745` - [Coverity CID :210052] Side effect in assertion in tests/kernel/fpu_sharing/generic/src/pi.c
* :github:`25744` - [Coverity CID :210045] Side effect in assertion in tests/kernel/fpu_sharing/generic/src/pi.c
* :github:`25743` - [Coverity CID :209944] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`25742` - [Coverity CID :209943] Pointless string comparison in tests/lib/devicetree/src/main.c
* :github:`25741` - [Coverity CID :210618] Unchecked return value in drivers/wifi/esp/esp.c
* :github:`25740` - [Coverity CID :210617] Argument cannot be negative in tests/net/pm/src/main.c
* :github:`25739` - [Coverity CID :210610] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`25738` - [Coverity CID :210602] Unchecked return value in tests/drivers/uart/uart_basic_api/src/test_uart_fifo.c
* :github:`25735` - [Coverity CID :210582] Unchecked return value in tests/net/socket/getaddrinfo/src/main.c
* :github:`25734` - [Coverity CID :210580] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`25733` - [Coverity CID :210575] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`25732` - [Coverity CID :210570] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`25729` - [Coverity CID :210056] Unchecked return value in subsys/net/ip/tcp2.c
* :github:`25728` - [Coverity CID :210050] Unchecked return value in tests/subsys/settings/littlefs/src/settings_setup_littlefs.c
* :github:`25726` - [Coverity CID :210598] Missing break in switch in subsys/net/l2/ieee802154/ieee802154_frame.c
* :github:`25725` - [Coverity CID :210578] Structurally dead code in kernel/mem_domain.c
* :github:`25724` - [Coverity CID :210566] Missing break in switch in subsys/net/l2/ieee802154/ieee802154_frame.c
* :github:`25723` - [Coverity CID :210559] Unsigned compared against 0 in subsys/net/ip/tcp2.c
* :github:`25722` - [Coverity CID :210058] Logically dead code in samples/net/sockets/big_http_download/src/big_http_download.c
* :github:`25721` - [Coverity CID :209945] Logically dead code in tests/net/tcp2/src/main.c
* :github:`25720` - [Coverity CID :210073] Arguments in wrong order in drivers/modem/wncm14a2a.c
* :github:`25713` - Miss shift i2c slave address in i2c_sifive
* :github:`25710` - FS: Buffer Overflow when enabling Long File Names in FAT_FS and calling fs_stat
* :github:`25704` - lib: updatehub: Corrupted updated when receiving CoAP duplicate packages
* :github:`25693` - ESP WiFi MPU Fault causes zephyr fatal error
* :github:`25682` - [v2.2] Shell freezes with cout printf, prink on float
* :github:`25678` - enhance k_mutex to be ISR safe
* :github:`25672` - Bluetooth: Mesh: scan_start fails with synchronous bt_enable
* :github:`25664` - nRF Boards: unify static partition size for Bootloader
* :github:`25658` - Issue to run sample on nucleo_g474re
* :github:`25652` - smp_svr fails for nrf5340
* :github:`25645` - USB RNDIS driver can't work with Windows 10 (10.0.18363)
* :github:`25601` - UART input does not work on mps2_an{385,521}
* :github:`25599` - scanf() not functional with newlib out of the box
* :github:`25566` - LSPI of NXP i.MX RT Other interrupts treated as transfer completion
* :github:`25554` - lib: posix: nanosleep
* :github:`25501` - shields: mikroe_eth_click config should be made conditional
* :github:`25499` - Out of tree board: No sources given to target
* :github:`25474` - ipv6 client-server between ble's failed
* :github:`25458` - Multiple issues with timing benchmark
* :github:`25453` - tests/posix/common fails on nucleo_wb55rg
* :github:`25444` - No IPv6 routes from BLE IPSP node (NRF52840DK)
* :github:`25398` - UpSquared Grub build docs don't work on Ubuntu 20.04
* :github:`25358` - net: config: application starts with 3s delay when CONFIG_NET_CONFIG_SETTINGS=y
* :github:`25328` - mesh_demo is failing
* :github:`25327` - Move to C99 integer types and deprecate zephyr specific types
* :github:`25317` - RFC: Unstable API Change: uart_async: Call UART_RX_RDY event after rx_disable()
* :github:`25312` - samples:mimxrt1010_evk:samples/net/openthread/ncp: build error
* :github:`25311` - samples:frdmkw64f:bluetooth/peripheral_hr| peripheral_ht: could not get ADC device
* :github:`25308` - I2C simulation in native_posix
* :github:`25299` - SYSTICK: Inconsistency between dts status and Kconfig
* :github:`25295` - sanitycheck: race when running sanitycheck on native_posix producing false negatives.
* :github:`25294` - Nordic mcuboot + smp_svr + QSPI smp_shell incompatibility
* :github:`25293` - Add USB Device Support to STM32411E-DISCO
* :github:`25283` - sam0: watchdog: Times out twice as fast as expected
* :github:`25268` - sanitycheck doesn't report native_posix failures properly
* :github:`25258` - drivers: i2c_nios2: device config_info content mutated
* :github:`25257` - drivers: audio: dma_nios2_msgdma: device config_info content mutated
* :github:`25256` - drivers: audio: tlv320dac310x: device config_info content mutated
* :github:`25255` - drivers: i2c: gecko: device config_info content mutated
* :github:`25231` - net.offload test fails on atsame54_xpro
* :github:`25229` - net.neighbour test fails on atsame54_xpro
* :github:`25228` - net.util test fails on atsame54_xpro
* :github:`25227` - net.icmpv6 test fails on atsame54_xpro
* :github:`25226` - net.vlan test fails on atsame54_xpro
* :github:`25215` - enable modules to append to $DTS_ROOT
* :github:`25189` - Wrong flash size set in the XIP boot header for NXP imxrt SoCs
* :github:`25171` - Can only run the flash_simulator test once on native_posix
* :github:`25165` - LE Coded Phy code rate switch [s2/s8]
* :github:`25156` - Unable to use --use-elf option in 'west flash' to correctly flash the .elf file
* :github:`25148` - tests: gpio: Add check to validate initial values of gpio output
* :github:`25140` - Unable to obtain dhcp lease
* :github:`25104` - whitelist in {sample,testcase}.yaml precludes a test from being run with sanitycheck
* :github:`25101` - driver: gpio: mchp: GPIO initialization value doesn't get reflected when using new flags
* :github:`25098` - MCUX I2C bus errors leave state machine in busy state
* :github:`25076` - Remove potential I2C deadlock on NRFX implementation
* :github:`25063` - USB Console + USB CDC_ACM co-existing
* :github:`25051` - tests/drivers/gpio/gpio_api_1pin failed on reel_board.
* :github:`25022` - hsdk:There is no case’s information in serial log for ARC(R) HS Development Kit after one case was been flashed into the board.
* :github:`25021` - Problems getting open62541 to run on Zephyr
* :github:`24960` - The example "blinky" didn't work on MIMXRT1050-EVK
* :github:`24939` - LSPI of NXP i.MX RT timing delay issue
* :github:`24918` - Segger RTT using j-link doesn't work on NXP i.MX RT
* :github:`24916` - echo_client sample return: Cannot connect to TCP remote (IPv6): 60 (frdm_k64f <--> native_posix)
* :github:`24910` - kernel: stack sentinel crashes
* :github:`24859` - os: Add memory partition overlap assert check is not made for x86 boards
* :github:`24844` - Setting esp-idf path to match Espressif's documentation
* :github:`24770` - Low throughput with the zperf sample using stm32f746g_disco
* :github:`24767` - Ethernet support for STM32H747
* :github:`24750` - need API to get list of succeed initialization device or add initialization status flag in struct device
* :github:`24747` - tests/lib/heap fails on ARC nsim_sem nsim_em
* :github:`24745` - Mitigate changes in peripheral enable state after Kconfig replaced by DT status
* :github:`24730` - C standard library <time.h> functions and structures not available when using POSIX API
* :github:`24703` - hal_nuvoton: Add new module for Nuvoton numicro M480 HAL layer
* :github:`24700` - mimxrt1050_evk:tests/drivers/kscan/kscan_api failed.
* :github:`24632` - Devices vs. drivers
* :github:`24627` - tests/subsys/usb/device fails on SAM E54
* :github:`24625` - lib: drivers: clock: rtc:  rtc api to maintain calendar time through reboot
* :github:`24619` - CONFIG_USERSPACE=y CONFIG_XIP=n causes .bin space to be wasted
* :github:`24546` - Implement MDB runner for ARC
* :github:`24499` - devicetree:  node name for SPI buses should be 'spi' warning
* :github:`24429` - LPC55S69 flash faults when reading unwritten areas
* :github:`24372` - Json: array of objects is not properly handled
* :github:`24318` - Postpone driver initialization
* :github:`24301` - Support for multi core STM32 H75/H77 boards
* :github:`24300` - tests/net/trickle failed on frdm_k64f and sam_e70_xplained with v1.14 branch.
* :github:`24293` - subsys: shell: bug: shell_fprintf() before shell_enable() causes shell deadlock
* :github:`24233` - adxl362_trigger.c adxl362_init_interrupt function :const struct adxl362_config \*cfg  not found gpio_cs_port
* :github:`24224` - Possible uninitialized variable in zephyr\subsys\logging\log_msg.c
* :github:`24221` - Do not run cron workflow on forks
* :github:`24217` - Shell: provide mechanism to call any command while in select command
* :github:`24191` - obj_tracing: Local IPC variables are not removed from obj tracing list after function return
* :github:`24147` - nrf5340 pdk: BOARDS_ENABLE_CPUNET does not allow proper NET MCU configuration
* :github:`24134` - [NXP i.MX RT Flash]: evkmimxrt1020 does not boot with a new flash chip
* :github:`24133` - Question: Context save/restore after deep sleep using device driver
* :github:`24111` - drivers: flash: littlefs: add sync to flash API & update LittleFS to use it
* :github:`24092` - Unable to change recv() buffer size in frdm_k64f board.
* :github:`24076` - [v1.14] UARTE high current consumption on NRF
* :github:`24030` - [Coverity CID :209379] Unchecked return value in tests/kernel/mem_protect/sys_sem/src/main.c
* :github:`24029` - [Coverity CID :209380] Unchecked return value in tests/kernel/poll/src/test_poll.c
* :github:`24028` - [Coverity CID :209381] Unrecoverable parse warning in include/bluetooth/bluetooth.h
* :github:`23961` - CCC does not get cleared when CONFIG_BT_KEYS_OVERWRITE_OLDEST is enabled
* :github:`23949` - Question: Is there any example for BR/EDR profile/protocols (like A2DP, AVDTP, RFCOMM)?
* :github:`23887` - drivers: modem: question: Should modem stack include headers to put into zephyr/include?
* :github:`23886` - drivers: modem_socket: Question:  socket ID appears to be the same for all sockets
* :github:`23873` - GNA subsystem does not provide any system calls
* :github:`23825` - edtlib.py fails to find bindings when DTS_ROOT is a relative path
* :github:`23808` - ARM bus fault with code coverage
* :github:`23802` - up_squared(acrn):tests/kernel/timer/timer_api/ failed.
* :github:`23801` - up_squared(acrn):tests/kernel/sched/schedule_api failed.
* :github:`23800` - tests/drivers/counter/counter_cmos failed on up_squared platform
* :github:`23775` - k_poll() documentation is wrong or unclear
* :github:`23713` - CMake integration with libmetal errors-out with the bleeding edge CMake release
* :github:`23702` - STACK_POINTER_RANDOM is not working on ARM for the main thread
* :github:`23672` - dts: sam0: question: Is it possible to clean up samd.dtsi devicetree warning?
* :github:`23629` - support inverted PWM on STM32
* :github:`23599` - zephyr/samples/application_development/code_relocation execution stop at z_arm_bus_fault
* :github:`23578` - [Coverity CID :208922] Uninitialized pointer read in tests/posix/common/src/pthread.c
* :github:`23574` - [Coverity CID :208926] Side effect in assertion in tests/kernel/interrupt/src/nested_irq.c
* :github:`23546` - Kconfig: default value not assigned when inheriting Kconfig values in range
* :github:`23514` - Allocate executable memory for ESP32
* :github:`23474` - tests/subsys/usb/device failed on reel_board.
* :github:`23443` - esp32 needs i2c_transfer call  to turn on the display
* :github:`23423` - Mitigation in case system [created] threads hang/non-responsive
* :github:`23419` - posix: clock: No thread safety clock_[get/set]time
* :github:`23366` - ARM: Core Stack Improvements/Bug fixes for 2.3 release
* :github:`23364` - Bluetooth: bt_recv deadlock on supervision timeout with pending GATT Write Commands
* :github:`23349` - Question: How to add external soc, board, DTS, drivers and libs?
* :github:`23322` - flash, spi-nor: Configuration of jedec spi nor flash device driver
* :github:`23319` - hci interface stopped working after few hours/days
* :github:`23248` - Add secure version of strcpy
* :github:`23246` - net: tx_bufs are not freed when NET_TCP_BACKLOG_SIZE is too high
* :github:`23243` - test/kernel/gen_isr_table fails in v2.2.0-rc3 on lpcxpresso54114_m4 board
* :github:`23215` - fujitsu FRAM read error on stm32_olimexino
* :github:`23211` - need a proper arch_system_halt() for x86_64
* :github:`23178` - usb: endpoint buffer leak upon SET_CONFIGURATION, SET_INTERFACE
* :github:`23177` - Bluetooth: Mesh: Access structure member with a possible NULL pointer
* :github:`23149` - [v1.14] sam_e70_xplained:tests/drivers/watchdog/wdt_basic_api failed with v1.14 branch.
* :github:`23139` - USB Mass storage - Unexpected USB restart from host
* :github:`23138` - Codegen for an C structure that stores pinmux definitions
* :github:`23134` - BT: Host: Notification dropped instead of truncated if bigger than ATT_MTU-3
* :github:`23111` - drivers:usb:device:sam0:  Descriptor tables are filled with zeros in attach()
* :github:`23052` - nrf52840_pca10056: Spurious RTS pulse and incorrect line level with hardware flow control disabled
* :github:`23040` - samples: net/wifi: net ping shell causes USAGE FAULT once wifi is connected
* :github:`23039` - SystemView does not work with C++ enabled
* :github:`22996` - scripts/footprint/size_report doesn't work on qemu_x86_64
* :github:`22980` - bluetooth: logging: Build assertion prevents immediate logging when using legacy LL
* :github:`22975` - tests/kernel/gen_isr_table: filtered in CI only for Cortex-M Mainline
* :github:`22974` - Add cancel function to onoff service
* :github:`22955` - tests/kernel/interrupt fails intermittently on qemu_cortex_m0
* :github:`22906` - Slow read/write speed of microSD card via SPI and FatFS
* :github:`22892` - Kconfig warning when serial disable on PCA10059
* :github:`22873` - Bluetooth: RSSI Read command can be configured out even when mandatory
* :github:`22872` - Hello world application for mps2_an521 board when build as a secure/non-secure with Trusted Firmware is crashing on qemu
* :github:`22865` - drivers: enc28j60: sample: dumb_http_server: TX failed errors
* :github:`22758` - RFC: Require system clock stability on startup
* :github:`22751` - STM32F407 I2C driver hangs
* :github:`22722` - posix: redefinition of symbols while porting zeromq to zephyr
* :github:`22704` - Implement watchdog driver for lpcxpresso55s69
* :github:`22637` - 2.3 Release Checklist
* :github:`22594` - NXP S32K144 MCU support
* :github:`22562` - West: Allow configuring ``west sign`` similar to west runners
* :github:`22466` - Add hx711 sensor
* :github:`22391` - Resuming from suspend should check device usage count in device idle PM
* :github:`22344` - convert espi sample to devicetree
* :github:`22340` - Security problem with settings and littlefs
* :github:`22322` - Clang linking error
* :github:`22301` - k_msgq_put() semantics definition
* :github:`22151` - hal_nordic: nrfx: doxygen: Reference to missing nrfx/templates
* :github:`22145` - RISCV arch_irq_connect_dynamic() broken with PLIC interrupts
* :github:`22144` - arch: arm64: interrupt test is failing
* :github:`22140` - Exiting deep sleep without button help; nrf52832
* :github:`22091` - Blink-Led example doesn't build on Nucleo_L476RG, STM32F4_DISCOVERY, Nucleo_F302R8, Nucleo_F401RE
* :github:`22077` - W25Q32fv supported in spi_flash examples ?
* :github:`22063` - fs/NVS: NVS is not compatible with flash memories which have 0x00 as erased
* :github:`22060` - Build fails with gcc-arm-none-eabi-9-2019-q4-major
* :github:`21994` - Bluetooth: controller: split: Fix procedure complete event generation
* :github:`21848` - sanitycheck duplicate tests Testing/Ztest
* :github:`21843` - CONFIG_INIT_STACKS issue on x86_64
* :github:`21819` - Shell fails when dynamic command has empty subcommand
* :github:`21801` - Logger sample's performance estimates are incorrect
* :github:`21798` - Bluetooth: host: Allow GATT client to restore subscription info without resubscribing
* :github:`21772` - Adding I2C devices to device tree with the same address on different busses generates excessive warnings.
* :github:`21762` - [v1.14] stm32: k_sleep() actual sleep times are different than its input
* :github:`21754` - Arduino Due shell does not accept input (UART0)
* :github:`21725` - device power management by device
* :github:`21711` - sam0 i2c slave
* :github:`21708` - Multiple partitions for LittleFS
* :github:`21707` - Timing violation for all sensor drivers
* :github:`21670` - Keep device structures in ROM
* :github:`21635` - sht3xd error -5 on olimexino_stm32
* :github:`21616` - LWM2M: unable to get plain text from incoming message
* :github:`21611` - IS25LP032D-JNLE Flash support
* :github:`21554` - ldscript:datas section is not properly aligned in ROM
* :github:`21549` - i2c_sam0 interrupt latency is excessive
* :github:`21511` - re-visit k_thread_abort wrt SMP
* :github:`21498` - Zephyr Peripheral not responding to Terminate command from central
* :github:`21455` - driver: subsys: sdhc: USAGE FAULT trace and no cs control
* :github:`21452` - drivers: ethernet: unify the initiaization
* :github:`21445` - drivers/i2c: add I2C slave support for nrfx
* :github:`21436` - refactor and augment CPU cache management APIs
* :github:`21399` - NUCLEO-H745ZI-Q Support
* :github:`21378` - The program cannot be downloaded to nrf52840, only to pca10056
* :github:`21240` - Error west flash
* :github:`21234` - drivers: usb_dc_sam0: usb detach and reattach does not work
* :github:`21233` - i2c_sam0 driver does not execute a STOP condition
* :github:`21232` - i2c_sam0 LOWTOUT is not functional
* :github:`21229` - cc1plus: warning: '-Werror=' argument '-Werror=implicit-int' is not valid for C++
* :github:`21187` - Can not ping or run http server via ethernet when gPTP is enabled
* :github:`21114` - Invalid interaction between the RTC and the I2C drivers for the sam0
* :github:`21111` - Reschedule points are currently undocumented
* :github:`21092` - i2c-sam0 sleeps waiting for interrupt
* :github:`21053` - net: 6lo: Use context 0 as default when CID-bit is not set
* :github:`21016` - Unexpected ethernet network traffic after power up
* :github:`20987` - Console showing frequent usb warnings:  <wrn> usb_device: Failed to write endpoint buffer 0x82
* :github:`20978` - Add bond_deleted callback
* :github:`20870` - [Coverity CID :205816] Control flow issues in subsys/settings/src/settings_file.c
* :github:`20844` - [Coverity CID :205781] Integer handling issues in lib/os/printk.c
* :github:`20806` - nrf: clock control: clock control on/off routines are refcounted
* :github:`20780` - Feature Request: Half-duplex UART shell backend
* :github:`20750` - shell: shell_execute_cmd introduce new line
* :github:`20734` - Are cooperative threads cooperative in SMP?
* :github:`20729` - Coverage reporting hangs for C++ tests on X86 qemu
* :github:`20712` - nRF clock_control_on() is nonblocking
* :github:`20693` - tests: watchdog: test_wdt_callback_1() implementation vs API specification
* :github:`20687` - Clarification: How to enable on-board nor-flash following the board porting guide?
* :github:`20671` - ARC: remove scheduler code from arch layer
* :github:`20663` - kernel objects are being included always, regardless of usage
* :github:`20595` - tests/arch/arm/arm_thread_swap failed on frdm_k64f board.
* :github:`20589` - RV32M1 SPI loopback needs DEBUG_OPTIMIZATIONS
* :github:`20541` - [Coverity CID :205639]Security best practices violations in /tests/subsys/settings/functional/src/settings_basic_test.c
* :github:`20520` - [Coverity CID :205652]Memory - corruptions in /tests/crypto/tinycrypt/src/ecc_dsa.c
* :github:`20519` - [Coverity CID :205616]Memory - corruptions in /tests/crypto/tinycrypt/src/ecc_dsa.c
* :github:`20517` - [Coverity CID :205640]Control flow issues in /subsys/testsuite/ztest/src/ztest.c
* :github:`20516` - [Coverity CID :205609]Control flow issues in /subsys/testsuite/ztest/src/ztest.c
* :github:`20500` - [Coverity CID :205629]Control flow issues in /drivers/timer/cc13x2_cc26x2_rtc_timer.c
* :github:`20418` - CONFIG_HEAP_MEM_POOL_SIZE should not be limited
* :github:`20297` - Bluetooth: can't close bt_driver log output
* :github:`20012` - Support peripheral deallocation at runtime
* :github:`19824` - Build sample net app for ACRN (nuc i7dnhe)
* :github:`19739` - stty: standard input: Inappropriate ioctl for device
* :github:`19701` - mem_pool_threadsafe sporadic failures impacting CI
* :github:`19684` - doc: [message_queues.rst] unclear about data_item structure type
* :github:`19670` - samples/drivers/spi_fujitsu_fram crashs due to uninitialized variables
* :github:`19661` - missing files in xtensa/xt-sim doc
* :github:`19550` - drivers/pcie: ``pcie_get_mbar()`` should return a ``void *`` not ``u32_t``
* :github:`19483` - Add support for Open Supervised Device Protocol (OSDP)
* :github:`19414` - UART and prf not working
* :github:`19376` - Build on a ARM host
* :github:`19348` - net: TCP/IPv6 set of fragmented packets causes Zephyr QEMU to double free
* :github:`19063` - can we increase qemu_riscv32/64 RAM sizes
* :github:`18960` - [Coverity CID :203908]Error handling issues in /lib/libc/newlib/libc-hooks.c
* :github:`18843` - Usage Fault with CONFIG_NO_OPTIMIZATIONS even on samples/hello_world
* :github:`18815` - UART API documentation
* :github:`18629` - Some tests fail to reach test_main() on cc1352r1_launchxl
* :github:`18570` - Dynamic interrupt does not work with multi-level interrupts
* :github:`18345` - Is there a way to get the bytes that shell receives?
* :github:`18157` - adding an offset to the zephyr code via dts overlay breaks linking + the image size changes
* :github:`18045` - BT Host: Advertising Extensions - Periodic Advertisement
* :github:`17814` - Zephyr support for NXP i.MX8M SoC
* :github:`17688` - Unable to Read data from SCC811
* :github:`17624` - SRAM size configurations aren't always consistent
* :github:`17372` - sanitycheck does not parse extra_args with spaces correctly
* :github:`16968` - silabs/gecko/emlib/src/em_gpio.c:111:35: warning: ?: using integer constants in boolean context [-Werror=int-in-bool-context]
* :github:`16886` - Bluetooth Mesh: Receive segmented message multiple times
* :github:`16809` - TCP2 integration
* :github:`16790` - adxl362 sample isn't build by sanitycheck
* :github:`16661` - Symmetric multiprocessing (SMP) for ARC HS cores
* :github:`16638` - Filesystem API is missing fs_open() flags
* :github:`16439` - flash: unify read alignment requirements
* :github:`16387` - STM32wb55 bluetooth samples fail
* :github:`16363` - Error building x_nucleo_iks01a1 sample on nucleo_wb55rg after activating I2C Bus
* :github:`16210` - ARM: initialization sequence might be not using all of interrupt stack
* :github:`16031` - Toolchain abstraction
* :github:`15968` - rom_report very imprecise
* :github:`15845` - _RESET_VECTOR different from 0x00 gives invalid .elf size on nios2
* :github:`15286` - HF clock's m16src_grd and BLE stack
* :github:`15246` - doc: confusion about dtc version
* :github:`14591` - Infineon Tricore architecture support
* :github:`14587` - IPv6 support in cc3220sf_launchxl
* :github:`14520` - invalid locking in shell
* :github:`14302` - USB MSC fails USB3CV tests
* :github:`14269` - Enforce usage of K_THREAD_STACK_SIZEOF macro in k_thread_create()
* :github:`14173` - Configure QEMU to run independent of the host clock
* :github:`13819` - mimxrt10xx: Wrong I2C transfer status
* :github:`13813` - Test suite mslab_threadsafe fails randomly
* :github:`13737` - Where can I find tutorial to make my own device driver for a device under I2C bus?
* :github:`13651` - ARC does not set thread->stack_info correctly
* :github:`13637` - Introduce supervisor-only stack declaration macros
* :github:`13276` - Do we need to update fatfs
* :github:`12987` - Fix workaround of using 'mmio-sram' compat for system memory (DRAM) in DTS
* :github:`12935` - Zephyr usurps "STRINGIFY" define
* :github:`12705` - Implement select() call for socket offloading and SimpleLink driver
* :github:`12025` - OS Pwr Manager doesn't put nrf52 into LPS_1
* :github:`11976` - APIs that support a callback should provide both the device pointer and a generic pointer
* :github:`11974` - rework eeprom driver to clearly indicate it is a test stub
* :github:`11908` - Power Manager does not handle K_FOREVER properly
* :github:`11890` - Reimplement getaddrinfo() to call SlNetUtil_getaddrinfo() in new SimpleLink SDK v 2.30+
* :github:`10628` - tests/kernel/common and tests/posix/fs crash on ESP32
* :github:`10436` - Mess with ssize_t, off_t definitions
* :github:`9893` - MISRA C Review switch statement usage
* :github:`9808` - remove single thread support
* :github:`9596` - tests/subsys/logging/log_core fails on ESP32 with no console output
* :github:`8469` - Zephyr types incompatibilities (e.g. u32_t vs uint32_t)
* :github:`8364` - mcumgr: unable to properly read big files
* :github:`8360` - CI should enforce that extract_dts_includes.py does not trigger warnings
* :github:`8262` - [Bluetooth] MPU FAULT on sdu_recv
* :github:`8257` - Unify TICKLESS_IDLE & TICKLESS_KERNEL
* :github:`7951` - doc: naming convention for requirements ids
* :github:`7385` - i2c_esp32 can write past checked buffer length
* :github:`6783` - Clean up wiki.zephyrproject.org content
* :github:`6184` - drivers: ISR-friendly driver APIs
* :github:`5934` - esp32: Output frequency is different from that configured on I2C and PWM drivers
* :github:`5443` - Deprecate # CONFIG`_ is not set
* :github:`4404` - Align k_poll with waiters
* :github:`3423` - Optimize MCUX shim drivers to reduce memory footprint
* :github:`3180` - implement direct interrupts on ARC
* :github:`3165` - xtensa: switch to clang-based frontend
* :github:`3066` - Improve Multi Core support
* :github:`2955` - Use interrupt-driven TX in hci_uart sample
