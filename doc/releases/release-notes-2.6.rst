:orphan:

.. _zephyr_2.6:

Zephyr 2.6.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr RTOS version 2.6.0.

Major enhancements with this release include:

* Added support for 64-bit ARCv3
* Split ARM32 and ARM64, ARM64 is now a top-level architecture
* Added initial support for Arm v8.1-m and Cortex-M55
* Removed legacy TCP stack support which was deprecated in 2.4

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Known issues
************

You can check all currently known issues by listing them using the GitHub
interface and listing all issues with the `bug label
<https://github.com/zephyrproject-rtos/zephyr/issues?q=is%3Aissue+is%3Aopen+label%3Abug>`_.

API Changes
***********

* Driver APIs now return ``-ENOSYS`` if optional functions are not implemented.
  If the feature is not supported by the hardware ``-ENOTSUP`` will be returned.
  Formerly ``-ENOTSUP`` was returned for both failure modes, meaning this change
  may require existing code that tests only for that value to be changed.

* The :c:func:`wait_for_usb_dfu` function now accepts a ``k_timeout_t`` argument instead of
  using the ``CONFIG_USB_DFU_WAIT_DELAY_MS`` macro.

* Added disconnect reason to the :c:func:`disconnected` callback of :c:struct:`bt_iso_chan_ops`.

* Align error handling of :c:func:bt_l2cap_chan_send and
  :c:func:bt_iso_chan_send so when an error occur the buffer is not unref.

* Added c:func:`lwm2m_engine_delete_obj_inst` function to the LwM2M library API.

Deprecated in this release

* :c:macro:`DT_CLOCKS_LABEL_BY_IDX`, :c:macro:`DT_CLOCKS_LABEL_BY_NAME`,
  :c:macro:`DT_CLOCKS_LABEL`, :c:macro:`DT_INST_CLOCKS_LABEL_BY_IDX`,
  :c:macro:`DT_INST_CLOCKS_LABEL_BY_NAME`, and
  :c:macro:`DT_INST_CLOCKS_LABEL` was deprecated in favor of utilizing
  :c:macro:`DT_CLOCKS_CTLR` and variants.

* :c:macro:`DT_PWMS_LABEL_BY_IDX`, :c:macro:`DT_PWMS_LABEL_BY_NAME`,
  :c:macro:`DT_PWMS_LABEL`, :c:macro:`DT_INST_PWMS_LABEL_BY_IDX`,
  :c:macro:`DT_INST_PWMS_LABEL_BY_NAME`, and
  :c:macro:`DT_INST_PWMS_LABEL` was deprecated in favor of utilizing
  :c:macro:`DT_PWMS_CTLR` and variants.

* :c:macro:`DT_IO_CHANNELS_LABEL_BY_IDX`,
  :c:macro:`DT_IO_CHANNELS_LABEL_BY_NAME`,
  :c:macro:`DT_IO_CHANNELS_LABEL`,
  :c:macro:`DT_INST_IO_CHANNELS_LABEL_BY_IDX`,
  :c:macro:`DT_INST_IO_CHANNELS_LABEL_BY_NAME`, and
  :c:macro:`DT_INST_IO_CHANNELS_LABEL` were deprecated in favor of utilizing
  :c:macro:`DT_IO_CHANNELS_CTLR` and variants.

* :c:macro:`DT_DMAS_LABEL_BY_IDX`,
  :c:macro:`DT_DMAS_LABEL_BY_NAME`,
  :c:macro:`DT_INST_DMAS_LABEL_BY_IDX`, and
  :c:macro:`DT_INST_DMAS_LABEL_BY_NAME` were deprecated in favor of utilizing
  :c:macro:`DT_DMAS_CTLR` and variants.

* USB HID specific macros in ``<include/usb/class/usb_hid.h>`` are deprecated
  in favor of new common HID macros defined in ``<include/usb/class/hid.h>``.

* USB HID Kconfig option USB_HID_PROTOCOL_CODE is deprecated.
  USB_HID_PROTOCOL_CODE does not allow to set boot protocol code for specific
  HID device. USB HID API function usb_hid_set_proto_code() can be used instead.

* USB HID class API is changed by removing get_protocol/set_protocol and
  get_idle/set_idle callbacks. These callbacks are redundant or do not provide
  any additional value and have led to incorrect usage of HID class API.

* The ``CONFIG_OPENOCD_SUPPORT`` Kconfig option has been deprecated in favor
  of ``CONFIG_DEBUG_THREAD_INFO``.

* Disk drivers (``disk_access_*.c``) are moved to ``drivers/disk`` and renamed
  according to their function. Driver's Kconfig options are revised and renamed.
  SDMMC host controller drivers are selected when the corresponding node
  in devicetree is enabled. Following application relevant Kconfig options
  are renamed: ``CONFIG_DISK_ACCESS_RAM`` -> `CONFIG_DISK_DRIVER_RAM`,
  ``CONFIG_DISK_ACCESS_FLASH`` -> `CONFIG_DISK_DRIVER_FLASH`,
  ``CONFIG_DISK_ACCESS_SDHC`` -> `CONFIG_DISK_DRIVER_SDMMC`.
  Disk API header ``<include/disk/disk_access.h>`` is deprecated in favor of
  ``<include/storage/disk_access.h>``.

* :c:func:`flash_write_protection_set()`.

* The ``CONFIG_NET_CONTEXT_TIMESTAMP`` is removed as it was only able to work
  with transmitted data. The same functionality can be achieved by setting
  ``CONFIG_NET_PKT_RXTIME_STATS`` and ``CONFIG_NET_PKT_TXTIME_STATS`` options.
  These options are also able to calculate the RX & TX times more accurately.
  This means that support for the SO_TIMESTAMPING socket option is also removed
  as it was used by the removed config option.

* The device power management (PM) APIs and data structures have been renamed
  from ``device_pm_*`` to ``pm_device_*`` since they are not device APIs but PM
  subsystem APIs. The same applies to enumerations and definitions, they now
  follow the ``PM_DEVICE_*`` convention. Some other API calls such as
  ``device_set_power_state`` and ``device_get_power_state`` have been renamed to
  ``pm_device_state_set`` and ``pm_device_state_get`` in order to align with
  the naming of other device PM APIs.

* The following functions, macros, and structures related to the kernel
  work queue API:

  * :c:func:`k_work_pending()` replace with :c:func:`k_work_is_pending()`
  * :c:func:`k_work_q_start()` replace with :c:func:`k_work_queue_start()`
  * :c:struct:`k_delayed_work` replace with :c:struct:`k_work_delayable`
  * :c:func:`k_delayed_work_init()` replace with
    :c:func:`k_work_init_delayable`
  * :c:func:`k_delayed_work_submit_to_queue()` replace with
    :c:func:`k_work_schedule_for_queue()` or
    :c:func:`k_work_reschedule_for_queue()`
  * :c:func:`k_delayed_work_submit()` replace with :c:func:`k_work_schedule()`
    or :c:func:`k_work_reschedule()`
  * :c:func:`k_delayed_work_pending()` replace with
    :c:func:`k_work_delayable_is_pending()`
  * :c:func:`k_delayed_work_cancel()` replace with
    :c:func:`k_work_cancel_delayable()`
  * :c:func:`k_delayed_work_remaining_get()` replace with
    :c:func:`k_work_delayable_remaining_get()`
  * :c:func:`k_delayed_work_expires_ticks()` replace with
    :c:func:`k_work_delayable_expires_get()`
  * :c:func:`k_delayed_work_remaining_ticks()` replace with
    :c:func:`k_work_delayable_remaining_get()`
  * :c:macro:`K_DELAYED_WORK_DEFINE` replace with
    :c:macro:`K_WORK_DELAYABLE_DEFINE`

==========================

Removed APIs in this release

* Removed support for the old zephyr integer typedefs (u8_t, u16_t, etc...).

* Removed support for k_mem_domain_destroy and k_mem_domain_remove_thread

* Removed support for counter_read and counter_get_max_relative_alarm

* Removed support for device_list_get

============================

Stable API changes in this release
==================================

Kernel
******

* Added :c:func:`k_mem_unmap()` so anonymous memory mapped via :c:func:`k_mem_map()`
  can be unmapped and virtual address reclaimed.

* Added the ability to gather more statistics for demand paging, including execution
  time histograms for eviction algorithms and backing stores.

Architectures
*************

* ARC

  * Added new ARCv3 64bit ISA support and corresponding HS6x processor support
  * Hardened SMP support
  * Various minor fixes/improvements for ARC MWDT toolchain infrastructure
  * Refactor of ARC Kconfig

* ARM

  * AARCH32

    * Added support for null pointer dereferencing detection in Cortex-M.
    * Added initial support for Arm v8.1-m and Cortex-M55.
    * Added support for preempting threads while they are performing secure calls in Cortex-M.
    * Added support for memory region generation by the linker based on device tree node information in Cortex-M.
    * Cleaned up definitions of SoC-specific memory regions in the common Cortex-M linker script.
    * Added support for clearing NXP MPU region configuration during Zephyr early boot stage.
    * Disallowed fpu hard ABI when building Non-Secure applications with TF-M on Cortex-M33.

  * AARCH64

    * SMP support

    * MMU dynamic mappings with page table sharing.

    * Userspace (unprivileged) thread support.

    * Standalone SMCCC support.

    * XIP support.

    * ARM64 is now a top-level standalone architecture.

    * Support for Cortex-R82 and Armv8-R AArch64 MPU.

    * Cache management support.

    * Revamped boot code.

    * Full FPU context switching.

* POSIX

* RISC-V

* x86

  * Added SoC configuration for Lakemont SoC.

  * Removed kconfig ``CONFIG_CPU_MINUTEIA`` as there is no user of this option.

  * Renamed kconfig ``CONFIG_SSE*`` to ``CONFIG_X86_SSE*``.

  * Extended the pagetable generation script to allow specifying additional
    memory mapping during build.

  * x86-32

    * Added support for kernel image to reside in virtual address space, allowing
      code execution and data manipulation via virtual addresses.

Bluetooth
*********

* Audio

  * Split up ISO handling from audio handling, and moved the latter to its
    own directory.
  * Added the Volume Offset Control Service and client.
  * Added the Audio Input Control Service and client.
  * Added the Volume Control Service and client.

* Host

  * Added basic support for Direction Finding.
  * Added support for CTE connectionless transimission and reception over
    periodic advertising.
  * Refactored the HCI and ECC handling implementations.
  * Stopped auto updating the device name in the adv data.
  * Added support for logging security keys to be used by an air sniffer.
  * Fixed a bonding issue where the local bond data was not being updated after
    the remote device had removed it when the peer was not using the IRK stored
    in the bonding information.
  * Implemented the directory listing object to OTS.
  * Added a function to access the ``bt_conn_iso`` object.
  * Added a new configuration option for writeable device name.
  * Added a new configuration option for minimum encryption key size.
  * Added support for keypress notifications as an SMP responder.
  * Added a new option, ``BT_LE_ADV_OPT_FORCE_NAME_IN_AD``, which forces the
    device name to appear in the adv packet instead of the scan response.
  * Added a security level check when sending a notification or indication.
  * Added the ability to send HCI monitor traces over RTT.
  * Refactored the Bluetooth buffer configuration for simplicity. See the
    commit message of 6483e12a8ac4f495b28279a6b84014f633b0d374 for more info.
  * Added support for concurrent advertising with multiple identities.
  * Changed the logic to disable scanning before setting the random address.
  * Fixed a crash where an ATT timeout occurred on a disconnected ATT channel.
  * Changed the pairing procedure to fail pairing when both sides have the same
    public key.
  * Fixed an issue where GATT requests could deadlock the RX thread.
  * Fixed an issue where a fixed passkey that was previously set could not be
    cleared.
  * Fixed an issue where callbacks for "security changed" and "pairing failed"
    were not always called.
  * Changed the pairing procedure to fail early if the remote device could not
    reach the required security level.
  * Fixed an issue where GATT notifications and Writes Without Response could
    be sent out of order.
  * Changed buffer ownership of ``bt_l2cap_chan_send``.
    The application must now release the buffer for all returned errors.

* Mesh

  * Added CDB handle key refresh phase.
  * Added the ability to perform replay checks on SeqAuth.
  * Added the sending of a Link Close message when closing a link.
  * Added a Proxy callback structure for Node ID enabling and disabling.
  * Added a check for the response address in the Configuration Client.
  * Introduced a new acknowledged messages API.
  * Reworked the periodic publication timer and poll timeout scheduling logic.
  * Added reporting configured ``LPNTimeout`` in ``cfg_srv``.
  * Ensured that provisioning output count number is at least 1.
  * Ensured to encrypt initial friend poll with friend credentials.
  * Stopped resetting the PB ADV reliable timer on retransmission.

* Bluetooth LE split software Controller

  * Removed support for the nRF5340 PDK. Use the nRF5340 DK instead.
  * Added basic support for Direction Finding.
  * Added support for CTE connectionless transimission and reception over
    periodic advertising.
  * Added support for antenna switching in the context of Direction Finding.
  * Added an invalid ACL data length check.
  * Added basic support for the ISO Adaptation Layer.
  * Added experimental support for Broadcast Isochronous Groups and Streams.
  * Added partial experimental support for Connected Isochronous Groups and
    Streams.
  * Implemented extended connection creation and cancellation.
  * Changed the policy to ignore connection requests from an already-connected
    peer.
  * Added a control procedure locking system.
  * Added GPIO PA/LNA support for the Nordic nRF53x SoC series.
  * Added FEM support for the nRF21540 IC.
  * Added a new radio API to configure the CTE RX path.

* HCI Driver

  * Added support for the Espressif ESP32 platform.

Boards & SoC Support
********************

* Added support for these SoC series:

  * STM32F205xx
  * STM32G03yxx, STM32G05yxx, STM32G070xx and STM32G0byxx
  * STM32G4x1, STM32G4x3 and STM32G484xE
  * STM32WL55xx
  * Nuvoton npcx7m6fc, and npcx7m6fc
  * Renesas RCar Gen3
  * Silicon Labs EFR32FG13P
  * ARM MPS3-AN547
  * ARM FVP-AEMv8A
  * ARM FVP-AEMv8R
  * NXP LS1046A
  * X86 Lakemont

* Removed support for these SoC series:

   * ARM Musca-A

* Made these changes in other SoC series:

  * Added Cypress PSoC-6 pinctrl support.
  * STM32 L4/L5/WB series were updated for better power management support (CONFIG_PM=y).
  * Backup SRAM added on a selection of STM32 series (F2/F4/F7/H7)
  * Set TRACE_MODE to asynchronous and enable trace output pin on STM32 SoCs

* Changes for ARC boards:

  * Added nSIM and QEMU simulation boards (nsim_hs6x and qemu_arc_hs6x)
    with ARCv3 64bit HS6x processors
  * Enabled MPU on qemu_arc_hs and qemu_arc_em boards
  * Added cy8c95xx GPIO expander support to HSDK board

* Added support for these ARM boards:

   * Actinius Icarus
   * Actinius Icarus SoM
   * Laird Connectivity BL654 Sensor Board
   * Laird Connectivity Sentrius BT6x0 Sensor
   * EFR32 Radio BRD4255A Board
   * MPS3-AN547
   * RAK4631
   * Renesas R-Car H3ULCB
   * Ronoth LoDev (based on AcSIP S76S / STM32L073)
   * nRF9160 Thing Plus
   * ST Nucleo F030R8
   * ST Nucleo G0B1RE
   * ST Nucleo H753ZI
   * ST Nucleo L412RP-P
   * ST Nucleo WL55JC
   * ST STM32G071B Discovery
   * Thingy:53
   * u-blox EVK-BMD-30/35: BMD-300-EVAL, BMD-301-EVAL, and BMD-350-EVAL
   * u-blox EVK-BMD-330: BMD-330-EVAL
   * u-blox EVK-BMD-34/38: BMD-340-EVAL and BMD-341-EVAL
   * u-blox EVK-BMD-34/38: BMD-345-EVAL
   * u-blox EVK-BMD-360: BMD-360-EVAL
   * u-blox EVK-BMD-34/48: BMD-380-EVAL
   * u-blox EVK-ANNA-B11x
   * u-blox EVK NINA-B11x
   * u-blox EVK-NINA-B3
   * u-blox EVK NINA-B40x

* Added support for these ARM64 boards:

   * fvp_base_revc_2xaemv8a
   * fvp_baser_aemv8r
   * nxp_ls1046ardb

* Removed support for these ARM boards:

   * ARM V2M Musca-A
   * Nordic nRF5340 PDK

* Made these changes in other boards:

  * cy8ckit_062_ble: Refactored to configure by pinctrl.
  * cy8ckit_062_ble: Added support to SCB[uart] with interrupt.
  * cy8ckit_062_ble: Added support to SCB[spi].
  * cy8ckit_062_ble: Added board revision schema.
  * cy8ckit_062_wifi_bt: Refactored to configure by pinctrl.
  * cy8ckit_062_wifi_bt: Added support to SCB[uart] with interrupt.
  * lpcxpresso55s16: Board renamed from lpcxpresso55s16_ns to
    lpcxpresso55s16 since the board does not have Trusted Firmware M
    (TF-M) support.
  * lpcxpresso55s28: Removed lpcxpresso55s28_ns config since the board
    does not have Trusted Firmware M (TF-M) support.

* Added support for these following shields:

  * FTDI VM800C Embedded Video Engine Board
  * Generic ST7735R Display Shield
  * NXP FRDM-STBC-AGM01
  * Semtech SX1272MB2DAS LoRa Shield

Drivers and Sensors
*******************

* ADC

  * Added support on TI CC32xx.
  * Added support on ITE IT8xxx2.
  * Added support for DMA and HW triggers in the MCUX ADC16 driver.
  * Added ADC emulator.
  * Moved definitions of ADC acquisition time macros so that those macros can be used in dts files.

* Bluetooth

  * The Kconfig option ``CONFIG_BT_CTLR_TO_HOST_UART_DEV_NAME`` was removed.
    Use the :ref:`zephyr,bt-c2h-uart chosen node <devicetree-chosen-nodes>`
    directly instead.

* CAN

  * A driver for CAN-FD based on the Bosch M_CAN IP was added. The driver
    currently supports STM32G4 series MCUs. Additional support for Microchip
    SAM and NXP chips is in progress.

  * The CAN ISO-TP subsystem was enhanced to allow padding and fixed
    addressing.

* Clock Control

  * On STM32 series, system clock configuration has been moved from Kconfig to DTS.
    Usage of existing Kconfig dedicated symbols (CONFIG_CLOCK_STM32_FOO) is now
    deprecated.

* Console

  * Added ``UART_CONSOLE_INPUT_EXPIRED`` and ``UART_CONSOLE_INPUT_EXPIRED_TIMEOUT``
    Kconfig options to notify the power management module that UART console is
    in use now and forbid it to enter low-power states.

* Counter

   * Added support for ESP32 Counter

* DAC

   * Added support for Microchip MCP4725

* Disk

  * Added SDMMC support on STM32L4+

* Display

  * Added support for ST7735R

* DMA

  * Added support on STM32G0 and STM32H7

* EEPROM

* ESPI

  * Added support for Microchip eSPI SAF

* Ethernet

  * Added simulated PTP clock to e1000 Ethernet controller. This allows simple PTP
    clock testing with Qemu.
  * Separated PTP clock from gPTP support in mcux and gmac drivers. This allows
    application to use PTP clock without enabling gPTP support.
  * Converted clock control to use DEVICE_DT_GET in mcux driver.
  * Changed to allow changing MAC address in gmac driver.
  * Driver for STM32H7 is now using specific memory layout to fit DMA constraints
    for RAM accesses.

* Flash

  * flash_write_protection_set() has been deprecated and will be removed in
    Zephyr 2.8. Responsibility for write/erase protection management has been
    moved to the driver-specific implementation of the flash_write() and
    flash_erase() API calls. All in-tree flash drivers have been updated,
    and the protect implementation removed from their API tables.
    During the deprecation period user code invoking
    flash_write_protection_set() will have no effect, but the flash_write() and
    flash_erase() driver shims will wrap their calls with calls to the protect
    implementation if it is present in the API table.
    Out-of-tree drivers must be updated before the wrapping in the shims is
    removed when the deprecation period ends.
  * Added QSPI support on STM32F7.

* GPIO

  * :c:struct:`gpio_dt_spec`: a new structure which makes it more convenient to
    access GPIO configuration in the :ref:`devicetree <dt-guide>`.
  * New macros for initializing ``gpio_dt_spec`` values:
    :c:macro:`GPIO_DT_SPEC_GET_BY_IDX`, :c:macro:`GPIO_DT_SPEC_GET_BY_IDX_OR`,
    :c:macro:`GPIO_DT_SPEC_GET`, :c:macro:`GPIO_DT_SPEC_GET_OR`,
    :c:macro:`GPIO_DT_SPEC_INST_GET_BY_IDX`,
    :c:macro:`GPIO_DT_SPEC_INST_GET_BY_IDX_OR`,
    :c:macro:`GPIO_DT_SPEC_INST_GET`, and :c:macro:`GPIO_DT_SPEC_INST_GET_OR`
  * New helper functions for using ``gpio_dt_spec`` values:
    :c:func:`gpio_pin_configure_dt`, :c:func:`gpio_pin_interrupt_configure_dt`
  * Remove support for ``GPIO_INT_*`` flags in :c:func:`gpio_pin_configure()`.
    The feature has been deprecated in the Zephyr 2.2 release. The interrupt
    flags are now accepted by :c:func:`gpio_pin_interrupt_configure()`
    function only.
  * STM32 GPIO driver now supports clock gating using PM_DEVICE and PM_DEVICE_RUNTIME

* Hardware Info

  * Added support on Silicon Labs Gecko SoCs

* I2C

  * Added support on STM32F2

* I2S

  * Added support for NXP LPC devices

* IEEE 802.15.4

  * Fixed various issues in IEEE 802.15.4 L2 driver.

  * nrf5:

    * Made HW Radio Capabilities runtime.
    * Enabled CSMA-CA on serialized host.
    * Changed driver to load EUI64 from UICR.

  * rf2xx:

    * Added support for tx mode direct.
    * Added support for tx mode CCA.
    * Added support to enable promiscuous mode.
    * Added support to enable pan coordinator mode.

* Interrupt Controller

* LED

  * Add support for LED GPIO
  * Added power management support for LED PWM

* LoRa

  * Added support for SX1272

* Modem

  * Converted wncm14a2a, quectel-bg9x, hl7800 and ublox-sara-r4 drivers to use
    new DT device macros.
  * Changed GSM modem to optionally do a factory reset when booting.
  * Added autostarting support to GSM modem.
  * Added wait for RDY instead of polling AT in BG9X.
  * Fixed PDP context management for BG9X.
  * Added TLS offload support to ublox-sara-r4.
  * Made reset pin optional in ublox-sara-r4.
  * Fixed potential buffer overrun in hl7800.
  * Fixed build errors on 64-bit platforms.
  * Added support for dialup modem in PPP driver.

* Pinmux

* PWM

  * Added support on STM32F2 and STM32L1.
  * Added support on Silicon Labs Gecko SoCs.

* Sensor

  * Added support for STM32 internal (CPU) temperature sensor

* Serial

  * Extended Cypress PSoC-6 SCB[uart] driver to support interrupts.

* SPI

  * Added Cypress PSoC-6 SCB[spi] driver.
  * Default SPI_SCK configuration is now pull-down for all STM32 to minimize power
    consumption in stop mode.

* Timer

* USB

  * Added support on STM32H7

* Watchdog

* WiFi

  * Converted eswifi and esp drivers to new DT device macros

  * esp:

    * Fixed hostname configuration.
    * Removed POSIX API dependency.
    * Renamed offloading driver from esp to esp_at.
    * Added esp32 wifi driver support.

Networking
**********

* CoAP:

  * Fixed coap_find_options() to return 0 when options are empty.

* DHCPv4:

  * Fixed DHCPv4 dependency to network event management options.

* DNS:

  * Added locking to DNS library prevent concurrent access.
  * Added 10ms delay when rescheduling query timeout handler in DNS. This allows
    applications to run and handle the timeout gracefully.
  * Added support for reconfiguring DNS resolver when DNS servers are changed.
    This is supported by DHCPv4 and PPP.

* HTTP:

  * Added support for storing numeric HTTP error code in client API.

* IPv4:

  * Added IGMPv2 support to IPv4.
  * Removed IPv4 multicast address check when selecting source address during TX.

* LwM2M:

  * Fixed query buffer size so that it is large enough to encode all query strings.
  * Added data validation callback.
  * Fixed Register/Update to use link_format writer.
  * Added application/link-format content writer.
  * Removed .well-known/core handling.
  * Introduced attribute handling helper functions.
  * Removed obsolete LWM2M_IPSO_TIMESTAMP_EXTENSIONS option.
  * Added IPSO Buzzer, Push Button, On/Off Switch, Accelerometer, Pressure Sensor,
    Humidity, Generic Sensor and Temperature object implementation to support
    object model in version 1.1
  * Unified reusable resources creation.
  * Added support for object versioning.
  * Changed to allow cancel-observe to match path.
  * Made pmin and pmax attributes optional.
  * Added API function to delete object instance.
  * Fixed Registration Update send on object creation.
  * Changed to only parse TLV from the first block.
  * Changed to trigger registration update only when registered.

* Misc:

  * Added UDP packet sending support to net-shell.
  * Fixed source network interface setting when sending and when there are
    multiple network interfaces.
  * Changed connection managed to ignore not used network interfaces.
  * Added locking to network interface API function calls.
  * Changed to allow application to disable IPv4 or IPv6 support for a network interface.
  * Added support for virtual network interfaces.
  * Added support for IPv4/v6 tunneling network interface.
  * Added net events notification for PPP dead and running states.
  * Added PPP LCP MRU option support.
  * Added PPP IPCP IP and DNS address peer options support.
  * Added support for network packet capturing and sending data to external system
    for analysis.
  * Enabled running without TX or RX threads. By default, one RX thread and
    no TX thread is created. If userspace support is enabled, then one RX and one
    TX thread are created. This improves the network transmit latency when a
    packet is sent from application.
  * Changed to push highest priority net_pkt directly to driver when sending and if
    there is at least one TX thread.
  * Changed to use k_fifo instead of k_work in RX and TX processing. This prevents
    k_work from accessing already freed net_pkt struct. This also improves the latency
    of network packets when the data is passed between different network threads.
  * Changed to check network interface status when sending and return ENETDOWN to the
    application if data cannot be sent.
  * Fixed echo-server sample application and set netmask properly when VLAN is
    enabled.

* OpenThread:

  * Added microseconds timer API support.
  * Changed to switch radio off when stopping diagnostics.
  * Enabled CSL delayed transmissions.
  * Added CSL transmitter and receiver API support.
  * Changed to init NCP after USB communication is established.
  * Aligned with the new NCP API.
  * Aligned with the new CLI API.
  * Introduced new OpenThread options.
  * Added Link Metrics API support.
  * Selected ECDSA when SRP is enabled.
  * Made child related options only visible on FTD.
  * Changed OT shell not to execute OT commands when shell is not ready.

* Socket:

  * Added SO_PROTOCOL and SO_TYPE get socket option.
  * Added MSG_WAITALL receive socket option flag.
  * Added MSG_TRUNC socket option flag.
  * Added support for close method for packet sockets.
  * Added locking to socket API function calls.
  * Added support for SO_BINDTODEVICE socket option.
  * Added support for SO_SNDTIMEO socket option.
  * Made NET_SOCKETS_POSIX_NAMES be on by default. This allows application to use
    normal BSD socket API calls without adding the zsock prefix.
  * Added sample application to use SO_TXTIME socket option.

* TCP:

  * Implemented ISN calculation according to RFC6528 in TCP. This is optional and
    enabled by default, and can be disabled if needed.
  * Removed legacy TCP stack support.
  * Changed TCP to use private work queue in order not to block system work queue.

* TLS:

  * Fixed userspace access to TLS socket.
  * Added socket option support for setting and getting DTLS handshake timeout.

Bluetooth
*********

* Host

* Mesh

* BLE split software Controller

* HCI Driver

Build and Infrastructure
************************

* Improved support for additional toolchains:
  * Support for the Intel oneApi toolchain.

* Devicetree

  - :c:macro:`DT_COMPAT_GET_ANY_STATUS_OKAY`: new macro
  - the ``96b-lscon-3v3`` and ``96b-lscon-1v8`` :ref:`compatible properties
    <dt-important-props>` now have ``linaro,`` vendor prefixes, i.e. they are
    now respectively :dtcompatible:`linaro,96b-lscon-3v3` and
    :dtcompatible:`linaro,96b-lscon-1v8`.

    This change was made to bring Zephyr's devicetrees into compliance with an
    upstream Linux regular expression used to validate compatible properties.
    This regular expression requires a letter as the first character.

* West

  * Improve bossac runner.  Added legacy mode option into extended SAM-BA
    bootloader selection.  This extends compatibility between Zephyr and
    some Arduino IDE bootloaders.


Libraries / Subsystems
**********************

* Disk

* Management

  * MCUmgr

  * updatehub

* Settings

* Random

* POSIX subsystem

* Power management

  * ``device_pm_control_nop`` has been removed in favor of ``NULL`` when device
    PM is not supported by a device. In order to make transition easier for
    out-of-tree users a macro with the same name is provided as an alias to
    ``NULL``. The macro is flagged as deprecated to make users aware of the
    change.

* Logging

* LVGL

* Shell

* Storage

* Task Watchdog

  * This new subsystem was added with this release and allows supervision of
    individual threads. It is based on a regularly updated kernel timer,
    whose ISR is never actually called in regular system operation.

    An existing hardware watchdog can be used as an optional fallback if the
    task watchdog itself gets stuck.

* Tracing

  * ``CONFIG_TRACING_CPU_STATS`` was removed in favor of
    ``CONFIG_THREAD_RUNTIME_STATS`` which provides per thread statistics. The
    same functionality is also available when Thread analyzer is enabled with
    the runtime statistics enabled.
  * Expanded and overhauled tracing hooks with more coverage and support for
    tracing all kernel objects and basic power management operations.
  * Added support for Percepio Tracealyzer, a commercial tracing tool which now
    has built-in support for Zephyr. We now have the Percepio tracerecorder
    integrated as a module.
  * Expanded support for the new hooks in SEGGER Systemview and enhanced the
    description file with support for all APIs.

* Debug
  * SEGGER Systemview and RTT SDK updated to version v3.30

* OS

  * Reboot functionality has been moved to ``subsys/os`` from ``subsys/power``.
    A consequence of this movement is that the ``<power/reboot.h>`` header has
    been moved to ``<sys/reboot.h>``. ``<power/reboot.h>`` is still provided
    for compatibility, but it will produce a warning to inform users of the
    relocation.

HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.


Trusted Firmware-m
******************

* Synchronized Trusted-Firmware-M module to the upstream v1.3.0 release.
* Configured QEMU to run Zephyr samples and tests in CI on mps2_an521_nonsecure
  (Cortex-M33 Non-Secure) with TF-M as the secure firmware component.
* Added Kconfig options for selecting the desired TF-M profile and build type
* Added Kconfig options for enabling the desired TF-M secure partitions
* Added a new sample to run the PSA tests with Zephyr
* Added a new sample to run the TF-M regression tests using the Zephyr build system
* Added support for new platforms

   * BL5340 DVK
   * STM32L562E DK

* NOTE: Trusted-Firmware-M can not currently be used with mbedtls 2.26.0 when
  PSA APIs are enabled in mbedtls (``MBEDTLS_USE_PSA_CRYPTO`` and
  ``MBEDTLS_PSA_CRYPTO_C``). If both TF-M and mbedtls are required, mbedtls
  must be used without the PSA APIs. This will be resolved in a future
  update to mbedtls.

Documentation
*************

* Documentation look and feel has been improved by using a new stylesheet.
* Doxygen is now run by Sphinx using the ``doxyrunner`` custom extension. The
  new extension centralizes multiple scattered workarounds that existed before
  in a single place.
* Doxygen now runs with ``WARN_AS_ERROR`` enabled.
* Documentation known warnings are now filtered using a custom Sphinx extension:
  ``warnings_filter``. This extension removes the need of post-processing
  the Sphinx output and allows to use the ``-W`` option (treat warnings as
  errors) which has been enabled by default.
* External content, e.g. samples and boards documentation is now handled by
  the ``external_content`` extension.
* Sphinx is now run in parallel mode by default (``-j auto``).
* The documentation helper ``Makefile`` has been moved from the repository root
  to the ``doc`` folder.

Tests and Samples
*****************

* Twister's ``dt_compat_enabled_with_alias()`` test case filter was deprecated
  in favor of a new ``dt_enabled_alias_with_parent_compat()`` filter. The old
  filter is still supported, but it may be removed in a future release.

  To update, replace uses like this:

  .. code-block:: yaml

     filter: dt_compat_enabled_with_alias("gpio-leds", "led0")

  with:

  .. code-block:: yaml

     filter: dt_enabled_alias_with_parent_compat("led0", "gpio-leds")

* Add a feature which handles pytest script in twister and provide an example.
* Provide test excution time per ztest testcase.
* Added and refined some testcases, most of them are negative testcases, to
  improve the test code coverage:

   * Testcases of x86's regular/direct interrupts and offload job from ISR.
   * Testcases of SMP, and enabled SMP for existed testing of semaphore, condvar, etc.
   * Testcases of memory protection, userspace and memory heap.
   * Testcases of data structure include stack, queue, ringbuffer and rbtree.
   * Testcases of IPC include pipe, poll, mailbox, message queue.
   * Testcases of synchronization include mutex, semaphore, atomic operations.
   * Testcases of scheduling and thread.
   * Testcases of testing for arch_nop() and errno.
   * Testcases of libc and posix API.
   * Testcases of log and sensor subsystem.

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.5.0 tagged
release:
