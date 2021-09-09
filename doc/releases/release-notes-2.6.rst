:orphan:

.. _zephyr_2.6:

Zephyr 2.6.0
############

We are pleased to announce the release of Zephyr RTOS version 2.6.0.

Major enhancements with this release include:

* Logging subsystem overhauled
* Added support for 64-bit ARCv3
* Split ARM32 and ARM64, ARM64 is now a top-level architecture
* Added initial support for Arm v8.1-m and Cortex-M55
* Removed legacy TCP stack support which was deprecated in 2.4
* Tracing subsystem overhaul including expansion for tracing points and
  added support for Percepio Tracealyzer
* Device runtime power management (PM), former IDLE runtime, was
  completely overhauled.
* Added an example standalone Zephyr application in its own Git repository:
  https://github.com/zephyrproject-rtos/example-application

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2021-3581: Under embargo until 2021-09-04

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
  :c:func:`bt_iso_chan_send` so when an error occur the buffer is not unref.

* Added :c:func:`lwm2m_engine_delete_obj_inst` function to the LwM2M library API.

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

* Disk API header ``<include/disk/disk_access.h>`` is deprecated in favor of
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

* The runtime device power management (PM) APIs is now synchronous by default
  and the asynchronous API has the **_async** sufix. This change aligns the API
  with the convention used in Zephyr. The affected APIs are
  :c:func:`pm_device_put` and :c:func:`pm_device_get`.

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
    * Enhanced register information dump in fault exceptions in Cortex-R.
    * Fixed spurious interrupt handling in Cortex-R.

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
  * Renesas R-Car Gen3
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

* Removed support for these X86 boards:

   * up_squared_32
   * qemu_x86_coverage
   * minnowboard

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
  * mimxrt685_evk: Added support for octal SPI flash storage, LittleFS, I2S, OS
    timer, and power management.
  * mimxrt1060_evk: Added support for QSPI flash storage, LittleFS, and
    mcuboot.
  * mimxrt1064_evk: Added support for mcuboot.

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
  * Added clock control driver for Renesas R-Car platform

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

* Disk

  * Moved disk drivers (``disk_access_*.c``) to ``drivers/disk`` and renamed
    according to their function.
  * Fixed CMD6 support in USDHC driver.
  * Fixed clock frequency switching after initialization in ``sdmmc_spi.c`` driver.

* DMA

  * Added support on STM32G0 and STM32H7

* EEPROM

  * Added support for EEPROM emulated in flash.

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
  * Added GPIO driver for Renesas R-Car platform

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

  * Moved shared interrupt controller configuration to be based
    on devicetree.

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

* PWM

  * Added support on STM32F2 and STM32L1.
  * Added support on Silicon Labs Gecko SoCs.

* Sensor

  * Added support for STM32 internal (CPU) temperature sensor.
  * Refactored mulitple ST sensor drivers to use gpio_dt_spec macros and common
    stmemc routines, support multiple instances, and configure ODR/range
    properties in device tree.
  * Added SBS 1.1 compliant fuel gauge driver.
  * Added MAX17262 fuel gauge driver.
  * Added BMP388 pressure sensor driver.
  * Added Atmel SAM QDEC driver.
  * Added TI FDC2X1X driver.
  * Added support for MPU9250 to existing MPU6050 6-axis motion tracking driver.
  * Refactored BME280 temperature/pressure sensor driver.
  * Added BMI270 IMU driver.
  * Added Nuvoton tachometer sensor driver.
  * Added MAX6675 cold-junction-compensated K-thermocouple to digital
    converter.

* Serial

  * Extended Cypress PSoC-6 SCB[uart] driver to support interrupts.
  * Added UART driver for Renesas R-Car platform

* SPI

  * Added Cypress PSoC-6 SCB[spi] driver.
  * Default SPI_SCK configuration is now pull-down for all STM32 to minimize power
    consumption in stop mode.

* Timer

  * Added x86 APIC TSC_DEADLINE driver.
  * Added support for NXP MCUX OS Timer.
  * Added support for Nuvoton NPCX system timer.
  * Added CMT driver for Renesas R-Car platform.

* USB

  * Added support on STM32H7
  * Added attached event delay to usb_dc_nrfx driver

* Watchdog

  * Added support for TI CC32xx watchdog.

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

USB
***

* Reworked USB classes configuration. Various minor fixes in USB DFU class.

* USB HID class

  * Removed get_protocol/set_protocol from USB HID class API.
  * Allowed boot interface Protocol Code to be set per device.
  * Rework idle report implementation.

* Samples

  * Allowed to build USB Audio sampe for different platforms.
  * Added SD card support to USB MSC sample.
  * Reworked USB HID sample.

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

* West (extensions)

  * This section only covers west :ref:`west-extensions` maintained in the
    zephyr repository. For release notes on west's built-in features, see
    :ref:`west-release-notes`.

  * Changes to the runners backends used for :ref:`flashing and debugging
    commands <west-build-flash-debug>`:

    * bossac runner: added legacy mode option into extended SAM-BA
      bootloader selection. This extends compatibility between Zephyr and
      some Arduino IDE bootloaders.

    * jlink runner: Zephyr thread awareness is now available in GDB by default
      for application builds with :kconfig:`CONFIG_DEBUG_THREAD_INFO` set to ``y``
      in :ref:`kconfig`. This applies to ``west debug``, ``west debugserver``,
      and ``west attach``. JLink version 7.11b or later must be installed on the
      host system, with JLink 7.20 or later strongly recommended.

    * jlink runner: default ``west flash`` output is less verbose. The old
      behavior is still available when run as ``west --verbose flash``.

    * jlink runner: when flashing, this runner now prefers a ``.hex`` file
      generated by the build system to a ``.bin``. Unlike ``.bin``, the HEX
      format includes information on the image's address range in flash, so
      this works better when flashing Zephyr images linked for locations other
      than the target's boot address, e.g. images meant to be run by
      bootloaders. The ``.bin`` file is still used as a fallback if a HEX File
      is not present.

    * openocd runner: support for ``west debug`` and ``west attach`` has been
      fixed. Previously with this runner, ``west debug`` behaved like ``west
      attach`` should, and ``west attach`` behaved like ``west debugserver``
      should.

    * pyocd runner: board-specific pyOCD configuration files in YAML can now be
      placed in :file:`support/pyocd.yaml` inside the board directory. See
      :zephyr_file:`boards/arm/reel_board/support/pyocd.yaml` for an example,
      and the pyOCD documentation for details on its configuration format.

  * ``west spdx``: new command which can be used to generate SPDX software
    bills of materials for a Zephyr application build. See :ref:`west-spdx`.


Libraries / Subsystems
**********************

* Disk

  * Disk drivers (``disk_access_*.c``) are moved to ``drivers/disk`` and renamed
    according to their function. Driver's Kconfig options are revised and renamed.
    SDMMC host controller drivers are selected when the corresponding node
    in devicetree is enabled. Following application relevant Kconfig options
    are renamed: ``CONFIG_DISK_ACCESS_RAM`` -> `CONFIG_DISK_DRIVER_RAM`,
    ``CONFIG_DISK_ACCESS_FLASH`` -> `CONFIG_DISK_DRIVER_FLASH`,
    ``CONFIG_DISK_ACCESS_SDHC`` -> `CONFIG_DISK_DRIVER_SDMMC`.

* Management

  * MCUmgr

    * Fixed an issue with the file system management failing to
      open files due to missing initializations of `fs_file_t` structures.
    * Fixed an issue where multiple SMP commands sent one after the other would
      corrupt CBOR payload.
    * Fixed problem where mcumgr over shell would stall and wait for
      retransmissions of frames.

* CMSIS subsystem

  * Moved CMSIS portability layer headers to include/portability/cmsis_os.h
    and include/portability/cmsis_os2.h

* Power management

  * ``device_pm_control_nop`` has been removed in favor of ``NULL`` when device
    PM is not supported by a device. In order to make transition easier for
    out-of-tree users a macro with the same name is provided as an alias to
    ``NULL``. The macro is flagged as deprecated to make users aware of the
    change.

  * Devices set as busy are no longer suspended by the system power management.

  * The time necessary to exit a sleep state and return to the active state was
    added in ``zephyr,power-state`` binding and accounted when the system
    changes to a power state.

  * Device runtime power management (PM), former IDLE runtime, was
    completely overhauled.

    * Multiple threads can wait an operation (:c:func:`pm_device_get_async` and
      :c:func:`pm_device_put_async`) to finish.
    * A new API :c:func:`pm_device_wait` was added so that drivers can easily
      wait for an async request to finish.
    * The API can be used in  :ref:`pre-kernel <api_term_pre-kernel-ok>` stages.
    * Several concurrence issues related with atomics access and the usage
      of the polling API have been fixed. Condition variables are now used to
      handle notification.

* Logging

  * Introduced logging v2 which allows logging any variable types (including
    floating point variables). New version does not require transient string
    duplication (``log_strdup()``). Legacy mode remains and is still the default
    but user API is not changed and modes are interchangeable.
    ``CONFIG_LOG2_MODE_DEFERRED`` or ``CONFIG_LOG2_MODE_IMMEDIATE`` enable
    logging v2. Logging backend API is extended to support v2 and the most
    popular backends (UART, shell) are updated.

* Shell

  * Added ``CONFIG_SHELL_BACKEND_DUMMY_BUF_SIZE`` option that allows to set
    size of the dummy backend buffer; changing this parameter allows to work
    around issue, where output from command, shell that is gathered by the dummy
    backend, gets cut to the size of buffer.

* Storage

  * Added persistent write progress to stream_flash.

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

* :github:`35962` - drivers using deprecated Kconfigs
* :github:`35955` - Bluetooth: Controller: Regression in connection setup
* :github:`35949` - can: mcan: sjw-data devicetree configuration is not written correctly
* :github:`35941` - subsys: tracing: sysview: No SEGGER_SYSVIEW.h in path
* :github:`35926` - Shell tab-completion with more than two levels of nested dynamic commands fails
* :github:`35924` - Help with Configuring Custom GPIO Pins
* :github:`35916` - drivers: TI cc13xx_cc26xx: build error when PM is enabled (serial, entropy, spi, i2c modules)
* :github:`35911` - shield sample sensorhub does not produce and meaningful data
* :github:`35910` - LIS2MDL reporting wrong temperature
* :github:`35896` - frdm_k64f: build failure missing dt-bindings/clock/kinetis_sim.h: No such file or directory
* :github:`35890` - Build system ignores explicit ZephyrBuildConfiguration_ROOT variable
* :github:`35882` - Fixed width documentation makes DT bindings docs unreadable
* :github:`35876` - Bluetooth: host: CCC store not correctly handled for multiple connections
* :github:`35871` - LPS22HH sensor reporting wrong pressure data
* :github:`35840` - Bluetooth: host: L2CAP enhanced connection request conformance test issues
* :github:`35838` - Bluetooth: ISO: BIG termination doesn't fully unref the connection
* :github:`35826` - LORAWAN Compatibility with nrf52832 and sx1262
* :github:`35813` - Zephyr Native Posix Build Uses Linux Build Machine Headers out of Sandbox
* :github:`35812` - ESP32 Factory app partition is not bootable
* :github:`35781` - Missing response parameter for HCI_LE_Set_Connectionless_IQ_Sampling_Enable HCI command
* :github:`35772` - Support C++ exceptions on NIOS2
* :github:`35764` - tests: kernel: threads: no multithreading: fails with CONFIG_STACK_SENTINEL=y
* :github:`35762` - SAMPLES: shell_module gives no console output on qemu_leon3
* :github:`35756` - ESP32 Ethernet Support
* :github:`35737` - drivers: can: mcan: sjw not initialized when CAN_FD_MODE is enabled
* :github:`35714` - samples: subsys: testusb: I want to know how to test in window10.
* :github:`35713` - tests: kernel.scheduler.multiq: test_k_thread_suspend_init_null failure
* :github:`35694` - No console output from NIOS2 Max10
* :github:`35693` - gpio_mcux_lpc.c uses devicetree instance numbers incorrectly
* :github:`35686` - Bluetooth: Crash in bt_gatt_dm_attr_chrc_val when BLE device is disconnected during discovery process
* :github:`35681` - Unable to get ouput for samples/subsys/logging/logger and samples/philosophers
* :github:`35677` - samples/subsys/console/getchar and samples/subsys/console/getline build breaks for arduino_nano_33_ble
* :github:`35655` - Arm64: Assertion failed when CONFIG_MP_CPUS >= 3.
* :github:`35653` - ARC MWDT toolchain put __start and __reset at different address
* :github:`35633` - Out of bound read: Multiple Coverity sightings in generated code
* :github:`35631` - [Coverity CID: 205610] Out-of-bounds read in /zephyr/include/generated/syscalls/kernel.h (Generated Code)
* :github:`35630` - [Coverity CID: 205657] Out-of-bounds read in /zephyr/include/generated/syscalls/sample_driver.h (Generated Code)
* :github:`35629` - [Coverity CID: 207968] Out-of-bounds read in /zephyr/include/generated/syscalls/counter.h (Generated Code)
* :github:`35628` - [Coverity CID: 207976] Out-of-bounds read in /zephyr/include/generated/syscalls/counter.h (Generated Code)
* :github:`35627` - [Coverity CID: 208195] Out-of-bounds read in /zephyr/include/generated/syscalls/gpio.h (Generated Code)
* :github:`35626` - [Coverity CID: 210588] Out-of-bounds read in /zephyr/include/generated/syscalls/dac.h (Generated Code)
* :github:`35625` - [Coverity CID: 211042] Out-of-bounds read in /zephyr/include/generated/syscalls/socket.h (Generated Code)
* :github:`35624` - [Coverity CID: 214226] Out-of-bounds read in /zephyr/include/generated/syscalls/uart.h (Generated Code)
* :github:`35623` - [Coverity CID: 215223] Out-of-bounds read in /zephyr/include/generated/syscalls/net_ip.h (Generated Code)
* :github:`35622` - [Coverity CID: 215238] Out-of-bounds read in /zephyr/include/generated/syscalls/net_ip.h (Generated Code)
* :github:`35621` - [Coverity CID: 219477] Out-of-bounds read in /zephyr/include/generated/syscalls/pwm.h (Generated Code)
* :github:`35620` - [Coverity CID: 219482] Out-of-bounds read in /zephyr/include/generated/syscalls/pwm.h (Generated Code)
* :github:`35619` - [Coverity CID: 219496] Out-of-bounds read in /zephyr/include/generated/syscalls/ztest_error_hook.h (Generated Code)
* :github:`35618` - [Coverity CID: 219506] Out-of-bounds read in /zephyr/include/generated/syscalls/log_ctrl.h (Generated Code)
* :github:`35617` - [Coverity CID: 219568] Out-of-bounds read in /zephyr/include/generated/syscalls/net_if.h (Generated Code)
* :github:`35616` - [Coverity CID: 219586] Out-of-bounds read in /zephyr/include/generated/syscalls/net_if.h (Generated Code)
* :github:`35615` - [Coverity CID: 219648] Uninitialized scalar variable in /zephyr/include/generated/syscalls/test_syscalls.h (Generated Code)
* :github:`35614` - [Coverity CID: 219725] Out-of-bounds read in /zephyr/include/generated/syscalls/kernel.h (Generated Code)
* :github:`35613` - [Coverity CID: 225900] Out-of-bounds access in tests/net/lib/dns_addremove/src/main.c
* :github:`35612` - [Coverity CID: 229325] Out-of-bounds read in /zephyr/include/generated/syscalls/log_msg2.h (Generated Code)
* :github:`35611` - [Coverity CID: 230223] Out-of-bounds read in /zephyr/include/generated/syscalls/log_msg2.h (Generated Code)
* :github:`35610` - [Coverity CID: 232755] Out-of-bounds read in /zephyr/include/generated/syscalls/log_ctrl.h (Generated Code)
* :github:`35609` - [Coverity CID: 235917] Out-of-bounds read in /zephyr/include/generated/syscalls/log_msg2.h (Generated Code)
* :github:`35608` - [Coverity CID: 235923] Out-of-bounds read in /zephyr/include/generated/syscalls/log_msg2.h (Generated Code)
* :github:`35607` - [Coverity CID: 235933] Out-of-bounds read in /zephyr/include/generated/syscalls/gpio.h (Generated Code)
* :github:`35606` - [Coverity CID: 235951] Out-of-bounds read in /zephyr/include/generated/syscalls/log_ctrl.h (Generated Code)
* :github:`35605` - [Coverity CID: 236005] Out-of-bounds read in /zephyr/include/generated/syscalls/log_ctrl.h (Generated Code)
* :github:`35604` - [Coverity CID: 236129] Unused value in drivers/adc/adc_lmp90xxx.c
* :github:`35603` - [Coverity CID: 236130] Wrong sizeof argument in drivers/adc/adc_lmp90xxx.c
* :github:`35596` - Bluetooth: Cannot connect if extended advertising is enabled in ``prj.conf``
* :github:`35586` - Timer based example on docu using nrf52-dk compile error.
* :github:`35580` - Fault when logging
* :github:`35569` - tests/lib/mem_alloc failed with arcmwdt toolchain
* :github:`35567` - some mwdt compiler options can't be recognized by zephyr_cc_option
* :github:`35561` - Issue with fat_fs example on nucleo_f767zi
* :github:`35553` - all menuconfig interfaces contain sound open firmware/SOF text
* :github:`35543` - samples: subsys: display: lvgl: is run on nucleo_f429zi and nucleo_f746zg but should be skipped
* :github:`35541` - sockets_tls: when using dtls with sara-r4 modem, handshake hangs if no reply
* :github:`35540` - tests: ztest: error_hook: fails on nucleo_g071rb and nucleo_l073rz
* :github:`35539` - tests: drivers: spi: spi_loopback: test failed since #34731 is merged
* :github:`35524` - tests: samples: led: LED PWM sample fails on nrf platforms
* :github:`35522` - doc: Current section is not shown in the side pane nor the page top cookie
* :github:`35512` - OpenThread can't find TRNG driver on nRF5340
* :github:`35509` - tests: timer: Unstable tests using timer at nrf platforms
* :github:`35489` - samples: net: gsm_modem: build fails if CONFIG_GSM_MUX=y
* :github:`35480` - pm: device_runtime: ``pm_device_request`` can block forever
* :github:`35479` - address is not a known kernel object exception with arcmwdt toolchain
* :github:`35476` - bluetooth: controller assertion when scanning with multiple active connections
* :github:`35474` - The dma-stm32 driver don't build for STM32F0 MCUs
* :github:`35444` - drivers: sensor: sbs-gauge: The sbs-gauge cannot be read from sensor shell
* :github:`35401` - Enabling POSIX_API leads to SSL handshake error
* :github:`35395` - STM32F4: Infinite reboot loop due to Ethernet initialization
* :github:`35390` - net.socket.tls.tls_ext: frdm_k64f test failure
* :github:`35383` - Can't setup ISO Broadcast Demo on nrf53dk
* :github:`35380` - sys: timeutil: inconsistent types for local times
* :github:`35363` - bt_gatt_discover() retunrs incorrect handle (offset by -1)
* :github:`35360` - Power consumption nRF52
* :github:`35352` - [Coverity CID: 215376] Out-of-bounds access in drivers/sensor/lis2dh/lis2dh_trigger.c
* :github:`35351` - [Coverity CID: 219472] Unrecoverable parse warning in tests/kernel/mem_protect/mem_protect/src/mem_domain.c
* :github:`35350` - [Coverity CID: 236055] Out-of-bounds access in subsys/modbus/modbus_core.c
* :github:`35349` - [Coverity CID: 236057] Unrecoverable parse warning in tests/kernel/mem_protect/mem_protect/src/mem_domain.c
* :github:`35348` - [Coverity CID: 236060] Out-of-bounds access in subsys/net/l2/ppp/ppp_l2.c
* :github:`35347` - [Coverity CID: 236064] Dereference null return value in subsys/bluetooth/controller/ll_sw/ull.c
* :github:`35346` - [Coverity CID: 236069] Out-of-bounds access in tests/lib/c_lib/src/main.c
* :github:`35345` - [Coverity CID: 236074] Out-of-bounds access in tests/lib/c_lib/src/main.c
* :github:`35344` - [Coverity CID: 236075] Out-of-bounds access in subsys/bluetooth/controller/hci/hci.c
* :github:`35343` - [Coverity CID: 236079] Untrusted divisor in subsys/bluetooth/controller/hci/hci.c
* :github:`35342` - [Coverity CID: 236085] Dereference after null check in samples/userspace/prod_consumer/src/app_a.c
* :github:`35341` - twister: Hardware map creation is buggy (+ inaccurate docs)
* :github:`35338` - USB: ethernet CDC ECM/EEM support is broken
* :github:`35336` - tests: samples: power: samples/subsys/pm/device_pm/sample.power.ospm.dev_idle_pm fails on nrf52 platforms
* :github:`35329` - samples: gsm_modem: Compilation failed, likely related to logging changes
* :github:`35327` - Sensor Code for CC3220sf
* :github:`35325` - Shell: Kernel: Reboot: echo is abruptly terminated
* :github:`35321` - Improve STM32: Serial Driver,Handle uart mode per instance
* :github:`35307` - ARM64 system calls are entered with interrupts masked
* :github:`35305` - Linking order when using both TF-M and Mbed TLS
* :github:`35299` - PM suspend IPC message sporadically not being  delivered
* :github:`35297` - STM32 SPI - wrong behavior after PR 34731
* :github:`35286` - New logging breaks eclipse
* :github:`35278` - LittleFs Sample will not build for qemu_riscv64 sample target
* :github:`35263` - device_pm_control_nop is used in dac_mcp4725.c
* :github:`35242` - intel_adsp_cavs15: run kernel common testcases failed on ADSP
* :github:`35241` - intel_adsp_cavs15: run interrupt testcases failed on ADSP
* :github:`35236` - tests: doc: Document generation process FAILS with valid module ``samples:``
* :github:`35223` - Coverity [CID 221772]: Wrong operator used in logging subsystem, multiple violations
* :github:`35220` - tests: dma: memory-to-memory transfer fails on stm32f746zg nucleo board
* :github:`35219` - tests: driver: dma test case loop_transfer fails on stm32 with dmamux
* :github:`35215` - tests/kernel/msgq/msgq_usage failed on hsdk board
* :github:`35209` - tests/kernel/mem_heap/mheap_api_concept failed on hsdk board
* :github:`35204` - PPI channel assignment for Bluetooth controller is incorrect for nRF52805
* :github:`35202` - smp atomic_t global_lock will never be cleared when a thread oops with global_lock is set
* :github:`35200` - tests/kernel/smp failed on hsdk board
* :github:`35199` - Queues: there is no documentation about queue's implementation.
* :github:`35198` - subsys.pm.device_pm: frdm_k64f leave idel fails
* :github:`35197` - Zephyr Project Development with 2 Ethernet Interfaces Supported (eth0, and eth1)
* :github:`35195` - doc, coding guidelines: broken CERT-C links
* :github:`35191` - GIT Checkout of Master Branch is 2.6.0rc1 versus west update as 2.5.99
* :github:`35189` - Coding Guidelines: Resolve the issues under Rule 21.2
* :github:`35187` - Version selection not working
* :github:`35176` - strtol crashes
* :github:`35175` - quectel-bg9x crashes in modem_rssi_query_work
* :github:`35169` - esp32:    uart_poll_in never ready for UART2 only
* :github:`35163` - [Coverity CID: 236009] Wrong sizeof argument in tests/lib/cbprintf_package/src/test.inc
* :github:`35162` - [Coverity CID: 235972] Wrong sizeof argument in tests/lib/cbprintf_package/src/test.inc
* :github:`35161` - [Coverity CID: 235962] Unused value in tests/kernel/mem_protect/mem_map/src/main.c
* :github:`35160` - [Coverity CID: 235930] Unused value in kernel/mmu.c
* :github:`35159` - [Coverity CID: 232698] Uninitialized scalar variable in samples/net/sockets/txtime/src/main.c
* :github:`35158` - [Coverity CID: 224630] Uninitialized scalar variable in subsys/net/ip/igmp.c
* :github:`35157` - [Coverity CID: 221380] Uninitialized scalar variable in subsys/bluetooth/controller/ll_sw/ull_iso.c
* :github:`35156` - [Coverity CID: 235979] Unchecked return value in drivers/sensor/iis2mdc/iis2mdc_trigger.c
* :github:`35155` - [Coverity CID: 235677] Unchecked return value in drivers/gpio/gpio_cy8c95xx.c
* :github:`35154` - [Coverity CID: 233524] Unchecked return value in include/drivers/dma.h
* :github:`35153` - [Coverity CID: 236006] Structurally dead code in tests/subsys/logging/log_api/src/test.inc
* :github:`35152` - [Coverity CID: 235986] Structurally dead code in tests/subsys/logging/log_api/src/test.inc
* :github:`35151` - [Coverity CID: 235943] Reliance on integer endianness in include/sys/cbprintf_cxx.h
* :github:`35150` - [Coverity CID: 225136] Out-of-bounds write in tests/kernel/sched/deadline/src/main.c
* :github:`35149` - [Coverity CID: 234410] Out-of-bounds read in tests/kernel/sched/preempt/src/main.c
* :github:`35148` - [Coverity CID: 236015] Out-of-bounds access in tests/subsys/logging/log_api/src/mock_backend.c
* :github:`35147` - [Coverity CID: 236012] Out-of-bounds access in subsys/bluetooth/audio/vcs_client.c
* :github:`35146` - [Coverity CID: 235994] Out-of-bounds access in tests/kernel/interrupt/src/interrupt_offload.c
* :github:`35145` - [Coverity CID: 235984] Out-of-bounds access in include/sys/cbprintf_cxx.h
* :github:`35144` - [Coverity CID: 235944] Out-of-bounds access in subsys/bluetooth/audio/vcs_client.c
* :github:`35143` - [Coverity CID: 235921] Out-of-bounds access in include/sys/cbprintf_cxx.h
* :github:`35142` - [Coverity CID: 235914] Out-of-bounds access in subsys/bluetooth/audio/vcs.c
* :github:`35141` - [Coverity CID: 235913] Out-of-bounds access in subsys/bluetooth/audio/vcs.c
* :github:`35140` - [Coverity CID: 231072] Out-of-bounds access in tests/kernel/sched/preempt/src/main.c
* :github:`35139` - [Coverity CID: 229646] Out-of-bounds access in subsys/bluetooth/audio/vocs.c
* :github:`35138` - [Coverity CID: 229545] Out-of-bounds access in tests/subsys/canbus/isotp/conformance/src/main.c
* :github:`35137` - [Coverity CID: 225993] Out-of-bounds access in tests/subsys/canbus/isotp/conformance/src/main.c
* :github:`35136` - [Coverity CID: 235916] Operands don't affect result in drivers/adc/adc_stm32.c
* :github:`35135` - [Coverity CID: 235911] Negative array index write in tests/subsys/logging/log_api/src/mock_backend.c
* :github:`35134` - [Coverity CID: 222151] Negative array index write in tests/subsys/logging/log_msg2/src/main.c
* :github:`35133` - [Coverity CID: 232501] Missing varargs init or cleanup in subsys/logging/log_msg2.c
* :github:`35132` - [Coverity CID: 236003] Logically dead code in subsys/bluetooth/audio/vcs.c
* :github:`35131` - [Coverity CID: 235998] Logically dead code in subsys/bluetooth/audio/vcs.c
* :github:`35130` - [Coverity CID: 235997] Logically dead code in drivers/adc/adc_stm32.c
* :github:`35129` - [Coverity CID: 235990] Logically dead code in subsys/bluetooth/audio/vcs.c
* :github:`35128` - [Coverity CID: 235970] Logically dead code in subsys/bluetooth/audio/vcs.c
* :github:`35127` - [Coverity CID: 235965] Logically dead code in tests/subsys/logging/log_api/src/test.inc
* :github:`35126` - [Coverity CID: 235961] Logically dead code in tests/subsys/logging/log_api/src/test.inc
* :github:`35125` - [Coverity CID: 235956] Logically dead code in subsys/bluetooth/audio/vcs.c
* :github:`35124` - [Coverity CID: 235955] Logically dead code in subsys/bluetooth/audio/vcs.c
* :github:`35123` - [Coverity CID: 235954] Logically dead code in subsys/bluetooth/audio/vcs.c
* :github:`35122` - [Coverity CID: 235952] Logically dead code in subsys/bluetooth/audio/vcs.c
* :github:`35121` - [Coverity CID: 235950] Logically dead code in subsys/bluetooth/audio/vcs.c
* :github:`35120` - [Coverity CID: 235934] Logically dead code in subsys/bluetooth/audio/vcs.c
* :github:`35119` - [Coverity CID: 235932] Logically dead code in samples/sensor/adxl372/src/main.c
* :github:`35118` - [Coverity CID: 235919] Logically dead code in samples/sensor/bmg160/src/main.c
* :github:`35117` - [Coverity CID: 235945] Incorrect sizeof expression in include/sys/cbprintf_cxx.h
* :github:`35116` - [Coverity CID: 235987] Incompatible cast in include/sys/cbprintf_cxx.h
* :github:`35115` - [Coverity CID: 236000] Improper use of negative value in tests/lib/cbprintf_package/src/test.inc
* :github:`35114` - [Coverity CID: 221976] Division or modulo by zero in tests/drivers/can/timing/src/main.c
* :github:`35113` - [Coverity CID: 235985] Dereference before null check in subsys/bluetooth/audio/vcs_client.c
* :github:`35112` - [Coverity CID: 235983] Dereference after null check in samples/sensor/max17262/src/main.c
* :github:`35111` - [Coverity CID: 234630] Dereference after null check in tests/net/dhcpv4/src/main.c
* :github:`35110` - [Coverity CID: 220616] Arguments in wrong order in tests/subsys/canbus/isotp/conformance/src/main.c
* :github:`35108` - tests: drivers: pwm: pwm_api: failed on nucleo_f207zg
* :github:`35107` - Atmel SAM E70 / Cortex-M7 fails to boot if CONFIG_NOCACHE_MEMORY=y
* :github:`35104` - arch.interrupt.gen_isr_table.arm_mainline: fails on lpcxpresso55s16_ns
* :github:`35102` - testing.ztest.error_hook: fails on lpcxpresso55s16_ns
* :github:`35100` - libraries.libc.sprintf_new: fails on lpcxpresso55s16_ns and lpcxpresso55s69_ns
* :github:`35099` - benchmark.kernel.application.fp.arm: Illegal load of EXC_RETURN into PC on lpcxpresso55s16_ns and lpcxpresso55s69_ns
* :github:`35097` - arch.interrupt: fails on NXP Cortex-M0+ platforms
* :github:`35091` - enc424j600 does not work
* :github:`35089` - stm32h7: systematic crash at each second boot with NETWORKING=y
* :github:`35083` - dts: stm32mp1: SPI2 mixup with SAI2, SPI3 mixup with SAI3
* :github:`35082` - intel_adsp_cavs15: All the testcases run failed on ADSP
* :github:`35079` - acrn_ehl_crb: build warnings for old APIC_TIMER configs
* :github:`35076` - acrn_ehl_crb does not work with CPUs >1
* :github:`35075` - .west/config west.yml and zephyr versioning during project development
* :github:`35073` - timer: cortex_m_systick: uptime drifting in tickless mode
* :github:`35060` - tests/kernel/common: test_nop failed on ARMV7_M_ARMV8_M_MAINLINE
* :github:`35058` - Bluetooth: deadlock when canceling db_hash.work from settings commit handler
* :github:`35051` - CONFIG_LOG2 fails for floating point output with warning and bad output
* :github:`35048` - mcuboot with enabled  serial recovery does not compile
* :github:`35046` - Tracing shows k_busy_wait() being executed very often on nRF platforms
* :github:`35043` - NXP: Build error : ModuleNotFoundError: No module named 'elftools'
* :github:`35041` - Crash in net-shell when invoking "net dns" command
* :github:`35036` - STM32: Wrong uart_event_tx len calculation
* :github:`35033` - samples/boards: stm32 pm blinky fails when run with twister
* :github:`35028` - frdm_k64f: failed to run tests/subsys/pm/power_mgmt/
* :github:`35027` - frdm_k64f: failed to run testcase tests/drivers/adc/adc_emul/
* :github:`35026` - sam_e70b_xplained: failed to run testcases tests/drivers/adc/adc_emul/
* :github:`35013` - Bluetooth: Controller: Out-of-Bound ULL context access during connection completion
* :github:`34999` - Using BT_ISO bluetooth hci_usb sample, and  enable, but still shows no command supported
* :github:`34989` - Implement arch_page_phys_get() for ARM64
* :github:`34979` - MIMRT685-EVK board page has broken links
* :github:`34978` - misleading root folder size in footprint reports
* :github:`34969` - Documentation still mentions deprecated macro DT_INST_FOREACH_STATUS_OKAY
* :github:`34964` - net regression: Connection to Zephyr server non-deterministically leads to client timeout, ENOTCONN on server side
* :github:`34962` - tfm: cmake: Toolchain not being passed into psa-arch-tests
* :github:`34950` - xtensa arch ：The source code version is too old
* :github:`34948` - SoF module is not pointing at Zehpyr repo
* :github:`34935` - LwM2M: Block transfer with TLV format does not work
* :github:`34932` - drvers/flash/nrf_qspi_nor: high power consumption on nrf52840
* :github:`34931` - dns resolve timeout leads to CPU memory access violation error
* :github:`34925` - tests/lib/cbprintf_package fails to build
* :github:`34923` - net.socket.get_addr_info: frdm_k64f test fails
* :github:`34917` - arch.interrupt.arm| arch.interrupt.extra_exception_info: lpcxpresso55s28 series: test failure
* :github:`34915` - arch.interrupt.gen_isr_table.arm_mainline:lpcxpresso55s16_ns/lpcxpresso55s28: interrupt 57 does not work
* :github:`34911` - tests/kernel/mem_protect/mem_protect: frdm_k82f/frdm_k64f unexpected fatal error
* :github:`34909` - dma_loopback:lpcxpresso55s28_ns driver test failure
* :github:`34904` - uart_mcux_lpuart: Enable driver to work with ``CONFIG_MULTITHREADING=n``
* :github:`34903` - doc: Target name is wrong for rcar_h3ulcb board
* :github:`34891` - mcumgr timeout due to smp_shell_process stalling
* :github:`34880` - Convert SoF Module to new kwork API
* :github:`34865` - CONFIG_NET_SOCKETS_PACKET interferes with other network traffic (gptp, IP)
* :github:`34862` - CAN ISO-TP implementation not using local work queue
* :github:`34852` - Some bluetooth advertising packages never get transmitted over-air (Bluetooth Mesh application)
* :github:`34844` - qemu_cortex_a53_smp:  tests/ztest/error_hook failed after enabling the FPU context switching support
* :github:`34840` - CONFIG_MULTITHREADING=n is not tested on hardware platforms
* :github:`34838` - tests/subsys/logging/log_msg2 failes on qemu_cortex_a53
* :github:`34837` - Unstable multi connections between NRF52840
* :github:`34827` - tests: power management: test_power_state_trans fails on nrf boards
* :github:`34796` - x86 jlink runner fails on M1 macs
* :github:`34794` - LIS2DH Hard Fault when INT2 is not defined
* :github:`34788` - APIC timer does not support SMP
* :github:`34777` - semaphore and condvar_api tests fails after ARM64 FPU context switch commit on qemu_cortex_a53_smp
* :github:`34772` - Mixed usage of signed/unsigned integer by the logging subsystem
* :github:`34757` - west update: Default behavior should fetch only --depth 1
* :github:`34753` - Building and Debugging Zephyr for Native Platform on Linux using VSCode and/or QtCreator
* :github:`34748` - Native posix: Segmentation fault in case of allocations without explicit heap assignment
* :github:`34739` - tests/arch/arm/arm_no_multithreading/arch.arm.no_multithreading fails to build on a number of platforms
* :github:`34734` - Can handler doesn't compile with CONFIG_USERSPACE
* :github:`34722` - nvs: possibility of losing data
* :github:`34716` - flash: spi_nor: build fails when CONFIG_SPI_NOR_SFDP_RUNTIME is enabled
* :github:`34696` - Unable to select LOG_DICTIONARY_SUPPORT when TEST_LOGGING_DEFAULTS=y
* :github:`34690` - net: process_rx_packet() work handler violates requirements of Workqueue Threads implementation
* :github:`34687` - intel_adsp_cavs15: run tests/kernel/semaphore/semaphore/ failed on ADSP
* :github:`34683` - MCUboot not confirm image when using 'west flash'
* :github:`34672` - stm32h7: issue with CONFIG_UART_ASYNC_API=y
* :github:`34670` - smp_svr sample configured for serial port with shell management enabled does not work
* :github:`34669` - uart_read_fifo() reads only 2 chars on nucleo STM32L43KC and nRF52840-DK
* :github:`34668` - i2c_ite_it8xxx2.c fails to build - possibly related to device_pm_control_nop changes
* :github:`34667` - posix_apis:mimxrt685_evk_cm33 timeout in test_posix_realtime
* :github:`34662` - many udp networking cases fail on nxp platforms
* :github:`34658` - TF-M integration samples do not work with GNU ARM Embedded having GCC v10.x.x
* :github:`34656` - STM32 ADC - read of multiple channels in a sequence
* :github:`34644` - CAN - Bus Driver Sample
* :github:`34635` - BME280 build error
* :github:`34633` - STM32: Mass conversion of boards to dts based clock control configuration
* :github:`34624` - Coding guidelines 15.7 PR causes tests failures
* :github:`34605` - flash_stm32h7x.c fails to build
* :github:`34601` - sample: bluetooth: beacon: USAGE FAULT after few seconds on board b_l4s5i_iot01a
* :github:`34597` - Mismatch between ``ot ping`` and ``net ping``
* :github:`34593` - Using hci_usb with Bluez 5.55 or 5.58
* :github:`34585` - mec15xxevb_assy6853: test_timeout_order in tests/kernel/common assertion failed
* :github:`34584` - kernel: workqueue thread is occasionally not invoked when kernel is run in cooperative mode only
* :github:`34583` - twister failing: fails platform native_posix, test lib/cmsis_dsp/filtering
* :github:`34581` - Unable to work with SX1276 Lora module.
* :github:`34570` - IPC samples running secure but configured nonsecure (AN521)
* :github:`34568` - Compilation error with zephyr 2.3.0
* :github:`34563` - net: lib: sockets: Unable to select() file descriptors with number >= 32
* :github:`34558` - Compilation error with Log v2 and CONFIG_LOG_PRINTK
* :github:`34541` - per-adv-sync-create doesn't work on nRF52840, ./tests/bluetooth/shell/
* :github:`34538` - STM32 temperature sensor
* :github:`34534` - west sign regression when HEX file not exists
* :github:`34527` - Cpp compiling error: expected primary-expression before 'char'.    _Generic macros problem
* :github:`34526` - logging tests fails to build on a number of platforms
* :github:`34515` - samples: net: syslog_net: hard fault when running on frdm_k64f
* :github:`34505` - mimxrt1050_evk:failed to run testcases tests/net
* :github:`34503` - up_squared and ehl_crb: test fails from timeout in application_development.cpp.libcxx.exceptions
* :github:`34500` - thingy52 lis2dh12 sensor values too large
* :github:`34495` - logger: Logger API cannot be compiled with C++
* :github:`34492` - Logging still broken with SOF
* :github:`34482` - net_tunnel_virtual:frdm_k64f: build failure
* :github:`34474` - MPS2-AN385 SRAM does not match what the documentation page says
* :github:`34473` - Add Requirements repository with infrastructure and placeholder requirements
* :github:`34469` - nrf53: nrf5340dk_nrf5340_cpunet not executing.
* :github:`34463` - LwM2M bootstrap DELETE operation not working
* :github:`34462` - samples: net: sockets: packet: reception stops working after a while
* :github:`34461` - Unable to use PWM pins with STM Nucleo H743ZI
* :github:`34443` - Document font display is incomplete
* :github:`34439` - Logging subsystem causes build to fail with LLVM
* :github:`34434` - subsys: testsuite: ztest framework breaks if run in cooperative mode only
* :github:`34426` - RFC: API Change: USB HID remove get_protocol/set_protocol/get_idle/set_idle callbacks
* :github:`34423` - twister build issue with arm64
* :github:`34419` - significant build time increase with new logging subsystem
* :github:`34416` - Configuration HAS_DTS has no function, preventing compile for vendors without device tree
* :github:`34409` - mDNS response on link local when using DHCPv4 and AutoIP/Static IP
* :github:`34403` - Logging disable function causes Zephyr hard lockup
* :github:`34402` - spi: spi_nrfx_spim: wrong clock frequency selected
* :github:`34397` - Update getting started docs to reflect gdb python requirements
* :github:`34387` - Error message in include/linker/kobject-text.ld is unclear
* :github:`34382` - fs/nvs: if closing ATE has to high offset NVS iterates up to the end of flash.
* :github:`34372` - CPU Lockups when using own Log Backend
* :github:`34369` - Driver esp for wifi got a dead lock.
* :github:`34368` - Cmake's Python path breaks after using west build --pristine
* :github:`34363` - k_work: incorrect return values for synchronous cancel
* :github:`34355` - LittleFS sample code catch an "undefined symbol 'ITCM_ADDR' referenced in expression" in linker step
* :github:`34345` - samples/net/civetweb/websocket_server fails to build
* :github:`34342` - No output on SWO pin (STM32L4)
* :github:`34341` - SWO logging and DWT timing collision
* :github:`34329` - lwm2m: pmin and pmax attributes should be optional
* :github:`34325` - hal: microchip: Missing Wake bit definitions
* :github:`34309` - unable to connect to azure iot hub via mqtt protocol
* :github:`34308` - SPI transceive function only transmitting first tx_buffer on Sifive's MCU
* :github:`34304` - intel_adsp_cavs15: run tests/kernel/queue/ failed on ADSP
* :github:`34295` - TensorFlow Lite Micro Module
* :github:`34280` - Add USB to LPCXpresso55S69 board
* :github:`34275` - drivers: led_pwm: Improper label assignment
* :github:`34272` - twister: Add memory footprint info to json report
* :github:`34270` - NVS read after consecutive restarts.
* :github:`34265` - BME280 Pressure calculation
* :github:`34264` - CI: twister: Add  merged report from all sub-builds to buildkite build artifacts
* :github:`34262` - Unable to find detailed documentation on pinmux driver development
* :github:`34249` - Unable to initialize on STM32F103RE + Quectel EC21 using BG9x driver
* :github:`34246` - LoRa driver sending opcode of commands without parameters
* :github:`34234` - UART NS16550 Underflow Issue During Clearing Port
* :github:`34233` - OpenThread build issues
* :github:`34231` - uzlib (decompression library)
* :github:`34229` - C++ Exception Support in qemu_riscv32 emulation
* :github:`34225` - BBC micro:bit v1.5 LSM303AGR-ACCEL
* :github:`34216` - Using nrfx_gpiote library with spi(nrf52840)
* :github:`34214` - codes reference weak variable are optimized out
* :github:`34209` - BLE Mesh Provisioning generates value 0 outside of Specification for Blink, Beep, or Vibrate
* :github:`34206` - Question: Is zephyrproject actively maintaining the windows-curses sub-project?
* :github:`34202` - MPU Fault when running central coded bluetooth and ENC28J60 dhcpv4_client
* :github:`34201` - Fatal error when perform "bt phy-update" if there is not any connections at ./tests/bluetooth/shell
* :github:`34197` - samples: telnet: Tab completion not working in telnet shell
* :github:`34196` - st_lis2mdl: LSM303AGR-MAGN not detected
* :github:`34190` - Newbie: Simple C++ List App Builds for QEMU but not Native Posix Emulation
* :github:`34184` - video samples fail to build
* :github:`34178` - apds9960 sensor sample does not build on STM32
* :github:`34165` - SNTP fails to close the used socket
* :github:`34154` - AArch64 PR reviews and merges are lagging behind
* :github:`34152` - intel_adsp_cavs15: run tests/kernel/smp/ failed on ADSP
* :github:`34149` - Invalid link in Zephyr document to ACRN page
* :github:`34145` - Convert NXP kinetis boards to have pindata in devicetree
* :github:`34134` - USB do not works if bootloader badly use the device before
* :github:`34117` - ehl_crb: tests/kernel/context tests failed
* :github:`34116` - mec15xxevb_assy6853: tests/kernel/mutex/sys_mutex/
* :github:`34107` - Convert tests/benchmarks/mbedtls/src/benchmark.c to new kwork API
* :github:`34106` - Convert tests/kernel/pending/src/main.c to new kwork API
* :github:`34104` - Convert tests/benchmarks/footprints/src/workq.c to new kwork API
* :github:`34103` - Convert drivers/console/uart_mux.c to new kwork API
* :github:`34102` - Convert drivers/serial/uart_sam0.c to new kwork API
* :github:`34101` - Convert subsys/mgmt to new kwork API
* :github:`34100` - Convert subsys/shell/shell_telnet to new kwork API
* :github:`34099` - Convert subsys/tracing/cpu_stats.c to new kwork API
* :github:`34098` - Convert samples/drivers/led_sx1509b_intensity to new kwork API
* :github:`34097` - Convert samples/boards/reel_board/mesh_badge to new kwork API
* :github:`34096` - Convert samples nrf clock_skew to new kwork API
* :github:`34095` - Convert CAN to new kwork API
* :github:`34094` - Convert ubsys/ipc/rpmsg_service/rpmsg_backend.c to new kwork API
* :github:`34093` - Convert bluetooth to new kwork API
* :github:`34092` - Convert usb to new kwork API
* :github:`34091` - Convert uart_stm32.c to new kwork API
* :github:`34090` - Convert video_sw_generator.c to new kwork API
* :github:`34082` - Bullets are broken in documentation
* :github:`34076` - Unrecognized characters generated during document construction
* :github:`34068` - DOC BUILD FAIL
* :github:`34046` - Failed to build arm64 architecture related board
* :github:`34045` - samples: subsys: mgmt: smp_srv: UDP sample does not boot on frdm_k64f
* :github:`34026` - RISCV32 QEMU illegal instruction exception / floating point support
* :github:`34023` - test_prevent_interruption has wrong data type for key
* :github:`34014` - Toolchain Compile Error  of RISC-V(rv32m1-vega board)
* :github:`34011` - NRF52840 DTS questions
* :github:`34010` - [Coverity CID: 220531] Copy into fixed size buffer in tests/net/socket/misc/src/main.c
* :github:`34009` - [Coverity CID: 220532] Unrecoverable parse warning in subsys/bluetooth/controller/ll_sw/ull_peripheral_iso.c
* :github:`34008` - [Coverity CID: 220533] Improper use of negative value in tests/net/socket/misc/src/main.c
* :github:`34007` - [Coverity CID: 220534] Out-of-bounds access in tests/arch/arm/arm_no_multithreading/src/main.c
* :github:`34006` - [Coverity CID: 220535] Dereference before null check in subsys/net/l2/virtual/virtual.c
* :github:`34005` - [Coverity CID: 220536] Pointer to local outside scope in subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`34004` - [Coverity CID: 220537] Uninitialized pointer read in tests/net/virtual/src/main.c
* :github:`34003` - [Coverity CID: 220538] Logically dead code in subsys/net/l2/virtual/virtual.c
* :github:`34002` - [Coverity CID: 220539] Improper use of negative value in tests/net/socket/misc/src/main.c
* :github:`34001` - [Coverity CID: 220540] Uninitialized scalar variable in samples/drivers/flash_shell/src/main.c
* :github:`34000` - [Coverity CID: 220541] Dereference before null check in subsys/net/lib/capture/capture.c
* :github:`33986` - TCP stack doesn't handle data received in FIN_WAIT_1
* :github:`33983` - example-application module: add trivial driver
* :github:`33981` - example-application module: add board zxa_board_stub
* :github:`33978` - MCP2515 wrong BRP value
* :github:`33977` - Question: How best to contribute drivers upstream?
* :github:`33974` - The stm32wb55rc MCU does not operate on zephyr
* :github:`33969` - Hardfault error caused by ARM Cortex m0 non-4-byte alignment
* :github:`33968` - ESP32 Porting GSM Module Compile Error
* :github:`33967` - The printed total size differs from calculated from .json
* :github:`33966` - STM32: I-cache & D-cache
* :github:`33965` - example-application module: add trivial project
* :github:`33956` - tests: kernel: fpu: Several tests related to fpu fail on nrf5340dk_nrf5340_cpuappns
* :github:`33954` - I2C scan in UART shell is not detecting any I2C devices on ESP32
* :github:`33951` - periodic_adv not working with nRF5340 DK
* :github:`33950` - periodic_adv not working with nRF5340 DK
* :github:`33929` - subsys: logging: Sample app doesn't build if using Werror and logging with latest SDK
* :github:`33925` - Rework hl7800 driver to use new work queue APIs
* :github:`33923` - GSM modem automatic operation selection mode problems
* :github:`33911` - test:twr_ke18f: tests/kernel/sched/schedule_api - kernel_threads_sched_userspace cases meet out our space
* :github:`33904` - having issue compile a shell program and it is bug likely
* :github:`33898` - intel_adsp_cavs15: running testcases failed tests/kernel/workq/work on adsp
* :github:`33897` - Bluetooth: extended advertising can't restart after connection
* :github:`33896` - Device tree: STM32L4 defines can1 node for chips which do not support CAN peripheral
* :github:`33895` - Device tree: STM32L412 and STM32L422 are missing nodes
* :github:`33890` - Continuous Integration check patch false warnings
* :github:`33884` - CORTEX_M_DEBUG_NULL_POINTER_EXCEPTION_DETECTION_NONE is way too long
* :github:`33874` - twister: Add skip as error feature
* :github:`33868` - Bluetooth: controller: connectable advertisement disable race condition
* :github:`33866` - uart: TX_DONE occurs before transmission is complete.
* :github:`33860` - DEPRECATED, a replacement suggestion should be found somewhere
* :github:`33858` - tests: ztest: test trigger_fault_access from tests/ztest/error_hook fails on em_starterkit_em7d_v22
* :github:`33857` - atomic xtensa  build fail
* :github:`33843` - ESP32 example does not connect to WiFi
* :github:`33840` - [Coverity CID: 220301] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33839` - [Coverity CID: 220302] Uninitialized scalar variable in subsys/net/lib/lwm2m/lwm2m_rw_link_format.c
* :github:`33838` - [Coverity CID: 220304] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33837` - [Coverity CID: 220305] Logically dead code in drivers/gpio/gpio_nrfx.c
* :github:`33836` - [Coverity CID: 220306] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33835` - [Coverity CID: 220309] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33834` - [Coverity CID: 220310] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33833` - [Coverity CID: 220311] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33832` - [Coverity CID: 220312] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33831` - [Coverity CID: 220313] Logically dead code in subsys/bluetooth/services/ots/ots_obj_manager.c
* :github:`33830` - [Coverity CID: 220314] Untrusted value as argument in subsys/bluetooth/services/ots/ots_dir_list.c
* :github:`33829` - [Coverity CID: 220315] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33828` - [Coverity CID: 220316] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33827` - [Coverity CID: 220317] Unchecked return value in tests/kernel/pipe/pipe_api/src/test_pipe_contexts.c
* :github:`33826` - [Coverity CID: 220318] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33825` - [Coverity CID: 220319] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33824` - [Coverity CID: 220320] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33823` - [Coverity CID: 220321] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33822` - [Coverity CID: 220413] Explicit null dereferenced in tests/lib/sprintf/src/main.c
* :github:`33821` - [Coverity CID: 220414] Unused value in tests/subsys/logging/log_backend_fs/src/log_fs_test.c
* :github:`33820` - [Coverity CID: 220415] Uninitialized scalar variable in tests/posix/common/src/pthread.c
* :github:`33819` - [Coverity CID: 220417] Out-of-bounds access in subsys/modbus/modbus_core.c
* :github:`33818` - [Coverity CID: 220418] Destination buffer too small in subsys/modbus/modbus_raw.c
* :github:`33817` - [Coverity CID: 220419] Unchecked return value in subsys/bluetooth/host/gatt.c
* :github:`33816` - [Coverity CID: 220420] Out-of-bounds access in tests/subsys/modbus/src/test_modbus_raw.c
* :github:`33815` - [Coverity CID: 220421] Incorrect sizeof expression in tests/lib/cbprintf_package/src/main.c
* :github:`33814` - [Coverity CID: 220422] Extra argument to printf format specifier in tests/lib/sprintf/src/main.c
* :github:`33813` - [Coverity CID: 220423] Out-of-bounds access in subsys/net/l2/ppp/ppp_l2.c
* :github:`33812` - [Coverity CID: 220424] Out-of-bounds access in drivers/watchdog/wdt_mcux_imx_wdog.c
* :github:`33811` - [Coverity CID: 220425] Destination buffer too small in tests/subsys/modbus/src/test_modbus_raw.c
* :github:`33810` - [Coverity CID: 220426] Out-of-bounds access in tests/lib/c_lib/src/main.c
* :github:`33809` - [Coverity CID: 220427] Unchecked return value in tests/posix/common/src/pthread.c
* :github:`33808` - [Coverity CID: 220428] Out-of-bounds access in subsys/bluetooth/audio/vocs.c
* :github:`33807` - [Coverity CID: 220429] Out-of-bounds access in subsys/net/l2/ppp/ppp_l2.c
* :github:`33806` - [Coverity CID: 220430] Operands don't affect result in tests/lib/c_lib/src/main.c
* :github:`33805` - [Coverity CID: 220431] Extra argument to printf format specifier in tests/lib/sprintf/src/main.c
* :github:`33804` - [Coverity CID: 220432] Out-of-bounds access in subsys/net/l2/ethernet/ethernet.c
* :github:`33803` - [Coverity CID: 220433] Printf arg count mismatch in tests/lib/sprintf/src/main.c
* :github:`33802` - [Coverity CID: 220434] Resource leak in tests/lib/mem_alloc/src/main.c
* :github:`33801` - [Coverity CID: 220435] Extra argument to printf format specifier in tests/lib/sprintf/src/main.c
* :github:`33800` - [Coverity CID: 220436] Explicit null dereferenced in tests/lib/sprintf/src/main.c
* :github:`33799` - [Coverity CID: 220437] Wrong size argument in tests/lib/mem_alloc/src/main.c
* :github:`33798` - [Coverity CID: 220438] Out-of-bounds access in subsys/bluetooth/audio/vocs_client.c
* :github:`33797` - [Coverity CID: 220439] Destination buffer too small in tests/subsys/modbus/src/test_modbus_raw.c
* :github:`33796` - [Coverity CID: 220440] Out-of-bounds access in tests/subsys/modbus/src/test_modbus_raw.c
* :github:`33795` - [Coverity CID: 220441] Untrusted loop bound in subsys/modbus/modbus_client.c
* :github:`33794` - [Coverity CID: 220442] Pointless string comparison in tests/lib/c_lib/src/main.c
* :github:`33793` - [Coverity CID: 220443] Out-of-bounds access in tests/subsys/modbus/src/test_modbus_raw.c
* :github:`33792` - [Coverity CID: 220444] Out-of-bounds access in subsys/modbus/modbus_raw.c
* :github:`33791` - [Coverity CID: 220445] Unchecked return value in subsys/logging/log_backend_fs.c
* :github:`33790` - [Coverity CID: 220446] Printf arg count mismatch in tests/lib/sprintf/src/main.c
* :github:`33789` - [Coverity CID: 220447] Out-of-bounds access in subsys/modbus/modbus_raw.c
* :github:`33788` - [Coverity CID: 220448] Out-of-bounds access in tests/subsys/modbus/src/test_modbus_raw.c
* :github:`33787` - [Coverity CID: 220449] Unused value in tests/subsys/logging/log_backend_fs/src/log_fs_test.c
* :github:`33786` - [Coverity CID: 220450] Untrusted loop bound in subsys/modbus/modbus_client.c
* :github:`33785` - [Coverity CID: 220451] Resource leak in tests/lib/mem_alloc/src/main.c
* :github:`33784` - [Coverity CID: 220452] Out-of-bounds access in subsys/net/l2/ethernet/ethernet.c
* :github:`33783` - [Coverity CID: 220453] Extra argument to printf format specifier in tests/lib/sprintf/src/main.c
* :github:`33768` - Rpmsg initialisation on nRF53 may fail
* :github:`33765` - Regular loss of a few connection intervals
* :github:`33761` - Documentation: K_WORK_DEFINE usage is not shown in workqueue doc
* :github:`33754` - xtensa sys timer Interrupt bug？
* :github:`33745` - ``west attach`` silently downgrades to ``debugserver`` for openocd runner
* :github:`33729` - flash_write() in STM32L0 MCU throws hard fault
* :github:`33727` - mec15xxevb_assy6853: multiple tests failed due to assertion failure at kernel/sched.c:841
* :github:`33726` - test:mimxrt1010_evk:  tests/kernel/sched/schedule_api - kernel_threads_sched_userspace cases meet out our space
* :github:`33721` - STM32 serial driver configure api doesn't set correct datalength when even or odd parity is used.
* :github:`33712` - kernel/poll: no error happened when mutil-threads poll a same event at a same time.
* :github:`33702` - cfb sample build error for esp32 when SSD1306 is enabled
* :github:`33697` - dts:dt-bindings No OCTOSPIM dt-bindings available for stm32h723
* :github:`33693` - cmake -E env: unknown option '-Wno-unique_unit_address_if_enabled'
* :github:`33667` - tests: kernel: timer: Test timeout_abs from tests/kernel/timer/timer_api hangs causing test scenarios to fail
* :github:`33665` - tests: kernel: timer_api fails with hard fault in CONFIG_TICKLESS_KERNEL
* :github:`33662` - Make twister dig deeper in directory structure to find additional .yaml files
* :github:`33658` - Question: How is NUM_IRQS determined for example for STM32F401xC
* :github:`33655` - Add support for board: Nucleo-L412RB-P
* :github:`33646` - Expose net_ipv4_create, net_ipv6_create, and net_udp_create in standard header
* :github:`33645` - Random MAC after RESET - NRF52832
* :github:`33641` - API Meeting Minutes
* :github:`33635` - subsys/ipc/openamp sample on QEMU not working when debugging
* :github:`33633` - NXP imx rt1064 evk: Application does not boot when flash/flexSPI driver is enabled
* :github:`33629` - tests: subsys: logging:  Tests from /tests/subsys/logging/log_backend_fs fail on nrf52840dk
* :github:`33625` - NVS: replace dev_name parameter by device reference in nvs_init()
* :github:`33612` - Add support to get adv address of a per_adv_sync object and lookup per_adv_sync object from adv address
* :github:`33610` - ARC: add ARCv3 HS6x support
* :github:`33609` - Question about memory usage of the binary zephyr.exe
* :github:`33600` - Master is broken at build-time when SRAM is mapped at an high address
* :github:`33593` - acrn_ehl_crb: general tests and samples execution slowdown
* :github:`33591` - wordlist (kobject hash) is not generated correctly when using high addresses for SRAM on 64-bit platforms
* :github:`33590` - nrf: Debugging any test fails when CORTEX_M_DEBUG_NULL_POINTER_EXCEPTION_DETECTION_DWT is enabled
* :github:`33589` - SSD1306 driver no longer works for I2C displays
* :github:`33583` - nRF SPI CS control: CS set / release delay is longer than configured
* :github:`33572` -  <err> esp_event: SYSTEM_EVENT_STA_DISCONNECTED for wifi sample for esp32 board
* :github:`33568` - Test tests/arch/x86/info fails for ehl_crb
* :github:`33567` - sof: framework is redefnining MAX, MIN to version with limited capabilities
* :github:`33559` - pin setting error on frdm_kl25z boards
* :github:`33558` - qemu_cortex_a53_smp and qemu_x86_64 failed in tests/kernel/condvar/condvar while enabling for SMP
* :github:`33557` - there is no network interface to work with for  wifi sample for esp32 board
* :github:`33551` - tests: SMP: Two threads synchronize failed using mutex or semaphore while both doing irq_lock()
* :github:`33549` - xt-xcc unknown field 'obj' specified in initializer
* :github:`33548` - xt-xcc does not support deprecated attribute
* :github:`33545` - ehl_crb: tests/arch/x86/info failed.
* :github:`33544` - ehl_crb: portability.posix.common.posix_realtime failed.
* :github:`33543` - ehl_crb: tests/subsys/edac/ibecc failed.
* :github:`33542` - reel_board: samples/subsys/usb/hid/ timeout failure
* :github:`33539` - ehl_crb: tests/kernel/mem_heap/mheap_api_concept failed.
* :github:`33529` - adafruit_feather_nrf52840 dts not setting I2C controller compat (was: SSD1306 DTS properties not being generated in devicetree_unfixed.h)
* :github:`33526` - boards:  Optimal way to have customized dts for my project.
* :github:`33525` - ST Nucleo G071RB board support issue
* :github:`33524` - minor: kswap.h is included twice in kernel/init.c
* :github:`33523` - Bossac runner flashes at an incorrect offset
* :github:`33516` - socket: tcp application crashes when there are no more net buffers in case of reception
* :github:`33515` - arm64/mmu: Are you sure it's OK to use atomic_cas before the MMU is initialized?
* :github:`33512` - build: build target is always out-of-date
* :github:`33509` - samples: tests: watchdog: samples/subsys/task_wdt breaks nrf platforms performace
* :github:`33505` - WS2812 SPI LED driver with DMA on nrf52 bad SPI data
* :github:`33498` - west: Question on ``west flash --hex-file`` behavior with build.dir-fmt
* :github:`33491` - fwrite() function will cause the program to crash when wrong parameters passed
* :github:`33488` - Ring buffer makes it hard to discard items
* :github:`33479` - disk_access_spi_sdhc: Missing stop/end bit
* :github:`33475` - Need to add device node for UART10 in dts/arm/st/h7/stm32h723.dtsi
* :github:`33464` - SYS_INIT initialize priority "2-9" ordering error
* :github:`33459` - Divide zero exception is not enabled in ARC
* :github:`33457` - Fail to build ARC zephyr with MetaWare toolchain
* :github:`33456` - lorawan: unconfirmed messages leave stack in busy state
* :github:`33426` - a few failures with CONFIG_HCI_ACL_DATA_SIZE in nightly builds
* :github:`33424` - tests: ztest: Test from tests/ztest/error_hook fails on nrf5340dk_nrf5340_cpuappns
* :github:`33423` - tests: portability: tests/portability/cmsis_rtos_v2 fails on nrf5340dk_nrf5340_cpuappns
* :github:`33422` - samples/subsys/usb/dfu/sample.usb.dfu fails on multiple platforms in daily build
* :github:`33421` - Add BT_LE_FEAT_BIT_PER_ADV checks for periodic advertising commands
* :github:`33403` - trigger_fault_divide_zero test case didn't run divide instruction
* :github:`33381` - West debug does not work with Bluetooth shell and nRF52840 DK
* :github:`33378` - Extended advertising switch on / switch off loop impossible
* :github:`33374` - Network interface routines are not thread safe
* :github:`33371` - mec15xxevb_assy6853: tests/drivers/gpio/gpio_basic_api/ failed
* :github:`33365` - Add STM32H7 Series USB Device Support
* :github:`33363` - Properly indicate ISR number in SystemView
* :github:`33356` - Using AT HOST  fails build
* :github:`33353` - work: k_work_schedule from running work item does not schedule
* :github:`33352` - Arduino Nano 33 BLE sense constantly resetting.
* :github:`33351` - uart peripheral outputs 7 bits when configured in 8 bits + parity on stm32
* :github:`33348` - ip/dhcpv4 is not thread-safe in SMP/preemptive thread configurations
* :github:`33342` - disco_l475_iot1: Multiple definitions of z_timer_cycle_get_32, etc.
* :github:`33339` - API/functions to get remaining free heap size
* :github:`33330` - Poll on DTLS socket returns -EAGAIN if bind & receive any data.
* :github:`33326` - The gpio-map for adafruit_feather_stm32f405 looks like it contains conflicts
* :github:`33324` - Using bluetooth hci_usb sample, and set periodic adv enable, but bluez still shows no command supported
* :github:`33322` - Questions on ztest : 1) Can twister/ztests run on windows? 2) Project structure
* :github:`33319` - Kernel doesn't validate lock state on swap
* :github:`33318` - [Coverity CID: 219722] Resource leak in tests/lib/mem_alloc/src/main.c
* :github:`33317` - [Coverity CID: 219727] Improper use of negative value in tests/lib/cbprintf_package/src/main.c
* :github:`33316` - [Coverity CID: 219724] Side effect in assertion in tests/kernel/queue/src/test_queue_contexts.c
* :github:`33315` - [Coverity CID: 219723] Side effect in assertion in tests/kernel/queue/src/test_queue_contexts.c
* :github:`33314` - [Coverity CID: 219726] Side effect in assertion in tests/kernel/lifo/lifo_usage/src/main.c
* :github:`33313` - [Coverity CID: 219728] Untrusted array index read in subsys/bluetooth/host/iso.c
* :github:`33312` - [Coverity CID: 219721] Untrusted array index read in subsys/bluetooth/host/iso.c
* :github:`33311` - [Coverity CID: 219729] Logically dead code in lib/os/cbprintf_packaged.c
* :github:`33303` - __ASSERT does not display message or register info in v2.5.0
* :github:`33291` - Using both NET_SOCKETS_SOCKOPT_TLS and POSIX_API fails build
* :github:`33280` - drivers: serial: nrf uarte: The application receives one more byte that was received over UART
* :github:`33273` - The z_smp_reacquire_global_lock() internal API is not used any where inside zephyr code base
* :github:`33269` - ILI9341 (ILI9XXX) set orientation function fails to update the display area correctly
* :github:`33265` - Power Management Overhaul
* :github:`33261` - gatt_notify too slow on Broadcast
* :github:`33253` - STM32G4 with USB-C PD: Some pins cannot be used as input by default
* :github:`33239` - lib/rbtree: Remove dead case in rb_remove()
* :github:`33238` - tests: drivers: pwm api fails on many boards
* :github:`33233` - uart9 missing from <st/h7/stm32h7.dtsi>
* :github:`33218` - Incorrect documentation CONFIG_LOG_STRDUP_MAX_STRING
* :github:`33213` - Configuring a project with a sub-project (e.g. nRF5340) and an overlay causes an infinite configuring loop
* :github:`33212` - GUI configuration system (ninja menuconfig) exists with an error when the windows key is pressed
* :github:`33208` - cbprintf: Package size calculation is using best case alignment
* :github:`33207` - twister: Add option to load list with quarantined tests
* :github:`33203` - Bluetooth: host: ISO: Missing terminate reason in ISO disconnected callback
* :github:`33200` - USB CDC ACM sample application fails to compile
* :github:`33196` - I2C doesn't work on STM32F103RE
* :github:`33185` - TCP traffic with IPSP sample not working on 96Boards Nitrogen
* :github:`33176` - tests: kernel: Multiple test cases from tests/kernel/workq/work_queue are failing
* :github:`33173` - tests/kernel/workq/work_queue fails on sam_e70_xplained
* :github:`33171` - Create Renesas HAL
* :github:`33169` - STM32 SPI Driver - Transmit (MOSI) Only - Infinite Loop on Tranceive
* :github:`33168` - CONFIG_HEAP_MEM_POOL_SIZE=64 doesn't work
* :github:`33164` - Newlib has no synchronization
* :github:`33153` - west flash cannot find OpenOCD
* :github:`33149` - subsys: canbus: canopen EDSEditor / libedssharp version that works with Zephyr's CANopenNode
* :github:`33147` - Not able to build blinky or set toolchain to zephyr
* :github:`33142` - fs_mount for FAT FS does not distingush between no file system and other errors
* :github:`33140` - STM32H7: Bus fault when reading corrupt flash sectors
* :github:`33138` - invalid west cmake diagnostics when using board alias
* :github:`33137` - Enabling DHCP without NET_MGMT shouldn't be allowed
* :github:`33127` - Improve documentation user experience
* :github:`33122` - Device-level Cache API
* :github:`33120` - iotdk: running testcase tests/kernel/mbox/mbox_api/ failed
* :github:`33114` - tests: mbox_api: testcase test_mbox_data_get_null has some bugs.
* :github:`33104` - Updating Zephyr to fix Work Queue Problems
* :github:`33101` - DNS resolver misbehaves if receiving response too late
* :github:`33100` - tcp2 not working with ppp
* :github:`33097` - Coverity ID links in associated GitHub issues are broken
* :github:`33096` - [Coverity CID :215373] Unchecked return value in subsys/net/lib/lwm2m/lwm2m_rd_client.c
* :github:`33095` - [Coverity CID :215379] Out-of-bounds write in subsys/mgmt/osdp/src/osdp_cp.c
* :github:`33094` - [Coverity CID :215381] Resource leak in samples/net/mdns_responder/src/service.c
* :github:`33093` - [Coverity CID :215391] Unchecked return value from library in samples/net/mdns_responder/src/service.c
* :github:`33092` - [Coverity CID :215392] Logically dead code in subsys/mgmt/osdp/src/osdp_cp.c
* :github:`33091` - [Coverity CID :219474] Logically dead code in subsys/bluetooth/controller/ll_sw/ull_scan.c
* :github:`33090` - [Coverity CID :219476] Dereference after null check in subsys/bluetooth/controller/ll_sw/ull_conn.c
* :github:`33089` - [Coverity CID :219556] Self assignment in drivers/espi/host_subs_npcx.c
* :github:`33088` - [Coverity CID :219558] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33087` - [Coverity CID :219559] Out-of-bounds access in tests/arch/arm/arm_interrupt/src/arm_interrupt.c
* :github:`33086` - [Coverity CID :219561] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33085` - [Coverity CID :219562] Out-of-bounds access in tests/bluetooth/tester/src/gatt.c
* :github:`33084` - [Coverity CID :219563] Dereference after null check in arch/x86/core/multiboot.c
* :github:`33083` - [Coverity CID :219564] Untrusted value as argument in subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`33082` - [Coverity CID :219566] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33081` - [Coverity CID :219572] Untrusted value as argument in tests/net/lib/coap/src/main.c
* :github:`33080` - [Coverity CID :219573] Untrusted value as argument in samples/net/sockets/coap_client/src/coap-client.c
* :github:`33079` - [Coverity CID :219574] Side effect in assertion in tests/subsys/edac/ibecc/src/ibecc.c
* :github:`33078` - [Coverity CID :219576] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33077` - [Coverity CID :219579] Out-of-bounds read in drivers/ipm/ipm_nrfx_ipc.c
* :github:`33076` - [Coverity CID :219583] Operands don't affect result in drivers/clock_control/clock_control_npcx.c
* :github:`33075` - [Coverity CID :219588] Out-of-bounds access in tests/bluetooth/tester/src/gatt.c
* :github:`33074` - [Coverity CID :219590] Unchecked return value in subsys/bluetooth/mesh/proxy.c
* :github:`33073` - [Coverity CID :219591] Untrusted divisor in drivers/sensor/bme680/bme680.c
* :github:`33072` - [Coverity CID :219593] Logically dead code in tests/arch/x86/pagetables/src/main.c
* :github:`33071` - [Coverity CID :219595] Dereference before null check in subsys/net/ip/net_context.c
* :github:`33070` - [Coverity CID :219596] Out-of-bounds read in tests/kernel/interrupt/src/dynamic_isr.c
* :github:`33069` - [Coverity CID :219597] Untrusted value as argument in tests/net/lib/coap/src/main.c
* :github:`33068` - [Coverity CID :219598] Out-of-bounds access in tests/bluetooth/tester/src/gatt.c
* :github:`33067` - [Coverity CID :219600] Unchecked return value in drivers/watchdog/wdt_wwdg_stm32.c
* :github:`33066` - [Coverity CID :219601] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33065` - [Coverity CID :219603] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33064` - [Coverity CID :219608] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33063` - [Coverity CID :219609] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33062` - [Coverity CID :219610] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33061` - [Coverity CID :219611] Dereference after null check in subsys/net/ip/tcp2.c
* :github:`33060` - [Coverity CID :219613] Uninitialized scalar variable in lib/cmsis_rtos_v1/cmsis_signal.c
* :github:`33059` - [Coverity CID :219615] Out-of-bounds access in tests/arch/arm/arm_irq_advanced_features/src/arm_zero_latency_irqs.c
* :github:`33058` - [Coverity CID :219616] Untrusted value as argument in subsys/net/lib/coap/coap_link_format.c
* :github:`33057` - [Coverity CID :219619] Untrusted divisor in subsys/bluetooth/controller/hci/hci.c
* :github:`33056` - [Coverity CID :219620] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33055` - [Coverity CID :219621] Uninitialized scalar variable in tests/net/socket/getaddrinfo/src/main.c
* :github:`33054` - [Coverity CID :219622] Untrusted value as argument in subsys/net/lib/coap/coap.c
* :github:`33053` - [Coverity CID :219623] Out-of-bounds read in drivers/ipm/ipm_nrfx_ipc.c
* :github:`33051` - [Coverity CID :219625] Unchecked return value in subsys/bluetooth/mesh/proxy.c
* :github:`33050` - [Coverity CID :219628] Untrusted value as argument in subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`33049` - [Coverity CID :219629] Operands don't affect result in drivers/clock_control/clock_control_npcx.c
* :github:`33048` - [Coverity CID :219631] Out-of-bounds read in drivers/espi/espi_npcx.c
* :github:`33047` - [Coverity CID :219634] Out-of-bounds access in tests/net/lib/dns_addremove/src/main.c
* :github:`33046` - [Coverity CID :219636] Untrusted value as argument in subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`33045` - [Coverity CID :219637] Untrusted value as argument in tests/net/lib/coap/src/main.c
* :github:`33044` - [Coverity CID :219638] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33043` - [Coverity CID :219641] Out-of-bounds access in tests/bluetooth/tester/src/gatt.c
* :github:`33042` - [Coverity CID :219644] Side effect in assertion in tests/subsys/edac/ibecc/src/ibecc.c
* :github:`33040` - [Coverity CID :219646] Untrusted value as argument in subsys/net/lib/coap/coap.c
* :github:`33039` - [Coverity CID :219647] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33038` - [Coverity CID :219649] Operands don't affect result in kernel/sem.c
* :github:`33037` - [Coverity CID :219650] Out-of-bounds access in samples/bluetooth/central_ht/src/main.c
* :github:`33036` - [Coverity CID :219651] Logically dead code in subsys/bluetooth/mesh/net.c
* :github:`33035` - [Coverity CID :219652] Unchecked return value in drivers/gpio/gpio_stm32.c
* :github:`33034` - [Coverity CID :219653] Unchecked return value in drivers/modem/hl7800.c
* :github:`33033` - [Coverity CID :219654] Out-of-bounds access in tests/net/lib/dns_addremove/src/main.c
* :github:`33032` - [Coverity CID :219656] Uninitialized scalar variable in tests/kernel/threads/thread_stack/src/main.c
* :github:`33031` - [Coverity CID :219658] Untrusted value as argument in samples/net/sockets/coap_client/src/coap-client.c
* :github:`33030` - [Coverity CID :219659] Side effect in assertion in tests/subsys/edac/ibecc/src/ibecc.c
* :github:`33029` - [Coverity CID :219660] Untrusted divisor in drivers/sensor/bme680/bme680.c
* :github:`33028` - [Coverity CID :219661] Unchecked return value in subsys/bluetooth/host/gatt.c
* :github:`33027` - [Coverity CID :219662] Inequality comparison against NULL in lib/os/cbprintf_packaged.c
* :github:`33026` - [Coverity CID :219666] Out-of-bounds access in tests/bluetooth/tester/src/gatt.c
* :github:`33025` - [Coverity CID :219667] Untrusted value as argument in tests/net/lib/coap/src/main.c
* :github:`33024` - [Coverity CID :219668] Dereference after null check in drivers/espi/host_subs_npcx.c
* :github:`33023` - [Coverity CID :219669] Untrusted value as argument in subsys/mgmt/updatehub/updatehub.c
* :github:`33022` - [Coverity CID :219672] Untrusted value as argument in samples/net/sockets/coap_server/src/coap-server.c
* :github:`33021` - [Coverity CID :219673] Untrusted value as argument in samples/net/sockets/coap_client/src/coap-client.c
* :github:`33020` - [Coverity CID :219675] Macro compares unsigned to 0 in kernel/include/mmu.h
* :github:`33019` - [Coverity CID :219676] Unchecked return value in drivers/modem/wncm14a2a.c
* :github:`33018` - [Coverity CID :219677] Logically dead code in drivers/timer/npcx_itim_timer.c
* :github:`33009` - kernel: k_heap failures on small heaps
* :github:`33001` - stm32: window watchdog (wwdg): setup of prescaler not valid for newer series
* :github:`33000` - stm32: window watchdog (wwdg): invalid interrupts priority for CM0 Series Socs
* :github:`32996` - SPI speed when using SDHC via SPI in Zephyr
* :github:`32994` - Question: Possible simplification in mutex.h?
* :github:`32975` - Where should a few include/ headers live
* :github:`32969` - Wrong board target in microbit v2 documentation
* :github:`32966` - frdm_k64f: Run some testcases timeout failed by using twister
* :github:`32963` - USB device is not supported by qemu_x86 platform
* :github:`32961` - [Coverity CID :219478] Unchecked return value in subsys/bluetooth/controller/ll_sw/ull_filter.c
* :github:`32960` - [Coverity CID :219479] Out-of-bounds access in subsys/net/l2/bluetooth/bluetooth.c
* :github:`32959` - [Coverity CID :219480] Out-of-bounds read in lib/os/cbprintf_nano.c
* :github:`32958` - [Coverity CID :219481] Out-of-bounds access in subsys/bluetooth/controller/ll_sw/nordic/lll/lll_adv_aux.c
* :github:`32957` - [Coverity CID :219483] Improper use of negative value in tests/drivers/timer/nrf_rtc_timer/src/main.c
* :github:`32956` - [Coverity CID :219484] Out-of-bounds access in tests/drivers/timer/nrf_rtc_timer/src/main.c
* :github:`32955` - [Coverity CID :219485] Logically dead code in subsys/bluetooth/controller/ll_sw/ull.c
* :github:`32954` - [Coverity CID :219486] Unrecoverable parse warning in tests/kernel/mem_protect/mem_protect/src/mem_domain.c
* :github:`32953` - [Coverity CID :219487] Resource leak in tests/net/socket/getaddrinfo/src/main.c
* :github:`32952` - [Coverity CID :219488] Unrecoverable parse warning in tests/kernel/mem_protect/mem_protect/src/mem_domain.c
* :github:`32951` - [Coverity CID :219489] Structurally dead code in tests/drivers/dma/loop_transfer/src/test_dma_loop.c
* :github:`32950` - [Coverity CID :219490] Unsigned compared against 0 in drivers/wifi/esp/esp.c
* :github:`32949` - [Coverity CID :219491] Resource leak in tests/net/socket/af_packet/src/main.c
* :github:`32948` - [Coverity CID :219492] Out-of-bounds access in tests/drivers/timer/nrf_rtc_timer/src/main.c
* :github:`32947` - [Coverity CID :219493] Unchecked return value in tests/kernel/workq/work/src/main.c
* :github:`32946` - [Coverity CID :219494] Logically dead code in boards/xtensa/intel_s1000_crb/pinmux.c
* :github:`32945` - [Coverity CID :219495] Pointless string comparison in tests/lib/devicetree/api/src/main.c
* :github:`32944` - [Coverity CID :219497] Logically dead code in subsys/bluetooth/host/gatt.c
* :github:`32943` - [Coverity CID :219498] Out-of-bounds access in tests/drivers/timer/nrf_rtc_timer/src/main.c
* :github:`32942` - [Coverity CID :219499] Argument cannot be negative in tests/net/socket/af_packet/src/main.c
* :github:`32941` - [Coverity CID :219501] Unchecked return value in subsys/net/l2/bluetooth/bluetooth.c
* :github:`32940` - [Coverity CID :219502] Improper use of negative value in tests/net/socket/af_packet/src/main.c
* :github:`32939` - [Coverity CID :219504] Logically dead code in subsys/net/lib/lwm2m/lwm2m_engine.c
* :github:`32938` - [Coverity CID :219508] Unchecked return value in lib/libc/minimal/source/stdlib/malloc.c
* :github:`32937` - [Coverity CID :219508] Unchecked return value in lib/libc/minimal/source/stdlib/malloc.c
* :github:`32936` - [Coverity CID :219509] Side effect in assertion in tests/net/socket/tcp/src/main.c
* :github:`32935` - [Coverity CID :219510] Improper use of negative value in tests/drivers/timer/nrf_rtc_timer/src/main.c
* :github:`32934` - [Coverity CID :219511] Uninitialized scalar variable in tests/kernel/mbox/mbox_api/src/test_mbox_api.c
* :github:`32933` - [Coverity CID :219512] Unrecoverable parse warning in tests/kernel/mem_protect/mem_protect/src/mem_domain.c
* :github:`32932` - [Coverity CID :219513] Logically dead code in drivers/wifi/esp/esp.c
* :github:`32931` - [Coverity CID :219514] Out-of-bounds access in tests/drivers/timer/nrf_rtc_timer/src/main.c
* :github:`32930` - [Coverity CID :219515] Side effect in assertion in subsys/bluetooth/controller/ll_sw/ull_sched.c
* :github:`32929` - [Coverity CID :219516] Improper use of negative value in tests/drivers/timer/nrf_rtc_timer/src/main.c
* :github:`32928` - [Coverity CID :219518] Macro compares unsigned to 0 in subsys/bluetooth/mesh/transport.c
* :github:`32927` - [Coverity CID :219519] Unchecked return value in drivers/ethernet/eth_sam_gmac.c
* :github:`32926` - [Coverity CID :219520] Unsigned compared against 0 in drivers/wifi/esp/esp.c
* :github:`32925` - [Coverity CID :219521] Unchecked return value in tests/kernel/workq/work/src/main.c
* :github:`32924` - [Coverity CID :219522] Unchecked return value in tests/subsys/dfu/mcuboot/src/main.c
* :github:`32923` - [Coverity CID :219523] Side effect in assertion in subsys/bluetooth/controller/ll_sw/ull_adv_sync.c
* :github:`32922` - [Coverity CID :219524] Logically dead code in drivers/wifi/esp/esp.c
* :github:`32921` - [Coverity CID :219525] Unchecked return value in tests/subsys/settings/functional/src/settings_basic_test.c
* :github:`32920` - [Coverity CID :219526] Operands don't affect result in tests/boards/mec15xxevb_assy6853/qspi/src/main.c
* :github:`32919` - [Coverity CID :219527] Resource leak in tests/net/socket/getaddrinfo/src/main.c
* :github:`32918` - [Coverity CID :219528] Arguments in wrong order in tests/drivers/pwm/pwm_loopback/src/main.c
* :github:`32917` - [Coverity CID :219529] Unchecked return value in subsys/bluetooth/controller/ll_sw/ull_filter.c
* :github:`32916` - [Coverity CID :219530] Dereference before null check in drivers/modem/ublox-sara-r4.c
* :github:`32915` - [Coverity CID :219531] Improper use of negative value in tests/drivers/timer/nrf_rtc_timer/src/main.c
* :github:`32914` - [Coverity CID :219532] Out-of-bounds access in tests/drivers/timer/nrf_rtc_timer/src/main.c
* :github:`32913` - [Coverity CID :219535] Dereference after null check in drivers/sensor/icm42605/icm42605.c
* :github:`32912` - [Coverity CID :219536] Dereference before null check in drivers/ieee802154/ieee802154_nrf5.c
* :github:`32911` - [Coverity CID :219537] Out-of-bounds access in samples/boards/nrf/system_off/src/retained.c
* :github:`32910` - [Coverity CID :219538] Illegal address computation in subsys/bluetooth/controller/ll_sw/nordic/lll/lll_adv_aux.c
* :github:`32909` - [Coverity CID :219540] Side effect in assertion in subsys/bluetooth/controller/ll_sw/ull_sched.c
* :github:`32908` - [Coverity CID :219541] Unused value in subsys/bluetooth/controller/ticker/ticker.c
* :github:`32907` - [Coverity CID :219542] Dereference null return value in subsys/bluetooth/mesh/heartbeat.c
* :github:`32906` - [Coverity CID :219543] Out-of-bounds access in samples/boards/nrf/mesh/onoff_level_lighting_vnd_app/src/smp_svr.c
* :github:`32905` - [Coverity CID :219544] Logically dead code in drivers/wifi/esp/esp.c
* :github:`32904` - [Coverity CID :219545] Side effect in assertion in subsys/bluetooth/controller/ll_sw/ull_adv_aux.c
* :github:`32903` - [Coverity CID :219546] Unchecked return value in tests/kernel/workq/work/src/main.c
* :github:`32902` - [Coverity CID :219547] Improper use of negative value in tests/drivers/timer/nrf_rtc_timer/src/main.c
* :github:`32898` - Bluetooth: controller: Control PDU buffer leak into Data PDU buffer pool
* :github:`32877` - acrn:  test case of kernel.timer and kernel.timer.tickless failed
* :github:`32875` - Benchmarking Zephyr vs. RIOT-OS
* :github:`32867` - tests/kernel/sched/schedule_api does not start with stm32wb55 on nucleo
* :github:`32866` - bt_le_ext_adv_create returns -5 with 0x2036 opcode status 1
* :github:`32862` - MCUboot, use default UART_0 value for RECOVERY_UART_DEV_NAME
* :github:`32860` - Periodic advertising/synchronization on nRF52810
* :github:`32853` - lwm2m: uninitialized variable in this function
* :github:`32839` - tests/kernel/timer/timer_api/test_timeout_abs still fails on multiple platforms
* :github:`32835` - twister: integration_platforms stopped working as it should
* :github:`32828` - tests: posix: Test case portability.posix.common.tls.newlib.posix_realtime fails on nrf9160dk_nrf9160
* :github:`32827` - question: Specify size of malloc arena
* :github:`32818` - Function z_swap_next_thread() missing coverage in sched.c
* :github:`32817` - Supporting fedora in the getting started docs
* :github:`32816` - ehl_crb: tests/kernel/timer/timer_api/timer_api/test_sleep_abs (kernel.timer.tickless) failed.
* :github:`32809` - Fail to build ARC zephyr with MetaWare toolchain
* :github:`32800` - Race conditions with setting thread attributes after ``z_ready_thread``?
* :github:`32798` - west flash fails for reel board
* :github:`32778` - Cannot support both HID boot report keyboard and mouse on a USB HID device
* :github:`32774` - Sensor BMI160: set of undersampling mode is not working
* :github:`32771` - STM32 with Ethernet crashes when receiving packets early
* :github:`32757` - Need openthread merge
* :github:`32755` - mcumgr: shell output gets truncated
* :github:`32745` - Bluetooth PAST shell command
* :github:`32742` - Bluetooth: GAP: LE connection complete event handling priority
* :github:`32741` - ARM32 / ARM64 separation
* :github:`32735` - Subsys: Logging: Functions purely to avoid compiling error affects test coverage
* :github:`32724` - qemu timer change introducing new CI failures
* :github:`32723` - kernel/sched: Only send IPI to abort a thread if the hardware supports it
* :github:`32721` - samples/bluetooth/periodic_adv/, print random address after every reset
* :github:`32720` - ./samples/microbit/central_eatt builds failed at v2.5.0 release
* :github:`32718` - NUCLEO-F446RE: Enable CAN Module
* :github:`32715` - async uart api not working on stm32 with dmamux
* :github:`32705` - KERNEL_COHERENCE on xtensa doesn't quite work yet
* :github:`32702` - RAM overflow with bbc:microbit for samples/bluetooth/peripheral...
* :github:`32699` - Setting custom BOARD_ROOT raises FileNotFoundError
* :github:`32697` - sam_e70b_xplained: running tests/kernel/timer/timer_api failed
* :github:`32696` - intel_adsp_cavs15: run testcases failed of tests/kernel/common
* :github:`32695` - intel_adsp_cavs15: cannot get output of some testcases
* :github:`32691` - Function z_find_first_thread_to_unpend() missing coverage in sched.c
* :github:`32688` - up_squared: tests/kernel/timer/timer_api failed.
* :github:`32679` - twister with --device-testing sometimes overlaps tests
* :github:`32677` - z_user_string_nlen() might lead to non-recoverable errors, despite suggesting the opposite
* :github:`32657` - canopen sample wont respond on pdo mapping with CO_ODs from objdict.eds
* :github:`32656` - reel_board: tests/lib/devicetree/devices/ build failure
* :github:`32655` - reel_board: tests/kernel/timer/timer_api/ failure
* :github:`32644` - Cannot build any project with system-wide DTC
* :github:`32619` - samples: usb: audio: Samples for usb audio fail building
* :github:`32599` - bbc_microbit build failure is blocking CI
* :github:`32589` - ehl_crb: tests/kernel/context fails sporadically
* :github:`32581` - shell/usb cdc_acm: shell does not work on CDC_ACM
* :github:`32579` - Corrupt CBOR payloads in MCUMGR when sending multiple commands together
* :github:`32572` - tests/kernel/timer/timer_api/test_timeout_abs fails on stm32 boards
* :github:`32566` - lwm2m: Long endpoint names are truncated due to short buffer
* :github:`32537` - Fatal error syscall_list.h
* :github:`32536` - Codec phy connection ASSERTION FAIL [event.curr.abort_cb]
* :github:`32515` - zephyr/kernel/thread.c:382 failed
* :github:`32514` - frdm_k64f:running testcase tests/subsys/debug/coredump/ and tests/subsys/debug/coredump_backends/ failed
* :github:`32513` - intel_adsp_cavs15: run testcases of fifo failed
* :github:`32512` - intel_adsp_cavs15: run testcases of queue failed
* :github:`32511` - Zephyr build fail with LLVM on Ubuntu for target ARC
* :github:`32509` - west build -p auto -b nrf52840dk_nrf52840 samples/bluetooth/hci_uart FAILED with  zephyr-v2.5.0
* :github:`32506` - k_sleep: Invalid return value when using absolute timeout.
* :github:`32499` - k_sleep duration is off by 1 tick
* :github:`32497` - No checks of buffer size in l2cap_chan_le_recv
* :github:`32492` - AArch64: Rework secure states (NS vs S) discussion
* :github:`32485` - CONFIG_NFCT_PINS_AS_GPIOS not in Kconfig.soc for nRF53
* :github:`32478` - twister: Twister cannot properly handle runners errors (flashing)
* :github:`32475` - twister error building mcux acmp driver
* :github:`32474` - pm: review post sleep wake up application notification scheduling
* :github:`32469` - Twister: potential race conditions.
* :github:`32468` - up_squared: tests/kernel/smp  failed.
* :github:`32467` - x86_64: page fault with access violation error complaining supervisor thread not allowed to rwx
* :github:`32459` - the cbprintf unit tests don't actually test all variations
* :github:`32457` - samples: drivers: espi: Need to handle failures in temperature retrieval
* :github:`32448` - C++ exceptions do not work on multiple platforms
* :github:`32445` - 2021 GSoC Project Idea: Transplantation for embarc_mli
* :github:`32444` - z_cstart bug
* :github:`32433` - pwm_stm32: warning: non-portable use of "defined"
* :github:`32431` - RTC driver support on STM32F1 series
* :github:`32429` - Only one PWM Channel supported in STM32L4 ?
* :github:`32420` - bbc_microbit_v2 build error for lis2dh
* :github:`32414` - [Coverity CID :218733] Explicit null dereferenced in tests/ztest/error_hook/src/main.c
* :github:`32413` - [Coverity CID :216799] Out-of-bounds access in tests/net/lib/dns_addremove/src/main.c
* :github:`32412` - [Coverity CID :216797] Dereference null return value in tests/net/6lo/src/main.c
* :github:`32411` - [Coverity CID :218744] Out-of-bounds access in subsys/bluetooth/host/gatt.c
* :github:`32410` - [Coverity CID :218743] Out-of-bounds access in subsys/bluetooth/host/gatt.c
* :github:`32409` - [Coverity CID :218742] Out-of-bounds access in subsys/bluetooth/host/gatt.c
* :github:`32408` - [Coverity CID :218741] Out-of-bounds access in subsys/bluetooth/host/att.c
* :github:`32407` - [Coverity CID :218740] Out-of-bounds access in subsys/bluetooth/host/gatt.c
* :github:`32406` - [Coverity CID :218737] Out-of-bounds write in subsys/bluetooth/host/gatt.c
* :github:`32405` - [Coverity CID :218735] Out-of-bounds access in subsys/bluetooth/host/att.c
* :github:`32404` - [Coverity CID :218734] Out-of-bounds access in subsys/bluetooth/host/smp.c
* :github:`32403` - [Coverity CID :218732] Out-of-bounds access in subsys/bluetooth/host/att.c
* :github:`32402` - [Coverity CID :218731] Out-of-bounds access in subsys/bluetooth/shell/gatt.c
* :github:`32401` - [Coverity CID :218730] Operands don't affect result in subsys/bluetooth/host/conn.c
* :github:`32400` - [Coverity CID :218729] Out-of-bounds access in subsys/bluetooth/host/gatt.c
* :github:`32399` - [Coverity CID :218728] Out-of-bounds access in subsys/bluetooth/host/gatt.c
* :github:`32398` - [Coverity CID :218727] Out-of-bounds access in subsys/bluetooth/host/gatt.c
* :github:`32397` - [Coverity CID :218726] Out-of-bounds access in subsys/bluetooth/host/att.c
* :github:`32385` - clock_control: int is returned on enum return value
* :github:`32382` - Clock issues for STM32F103RE custom board
* :github:`32376` - samples: driver: watchdog: Sample fails on disco_l475_iot1
* :github:`32375` - Networking stack crashes when run in cooperative scheduling mode only
* :github:`32365` - samples: hci_rpmsg build fail for nrf5340
* :github:`32344` - sporadic failure in tests/kernel/mem_protect/userspace/kernel.memory_protection.userspace
* :github:`32343` - openthread manual joiner hangs
* :github:`32342` - review use of cpsid in aarch32 / CONFIG_PM
* :github:`32331` - Use of unitialized variables in can_set_bitrate()
* :github:`32321` - /tmp/ccTlcyeD.s:242: Error: Unrecognized Symbol type  " "
* :github:`32320` - drivers: flash: stm32: Flush ART Flash cache after erase operation
* :github:`32314` - 2021 GSoC Project Idea: Enable Interactive Zephyr Test Suite
* :github:`32291` - Zephyr don't build sample hello world for Particle Xenon
* :github:`32289` - USDHC: Fails after reset
* :github:`32279` - Question about flasing Adafruit Feather Sense with Zephyr
* :github:`32270` - TCP connection stalls
* :github:`32269` - shield: cmake: Shield conf is not loaded during build
* :github:`32265` - STM32F4 stuck handling I2C interrupt
* :github:`32261` - problem with CONFIG_STACK_SENTINEL
* :github:`32260` - STM32 counter driver error in estimating alarm time
* :github:`32258` - power mgmt: pm_devices: Get rid of z_pm_core_devices array
* :github:`32257` - Common DFU partition enumeration API
* :github:`32256` - Bluetooth mesh : Long friendship establishment after reset
* :github:`32252` - Building anything for nrf5340pdk_nrf5340_cpuappns/mps2_an521_ns(any non-secure platforms) pulls external git trees
* :github:`32237` - twister failing locally - fails to link native_posix w/lld
* :github:`32234` - Documentation: How to update Zephyr itself (with west)
* :github:`32233` - Often disconnect timeouts when running the BLE peripheral HR sample on Nitrogen96
* :github:`32224` - Treat devicetree binding deprecation usage as build error when running w/twister
* :github:`32212` - Tx power levels are not similar on ADV and CONN modes when set manually (nRF52)
* :github:`32206` - CMSIS-DSP support seems broken on link
* :github:`32205` - [misc] AArch64 improvements and fixes
* :github:`32203` - Cannot set static address when using hci_usb or hci_uart on nRF5340 attached to Linux Host
* :github:`32201` - arch_switch() on ARM64 isn't quite right
* :github:`32197` - arch_switch() on SPARC isn't quite right
* :github:`32195` - ARMv8-nofp support
* :github:`32193` - Plan to support raspberry pi Pico?
* :github:`32158` - twister: inconsistent total testcases number with same configuration
* :github:`32145` - ``kernel threads`` and ``kernel stacks`` deadlock in many scenarios
* :github:`32137` - Provide test execution times per ztest testcase
* :github:`32118` - drivers/flash/soc_flash_nrf: nRF anomaly 242 workaround is not implemented
* :github:`32098` -  Implement a driver for the   Microphone / audio sensor (MP23ABS1) in Sensotile.Box
* :github:`32084` - Custom Log Backend breaks performance
* :github:`32075` - net: lwm2m: Testing Strategy for LWM2M Engine
* :github:`32072` - tests/kernel/common seems to fail on nrf52833dk_nrf52833
* :github:`32071` - devicetree: ``bus:`` does not work in ``child-binding``
* :github:`32052` - p4wq has a race with work item re-use
* :github:`32023` - samples: bluetooth: peripheral_hids: Unable to communicate with paired device after board reset
* :github:`32000` - 2021 GSoC Call for Project Ideas - deadline Feb.19, 2021 12:00PM PST
* :github:`31985` - riscv: Long execution time when TICKLESS_KERNEL=y
* :github:`31982` - tests: kernel: queue: regression introduced by FPU sharing PR #31772
* :github:`31980` - Communicating with BMI160 chip over SPI
* :github:`31969` - test_tcp_fn:net2 mximxrt1060/1064/1050 fails on test_client_invalid_rst with semaphore timed out
* :github:`31964` - up_squared: tests/kernel/timer/timer_api failed.
* :github:`31922` - hci_usb: HCI ACL packet with size divisible by 64 not sent
* :github:`31856` - power: ``device_pm_get_sync`` not thread-safe
* :github:`31854` - undefined reference to ``sys_arch_reboot``
* :github:`31829` - net: lwm2m: Update IPSO objects to version 1.1
* :github:`31799` - uart_configure does not return -ENOTSUP for stm32 uart with 9 bit data length.
* :github:`31774` - Add an application power management sample (PM_POLICY_APP)
* :github:`31762` - ivshmem application in acrn
* :github:`31761` - logging:buffer_write with len=0 causes kernel panic
* :github:`31757` - GCC compiler option should include '-mcpu=hs38' for HSDK
* :github:`31742` - Bluetooth: BLE 5 data throughput to Linux host
* :github:`31721` - tests: nrf: posix: portability.posix.common.tls.newlib fails on nrf9160dk_nrf9160
* :github:`31711` - UART failure with CONFIG_UART_ASYNC_API
* :github:`31613` - Undefined reference errors when using External Library with k_msgq_* calls
* :github:`31588` - Bluetooth: Support for multiple connectable advertising sets with different identities.
* :github:`31585` - BMD345: Extended BLE range with PA/LNA
* :github:`31503` - drivers: i2c: i2c_nrfx_twim  Power Consumption rises after I2C data transfer
* :github:`31416` - ARC MPU version number misuse ver3, should be ver4
* :github:`31348` - twister: CI: Run twister daily builds with "--overflow-as-errors"
* :github:`31323` - Compilation warning regards the SNTP subsys
* :github:`31299` - tests/kernel/mbox/mbox_usage failed on hsdk board
* :github:`31284` - [stm32] PM restore console after sleep mode
* :github:`31280` - devicetree: Add a macro to easily get optionnal devicetree GPIO properties
* :github:`31254` - Bluetooth: Extended advertising with one advertising set fails after the first time
* :github:`31248` - i2c shell functions non-functional on nRF53
* :github:`31217` - Multi-threading flash access is not supported by flash_write_protection_set()
* :github:`31191` - Samples: TF-M: Enable NS test app(s)
* :github:`31179` - ``iso bind`` for slave not working as intended
* :github:`31169` - kconfig configuration (prj.conf) based on zephyr version
* :github:`31164` - Problem to build lorawan from samples
* :github:`31150` - tests/subsys/canbus/isotp/conformance: failing on nucleo_f746zg
* :github:`31149` - tests/subsys/canbus/isotp/implementation: failing on nucleo_f746zg
* :github:`31103` - CMSIS RTOS v2 API implementation bugs in osEventFlagsWait
* :github:`31058` - SWO log backend clock frequency is off on some CPUs
* :github:`31036` - BMI 160 i2c version not working. I modified to i2c in kconfig to make it use i2c.
* :github:`31031` - samples/bluetooth/mesh is not helpful
* :github:`30991` - Unable to add new i2c sensor to nrf/samples
* :github:`30943` - MPU fault with STM32L452
* :github:`30936` - tests: sockets: tcp: add a tls test
* :github:`30929` - PDM Driver for nrf52840dk
* :github:`30841` - How to disable reception of socket CAN frames
* :github:`30771` - Logging: Fault instruction address in the logging thread
* :github:`30770` - mps2_an521: no input to shell from Windows qemu host
* :github:`30618` - Add arduino header/spi support for HSDK board
* :github:`30544` - nrf5340 pwm "Unsupported board: pwm-led0 devicetree alias is not defined"
* :github:`30540` - CONFIG_TRACING not working after updating from v2.1.0 to 2.2.0
* :github:`30520` - recv() call to Ublox Sara R4 cant return 0
* :github:`30465` - Spurious interrupts not handled in ARMv7-R code with GICv2.
* :github:`30441` - hci_uart uses wrong BT_BUF_ACL_SIZE on dual chip solutions + multicore
* :github:`30429` - Thread Border Router with NRC/RCP sample and nrf52840dk not starting
* :github:`30416` - No restore possible with mesh shell app from /tests using qemu_x86 on RaspberryPi3
* :github:`30395` - Add possibility to use alternative list of platforms default for CI runs.
* :github:`30355` - Multiple vlan interfaces on same interface not working
* :github:`30353` - RFC: Logging subsystem overhaul
* :github:`30325` - Stack overflow with http post when using civetweb
* :github:`30204` - Support for Teensy 4.0 and 4.1
* :github:`30195` - Missing error check of device_get_binding() and flash_area_open()
* :github:`30192` - mec15xxevb_assy6853: running tests/subsys/power/power_mgmt_soc failed
* :github:`30162` - Build zephyr with Metaware toolchain for HSDK fails
* :github:`30121` - Make log subsystem power aware
* :github:`30101` - tests should not be silently skipped due to insufficient RAM
* :github:`30074` - Occasional Spinlocks on zephyr 2.4.0 (ASSERTION FAIL [z_spin_lock_valid(l)] @ WEST_TOPDIR/zephyr/include/spinlock.h:92)
* :github:`30055` - Memory corruption for newlib-nano with float printf and disabled heap
* :github:`29946` - SD card initialization is wrong
* :github:`29915` - eth: stm32h747i_disco: sem timeout and hang on debug build
* :github:`29798` - test spi loopback with dma fails on nucleo_f746zg
* :github:`29733` - SAM0 will wake up with interrupted execution after deep sleep
* :github:`29722` - West flash is not able to flash with openocd
* :github:`29689` - tests: drivers: gpio: gpio_basic_api: correct dts binding
* :github:`29610` - documentation says giving a semaphore can release IRQ lock
* :github:`29599` - gPTP: frdm_k64f : can not converge time
* :github:`29581` - LoRaWAN sample for 96b_wistrio - Tx timeout
* :github:`29545` - samples: tfm_integration: tfm_ipc: No module named 'cryptography.hazmat.primitives.asymmetric.ed25519'
* :github:`29526` - generic page pool for MMU-based systems
* :github:`29476` - Samples: TF-M: Add PSA FF API Sample
* :github:`29349` - Using float when driver init in RISC-V arch might cause system to get stuck.
* :github:`29224` - PTS: Test framework: Bluetooth: GATT/SR/GAC/BI-01-C - FAIL
* :github:`29107` - Bluetooth: hci-usb uses non-standard interfaces
* :github:`29038` - drivers/usb/device/usb_dc_native_posix_adapt.c sees weird commands and aborts
* :github:`28901` - implement searchable bitfields
* :github:`28900` - add support for memory un-mapping
* :github:`28873` - z_device_ready() lies
* :github:`28803` - Use generic config option for early platfrom init (AKA "warm boot")
* :github:`28722` - Bluetooth: provide ``struct bt_conn`` to ccc_changed callback
* :github:`28597` - Bluetooth: controller: Redesign the implementation/use of NODE_RX_TYPE_DC_PDU_RELEASE
* :github:`28551` - up_squared: samples/boards/up_squared/gpio_counter failed.
* :github:`28535` - RFC: Add lz4 Data Compresssion library Support
* :github:`28438` - Example downstream manifest+module repo in zephyrproject-rtos
* :github:`28249` - driver: espi: mchp: eSPI OOB driver does not support callbacks for incoming OOB messages from eSPI master.
* :github:`28105` - sporadic "Attempt to resume un-suspended thread object" faults on x86-64
* :github:`28096` - fatfs update to latest upstream version
* :github:`27855` - i2c bitbanging on nrf52840
* :github:`27697` - Add support for passing -nogui 1 to J-Link Commander on MS Windows
* :github:`27692` - Allow to select between advertising packet/scan response for BT LE device name
* :github:`27525` - Including STM32Cube's USB PD support to Zephyr
* :github:`27484` - sanitycheck: Ease error interception from calling script
* :github:`27415` - Decide if we keep a single thread support (CONFIG_MULTITHREADING=n) in Zephyr
* :github:`27356` - deep review and redesign of API for work queue functionality
* :github:`27203` - tests/subsys/storage/flash_map failure on twr_ke18f
* :github:`27048` - Improve out-of-tree driver experience
* :github:`27033` - Update terminology related to I2C
* :github:`27032` - zephyr network stack socket APIs are not thread safe
* :github:`27000` - Avoid oppressive language in code base
* :github:`26952` - SMP support on ARM64 platform
* :github:`26889` - subsys: power: Need syscall that allows to force sleep state
* :github:`26760` - Improve caching configuration and move it to be cross architecture
* :github:`26728` - Allow k_poll for multiple message queues
* :github:`26495` - Make k_poll work with KERNEL_COHERENCE
* :github:`26491` - PCIe: add API to get BAR region size
* :github:`26363` - samples: subsys: canbus: canopen: objdict: CO_OD.h is not normally made.
* :github:`26246` - Printing 64-bit values in LOG_DBG
* :github:`26172` - Zephyr Master/Slave not conforming with Core Spec. 5.2 connection policies
* :github:`26170` - HEXDUMP log gives warning when logging function name.
* :github:`26118` - Bluetooth: controller: nRF5x: refactor radio_nrf5_ppi.h
* :github:`25956` - Including header files from modules into app
* :github:`25865` - Device Tree Memory Layout
* :github:`25775` - [Coverity CID :210075] Negative array index write in samples/net/cloud/mqtt_azure/src/main.c
* :github:`25719` - sanitycheck log mixing between tests
* :github:`25601` - UART input does not work on mps2_an{385,521}
* :github:`25440` - Bluetooth: controller: ensure deferred PDU populations complete on time
* :github:`25389` - driver MMIO address range management
* :github:`25313` - samples:mimxrt1010_evk:samples/subsys/usb/audio: build error no usbd found
* :github:`25187` - Generic ethernet packet filtering based on link (MAC) address
* :github:`24986` - Convert dma_nios2_msgdma to DTS
* :github:`24731` - Bluetooth: controller: Network privacy not respected when address resolution is disabled
* :github:`24625` - lib: drivers: clock: rtc:  rtc api to maintain calendar time through reboot
* :github:`24228` - system power states
* :github:`24142` - NRF5340 PA/LNA support
* :github:`24119` - STM32: SPI: Extend power saving SPI pin config to all stm32 series
* :github:`24113` - STM32 Flash: Device tree updates
* :github:`23727` - RFC: clarification and standardization of ENOTSUP/ENOSYS error returns
* :github:`23520` - JLink Thread-Aware Debugging (RTOS Plugin)
* :github:`23465` - Zephyr Authentication - ZAUTH
* :github:`23449` - "make clean" doesn't clean a lot of generated files
* :github:`23225` - Bluetooth: Quality of service: Adaptive channel map
* :github:`23165` - macOS setup fails to build for lack of "elftools" Python package
* :github:`22965` - 4 byte addressing in spi_nor driver for memory larger than 128Mb.
* :github:`22956` - nios2: k_busy_wait() never returns if called with interrupts locked
* :github:`22731` - Improve docker CI documentation
* :github:`22620` - adapt cpu stats tracing to tracing infrastructure
* :github:`22078` - stm32: Shell module sample doesn't work on nucleo_l152re
* :github:`22061` - Ethernet switch support
* :github:`22060` - Build fails with gcc-arm-none-eabi-9-2019-q4-major
* :github:`22027` - i2c_scanner does not work on olimexino_stm32
* :github:`21993` - Bluetooth: controller: split: Move the LLL event prepare/resume queue handling into LLL
* :github:`21840` - need test case for CONFIG_MULTIBOOT on x86
* :github:`21811` - Investigate using Doxygen-generated API docs vs. Sphinx/Breath API docs
* :github:`21809` - Update document generating tools to newer versions
* :github:`21783` - rename zassert functions
* :github:`21489` - Allow to read any types during discovery
* :github:`21484` - Option for safe k_thread_abort
* :github:`21342` - z_arch_cpu_halt() should enter deep power-down where supported
* :github:`21293` - adding timeout the I2C read/write functions for the stm32 port
* :github:`21136` - ARC: Add support for reduced register file
* :github:`21061` - Document where APIs can be called from using doxygen
* :github:`21033` - Read out heap space used and unallocated
* :github:`20707` - Define GATT service at run-time
* :github:`20576` - DTS overlay files must include full path name
* :github:`20366` - Make babbelsim testing more easily available outside of CI
* :github:`19655` - Milestones toward generalized representation of timeouts
* :github:`19582` - zephyr_library: missing 'kernel-mode' vs 'app-mode' documentation
* :github:`19340` - ARM: Cortex-M: Stack Overflows when building with NO_OPTIMIZATIONS
* :github:`19244` - BLE throughput of DFU by Mcumgr is too slow
* :github:`19224` - deprecate spi_flash_w25qxxdv
* :github:`18934` - Update Documentation To Reflect No Concurrent Multi-Protocol
* :github:`18554` - Tracking Issue for C++ Support as of release 2.1
* :github:`18509` - Bluetooth:Mesh:Memory allocation is too large
* :github:`18351` - logging: 32 bit float values don't work.
* :github:`17991` - Cannot generate coverage reports on qemu_x86_64
* :github:`17748` - stm32: clock-control: Remove usage of SystemCoreClock
* :github:`17745` - stm32: Move clock configuration to device tree
* :github:`17571` - mempool is expensive for cyclic use
* :github:`17486` - nRF52: SPIM: Errata work-around status?
* :github:`17375` - Add VREF, TEMPSENSOR, VBAT internal channels to the stm32 adc driver
* :github:`17353` - Configuring with POSIX_API disables NET_SOCKETS_POSIX_NAMES
* :github:`17314` - doc: add tutorial for using mbed TLS
* :github:`16539` - include/ directory and header cleanup
* :github:`16150` - Make more LWM2M parameters configurable at runtime
* :github:`15855` - Create a reliable footprint tracking benchmark
* :github:`15854` - Footprint Enhancements
* :github:`15738` - Networking with QEMU for mac
* :github:`15497` - USB DFU: STM32: usb dfu mode doesn't work
* :github:`15134` - samples/boards/reel_board/mesh_badge/README.rst : No compilation or flash instructions
* :github:`14996` - enhance mutex tests
* :github:`14973` - samples: Specify the board target required for the sample
* :github:`14806` - Assorted pylint warnings in Python scripts
* :github:`14591` - Infineon Tricore architecture support
* :github:`14581` - Network interfaces should be able to declare if they work with IPv4 and IPv6 dynamically
* :github:`14442` - x86 SOC/CPU definitions need clean up.
* :github:`14309` - Automatic device dependency tracking
* :github:`19760` - Provide command to check board supported features
* :github:`13553` - Ways to reduce Bluetooth Mesh message loss
* :github:`13469` - Shell does not show ISR stack usage
* :github:`13436` - Enabling cooperative scheduling via menuconfig may result in an invalid configuration
* :github:`13091` - sockets: Implement MSG_WAITALL flag
* :github:`12718` - Generic MbedTLS setup doesn't use MBEDTLS_ENTROPY_C
* :github:`12098` - Not possible to print 64 bit decimal values with minimal libc
* :github:`12028` - Enable 16550 UART driver on x86_64
* :github:`11820` - sockets: Implement (POSIX-compatible) timeout support
* :github:`11529` - Get configuration of a running kernel in console
* :github:`11449` - checkpatch warns on .dtsi files about line length
* :github:`11207` - ENOSYS has ambiguous meaning.
* :github:`10621` - RFC: Enable device by using dts, not Kconfig
* :github:`10499` - docs: References to "user threads" are confusing
* :github:`10494` - sockets: Implement MSG_TRUNC flag for recv()
* :github:`10436` - Mess with ssize_t, off_t definitions
* :github:`10268` - Clean up remaining SPI_DW defines in soc.h
* :github:`10042` - MISRA C - Do not cast an arithimetic type to void pointer
* :github:`10041` - MISRA C - types that indicate size and signedness should be used instead of basic numerical types
* :github:`10030` - MISRA C - Document Zephyr's code guideline based on MISRA C 2012
* :github:`9954` - samples/hello_world build failed on Windows/MSYS
* :github:`9895` - MISRA C Add 'u' or 'U' suffix for all unsigned integer constants
* :github:`9894` - MISRA C Make external identifiers distinct according with C99
* :github:`9889` - MISRA C  Avoid implicit conversion between integers and boolean expressions
* :github:`9886` - MISRA C Do not mix signed and unsigned types
* :github:`9885` - MISRA C Do not have dead code
* :github:`9884` - MISRA-C Check return value of a non-void function
* :github:`9882` - MISRA-C - Do not use reserved names
* :github:`9778` - Implement zephyr specific SEGGER_RTT_LOCK and SEGGER_RTT_UNLOCK macros
* :github:`9626` - Add support for the FRDM-KL28Z board
* :github:`9507` - pwm: No clear semantics to stop a PWM leads to diverse implementations
* :github:`9284` - Issues/experience trying to use TI ARM code gen tools in Zephyr
* :github:`8958` - Bluetooth: Proprietary vendor specific opcode discovery
* :github:`8400` - test kernel XIP case seems not well defined
* :github:`8393` - ``CONFIG_MULTITHREADING=n`` builds call ``main()`` with interrupts locked
* :github:`7317` - Add generation of SPDX TagValue documents to each build
* :github:`7297` - STM32: Drivers: Document series support for each driver
* :github:`7246` - esp32 fails to build with xtensa-esp32-elf-gcc: error: unrecognized command line option '-no-pie'
* :github:`7216` - Stop using gcc's "-include" flag
* :github:`7214` - Defines from DTS and Kconfig should be available simultaneously
* :github:`7151` - boards: Move existing boards to "default configuration guidelines"
* :github:`6925` - Provide Reviewer Guidelines as part of the documentation
* :github:`6291` - userspace: support MMU-based memory virtualization
* :github:`6066` - LwM2M: support object versioning in register / discover operations
* :github:`5517` - Expand toolchain/SDK support
* :github:`5325` - Support interaction with console in twister
* :github:`5116` - [Coverity CID: 179986] Null pointer dereferences in /subsys/bluetooth/host/mesh/access.c
* :github:`4911` - Filesystem support for qemu
* :github:`4569` - LoRa:  support LoRa
* :github:`1418` - kconfig options need some cleanup and reorganisation
* :github:`1415` - Problem with forcing new line in generated documentation.
* :github:`1392` - No module named 'elftools'
* :github:`3933` - LWM2M: Create application/link-format writer object to handle discovery formatting
* :github:`3931` - LWM2M: Data Validation Callback
* :github:`3723` - WiFi support for ESP32
* :github:`3675` - LE Adv. Ext.: Extended Scan with PHY selection for non-conn non-scan un-directed without aux packets
* :github:`3674` - LE Adv. Ext.: Non-Connectable and Non-Scannable Undirected without auxiliary packet
* :github:`3634` - ARM: implement NULL pointer protection
* :github:`3514` - Bluetooth: controller: LE Advertising Extensions
* :github:`3487` - Keep Zephyr Device tree Linux compatible
* :github:`3420` - Percepio Tracealyzer Support
* :github:`3280` - Paging Support
* :github:`2854` - Modbus RTU Support
* :github:`2542` - battery: Add standard APIs for Battery Charging and Fuel Gauge Handling (Energy Management)
* :github:`2470` - Supervisory and Monitoring Task
* :github:`2381` - SQL Database
* :github:`2336` - IPv4 - Multicast Join/Leave Support
