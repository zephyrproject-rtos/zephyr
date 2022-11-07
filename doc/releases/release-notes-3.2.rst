:orphan:

.. _zephyr_3.2:

Zephyr 3.2.0
############

We are pleased to announce the release of Zephyr version 3.2.0.

Major enhancements with this release include:

* Introduced :ref:`sysbuild`.
* Added support for :ref:`bin-blobs` (also see :ref:`west-blobs`).
* Added support for Picolibc (see :kconfig:option:`CONFIG_PICOLIBC`).
* Converted all supported boards from ``pinmux`` to :ref:`pinctrl-guide`.
* Initial support for :ref:`i3c_api` controllers.
* Support for :ref:`W1 api<w1_api>`.
* Improved access to Devicetree compatibles from Kconfig (new generated
  ``DTS_HAS_..._ENABLED`` configs).

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2022-2993: Under embargo until 2022-11-03

* CVE-2022-2741: Under embargo until 2022-10-14

API Changes
***********

Changes in this release
=======================

* Zephyr now requires Python 3.8 or higher

* Changed :c:struct:`spi_cs_control` to remove anonymous struct.
  This causes possible breakage for static initialization of the
  struct.  Updated :c:macro:`SPI_CS_CONTROL_PTR_DT` to reflect
  this change.

* The :kconfig:option:`CONFIG_LEGACY_INCLUDE_PATH` option has been disabled by
  default, all upstream code and modules have been converted to use
  ``<zephyr/...>`` header paths. The option is still available to facilitate
  the migration of external applications, but will be removed with the 3.4
  release.  The :zephyr_file:`scripts/utils/migrate_includes.py` script is
  provided to automate the migration.

* :zephyr_file:`include/zephyr/zephyr.h` no longer defines ``__ZEPHYR__``.
  This definition can be used by third-party code to compile code conditional
  to Zephyr. The definition is already injected by the Zephyr build system.
  Therefore, any third-party code integrated using the Zephyr build system will
  require no changes. External build systems will need to inject the definition
  by themselves, if they did not already.

* :zephyr_file:`include/zephyr/zephyr.h` has been deprecated in favor of
  :zephyr_file:`include/zephyr/kernel.h`, since it only included that header. No
  changes are required by applications other than replacing ``#include
  <zephyr/zephyr.h>`` with ``#include <zephyr/kernel.h>``.

* Bluetooth: Applications where :kconfig:option:`CONFIG_BT_EATT` is enabled
  must set the :c:member:`chan_opt` field on the GATT parameter structs.
  To keep the old behavior use :c:enumerator:`BT_ATT_CHAN_OPT_NONE`.

* CAN

  * The Zephyr SocketCAN definitions have been moved from :zephyr_file:`include/zephyr/drivers/can.h`
    to :zephyr_file:`include/zephyr/net/socketcan.h`, the SocketCAN ``struct can_frame`` has been
    renamed to :c:struct:`socketcan_frame`, and the SocketCAN ``struct can_filter`` has been renamed
    to :c:struct:`socketcan_filter`. The SocketCAN utility functions are now available in
    :zephyr_file:`include/zephyr/net/socketcan_utils.h`.

  * The CAN controller ``struct zcan_frame`` has been renamed to :c:struct:`can_frame`, and ``struct
    zcan_filter`` has been renamed to :c:struct:`can_filter`.

  * The :c:enum:`can_state` enumerations have been renamed to contain the word STATE in order to make
    their context more clear:

    * ``CAN_ERROR_ACTIVE`` renamed to :c:enumerator:`CAN_STATE_ERROR_ACTIVE`.
    * ``CAN_ERROR_WARNING`` renamed to :c:enumerator:`CAN_STATE_ERROR_WARNING`.
    * ``CAN_ERROR_PASSIVE`` renamed to :c:enumerator:`CAN_STATE_ERROR_PASSIVE`.
    * ``CAN_BUS_OFF`` renamed to :c:enumerator:`CAN_STATE_BUS_OFF`.

  * The error code for :c:func:`can_send` when the CAN controller is in bus off state has been
    changed from ``-ENETDOWN`` to ``-ENETUNREACH``. A return value of ``-ENETDOWN`` now indicates
    that the CAN controller is in :c:enumerator:`CAN_STATE_STOPPED`.

  * The list of valid return values for the CAN timing calculation functions have been expanded to
    allow distinguishing between an out of range bitrate/sample point, an unsupported bitrate, and a
    resulting sample point outside the guard limit.

* Memory Management Drivers

  * Added :c:func:`sys_mm_drv_update_page_flags` and
    :c:func:`sys_mm_drv_update_region_flags` to update flags associated
    with memory pages and regions.

Removed APIs in this release
============================

* The following functions, macros, and structures related to the
  deprecated kernel work queue API have been removed:

  * ``k_work_pending()``
  * ``k_work_q_start()``
  * ``k_delayed_work``
  * ``k_delayed_work_init()``
  * ``k_delayed_work_submit_to_queue()``
  * ``k_delayed_work_submit()``
  * ``k_delayed_work_pending()``
  * ``k_delayed_work_cancel()``
  * ``k_delayed_work_remaining_get()``
  * ``k_delayed_work_expires_ticks()``
  * ``k_delayed_work_remaining_ticks()``
  * ``K_DELAYED_WORK_DEFINE``

* Removed support for enabling passthrough mode on MPU9150 to
  AK8975 sensor.

* Removed deprecated SPI :c:struct:`spi_cs_control` fields for GPIO management
  that have been replaced with :c:struct:`gpio_dt_spec`.

* Removed support for configuring the CAN-FD maximum DLC value via Kconfig
  ``CONFIG_CANFD_MAX_DLC``.

* Removed deprecated civetweb module and the associated support code and samples.

Deprecated in this release
==========================

* :c:macro:`DT_SPI_DEV_CS_GPIOS_LABEL` and
  :c:macro:`DT_INST_SPI_DEV_CS_GPIOS_LABEL` are deprecated in favor of
  utilizing :c:macro:`DT_SPI_DEV_CS_GPIOS_CTLR` and variants.

* :c:macro:`DT_GPIO_LABEL`, :c:macro:`DT_INST_GPIO_LABEL`,
  :c:macro:`DT_GPIO_LABEL_BY_IDX`, and :c:macro:`DT_INST_GPIO_LABEL_BY_IDX`,
  are deprecated in favor of utilizing :c:macro:`DT_GPIO_CTLR` and variants.

* :c:macro:`DT_LABEL`, and :c:macro:`DT_INST_LABEL`, are deprecated
  in favor of utilizing :c:macro:`DT_PROP` and variants.

* :c:macro:`DT_BUS_LABEL`, and :c:macro:`DT_INST_BUS_LABEL`, are deprecated
  in favor of utilizing :c:macro:`DT_BUS` and variants.

* STM32 LPTIM domain clock should now be configured using devicetree.
  Related Kconfig :kconfig:option:`CONFIG_STM32_LPTIM_CLOCK` option is now
  deprecated.

* ``label`` property from devicetree as a base property. The property is still
  valid for specific bindings to specify like :dtcompatible:`gpio-leds` and
  :dtcompatible:`fixed-partitions`.

* Bluetooth mesh Configuration Client API prefixed with ``bt_mesh_cfg_``
  is deprecated in favor of a new API with prefix ``bt_mesh_cfg_cli_``.

* Pinmux API is now officially deprecated in favor of the pin control API.
  Its removal is scheduled for the 3.4 release. Refer to :ref:`pinctrl-guide`
  for more details on pin control.

* Flash Map API macros :c:macro:`FLASH_MAP_`, which have been using DTS node label
  property to reference partitions, have been deprecated and replaced with
  :c:macro:`FIXED_PARTITION_` whch use DTS node label instead.
  Replacement list:

  .. table::
     :align: center

     +-----------------------------------+------------------------------------+
     | Deprecated, takes label property  | Replacement, takes DTS node label  |
     +===================================+====================================+
     | :c:macro:`FLASH_AREA_ID`          | :c:macro:`FIXED_PARTITION_ID`      |
     +-----------------------------------+------------------------------------+
     | :c:macro:`FLASH_AREA_OFFSET`      | :c:macro:`FIXED_PARTITION_OFFSET`  |
     +-----------------------------------+------------------------------------+
     | :c:macro:`FLASH_AREA_SIZE`        | :c:macro:`FIXED_PARTITION_SIZE`    |
     +-----------------------------------+------------------------------------+
     | :c:macro:`FLASH_AREA_LABEL_EXISTS`| :c:macro:`FIXED_PARTITION_EXISTS`  |
     +-----------------------------------+------------------------------------+
     | :c:macro:`FLASH_AREA_DEVICE`      | :c:macro:`FIXED_PARTITION_DEVICE`  |
     +-----------------------------------+------------------------------------+

  :c:macro:`FLASH_AREA_LABEL_STR` is deprecated with no replacement as its sole
  purpose was to obtain the DTS node property label.

Stable API changes in this release
==================================

New APIs in this release
========================

* CAN

  * Added :c:func:`can_start` and :c:func:`can_stop` API functions for starting and stopping a CAN
    controller. Applications will need to call :c:func:`can_start` to bring the CAN controller out
    of :c:enumerator:`CAN_STATE_STOPPED` before being able to transmit and receive CAN frames.
  * Added :c:func:`can_get_capabilities` for retrieving a bitmask of the capabilities supported by a
    CAN controller.
  * Added :c:enumerator:`CAN_MODE_ONE_SHOT` for enabling CAN controller one-shot transmission mode.
  * Added :c:enumerator:`CAN_MODE_3_SAMPLES` for enabling CAN controller triple-sampling receive
    mode.

* I3C

  * Added a set of new API for I3C controllers.

* W1

  * Introduced the :ref:`W1 api<w1_api>`, used to interact with 1-Wire masters.

Kernel
******

* Source files using multiple :c:macro:`SYS_INIT` macros with the
  same initialisation function must now use :c:macro:`SYS_INIT_NAMED`
  with unique names per instance.

Architectures
*************

* ARC

  * Added support of non-multithreading mode for all UP ARC targets.
  * Added extra compile-time checks of :kconfig:option:`CONFIG_ISR_STACK_SIZE`
    and :kconfig:option:`CONFIG_ARC_EXCEPTION_STACK_SIZE` value.
  * Added support of generation symbol file for ARC MWDT toolchain variant.
  * Added ARC MWDT toolchain version check.
  * Added support for GCC mcpu option tuning for ARC targets on SoC level.
  * Switched ARCv3 64bit targets for usage of new linker output format value.
  * Added ARCv3 64bit accumulator reg save / restore, cleanup it for ARCv3
    32bit targets.
  * Fixed SMP race in ASM ARC interrupt handling code.

* ARM

  * Improved HardFault handling on Cortex-M.
  * Enabled automatic placement of the IRQ vector table.
  * Enabled S2RAM for Cortex-M, hooking up the provided API functions.
  * Added icache and dcache maintenance functions, and switched to the new
    Kconfig symbols (:kconfig:option:`CONFIG_CPU_HAS_DCACHE` and
    :kconfig:option:`CONFIG_CPU_HAS_ICACHE`).
  * Added data/instr. sync barriers after writing to ``SCTLR`` to disable MPU.
  * Use ``spsr_cxsf`` instead of unpredictable ``spsr_hyp`` on Cortex-R52.
  * Removes ``-Wstringop-overread`` warning with GCC 12.
  * Fixed handling of system off failure.
  * Fixed issue with incorrect ``ssf`` under bad syscall.
  * Fixed region check issue with mmu.

* ARM64

  * :c:func:`arch_mem_map` now supports :c:enumerator:`K_MEM_PERM_USER`.
  * Added :kconfig:option:`CONFIG_WAIT_AT_RESET_VECTOR` to spin at reset vector
    allowing a debugger to be attached.
  * Implemented erratum 822227 "Using unsupported 16K translation granules
    might cause Cortex-A57 to incorrectly trigger a domain fault".
  * Enabled single-threaded support for some platforms.
  * IRQ stack is now initialized when :kconfig:option:`CONFIG_INIT_STACKS` is set.
  * Fixed issue when cache API are used from userspace.
  * Fixed issue about the way IPI are delivered.
  * TF-A (TrustedFirmware-A) is now shipped as module.

* RISC-V

  * Introduced support for RV32E.
  * Reduced callee-saved registers for RV32E.
  * Introduced Zicsr, Zifencei and BitManip as separate extensions.
  * Introduced :kconfig:option:`CONFIG_RISCV_ALWAYS_SWITCH_THROUGH_ECALL` for
    plaforms that require every ``mret`` to be balanced by ``ecall``.
  * IRQ vector table is now used for vectored mode.
  * Disabled :kconfig:option:`CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE` for CLIC.
  * ``STRINGIFY`` macro is now used for CSR helpers.
  * :kconfig:option:`CONFIG_CODE_DATA_RELOCATION` is now supported.
  * PLIC and CLIC are now decoupled.
  * ``jedec,spi-nor`` is no longer required to be ``okay`` by the RISC-V arch
    linker script.
  * Removed usage of ``SOC_ERET``.
  * Removed usage of ``ulong_t``.
  * Added new TLS-based :c:func:`arch_is_user_context` implementation.
  * Fixed PMP for builds with SMP enabled.
  * Fixed the per-thread m-mode/u-mode entry array.
  * :c:func:`semihost_exec` function is now aligned at 16-byte boundary.

* Xtensa

  * Macros ``RSR`` and ``WSR`` have been renamed to :c:macro:`XTENSA_RSR`
    and :c:macro:`XTENSA_WSR` to give them proper namespace.
  * Fixed a rounding error in timing function when converting from cycles
    to nanoseconds.
  * Fixed the calculation of average "cycles to nanoseconds" to actually
    return nanoseconds instead of cycles.

Bluetooth
*********

* Audio

  * Implemented central security establishment when required.
  * Added additional security level options to the connection call.
  * Switched the unicast client and server to bidirectional CIS if available.
  * Added a new RSI advertising callback for CSIS.
  * Added multiple improvements to context handling, including public functions
    to get contexts.
  * Added ordered access procedure for the CSIS client, as well as storing
    active members by rank.
  * Added support for Write Preset Name in HAS.
  * Added support for using PACS for the broadcast sink role.
  * Cleaned up the MICP implementation, including renaming several structures
    and functions.
  * Implemented the CAP Acceptor role.
  * Added ASCS Metadata verification support.
  * Started exposing broadcast sink advertising data to the application.
  * Added support for unicast server start, reconfigure, release, disable and
    metadata.
  * Added support for multi-CIS.
  * Implemented HAS client support for preset switching.
  * Added support for setting vendor-specific non-HCI data paths for audio
    streams.

* Direction Finding

  * Added support for selectable IQ samples conversion to 8-bit.
  * Added support for VS IQ sample reports in ``int16_t`` format.

* Host

  * Added support for LE Secure Connections permission checking.
  * Added support for Multiple Variable Length Read procedure without EATT.
  * Added a new callback :c:func:`rpa_expired` in the struct
    :c:struct:`bt_le_ext_adv_cb` to enable synchronization of the advertising
    payload updates with the Resolvable Private Address (RPA) rotations when
    the :kconfig:option:`CONFIG_BT_PRIVACY` is enabled.
  * Added a new :c:func:`bt_le_set_rpa_timeout()` API call to dynamically change
    the the Resolvable Private Address (RPA) timeout when the
    :kconfig:option:`CONFIG_BT_RPA_TIMEOUT_DYNAMIC` is enabled.
  * Added :c:func:`bt_conn_auth_cb_overlay` to overlay authentication callbacks
    for a Bluetooth LE connection.
  * Removed ``CONFIG_BT_HCI_ECC_STACK_SIZE``. A new Bluetooth long workqueue
    (:kconfig:option:`CONFIG_BT_LONG_WQ`) is used for processing ECC commands
    instead of the former dedicated thread.
  * :c:func:`bt_conn_get_security` and :c:func:`bt_conn_enc_key_size` now take
    a ``const struct bt_conn*`` argument.
  * The handling of GATT multiple notifications has been rewritten, and is now
    only to be used as a low-level API.
  * Added support for GATT CCCs in arbitrary locations as a client.
  * Extended the :c:struct:`bt_conn_info` structure with security information.
  * Added a new :kconfig:option:`CONFIG_BT_PRIVACY_RANDOMIZE_IR` that prevents
    the Host from using Controller-provided identity roots.
  * Added support for GATT over EATT.
  * Implemented the Immediate Alert Client.

* Mesh

  * Added support for selectable RPL backends.
  * Changed the way segmented messages are sent, avoiding bulk transmission.
  * Added an async config client API.
  * Added model publish support to the Health Client.
  * Moved relayed messages to a separate buffer pool.
  * Reduced delay of sending segment acknowledge message. Set
    :kconfig:option:`CONFIG_BT_MESH_SEG_ACK_PER_SEGMENT_TIMEOUT` to 100 to get
    the previous timing.
  * Restructured shell commands.

* Controller

  * Made the new LLCP implementation the default one. Enable
    :kconfig:option:`CONFIG_BT_LL_SW_LLCP_LEGACY` to revert back to the legacy
    implementation. :kconfig:option:`CONFIG_BT_LL_SW_LLCP_LEGACY` is marked
    deprecated in favor of the new :kconfig:option:`CONFIG_BT_LL_SW_LLCP`, which
    is the default now.
  * Marked Extended Advertising as stable, no longer experimental.
  * Added deinit() infrastructure in order to properly support disabling
    Bluetooth support, including the controller.
  * Implemented the Peripheral CIS Create procedure.
  * Implemented the CIS Terminate procedure.
  * Added support for Periodic Advertising ADI.
  * Implemented support for Extended Scan Response Data fragment operations.
  * Enable back-to-back PDU chaining for AD data.
  * Added a new :kconfig:option:`CONFIG_BT_CTLR_SYNC_PERIODIC_SKIP_ON_SCAN_AUX`
    for allowing periodic sync event skipping.
  * Added a new :kconfig:option:`CONFIG_BT_CTLR_SCAN_AUX_SYNC_RESERVE_MIN` for
    minimal time reservation.
  * Implemented ISO Test Mode HCI commands.
  * Added support for multiple BIS sync selection within a BIG.
  * Implement flushing pending ISO TX PDUs when a BIG event is terminated.
  * Added a new :kconfig:option:`CONFIG_BT_CTLR_ADV_DATA_CHAIN` to enable
    experimental Advertising Data chaining support.

* HCI Driver

  * Added a new Telink B91 HCI driver.

Boards & SoC Support
********************

* Added support for these SoC series:

  * Atmel SAML21, SAMR34, SAMR35
  * GigaDevice GD32E50X
  * GigaDevice GD32F470
  * NXP i.MX8MN, LPC55S36, LPC51U68
  * renesas_smartbond da1469x SoC series

* Made these changes in other SoC series:

  * gigadevice: Enable SEGGER RTT
  * Raspberry Pi Pico: Added ADC support
  * Raspberry Pi Pico: Added PWM support
  * Raspberry Pi Pico: Added SPI support
  * Raspberry Pi Pico: Added watchdog support

* Changes for ARC boards:

  * Added support for qemu_arc_hs5x board (ARCv3, 32bit, UP, HS5x)
  * Simplified multi-runner setup for SMP nSIM ARC platforms
  * Fixed mdb execution folder for mdb-based west runners (mdb-nsim and mdb-hw)

* Added support for these ARM boards:

  * Arduino MKR Zero
  * Atmel atsaml21_xpro
  * Atmel atsamr34_xpro
  * Blues Wireless Swan
  * Digilent Zybo
  * EBYTE E73-TBB
  * GigaDevice GD32E507V-START
  * GigaDevice GD32E507Z-EVAL
  * GigaDevice GD32F407V-START
  * GigaDevice GD32F450V-START
  * GigaDevice GD32F450Z-EVAL
  * GigaDevice GD32F470I-EVAL
  * NXP lpcxpresso51u68, RT1060 EVKB
  * NXP lpcxpresso55s36
  * Olimex LoRa STM32WL DevKit
  * PAN1770 Evaluation Board
  * PAN1780 Evaluation Board
  * PAN1781 Evaluation Board
  * PAN1782 Evaluation Board
  * ST STM32F7508-DK Discovery Kit
  * TDK RoboKit 1
  * WeAct Studio Black Pill V1.2
  * WeAct Studio Black Pill V3.0
  * XIAO BLE
  * da1469x_dk_pro

* Added support for these ARM64 boards:

  * i.MX8M Nano LPDDR4 EVK board series

* Added support for these RISC-V boards:

  * ICE-V Wireless
  * RISCV32E Emulation (QEMU)

* Added support for these Xtensa boards:

  * ESP32-NET
  * intel_adsp_ace15_mtpm

* Removed support for these Xtensa boards:

  * Intel S1000

* Made these changes in other boards:

  * sam_e70_xplained: Uses EEPROM devicetree bindings for Ethernet MAC
  * sam_v71_xult: Uses EEPROM devicetree bindings for Ethernet MAC
  * rpi_pico: Added west runner configurations for Picoprobe, Jlink and Blackmagicprobe

* Added support for these following shields:

  * ARCELI W5500 ETH
  * MAX7219 LED display driver shield
  * Panasonic Grid-EYE (AMG88xx)

Build system and infrastructure
*******************************

* Introduced sysbuild, a new higher-level build system layer that enables
  combining multiple build systems together. It can be used to generate multiple
  images from a single build system invocation while maintaining a link between
  those different applications/images via a shared Kconfig layer.
* Introduced support for binary blobs in west, via a new ``west blobs`` command
  that allows users to list, fetch and delete binary blobs from their
  filesystem. Vendors can thus now integrate binary blobs, be it images or
  libraries, with upstream Zephyr.
* Removed deprecated ``GCCARMEMB_TOOLCHAIN_PATH`` setting.

Drivers and Sensors
*******************

* ADC

  * STM32: Now supports Vbat monitoring channel and STM32U5 series.
  * Added driver for GigaDevice GD32 SoCs.
  * Raspberry Pi Pico: Added ADC support for the Pico series.
  * Added :c:struct:`adc_dt_spec` related helpers for sequence initialization,
    setting up channels, and converting raw values to millivolts.
  * Fixed :c:macro:`ADC_DT_SPEC_GET` and related macros to properly handle
    channel identifiers >= 10.

* CAN

  * A driver for bridging from :ref:`native_posix` to Linux SocketCAN has been added.
  * A driver for the Espressif ESP32 TWAI has been added. See the
    :dtcompatible:`espressif,esp32-twai` devicetree binding for more information.
  * The STM32 CAN-FD CAN driver clock configuration has been moved from Kconfig to :ref:`devicetree
    <dt-guide>`. See the :dtcompatible:`st,stm32-fdcan` devicetree binding for more information.
  * The filter handling of STM32 bxCAN driver has been simplified and made more reliable.
  * The STM32 bxCAN driver now supports dual intances.
  * The CAN loopback driver now supports CAN-FD.
  * The CAN shell module has been rewritten to properly support the additions and changes to the CAN
    controller API.
  * The Zephyr network CAN bus driver, which provides raw L2 access to the CAN bus via a CAN
    controller driver, has been moved to :zephyr_file:`drivers/net/canbus.c` and can now be enabled
    using :kconfig:option:`CONFIG_NET_CANBUS`.
  * Added CAN support for NXP LPC55S36.

* Clock control

  * STM32: PLL_P, PLL_Q, PLL_R outputs can now be used as domain clock.
  * Added driver for GigaDevice GD32 SoCs (peripheral clocks configuration only).
  * Documented behavior when clock is not on.

* Counter

  * Added :c:func:`counter_get_value_64` function.
  * STM32: RTC : Now supports STM32U5 and STM32F1 series.
  * STM32: Timer : Now supports STM32L4 series.
  * Added counter support using CTimer for NXP MIMXRT595.
  * ESP32: Added support to Pulse Counter Mode (PCNT) and RTC.

* Crypto

  * Added Intel ADSP sha driver.
  * stm32: Check if clock device is ready before accessing clock control
    devices.
  * ataes132a: Convert to devicetree.

* DFU

  * Fixed fetch of the flash write block size from incorect device by
    ``flash_img``.
  * Fixed possible build failure in the image manager for mcuboot on
    redefinitions of :c:macro:`BOOT_MAX_ALIGN` and :c:macro:`BOOT_MAGIC_SZ`.

* Display

  * Renamed EPD controller driver GD7965 to UC81xx.
  * Improved support for different controllers in ssd16xx and uc81xx drivers.
  * Added basic read support for ssd16xx compatible EPD controllers.
  * Revised intel_multibootfb driver.
  * Added MAX7219 display controller driver.

* Disk

  * Added support for DMA transfers when using STM32 SD host controller.
  * Added support for SD host controller present on STM32L5X family.

* DMA

  * STM32: Now supports stm32u5 series.
  * cAVS drivers renamed with the broader Intel ADSP naming.
  * Kconfig depends on improvements with device tree statuses.
  * Added driver for GigaDevice GD32 SoCs.
  * Added DMA support for NXP MIMXRT595.

* EEPROM

  * Added Microchip XEC (MEC172x) on-chip EEPROM driver. See the
    :dtcompatible:`microchip,xec-eeprom` devicetree binding for more information.

* Entropy

  * Update drivers to use devicetree Kconfig symbol.
  * gecko: Add driver using Secure Element module of EFR3.
  * Added entropy driver for MCUX CAAM.
  * stm32: Check if clock device is ready before accessing.

* ESPI

  * eSPI emulator initialization improvements.
  * Nuvoton: Enabled platform specific Virtual Wire GPIO.
  * Microchip: Added XEC (MEC152x) overcurrent platform-specific virtual wires.
  * Nuvoton: Added driver flash channel operations support.

* Ethernet

  * Atmel gmac: Add EEPROM devicetree bindings for MAC address.
  * Performance improvements on the NXP MCUX Ethernet Driver.

* Flash

  * Atmel eefc: Fix support for Cortex-M4 variants.
  * Added flash driver for Renesas Smartbond platform
  * Added support for STM32H7 and STM32U5 in the STM32 OSPI driver.
  * Added DMA transfer support in the STM32 OSPI driver.
  * Added driver for GigaDevice GD32 SoCs
  * Added Flash support for NXP LPCXpresso55S36.
  * Added Flash support for NXP MIMXRT595 EVK.
  * Added on-chip flash driver for TI CC13xx/CC26xx.
  * Fixed flash to flash write for Telink B91.
  * Fixed DMA priority configuration in the stm32 QSPI driver.
  * Drivers are enabled by default based on their devicetree hardware declarations.
  * Fixed write from unaligned source for STM32G0x.
  * Added Flash support for Renesas Smartbond platform.
  * Added Flash support for Cadence QSPI NOR FLASH.
  * Fixed usage fault on nRF driver (along with BLE) due to possible incorrect handling of the ticker stop operation.

* GPIO

  * Added GPIO driver for Renesas Smartbond platform.

* I2C

  * Terminology updated to latest i2c specification removing master/slave
    terminology and replacing with controller/target terminology.
  * Asynchronous APIs added for requesting i2c transactions without
    waiting for the completion of them.
  * Added NXP LPI2C driver asynchronous i2c implementation with sample
    showing usage with a FRDM-K64F board.
  * STM32: support for second target address was added.
  * Kconfig depends on improvements with device tree statuses.
  * Improved ITE I2C support with FIFO and command queue mode.
  * Improve gd32 driver stability (remove repeated START, use STOP + START conditions instead).
  * Fixed gd32 driver incorrect Fast-mode config.
  * Add bus recovery support to the NXP MCUX LPI2C driver.
  * Enable I2C support on NXP MIMXRT595 EVK.

* I2S

  * Removed the Intel S1000 I2S driver.

* I3C

  * Added a driver to support the NXP MCUX I3C hardware acting as the primary controller
    on the bus (tested using RT685).

* IEEE 802.15.4

  * All IEEE 802.15.4 drivers have been converted to Devicetree-based drivers.
  * Atmel AT86RF2xx: Add Power Table on devicetree.
  * Atmel AT86RF2xx: Add support to RF212/212B Sub-Giga devices.

* Interrupt Controller

  * Added support for ACE V1X.
  * Fixed an addressing issue on GICv3 controllers.
  * Removed support for ``intel_s1000_crb``.

* IPM

  * Kconfig is split into smaller, vendor oriented files.
  * Support for Intel S1000 in cAVS IDC driver has been removed as the board
    ``intel_s1000_crb`` has been removed.

* KSCAN

  * Enabled the touch panel on the NXP MIMXRT1170 EVK.

* LED

  * Added support for using multiple instances of LP5562 LED module.
  * Devicetree usage cleanups.
  * i2c_dt_spec migration.
  * Updated LED PWM support for ESP32 boards.

* LoRa

  * Added support for setting the sync-word and iq-inverted modes in LoRa modems.
  * Removed ``REQUIRES_FULL_LIBC`` library dependency from LoRa drivers. This
    results in considerable flash memory savings.
  * Devicetree usage cleanups.

* MEMC

  * Added support for Atmel SAM SMC/EBI.

* PCIE

  * Added a ``dump`` subcommand to the ``pcie`` shell command to print out
    the first 16 configuration space registers.
  * Added a ``ls`` subcommand to the ``pcie`` shell command to list
    devices.

* PECI

  * Added PECI driver for Nuvoton NPCX family.
  * Devicetree binding for ITE it8xxx2 PECI driver has changed from
    ``ite,peci-it8xxx2`` to :dtcompatible:`ite,it8xxx2-peci` so that this aligns
    with other ITE devices.

* Pin control

  * Added driver for Infineon XMC4XXX.
  * Added driver for Renesas Smartbond platform.
  * Added driver for Xilinx Zynq-7000.
  * Added support for PSL pads in NPCX driver.
  * MEC15XX driver now supports both MEC15XX and MEC17XX.
  * nRF driver now supports disconnecting a pin by using ``NRF_PSEL_DISCONNECT``.
  * nRF driver will use S0D1 drive mode for TWI/TWIM pins by default.

* PWM

  * Added PWM driver for Renesas R-Car platform.
  * Added PWM driver for Raspberry Pi Pico series.
  * Added PWM support for NXP LPC55S36.
  * Added MCPWM support for ESP32 boards.
  * Fixed the nRF PWM driver to properly handle cases where PWM generation is
    used for some channels while some others are set to a constant level (active
    or inactive), e.g. when the LED driver API is used to turn off a PWM driven
    LED while another one (within the same PWM instance) is blinking.

* Power Domain

  * Enabled access to the PMIC on NXP MXRT595 EVK.
  * Added soft off mode to the RT10xx Power Management.
  * Added support for power gating for Intel ADSP devices.

* Reset

  * Added driver for GigaDevice GD32 SoCs.

* SDHC

  * Added SDHC driver for NXP LPCXpresso platform.
  * Added support for card busy signal in SDHC SPI driver, to support
    the :ref:`File System API <file_system_api>`.

* Sensor

  * Converted drivers to use Kconfig 'select' instead of 'depends on' for I2C,
    SPI, and GPIO dependencies.
  * Converted drivers to use I2C, SPI, and GPIO dt_spec helpers.
  * Added multi-instance support to various drivers.
  * Added DS18B20 1-wire temperature sensor driver.
  * Added Würth Elektronik WSEN-HIDS driver.
  * Fixed unit conversion in the ADXL345 driver.
  * Fixed TTE and TTF time units in the MAX17055 driver.
  * Removed MPU9150 passthrough support from the AK8975 driver.
  * Changed the FXOS8700 driver default mode from accel-only to hybrid.
  * Enhanced the ADXL345 driver to support SPI.
  * Enhanced the BQ274XX driver to support the data ready interrupt trigger.
  * Enhanced the INA237 driver to support triggered mode.
  * Enhanced the LPS22HH driver to support being on an I3C bus.
  * Enhanced the MAX17055 driver to support VFOCV.

* Serial

  * Added serial driver for Renesas Smartbond platform.
  * The STM32 driver now allows to use serial device as stop mode wake up source.
  * Added check for clock control device readiness during configuration
    for various drivers.
  * Various fixes on ``lpuart``.
  * Added a workaround on bytes dropping on ``nrfx_uarte``.
  * Fixed compilation error on ``uart_pl011`` when interrupt is diabled.
  * Added power management support on ``stm32``.
  * ``xlnx_ps`` has moved to using ``DEVICE_MMIO`` API.
  * ``gd32`` now supports using reset API to reset hardware and clock
    control API to enable UART clock.

* SPI

  * Add interrupt-driven mode support for gd32 driver.
  * Enable SPI support on NXP MIMXRT595 EVK.
  * PL022: Added SPI driver for the PL022 peripheral.

* Timer

  * STM32 LPTIM based timer should now be configured using device tree.

* USB

  * Restructured the NXP MCUX USB driver.
  * Added USB support for NXP MXRT595.
  * Fixed detach behavior in nRF USBD and Atmel SAM drivers.

* W1

  * Added Zephyr-Serial 1-Wire master driver.
  * Added DS2484 1-Wire master driver. See the :dtcompatible:`maxim,ds2484`
    devicetree binding for more information.
  * Added DS2485 1-Wire master driver. See the :dtcompatible:`maxim,ds2485`
    devicetree binding for more information.
  * Introduced a shell module for 1-Wire.

* Watchdog

  * Added support for Raspberry Pi Pico watchdog.
  * Added watchdog support on NXP MIMXRT595 EVK.

* WiFi

  * Added ESP32 WiFi integration to Wi-Fi API management.

Networking
**********

* CoAP:

  * Replaced constant CoAP retransmission count and acknowledgment random factor
    with configurable :kconfig:option:`CONFIG_COAP_ACK_RANDOM_PERCENT` and
    :kconfig:option:`CONFIG_COAP_MAX_RETRANSMIT`.
  * Updated :c:func:`coap_packet_parse` and :c:func:`coap_handle_request` to
    return different error code based on the reason of parsing error.

* Ethernet:

  * Added EAPoL and IEEE802154 Ethernet protocol types.

* HTTP:

  * Improved API documentation.

* LwM2M:

  * Moved LwM2M 1.1 support out of experimental.
  * Refactored SenML-JSON and JSON encoder/decoder to use Zephyr's JSON library
    internally.
  * Extended LwM2M shell module with the following commands: ``exec``, ``read``,
    ``write``, ``start``, ``stop``, ``update``, ``pause``, ``resume``.
  * Refactored LwM2M engine module into smaller sub-modules: LwM2M registry,
    LwM2M observation, LwM2M message handling.
  * Added an implementation of the LwM2M Access Control object (object ID 2).
  * Added support for LwM2M engine pause/resume.
  * Improved API documentation of the LwM2M engine.
  * Improved thread safety of the LwM2M library.
  * Added :c:func:`lwm2m_registry_lock` and :c:func:`lwm2m_registry_unlock`
    functions, which allow to update multiple resources w/o sending a
    notification for every update.
  * Multiple minor fixes/improvements.

* Misc:

  * ``CONFIG_NET_CONFIG_IEEE802154_DEV_NAME`` has been removed in favor of
    using a Devicetree choice given by ``zephyr,ieee802154``.
  * Fixed net_pkt leak with shallow clone.
  * Fixed websocket build with :kconfig:option:`CONFIG_POSIX_API`.
  * Extracted zperf shell commands into a library.
  * Added support for building and using IEEE 802.15.4 L2 without IP support.
  * General clean up of inbound packet handling.
  * Added support for restarting DHCP w/o randomized delay.
  * Fixed a bug, where only one packet could be queued on a pending ARP
    request.

* OpenThread:

  * Moved OpenThread glue code into ``modules`` directory.
  * Fixed OpenThread build with :kconfig:option:`CONFIG_NET_MGMT_EVENT_INFO`
    disabled.
  * Fixed mbed TLS configuration for Service Registration Protocol (SRP)
    OpenThread feature.
  * Added Kconfig option to enable Thread 1.3 support
    (:kconfig:option:`CONFIG_OPENTHREAD_THREAD_VERSION_1_3`).
  * Updated :c:func:`otPlatSettingsSet` according to new API documentation.
  * Added new Kconfig options:

    * :kconfig:option:`CONFIG_OPENTHREAD_MESSAGE_BUFFER_SIZE`
    * :kconfig:option:`CONFIG_OPENTHREAD_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS`

* Sockets:

  * Fixed filling of the address structure provided in :c:func:`recvfrom` for
    packet socket.
  * Fixed a potential deadlock in TCP :c:func:`send` call.
  * Added support for raw 802.15.4 packet socket.

* TCP:

  * Added support for Nagle's algorithm.
  * Added "Silly Window Syndrome" avoidance.
  * Fixed MSS calculation.
  * Avoid unnecessary packet cloning on the RX path.
  * Implemented randomized retransmission timeouts and exponential backoff.
  * Fixed out-of-order data processing.
  * Implemented fast retransmit algorithm.
  * Multiple minor fixes/improvements.

* Wi-Fi

  * Added support for using offloaded wifi_mgmt API with native network stack.
  * Extended Wi-Fi headers with additional Wi-Fi parameters (security, bands,
    modes).
  * Added new Wi-Fi management APIs for retrieving status and statistics.

USB
***

  * Minor bug fixes and improvements in class implementations CDC ACM, DFU, and MSC.
    Otherwise no significant changes.

Devicetree
**********

* Use of the devicetree *label property* has been deprecated, and the property
  has been made optional in almost all bindings throughout the tree.

  In previous versions of zephyr, label properties like this commonly appeared
  in devicetree files:

  .. code-block:: dts

     foo {
             label = "FOO";
             /* ... */
     };

  You could then use something like the following to retrieve a device
  structure for use in the :ref:`device_model_api`:

  .. code-block:: c

     const struct device *my_dev = device_get_binding("FOO");
     if (my_dev == NULL) {
             /* either device initialization failed, or no such device */
     } else {
             /* device is ready for use */
     }

  This approach has multiple problems.

  First, it incurs a runtime string comparison over all devices in the system
  to look up each device, which is inefficient since devices are statically
  allocated and known at build time. Second, missing devices due to
  misconfigured device drivers could not easily be distinguished from device
  initialization failures, since both produced ``NULL`` return values from
  ``device_get_binding()``. This led to frequent confusion. Third, the
  distinction between the label property and devicetree *node labels* -- which
  are different despite the similar terms -- was a frequent source of user
  confusion, especially since either one can be used to retrieve device
  structures.

  Instead of using label properties, you should now generally be using node
  labels to retrieve devices instead. Node labels look like the ``lbl`` token
  in the following devicetree:

  .. code-block:: dts

     lbl: foo {
             /* ... */
     };

  and you can retrieve the device structure pointer like this:

  .. code-block:: c

     /* If the next line causes a build error, then there
      * is no such device. Either fix your devicetree or make sure your
      * device driver is allocating a device. */
     const struct device *my_dev = DEVICE_DT_GET(DT_NODELABEL(lbl));

     if (!device_is_ready(my_dev)) {
             /* device exists, but it failed to initialize */
     } else {
             /* device is ready for use */
     }

  As shown in the above snippet, :c:macro:`DEVICE_DT_GET` should generally be
  used instead of ``device_get_binding()`` when getting device structures from
  devicetree nodes. Note that you can pass ``DEVICE_DT_GET`` any devicetree
  :ref:`node identifier <dt-node-identifiers>` -- you don't have to use
  :c:macro:`DT_NODELABEL`, though it is usually convenient to do so.

* Support for devicetree "fixups" was removed. For more details, see `commit
  b2520b09a7
  <https://github.com/zephyrproject-rtos/zephyr/commit/b2520b09a78b86b982a659805e0c65b34e3112a5>`_

* :ref:`devicetree_api`

  * All devicetree macros now recursively expand their arguments. This means
    that in the following example, ``INDEX`` is always replaced with the number
    ``3`` for any hypothetical devicetree macro ``DT_FOO()``:

    .. code-block:: c

       #define INDEX 3
       int foo = DT_FOO(..., INDEX)

    Previously, devicetree macro arguments were expanded or not on a
    case-by-case basis. The current behavior ensures you can always rely on
    macro expansion when using devicetree APIs.

  * New API macros:

     * :c:macro:`DT_FIXED_PARTITION_EXISTS`
     * :c:macro:`DT_FOREACH_CHILD_SEP_VARGS`
     * :c:macro:`DT_FOREACH_CHILD_SEP`
     * :c:macro:`DT_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS`
     * :c:macro:`DT_FOREACH_CHILD_STATUS_OKAY_SEP`
     * :c:macro:`DT_FOREACH_NODE`
     * :c:macro:`DT_FOREACH_STATUS_OKAY_NODE`
     * :c:macro:`DT_INST_CHILD`
     * :c:macro:`DT_INST_FOREACH_CHILD_SEP_VARGS`
     * :c:macro:`DT_INST_FOREACH_CHILD_SEP`
     * :c:macro:`DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS`
     * :c:macro:`DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP`
     * :c:macro:`DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS`
     * :c:macro:`DT_INST_FOREACH_CHILD_STATUS_OKAY`
     * :c:macro:`DT_INST_STRING_TOKEN_BY_IDX`
     * :c:macro:`DT_INST_STRING_TOKEN`
     * :c:macro:`DT_INST_STRING_UPPER_TOKEN_BY_IDX`
     * :c:macro:`DT_INST_STRING_UPPER_TOKEN_OR`
     * :c:macro:`DT_INST_STRING_UPPER_TOKEN`
     * :c:macro:`DT_NODE_VENDOR_BY_IDX_OR`
     * :c:macro:`DT_NODE_VENDOR_BY_IDX`
     * :c:macro:`DT_NODE_VENDOR_HAS_IDX`
     * :c:macro:`DT_NODE_VENDOR_OR`
     * :c:macro:`DT_STRING_TOKEN_BY_IDX`
     * :c:macro:`DT_STRING_UPPER_TOKEN_BY_IDX`
     * :c:macro:`DT_STRING_UPPER_TOKEN_OR`

  * Deprecated macros:

     * ``DT_LABEL(node_id)``: use ``DT_PROP(node_id, label)`` instead. This is
       part of the general deprecation of the label property described at the
       top of this section.
     * ``DT_INST_LABEL(inst)``: use ``DT_INST_PROP(inst, label)`` instead.
     * ``DT_BUS_LABEL(node_id)``: use ``DT_PROP(DT_BUS(node_id), label))`` instead.
     * ``DT_INST_BUS_LABEL(node_id)``: use ```DT_PROP(DT_INST_BUS(inst),
       label)`` instead. Similar advice applies for the rest of the following
       deprecated macros: if you need to access a devicetree node's label
       property, do so explicitly using another property access API macro.
     * ``DT_GPIO_LABEL_BY_IDX()``
     * ``DT_GPIO_LABEL()``
     * ``DT_INST_GPIO_LABEL_BY_IDX()``
     * ``DT_INST_GPIO_LABEL()``
     * ``DT_SPI_DEV_CS_GPIOS_LABEL()``
     * ``DT_INST_SPI_DEV_CS_GPIOS_LABEL()``
     * ``DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL``

* Bindings

  * The :ref:`bus <dt-bindings-bus>` key in a bindings file can now be a list
    of strings as well as a string. This allows a single node to declare that
    it represents hardware which can communicate over multiple bus protocols.
    The primary use case is simultaneous support for I3C and I2C buses in the
    same nodes, with the base bus definition provided in
    :zephyr_file:`dts/bindings/i3c/i3c-controller.yaml`.

  * New:

    * :dtcompatible:`adi,adxl345`
    * :dtcompatible:`altr,nios2-qspi-nor`
    * :dtcompatible:`altr,nios2-qspi`
    * :dtcompatible:`andestech,atciic100`
    * :dtcompatible:`andestech,atcpit100`
    * :dtcompatible:`andestech,machine-timer`
    * :dtcompatible:`andestech,atcspi200`
    * :dtcompatible:`arduino-mkr-header`
    * :dtcompatible:`arm,armv6m-systick`
    * :dtcompatible:`arm,armv7m-itm`
    * :dtcompatible:`arm,armv7m-systick`
    * :dtcompatible:`arm,armv8.1m-systick`
    * :dtcompatible:`arm,armv8m-itm`
    * :dtcompatible:`arm,armv8m-systick`
    * :dtcompatible:`arm,beetle-syscon`
    * :dtcompatible:`arm,pl022`
    * :dtcompatible:`aspeed,ast10x0-clock`
    * :dtcompatible:`atmel,at24mac402`
    * :dtcompatible:`atmel,ataes132a`
    * :dtcompatible:`atmel,sam-smc`
    * :dtcompatible:`atmel,sam4l-flashcalw-controller`
    * :dtcompatible:`atmel,saml2x-gclk`
    * :dtcompatible:`atmel,saml2x-mclk`
    * :dtcompatible:`cdns,qspi-nor`
    * :dtcompatible:`espressif,esp32-ipm`
    * :dtcompatible:`espressif,esp32-mcpwm`
    * :dtcompatible:`espressif,esp32-pcnt`
    * :dtcompatible:`espressif,esp32-rtc-timer`
    * :dtcompatible:`espressif,esp32-timer`
    * :dtcompatible:`espressif,esp32-twai`
    * :dtcompatible:`espressif,esp32-usb-serial`
    * :dtcompatible:`espressif,esp32-wifi`
    * :dtcompatible:`gd,gd32-adc`
    * :dtcompatible:`gd,gd32-cctl`
    * :dtcompatible:`gd,gd32-dma`
    * :dtcompatible:`gd,gd32-flash-controller`
    * :dtcompatible:`gd,gd32-rcu`
    * :dtcompatible:`goodix,gt911`
    * :dtcompatible:`infineon,xmc4xxx-gpio`
    * :dtcompatible:`infineon,xmc4xxx-pinctrl`
    * :dtcompatible:`intel,ace-art-counter`
    * :dtcompatible:`intel,ace-intc`
    * :dtcompatible:`intel,ace-rtc-counter`
    * :dtcompatible:`intel,ace-timestamp`
    * :dtcompatible:`intel,adsp-gpdma` (formerly ``intel,cavs-gpdma``)
    * :dtcompatible:`intel,adsp-hda-host-in` (formerly ``intel,cavs-hda-host-in``)
    * :dtcompatible:`intel,adsp-hda-host-out` (formerly ``intel,cavs-hda-host-out``)
    * :dtcompatible:`intel,adsp-hda-link-in` (formerly ``intel,cavs-hda-link-in``)
    * :dtcompatible:`intel,adsp-hda-link-out` (formerly ``intel,cavs-hda-link-out``)
    * :dtcompatible:`intel,adsp-host-ipc`
    * :dtcompatible:`intel,adsp-idc` (formerly ``intel,cavs-idc``)
    * :dtcompatible:`intel,adsp-imr`
    * :dtcompatible:`intel,adsp-lps`
    * :dtcompatible:`intel,adsp-mtl-tlb`
    * :dtcompatible:`intel,adsp-power-domain`
    * :dtcompatible:`intel,adsp-shim-clkctl`
    * :dtcompatible:`intel,agilex-clock`
    * :dtcompatible:`intel,alh-dai`
    * :dtcompatible:`intel,multiboot-framebuffer`
    * :dtcompatible:`ite,it8xxx2-peci` (formerly ``ite,peci-it8xxx2``)
    * :dtcompatible:`maxim,ds18b20`
    * :dtcompatible:`maxim,ds2484`
    * :dtcompatible:`maxim,ds2485`
    * :dtcompatible:`maxim,max7219`
    * :dtcompatible:`microchip,mpfs-gpio`
    * :dtcompatible:`microchip,xec-eeprom`
    * :dtcompatible:`microchip,xec-espi`
    * :dtcompatible:`microchip,xec-i2c`
    * :dtcompatible:`microchip,xec-qmspi`
    * :dtcompatible:`neorv32-machine-timer`
    * :dtcompatible:`nordic,nrf-ieee802154`
    * :dtcompatible:`nuclei,systimer`
    * :dtcompatible:`nuvoton,npcx-leakage-io`
    * :dtcompatible:`nuvoton,npcx-peci`
    * :dtcompatible:`nuvoton,npcx-power-psl`
    * :dtcompatible:`nxp,gpt-hw-timer`
    * :dtcompatible:`nxp,iap-fmc11`
    * :dtcompatible:`nxp,imx-caam`
    * :dtcompatible:`nxp,kw41z-ieee802154`
    * :dtcompatible:`nxp,lpc-rtc`
    * :dtcompatible:`nxp,lpc-sdif`
    * :dtcompatible:`nxp,mcux-i3c`
    * :dtcompatible:`nxp,os-timer`
    * :dtcompatible:`panasonic,reduced-arduino-header`
    * :dtcompatible:`raspberrypi,pico-adc`
    * :dtcompatible:`raspberrypi,pico-pwm`
    * :dtcompatible:`raspberrypi,pico-spi`
    * :dtcompatible:`raspberrypi,pico-watchdog`
    * :dtcompatible:`renesas,pwm-rcar`
    * :dtcompatible:`renesas,r8a7795-cpg-mssr` (formerly ``renesas,rcar-cpg-mssr``)
    * :dtcompatible:`renesas,smartbond-flash-controller`
    * :dtcompatible:`renesas,smartbond-gpio`
    * :dtcompatible:`renesas,smartbond-pinctrl`
    * :dtcompatible:`renesas,smartbond-uart`
    * :dtcompatible:`sifive,clint0`
    * :dtcompatible:`sifive,e24` (formerly ``riscv,sifive-e24``)
    * :dtcompatible:`sifive,e31` (formerly ``riscv,sifive-e31``)
    * :dtcompatible:`sifive,e51` (formerly ``riscv,sifive-e51``)
    * :dtcompatible:`sifive,s7` (formerly ``riscv,sifive-s7``)
    * :dtcompatible:`silabs,gecko-semailbox`
    * :dtcompatible:`snps,arc-iot-sysconf`
    * :dtcompatible:`snps,arc-timer`
    * :dtcompatible:`snps,archs-ici`
    * :dtcompatible:`st,stm32-vbat`
    * :dtcompatible:`st,stm32g0-hsi-clock`
    * :dtcompatible:`st,stm32h7-spi`
    * :dtcompatible:`st,stm32u5-dma`
    * :dtcompatible:`starfive,jh7100-clint`
    * :dtcompatible:`telink,b91-adc`
    * :dtcompatible:`telink,machine-timer`
    * :dtcompatible:`ti,ads1119`
    * :dtcompatible:`ti,cc13xx-cc26xx-flash-controller`
    * :dtcompatible:`ti,cc13xx-cc26xx-ieee802154-subghz`
    * :dtcompatible:`ti,cc13xx-cc26xx-ieee802154`
    * :dtcompatible:`ti,sn74hc595`
    * :dtcompatible:`ultrachip,uc8176`
    * :dtcompatible:`ultrachip,uc8179`
    * :dtcompatible:`xen,hvc-uart`
    * :dtcompatible:`xen,xen-4.15`
    * :dtcompatible:`xlnx,pinctrl-zynq`
    * :dtcompatible:`zephyr,coredump`
    * :dtcompatible:`zephyr,ieee802154-uart-pipe`
    * :dtcompatible:`zephyr,native-posix-counter`
    * :dtcompatible:`zephyr,native-posix-linux-can`
    * :dtcompatible:`zephyr,sdl-kscan`
    * :dtcompatible:`zephyr,sdmmc-disk`
    * :dtcompatible:`zephyr,w1-serial`

  * :ref:`pinctrl-guide` support added via new ``pinctrl-0``, etc. properties:

    * :dtcompatible:`microchip,xec-qmspi`
    * :dtcompatible:`infineon,xmc4xxx-uart`
    * :dtcompatible:`nxp,lpc-mcan`
    * :dtcompatible:`xlnx,xuartps`

  * Other changes:

    * Analog Devices parts:

      * :dtcompatible:`adi,adxl372`: new properties as part of a general conversion
        of the associated upstream driver to support multiple instances.
      * :dtcompatible:`adi,adxl362`: new ``wakeup-mode``, ``autosleep`` properties.

    * Atmel SoCs:

      * :dtcompatible:`atmel,rf2xx`: new ``channel-page``, ``tx-pwr-table``,
        ``tx-pwr-min``, ``tx-pwr-max`` properties.
      * GMAC: new ``mac-eeprom`` property.

    * Espressif SoCs:

      * :dtcompatible:`espressif,esp32-i2c`: the ``sda-pin`` and ``scl-pin``
        properties are now ``scl-gpios`` and ``sda-gpios``.
      * :dtcompatible:`espressif,esp32-ledc`: device configuration moved to
        devicetree via a new child binding.
      * :dtcompatible:`espressif,esp32-pinctrl`: this now uses pin groups.
      * :dtcompatible:`espressif,esp32-spi`: new ``use-iomux`` property.
      * :dtcompatible:`espressif,esp32-usb-serial`: removed ``peripheral``
        property.

    * GigaDevice SoCs:

      * Various peripheral bindings have had their SoC-specific
        ``rcu-periph-clock`` properties replaced with the standard ``clocks``
        property as part of driver changes associated with the new
        :dtcompatible:`gd,gd32-cctl` clock controller binding:

        * :dtcompatible:`gd,gd32-afio`
        * :dtcompatible:`gd,gd32-dac`
        * :dtcompatible:`gd,gd32-gpio`
        * :dtcompatible:`gd,gd32-i2c`
        * :dtcompatible:`gd,gd32-pwm`
        * :dtcompatible:`gd,gd32-spi`
        * :dtcompatible:`gd,gd32-syscfg`
        * :dtcompatible:`gd,gd32-timer`
        * :dtcompatible:`gd,gd32-usart`

      * Similarly, various GigaDevice peripherals now support the standard
        ``resets`` property as part of related driver changes to support
        resetting the peripheral state before initialization via the
        :dtcompatible:`gd,gd32-rcu` binding:

        * :dtcompatible:`gd,gd32-dac`
        * :dtcompatible:`gd,gd32-gpio`
        * :dtcompatible:`gd,gd32-i2c`
        * :dtcompatible:`gd,gd32-pwm`
        * :dtcompatible:`gd,gd32-spi`
        * :dtcompatible:`gd,gd32-usart`

    * Intel SoCs:

      * :dtcompatible:`intel,adsp-tlb`:
        new ``paddr-size``, ``exec-bit-idx``, ``write-bit-idx`` properties.
      * :dtcompatible:`intel,adsp-shim-clkctl`: new ``wovcro-supported`` property.
      * Removed ``intel,dmic`` binding.
      * Removed ``intel,s1000-pinmux`` binding.

    * Nordic SoCs:

      * :dtcompatible:`nordic,nrf-pinctrl`: ``NRF_PSEL_DISCONNECTED`` can be used
        to disconnect a pin.
      * :dtcompatible:`nordic,nrf-spim`: new ``rx-delay-supported``,
        ``rx-delay`` properties.
      * :dtcompatible:`nordic,nrf-spim`, :dtcompatible:`nordic,nrf-spi`: new
         ``overrun-character``, ``max-frequency``, ``memory-region``,
         ``memory-region-names`` properties.
      * :dtcompatible:`nordic,nrf-uarte`: new ``memory-region``,
        ``memory-region-names`` properties.
      * Various bindings have had ``foo-pin`` properties deprecated. For
        example, :dtcompatible:`nordic,nrf-qspi` has a deprecated ``sck-pin``
        property. Uses of such properties should be replaced with pinctrl
        equivalents; see :dtcompatible:`nordic,nrfpinctrl`.

    * Nuvoton SoCs:

      * :dtcompatible:`nuvoton,npcx-leakage-io`: new ``lvol-maps`` property.
      * :dtcompatible:`nuvoton,npcx-scfg`: removed ``io_port``, ``io_bit``
        cells in ``lvol_cells`` specifiers.
      * Removed: ``nuvoton,npcx-lvolctrl-def``, ``nuvoton,npcx-psl-out``,
        ``nuvoton,npcx-pslctrl-conf``, ``nuvoton,npcx-pslctrl-def``.
      * Added pinctrl support for PSL (Power Switch Logic) pads.

    * NXP SoCs:

      * :dtcompatible:`nxp,imx-pwm`: new ``run-in-wait``, ``run-in-debug`` properties.
      * :dtcompatible:`nxp,lpc-spi`: new ``def-char`` property.
      * :dtcompatible:`nxp,lpc-iocon-pinctrl`: new ``nxp,analog-alt-mode`` property.
      * removed deprecated ``nxp,lpc-iap`` binding.
      * :dtcompatible:`nxp,imx-csi`: new ``sensor`` property replacing the
        ``sensor-label`` property.
      * :dtcompatible:`nxp,imx-lpi2c`: new ``scl-gpios``, ``sda-gpios`` properties.

    * STM32 SoCs:

      * :dtcompatible:`st,stm32-adc`: new ``has-vbat-channel`` property.
      * :dtcompatible:`st,stm32-can`: removed ``one-shot`` property.
      * :dtcompatible:`st,stm32-fdcan`: new ``clocks``, ``clk-divider`` properties.
      * :dtcompatible:`st,stm32-ospi`: new ``dmas``, ``dma-names`` properties.
      * :dtcompatible:`st,stm32-ospi-nor`: new ``four-byte-opcodes``,
        ``writeoc`` properties; new enum values ``2`` and ``4`` in
        ``spi-bus-width`` property.
      * :dtcompatible:`st,stm32-pwm`: removed deprecated ``st,prescaler`` property.
      * :dtcompatible:`st,stm32-rng`: new ``nist-config`` property.
      * :dtcompatible:`st,stm32-sdmmc`: new ``dmas``, ``dma-names``,
        ``bus-width`` properties.
      * :dtcompatible:`st,stm32-temp-cal`: new ``ts-cal-resolution`` property;
        removed ``ts-cal-offset`` property.
      * :dtcompatible:`st,stm32u5-pll-clock`: new ``div-p`` property.
      * temperature sensor bindings no longer have a ``ts-voltage-mv`` property.
      * UART bindings: new ``wakeup-line`` properties.

    * Texas Instruments parts:

      * :dtcompatible:`ti,ina237`: new ``alert-config``, ``irq-gpios`` properties.
      * :dtcompatible:`ti,bq274xx`: new ``zephyr,lazy-load`` property.

    * Ultrachip UC81xx displays:

      * The ``gooddisplay,gd7965`` binding was removed in favor of new
        UltraChip device-specific bindings (see list of new ``ultrachip,...``
        bindings above). Various required properties in the removed binding are
        now optional in the new bindings.

      * New ``pll``, ``vdcs``, ``lutc``, ``lutww``, ``lutkw``, ``lutwk``,
        ``lutkk``, ``lutbd``, ``softstart`` properties. Full and partial
        refresh profile support. The ``pwr`` property is now part of the child
        binding.

    * Zephyr-specific bindings:

      * :dtcompatible:`zephyr,bt-hci-spi`: new ``reset-assert-duration-ms`` property.
      * removed ``zephyr,ipm-console`` binding.
      * :dtcompatible:`zephyr,ipc-openamp-static-vrings`: new
        ``zephyr,buffer-size`` property.
      * :dtcompatible:`zephyr,memory-region`: new ``PPB`` and ``IO`` region support.

    * :dtcompatible:`infineon,xmc4xxx-uart`: new ``input-src`` property.
    * WSEN-HIDS sensors: new ``drdy-gpios``, ``odr`` properties.
    * :dtcompatible:`sitronix,st7789v`: ``cmd-data-gpios`` is now optional.
    * :dtcompatible:`solomon,ssd16xxfb`: new ``dummy-line``,
      ``gate-line-width`` properties. The ``gdv``, ``sdv``, ``vcom``, and
      ``border-waveform`` properties are now optional.
    * ``riscv,clint0`` removed; all in-tree users were converted to
      ``sifive,clint0`` or derived bindings.
    * :dtcompatible:`worldsemi,ws2812-spi`: SPI bindings have new ``spi-cpol``,
      ``spi-cpha`` properties.
    * :dtcompatible:`ns16550`: ``reg-shift`` is now required.
    * Removed ``reserved-memory`` binding.

* Implementation details

  * The generated devicetree header file placed in the build directory was
    renamed from ``devicetree_unfixed.h`` to ``devicetree_generated.h``.

  * The generated ``device_extern.h`` has been replaced using
    ``DT_FOREACH_STATUS_OKAY_NODE``. See `commit
    0224f2c508df154ffc9c1ecffaf0b06608d6b623
    <https://github.com/zephyrproject-rtos/zephyr/commit/0224f2c508df154ffc9c1ecffaf0b06608d6b623>`_

Libraries / Subsystems
**********************

* C Library

  * Added Picolibc as a Zephyr module. Picolibc module is a footprint-optimized
    full C standard library implementation that is configurable at the build
    time.
  * C library heap initialization call has been moved from the ``APPLICATION``
    phase to the ``POST_KERNEL`` phase to allow calling the libc dynamic memory
    management functions (e.g. ``malloc()``) during the application
    initialization phase.
  * Added ``strerror()`` and ``strerror_r()`` functions to the minimal libc.
  * Removed architecture-specific ``off_t`` type definition in the minimal
    libc. ``off_t`` is now defined as ``intptr_t`` regardless of the selected
    architecture.

* C++ Subsystem

  * Added ``std::ptrdiff_t``, ``std::size_t``, ``std::max_align_t`` and
    ``std::nullptr_t`` type definitions to the C++ subsystem ``cstddef``
    header.
  * Renamed global constructor list symbols to prevent the native POSIX host
    runtime from executing the constructors before Zephyr loads.

* Cbprintf

  * Updated cbprintf static packaging to interpret ``unsigned char *`` as a pointer
    to a string. See :ref:`cbprintf_packaging_limitations` for more details about
    how to efficienty use strings. Change mainly applies to the ``logging`` subsystem
    since it uses this feature.

* Emul

  * Added :c:macro:`EMUL_DT_DEFINE` and :c:macro:`EMUL_DT_INST_DEFINE` to mirror
    :c:macro:`DEVICE_DT_DEFINE` and :c:macro:`DEVICE_DT_INST_DEFINE` respectively.
  * Added :c:macro:`EMUL_DT_GET` to mirror :c:macro:`DEVICE_DT_GET`.
  * Removed the need to manually register emulators in their init function (automatically done).

* Filesystem

  * Added cash used to reduce the NVS data lookup time, see
    :kconfig:option:`CONFIG_NVS_LOOKUP_CACHE` option.
  * Changing mkfs options to create FAT32 on larger storage when FAT16 fails.

* Management

  * MCUMGR race condition when using the task status function whereby if a
    thread state changed it could give a falsely short process list has been
    fixed.
  * MCUMGR shell (group 9) CBOR structure has changed, the ``rc``
    response is now only used for mcumgr errors, shell command
    execution result codes are instead returned in the ``ret``
    variable instead, see :ref:`mcumgr_smp_group_9` for updated
    information. Legacy bahaviour can be restored by enabling
    :kconfig:option:`CONFIG_MCUMGR_CMD_SHELL_MGMT_LEGACY_RC_RETURN_CODE`.
  * MCUMGR img_mgmt erase command now accepts an optional slot number
    to select which image will be erased, using the ``slot`` input
    (will default to slot 1 if not provided).
  * MCUMGR :kconfig:option:`CONFIG_OS_MGMT_TASKSTAT_SIGNED_PRIORITY` is now
    enabled by default, this makes thread priorities in the taskstat command
    signed, which matches the signed priority of tasks in Zephyr, to revert
    to previous behaviour of using unsigned values, disable this Kconfig.
  * MCUMGR taskstat runtime field support has been added, if
    :kconfig:option:`CONFIG_OS_MGMT_TASKSTAT` is enabled, which will report the
    number of CPU cycles have been spent executing the thread.
  * MCUMGR transport API drops ``zst`` parameter, of :c:struct:`zephyr_smp_transport`
    type, from :c:func:`zephyr_smp_transport_out_fn` type callback as it has
    not been used, and the ``nb`` parameter, of :c:struct:`net_buf` type,
    can carry additional transport information when needed.
  * A dummy SMP transport has been added which allows for testing MCUMGR
    functionality and commands/responses.
  * An issue with the UART/shell transports whereby large packets would wrongly
    be split up with multiple start-of-frame headers instead of only one has been
    fixed.
  * SMP now runs in its own dedicated work queue which prevents issues running in
    the system workqueue with some transports, e.g. Bluetooth, which previously
    caused a deadlock if buffers could not be allocated.
  * Bluetooth transport will now reduce the size of packets that are sent if they
    are too large for the remote device instead of failing to send them, if the
    remote device cannot accept a notification of 20 bytes then the attempt is
    aborted.
  * Unaligned memory access problems for CPUs that do not support it in MCUMGR
    headers has been fixed.
  * Groups in MCUMGR now use kernel slist entries rather than the custom MCUMGR
    structs for storage.
  * Levels of function redirection which were previously used to support multiple
    OS's have been reduced to simplify code and reduce output size.
  * Bluetooth SMP debug output format specifier has been fixed to avoid a build
    warning on native_posix platforms.
  * Issue with :c:func:`img_mgmt_dfu_stopped` being wrongly called on success
    has been fixed.
  * Issue with MCUMGR img_mgmt image erase wrongly returning success during an
    error condition has been fixed.
  * Unused MCUMGR header files such as mcumgr_util.h have been removed.
  * Verbose error response reporting has been fixed and is now present when
    enabled.
  * Internal SMP functions have been removed from the public smp.h header file
    and moved to smp_internal.h
  * Kconfig files have been split up and moved to directories containing the
    systems they influence.
  * MCUMGR img_mgmt image upload over-riding/hiding of result codes has been
    fixed.

* Logging

  * Removed legacy (v1) implementation and removed any references to the logging
    v2.
  * Added :c:macro:`LOG_RAW` for logging strings without additional formatting.
    It is similar to :c:macro:`LOG_PRINTK` but do not append ``<cr>`` when new line is found.
  * Improvements in the ADSP backend.
  * File system backend: Only delete old files if necessary.

* IPC

  * Introduced a 'zephyr,buffer-size' DT property to set the sizes for TX and
    RX buffers per created instance.
  * Set WQ priority back to ``PRIO_PREEMPT`` to fix an issue that was starving
    the scheduler.
  * ``icmsg_buf`` library was renamed to ``spsc_pbuf``.
  * Added cache handling support to ``spsc_pbuf``.
  * Fixed an issue where the TX virtqueue was misaligned by 2 bytes due to the
    way the virtqueue start address is calculated
  * Added :c:func:`ipc_service_deregister_endpoint` function to deregister endpoints.

* LoRaWAN

  * Added Class-C support.
  * Upgraded the loramac-node repository to v4.6.0.
  * Moved the ``REQUIRES_FULL_LIBC`` library dependency from LoRa to LoRaWAN.
  * Fixed the async reception in SX127x modems.

* Modbus

  * Added user data entry for ADU callback.

* Power management

  * Allow multiple subscribers to latency changes in the policy manager.
  * Added new API to implement suspend-to-RAM (S2RAM). Select
    :kconfig:option:`CONFIG_PM_S2RAM` to enable this feature.
  * Added :c:func:`pm_device_is_powered` to query a device power state.

* POSIX

  * Made ``tz`` non-const in ``gettimeofday()`` for conformance to spec.
  * Fixed pthread descriptor resource leak. Previously only pthreads with state
    ``PTHREAD_TERMINATED`` could be reused. However, ``pthread_join()`` sets
    the state to ``PTHREAD_EXITED``. Consider both states as candidates in
    ``pthread_create()``.
  * Added ``perror()`` implementation.
  * Used consistent timebase in ``sem_timedwait()``.

* RTIO

  * Initial version of an asynchronous task and executor API for I/O similar inspired
    by Linux's very successful io_uring.
  * Provided a simple linear and limited concurrency executor, simple task queuing,
    and the ability to poll for task completions.

* SD Subsystem

  * SDMMC STM32: Now compatible with STM32L5 series. Added DMA support for DMA-V1
    compatible devices.
  * Added support for handling the :c:macro:`DISK_IOCTL_CTRL_SYNC` ioctl call.
    this enables the filesystem api :c:func:`fs_sync`.

* Settings

  * Added API function :c:func:`settings_storage_get` which allows to get
    the storage instance used by the settings backed to store its records.

* Shell

  * Added new API function checking shell readiness: :c:func:`shell_ready`.
  * Added option to control formatting of the logging timestamp.
  * Added missing asserts to the shell api functions.
  * MQTT backend: bug fix to handle negative return value of the wait function.
  * A new ``backends`` command that lists the name and number of active shell backends.
  * Fixed handling mandatory args with optional raw arg.

* Storage

  * :c:func:`flash_area_open` returns error if area's flash device is unreachable.
  * ``flash_area`` components were reworked so build-time reference to the flash
    device is used instead of its name with runtime driver buinding.
  * Added ``FIXED_PARTITION_`` macros that move flash_map to use DTS node labels.

* Testsuite

  * Added Kconfig support to ``unit_testing`` platform.
  * Migrated tests to use :kconfig:option:`CONFIG_ZTEST_NEW_API`.
  * Added ztest options for shuffling tests/suites via:

    * :kconfig:option:`CONFIG_ZTEST_SHUFFLE`
    * :kconfig:option:`CONFIG_ZTEST_SHUFFLE_SUITE_REPEAT_COUNT`
    * :kconfig:option:`CONFIG_ZTEST_SHUFFLE_TEST_REPEAT_COUNT`

  * Added ztest native_posix command line arguments for running specific tests/suites using
    ``--test suite_name:*`` or ``--test suite_name::test_name`` command line arguments.

* Storage

  * Flash Map API deprecates usage of :c:macro:`FLASH_AREA_` macros and replaces
    them with :c:macro:`FIXED_PARTITION_` macros. This follows removal of ``label``
    property from DTS nodes.

HALs
****

* Atmel

  * sam: Fixed incorrect CIDR values for revision b silicon of SAMV71 devices.

* Espressif

  * Updated Espressif HAL with esp-idf 4.4.1 updates.
  * Added support to binary blobs implementation.
  * Fixed ESP32-C3 wifi issues.

* GigaDevice

  * Added support for gd32e50x.
  * gd32e10x: upgraded to v1.3.0.
  * gd32f4xx: upgraded to v3.0.0.

* NXP

  * Updated the NXP MCUX SDK to version 2.12.
  * Updated the USB middleware to version 2.12.
  * Removed all binary Blobs for power management libraries.
  * Removed all binary archive files.

* Nordic

  * Updated nrfx to version 2.9.0.

* RPi Pico

  * Renamed ``adc_read`` to ``pico_adc_read``, to avoid name collision with Zephyr's API.

* STM32

  * stm32cube: update stm32f7 to cube version V1.17.0.
  * stm32cube: update stm32g0 to cube version V1.6.1.
  * stm32cube: update stm32g4 to cube version V1.5.1.
  * stm32cube: update stm32l4 to cube version V1.17.2.
  * stm32cube: update stm32u5 to cube version V1.1.1.
  * stm32cube: update stm32wb to cube version V1.14.0.
  * pinctrl: some pin definitions did not contain the "_c" suffix, used by pins
    with analog switch on certain H7 devices.

* TI

  * simplelink: cc13x2_cc26x2: include ADC driverlib sources.
  * simplelink: cc13x2_cc26x2: include flash driverlib sources.
  * cc13x2: kconfig conditions for P variant support & custom RF hwattrs.
  * cc13x2_cc26x2: update to TI SimpleLink SDK 4.40.04.04.

MCUboot
*******

* Added initial support for leveraging the RAM-LOAD mode with the zephyr-rtos port.
* Added the MCUboot status callback support.
  See :kconfig:option:`CONFIG_MCUBOOT_ACTION_HOOKS`.
* Edited includes to have the ``zephyr/`` prefix.
* Edited the DFU detection's GPIO-pin configuration to be done through DTS using the ``mcuboot-button0`` pin alias.
* Edited the LED usage to prefer DTS' ``mcuboot-led0`` alias over the ``bootloader-led0`` alias.
* Removed :c:func:`device_get_binding()` usage in favor of :c:func:`DEVICE_DT_GET()`.
* Added support for generic `watchdog0` alias.
* Enabled watchdog feed by default.
* Dropped the :kconfig:option:`CONFIG_BOOT_IMAGE_ACCESS_HOOKS_FILE` option.
  The inclusion of the Hooks implementation file is now up to the project's customization.
* Switched zephyr port from using ``FLASH_AREA_`` macros to ``FIXED_PARTITION_`` macros.
* Made flash_map_backend.h compatible with a C++ compiler.
* Allowed to get the flash write alignment based on the ``zephyr,flash`` DT chosen node property.

* boot_serial:

  * Upgraded from cddl-gen v0.1.0 to zcbor v0.4.0.
  * Refactored and optimized the code, mainly in what affects the progressive erase implementation.
  * Fixed a compilation issue with the echo command code.

* imgtool:

  * Added support for providing a signature through a third party.

Trusted Firmware-M
******************

* Allowed enabling FPU in the application when TF-M is enabled.
* Added option to exclude non-secure TF-M application from build.
* Relocated ``mergehex.py`` to ``scripts/build``.
* Added option for custom reset handlers.

Documentation
*************

Tests and Samples
*****************

* A large number of tests have been reworked to use the new ztest API, see
  :ref:`test-framework` for more details. This should be used for newly
  introduce tests as well.
* smp_svr Bluetooth overlay (overlay-bt) has been reworked to increase
  throughput and enable packet reassembly.
* Added test for the new shell API function: :c:func:`shell_ready`.

Issue Related Items
*******************

Known Issues
============

- :github:`22049` - Bluetooth: IRK handling issue when using multiple local identities
- :github:`25917` - Bluetooth: Deadlock with TX of ACL data and HCI commands (command blocked by data)
- :github:`30348` - XIP can't be enabled with ARC MWDT toolchain
- :github:`31298` - tests/kernel/gen_isr_table failed on hsdk and nsim_hs_smp sometimes
- :github:`33747` - gptp does not work well on NXP rt series platform
- :github:`34269` - LOG_MODE_MINIMAL BUILD error
- :github:`37193` - mcumgr: Probably incorrect error handling with udp backend
- :github:`37731` - Bluetooth: hci samples: Unable to allocate command buffer
- :github:`38041` - Logging-related tests fails on qemu_arc_hs6x
- :github:`38880` - ARC: ARCv2: qemu_arc_em / qemu_arc_hs don't work with XIP disabled
- :github:`38947` - Issue with SMP commands sent over the UART
- :github:`39598` - use of __noinit with ecc memory hangs system
- :github:`40023` - Build fails for ``native_posix`` board when using C++ <atomic> header
- :github:`41606` - stm32u5: Re-implement VCO input and EPOD configuration
- :github:`41622` - Infinite mutual recursion when SMP and ATOMIC_OPERATIONS_C are set
- :github:`41822` - BLE IPSP sample cannot handle large ICMPv6 Echo Request
- :github:`41823` - Bluetooth: Controller: llcp: Remote request are dropped due to lack of free proc_ctx
- :github:`42030` - can: "bosch,m-can-base": Warning "missing or empty reg/ranges property"
- :github:`43099` - CMake: ARCH roots issue
- :github:`43249` - MBEDTLS_ECP_C not build when MBEDTLS_USE_PSA_CRYPTO
- :github:`43308` - driver: serial: stm32: uart will lost data when use dma mode[async mode]
- :github:`43555` - Variables not properly initialized when using data relocation with SDRAM
- :github:`43562` - Setting and/or documentation of Timer and counter use/requirements for Nordic Bluetooth driver
- :github:`43836` - stm32: g0b1: RTT doesn't work properly after stop mode
- :github:`44339` - Bluetooth:controller: Implement support for Advanced Scheduling in refactored LLCP
- :github:`44377` - ISO Broadcast/Receive sample not working with coded PHY
- :github:`44410` - drivers: modem: shell: ``modem send`` doesn't honor line ending in modem cmd handler
- :github:`44948` - cmsis_dsp: transofrm: error during building cf64.fpu and rf64.fpu for mps2_an521_remote
- :github:`45218` - rddrone_fmuk66: I2C configuration incorrect
- :github:`45241` - (Probably) unnecessary branches in several modules
- :github:`45323` - Bluetooth: controller: llcp: Implement handling of delayed notifications in refactored LLCP
- :github:`45427` - Bluetooth: Controller: LLCP: Data structure for communication between the ISR and the thread
- :github:`45814` - Armclang build fails due to missing source file
- :github:`46073` - IPSP (IPv6 over BLE) example stop working after a short time
- :github:`46121` - Bluetooth: Controller: hci: Wrong periodic advertising report data status
- :github:`46126` - pm_device causes assertion error in sched.c with lis2dh
- :github:`46401` - ARM64: Relax 4K MMU mapping alignment
- :github:`46596` - STM32F74X RMII interface does not work
- :github:`46598` - Logging with RTT backend on STM32WB strange behavier
- :github:`46844` - Timer drivers likely have off-by-one in rapidly-presented timeouts
- :github:`46846` - lib: libc: newlib: strerror_r non-functional
- :github:`46986` - Logging (deferred v2) with a lot of output causes MPU fault
- :github:`47014` - can: iso-tp: implementation test failed with twister on nucleo_g474re
- :github:`47092` - driver: nrf: uarte: new dirver breaks our implementation for uart.
- :github:`47120` - shell uart: busy wait for DTR in ISR
- :github:`47477` - qemu_leon3: tests/kernel/fpu_sharing/generic/ failed when migrating to new ztest API
- :github:`47500` - twister: cmake: Failure of "--build-only -M" combined with "--test-only" for --device-testing
- :github:`47607` - Settings with FCB backend does not pass test on stm32h743
- :github:`47732` - Flash map does not fare well with MCU who do bank swaps
- :github:`47817` - samples/modules/nanopb/sample.modules.nanopb fails with protobuf > 3.19.0
- :github:`47908` - tests/kernel/mem_protect/stack_random works unreliably and sporadically fails
- :github:`47988` - JSON parser not consistent on extra data
- :github:`48018` - ztest: static threads are not re-launched for repeated test suite execution.
- :github:`48037` - Grove LCD Sample Not Working
- :github:`48094` - pre-commit scripts fail when there is a space in zephyr_base
- :github:`48102` - JSON parses uses recursion (breaks rule 17.2)
- :github:`48147` - ztest: before/after functions may run on different threads, which may cause potential issues.
- :github:`48287` - malloc_prepare ASSERT happens when enabling newlib libc with demand paging
- :github:`48299` - SHT3XD_CMD_WRITE_TH_LOW_SET should be SHT3XD_CMD_WRITE_TH_LOW_CLEAR
- :github:`48304` - bt_disable() does not work properly on nRF52
- :github:`48390` - [Intel Cavs] Boot failures on low optimization levels
- :github:`48394` - vsnprintfcb writes to ``*str`` if it is NULL
- :github:`48468` - GSM Mux does not transmit all queued data when uart_fifo_fill is called
- :github:`48473` - Setting CONFIG_GSM_MUX_INITIATOR=n results in a compile error
- :github:`48505` - BLE stack can get stuck in connected state despite connection failure
- :github:`48520` - clang-format: #include reorder due to default: SortIncludesOptions != SI_Never
- :github:`48603` - LoRa driver asynchronous receive callback clears data before the callback.
- :github:`48608` - boards: mps2_an385: Unstable system timer
- :github:`48625` - GSM_PPP api keeps sending commands to muxed AT channel
- :github:`48726` - net: tests/net/ieee802154/l2/net.ieee802154.l2 failed on reel board
- :github:`48841` - Bluetooth: df: Assert in lower link layer when requesting CTE from peer periodically with 7.5ms connection interval
- :github:`48850` - Bluetooth: LLCP: possible access to released control procedure context
- :github:`48857` - samples: Bluetooth: Buffer size mismatch in samples/bluetooth/hci_usb for nRF5340
- :github:`48953` - 'intel,sha' is missing binding and usage
- :github:`48954` - several NXP devicetree bindings are missing
- :github:`48992` - qemu_leon3: tests/posix/common/portability.posix.common fails
- :github:`49021` - uart async api does not provide all received data
- :github:`49032` - espi saf testing disabled
- :github:`49069` - log: cdc_acm: hard fault message does not output
- :github:`49148` - Asynchronous UART API triggers Zephyr assertion on STM32WB55
- :github:`49210` - BL5340 board cannot build bluetooth applications
- :github:`49213` - logging.add.log_user test fails when compiled with GCC 12
- :github:`49266` - Bluetooth: Host doesn't seem to handle INCOMPLETE per adv reports
- :github:`49313` - nRF51822 sometimes hard fault on connect
- :github:`49338` - Antenna switching for Bluetooth direction finding with the nRF5340
- :github:`49373` - BLE scanning - BT RX thread hangs on.
- :github:`49390` - shell_rtt thread can starve other threads of the same priority
- :github:`49484` - CONFIG_BOOTLOADER_SRAM_SIZE should not be defined by default
- :github:`49492` - kernel.poll test fails on qemu_arc_hs6x when compiled with GCC 12
- :github:`49494` - testing.ztest.ztress test fails on qemu_cortex_r5 when compiled with GCC 12
- :github:`49584` - STM32WB55 Failed read remote feature, remote version and LE set PHY
- :github:`49588` - Json parser is incorrect with undefined parameter
- :github:`49611` - ehl_crb: Failed to run timer testcases
- :github:`49614` - acrn_ehl_crb: The testcase tests/kernel/sched/schedule_api failed to run.
- :github:`49656` - acrn_ehl_crb: testcases tests/kernel/smp failed to run on v2.7-branch
- :github:`49746` - twister: extra test results
- :github:`49811` - DHCP cannot obtain IP, when CONFIG_NET_VLAN is enabled
- :github:`49816` - ISOTP receive fails for multiple binds with same CAN ID but different extended ID
- :github:`49889` - ctf trace: unknown event id when parsing samples/tracing result on reel board
- :github:`49917` - http_client_req() sometimes hangs when peer disconnects
- :github:`49963` - Random crash on the L475 due to work->handler set to NULL
- :github:`49996` - tests: drivers: clock_control: nrf_lf_clock_start and nrf_onoff_and_bt fails
- :github:`50028` - flash_stm32_ospi Write enable failed when building with TF-M
- :github:`50084` - drivers: nrf_802154: nrf_802154_trx.c - assertion fault when enabling Segger SystemView tracing
- :github:`50095` - ARC revision Kconfigs wrongly mixed with board name
- :github:`50149` - tests: drivers: flash fails on nucleo_l152re because of wrong erase flash size
- :github:`50196` - LSM6DSO interrupt handler not being called
- :github:`50256` - I2C on SAMC21 sends out stop condition incorrectly
- :github:`50306` - Not able to flash stm32h735g_disco - TARGET: stm32h7x.cpu0 - Not halted
- :github:`50345` - Network traffic occurs before Bluetooth NET L2 (IPSP) link setup complete
- :github:`50354` - ztest_new: _zassert_base : return without post processing
- :github:`50404` - Intel CAVS: tests/subsys/logging/log_immediate failed.
- :github:`50427` - Bluetooth: host: central connection context leak
- :github:`50446` - MCUX CAAM is disabled temporarily
- :github:`50452` - mec172xevb_assy6906: The testcase tests/lib/cmsis_dsp/matrix failed to run.
- :github:`50501` - STM32 SPI does not work properly with async + interrupts
- :github:`50506` - nxp,mcux-usbd devicetree binding issues
- :github:`50515` - Non-existing test cases reported as "Skipped" with reason  “No results captured, testsuite misconfiguration?” in test report
- :github:`50546` - drivers: can: rcar: likely inconsistent behavior when calling can_stop() with pending transmissions
- :github:`50554` - Test uart async failed on Nucleo F429ZI
- :github:`50565` - Fatal error after ``west flash`` for nucleo_l053r8
- :github:`50567` - Passed test cases are reported as "Skipped" because of incomplete test log
- :github:`50570` - samples/drivers/can/counter fails in twister for native_posix
- :github:`50587` - Regression in Link Layer Control Procedure (LLCP)
- :github:`50590` - openocd: Can't flash on various STM32 boards
- :github:`50598` - UDP over IPSP not working on nRF52840
- :github:`50614` - Zephyr if got the ip is "10.xxx.xxx.xxx" when join in the switchboard, then the device may can not visit the outer net, also unable to Ping.
- :github:`50620` - fifo test fails with CONFIG_CMAKE_LINKER_GENERATOR enabled on qemu_cortex_a9
- :github:`50652` - RAM Loading on i.MXRT1160_evk
- :github:`50655` - STM32WB55 Bus Fault when connecting then disconnecting then connecting then disconnecting then connecting
- :github:`50658` - BLE stack notifications blocks host side for too long
- :github:`50709` - tests: arch: arm: arm_thread_swap fails on stm32g0 or stm32l0
- :github:`50732` - net: tests/net/ieee802154/l2/net.ieee802154.l2 failed on reel_board due to build failure
- :github:`50735` - Intel CAVS18: tests/boards/intel_adsp/hda_log/boards.intel_adsp.hda_log.printk failed
- :github:`50746` - Stale kernel memory pool API references
- :github:`50766` - Zephyr build system doesn't setup CMake host environment correctly
- :github:`50776` - CAN Drivers allow sending FD frames without device being set to FD mode
- :github:`50777` - LE Audio: Receiver start ready command shall only be sent by the receiver
- :github:`50778` - LE Audio: Audio shell: Unicast server cannot execute commands for the default_stream
- :github:`50780` - LE Audio: Bidirectional handling of 2 audio streams as the unicast server when streams are configured separately not working as intended
- :github:`50781` - LE Audio: mpl init causes warnings when adding objects
- :github:`50783` - LE Audio: Reject ISO data if the stream is not in the streaming state
- :github:`50789` - west: runners: blackmagicprobe: Doesn't work on windows due to wrong path separator
- :github:`50801` - JSON parser fails on multidimensional arrays
- :github:`50812` - MCUmgr udp sample fails with shell - BUS FAULT
- :github:`50841` - high SRAM usage with picolibc on nRF platforms

Addressed issues
================

* :github:`50861` - Intel ADSP HDA and GPDMA Bugs
* :github:`50843` - tests: kernel: timer: timer_behavior: kernel.timer.timer - SRAM overflow on nrf5340dk_nrf5340_cpunet and nrf52dk_nrf52832
* :github:`50841` - high SRAM usage with picolibc on some userspace platforms
* :github:`50774` - ESP32 GPIO34 IRQ not working
* :github:`50771` - mcan driver has tx and rx error counts swapped
* :github:`50754` - MCUboot update breaks compilation for boards without CONFIG_WATCHDOG=y
* :github:`50737` - tfm_ram_report does not work with sdk-ng 0.15.0
* :github:`50728` - missing SMP fixes for RISC-V
* :github:`50691` - Bluetooth: Host: CONFIG_BT_LOG_SNIFFER_INFO doesn't work as intended without bonding
* :github:`50689` - Suspected unaligned access in Bluetooth controller connection handling
* :github:`50681` - gpio: ite: gpio_ite_configure() neither supporting nor throwing error when gpio is configured with GPIO_DISCONNECTED flag
* :github:`50656` - Wrong definition of bank size for intel memory management driver.
* :github:`50654` - Some files are being ALWAYS built, without them being used
* :github:`50635` - hal: stm32: valid pins were removed in the last version
* :github:`50631` - Please Add __heapstats() to stdlib.h
* :github:`50621` - The history of the multi API / MFD discussions 2022 July - Sep
* :github:`50619` - tests/kernel/timer/starve fails to run on devices
* :github:`50618` - STM32 Ethernet
* :github:`50615` - ESP32 GPIO driver
* :github:`50611` - k_heap_aligned_alloc does not handle a timeout of K_FOREVER correctly
* :github:`50603` - Upgrade to loramac-node 4.7.0 when it is released to fix async LoRa reception on SX1276
* :github:`50579` - arch: arm: Using ISR_DIRECT_PM with zero-latency-interrupt violation
* :github:`50549` - USB: samhs: Device does not work after detach-attach sequence
* :github:`50545` - drivers: can: inconsistent behavior when calling can_stop() with pending transmissions
* :github:`50538` - lpcxpresso55s69_cpu0 samples/subsys/usb/dfu/sample.usb.dfu build failure
* :github:`50525` - Passed test cases reported as "Skipped" because test log lost
* :github:`50522` - mgmt: mcumgr: img_mgmt: Failure of erase returns nothing
* :github:`50520` - Bluetooth: bsim eatt_notif test fails with assertion in some environments
* :github:`50502` - iMX 7D GPIO Pinmux Array Has Incorrect Ordering
* :github:`50482` - mcumgr: img_mgmt: zephyr_img_mgmt_flash_area_id has wrong slot3 ID
* :github:`50468` - Incorrect Z_THREAD_STACK_BUFFER in arch_start_cpu for Xtensa
* :github:`50467` - Possible memory corruption on ARC when userspace is enabled
* :github:`50465` - Possible memory corruption on RISCV when userspace is enabled
* :github:`50464` - Boot banner can cut through output of shell prompt
* :github:`50455` - Intel CAVS15/25: tests/subsys/shell/shell failed with no console output
* :github:`50438` - Bluetooth: Conn: Bluetooth stack becomes unusable when communicating with both centrals and peripherals
* :github:`50432` - Bluetooth: Controller: Restarting BLE scanning not always working and sometimes crashes together with periodic. adv.
* :github:`50421` - Sysbuild-configured project using ``west flash --recover`` will wrongly recover (and reset) the MCU each time it flashes an image
* :github:`50414` - smp_dummy.h file is outside of zephyr include folder
* :github:`50394` - RT685 flash chip size is incorrect
* :github:`50386` - Twister "FLASH overflow" does not account for imgtool trailer.
* :github:`50374` - CI failure in v3.1.0-rc2 full run
* :github:`50368` - esp32: counter driver not working with absolute value
* :github:`50344` - bl5340_dvk_cpuapp: undefined reference to ``__device_dts_ord_14``
* :github:`50343` - uninitialized variable in kernel.workqueue test
* :github:`50342` - mcuboot: BOOT_MAX_ALIGN is redefined
* :github:`50341` - undefined reference to ``log_output_flush`` in sample.logger.syst.catalog
* :github:`50331` - net mem shell output indents TX DATA line
* :github:`50330` - Fail to find GICv3 Redistributor base address for Cortex-R52 running in a cluster different than 0
* :github:`50327` - JLink needs flashloader for MIMXRT1060-EVK
* :github:`50317` - boards/arm/thingy53_nrf5340: lack of mcuboot's gpio aliases
* :github:`50306` - Not able to flash stm32h735g_disco - TARGET: stm32h7x.cpu0 - Not halted
* :github:`50299` - CI fails building stm32u5  tests/subsys/pm/device_runtime_api
* :github:`50297` - mcumgr: fs_mgmt: hash/checksum: Build warnings on native_posix_64
* :github:`50294` - test-ci: timer_behavior: mimxrt1170_evk_cm7/1160: test failure
* :github:`50284` - Generated linker scripts break when ZEPHYR_BASE and ZEPHYR_MODULES share structure that contains symlinks
* :github:`50282` - samples: drivers: can: babbling: can controller not started.
* :github:`50266` - drivers: can: native_posix_linux: should not receive frames while stopped
* :github:`50263` - drivers: can: mcan: transceiver is enabled at driver initialization
* :github:`50257` - twister: --coverage option does not work for qemu_x86_64 and other boards
* :github:`50255` - Test process crash when run twister with --coverage
* :github:`50244` - GPIO manipulation from a “counter” (ie HW timer) when Bluetooth (BLE) is enabled.
* :github:`50238` - ESP32: rtcio_ll_pullup_disable crash regression
* :github:`50235` - UDP: Memory leak when allocated packet is smaller than requested
* :github:`50232` - gpio_shell: Not functional anymore following DT label cleanup and deprecation
* :github:`50226` - MPU FAULT: Stacking error with lvgl on lv_timer_handler()
* :github:`50224` - tests/kernel/tickless/tickless_concept: Failed on STM32
* :github:`50219` - Kernel tests failing on qemu_riscv32_smp
* :github:`50218` - rcar_h3ulcb: can: failed to run RTR test cases
* :github:`50214` - Missing human readable names in names file od deive structure
* :github:`50202` - Configuring ``GPIO25`` crashes ESP32
* :github:`50192` - nrf_qspi_nor driver might crash if power management is enabled
* :github:`50191` - nrf_qspi_nor-driver leaves CS pin to undefined state when pinctrl is enabled
* :github:`50172` - QSPI NAND Flash driver question
* :github:`50165` - boards: riscv: ite: No flash and RAM stats are shown whenever building ITE board
* :github:`50158` - Drivers: gpio: stm32u5 portG not working
* :github:`50152` - SMT32: incorrect internal temperature value
* :github:`50150` - tests: drivers: flash: building error with b_u585i_iot02a_ns board
* :github:`50146` - tests: kernel: mem_protect fails on ARMv6-M and ARMv8-M Baseline
* :github:`50142` - NXP i.MX RT1024 CPU GPIO access bug.
* :github:`50140` - ARP handling causes dropped packets when multiple outgoing packets are queued
* :github:`50135` - cannot boot up on custom board
* :github:`50119` - non-IPI path of SMP is broken
* :github:`50118` - Twister: ``--coverage-formats`` Does not work despite ``--coverage`` added
* :github:`50108` - drivers: console: rtt_console: undefined reference to ``__printk_hook_install``
* :github:`50107` - subsys: pm: device_runtime.c: compile error
* :github:`50106` - ram_report stopped working with zephyr-sdk 0.15
* :github:`50099` - boards: pinnacle_100_dvk should enable QSPI and modem by default
* :github:`50096` - tests: drivers:  the gpio_basic_api test cannot be build successfully on bl5340_dvk_cpuapp board
* :github:`50073` - mcumgr: Bluetooth backend does not restart advertising after disconnect
* :github:`50070` - LoRa: Support on RFM95 LoRa module combined with a nRF52 board
* :github:`50066` - tests: tests/drivers/can/shell failed in daily test on many platforms
* :github:`50065` - tests: tests/subsys/shell/shell test case fail in daily test on many platforms
* :github:`50061` - Bluetooth: Samples: bluetooth_audio_source does not send multiple streams
* :github:`50044` - reel_board: pyocd.yaml causes flashing error on reel board
* :github:`50033` - tests: subsys: fs: littlefs: filesystem.littlefs.custom fails to build
* :github:`50032` - tests: subsys: shell: shell.core and drivers.can.shell fails at shell_setup
* :github:`50029` - Unable to use functions from gsm_ppp driver
* :github:`50023` - tests: some driver tests of frdm_k64f build failed in twister (shows devicetree error)
* :github:`50016` - jlinkarm.so files renamed in latest J-Link drivers
* :github:`49988` - boards: pinnacle_100_dvk: UART1 flow control is not turned on
* :github:`49987` - Unable to compile on windows
* :github:`49985` - STM32:NUCLEO_WL55JC No serial RX in STOP mode
* :github:`49982` - SD: f_sync will always fail using the sdhc_spi driver
* :github:`49970` - strange behavior in the spi_flash example
* :github:`49960` - LoRaWAN Code won't linking when config with CN470 region
* :github:`49956` - ``NRF_DRIVE_S0D1`` option is not always overridden in the ``nordic,nrf-twi`` and ``nordic,nrf-twim`` nodes
* :github:`49953` - stm32 gpio_basic_api test fail with CONFIG_ZTEST_NEW_API
* :github:`49939` - stm32 adc driver_api test fails on stm32wb55 and stm32l5
* :github:`49938` - drivers/modem/gsm_ppp.c:  unnecessary modem_cmd_handler_tx_lock when CONFIG_GSM_MUX disabled
* :github:`49924` - tests: drivers: pwm_api and pwm_loopback tests failed on frdm_k64f boards
* :github:`49923` - ASSERTION FAIL [!arch_is_in_isr()] @ WEST_TOPDIR/zephyr/kernel/sched.c:1449
* :github:`49916` - renesas smartbond family Kconfig visible to non-renesas devices
* :github:`49915` - Bluetooth: Controller: Syncing with devices with per. adv. int. < ~10ms eventually causes events from BT controller stop arriving
* :github:`49903` - riscv: Enabling IRQ vector table makes Zephyr unbootable
* :github:`49897` - STM32: NUCLEO_WL55JC internal (die) temperature incorrect
* :github:`49890` - drivers/can: stm32_fd: CONFIG_CAN_STM32FD_CLOCK_DIVISOR not applied in driver setup
* :github:`49876` - drivers: can: twai: driver fails initialization
* :github:`49874` - STM32G0 HW_STACK_PROTECTION Warning
* :github:`49852` - uart: extra XOFF byte in the read buffer
* :github:`49851` - Bluetooth Controller with Extended Advertising
* :github:`49846` - mimxrt1160_evk network samples stopped working
* :github:`49825` - net: ip: tcp: use zu format specifier for size_t
* :github:`49823` - Example Application: Use of undocumented zephyr/module.yml in application folder
* :github:`49814` - Cortex-A9 fails to build cmsis due to missing core_ca.h
* :github:`49805` - stm32f1: can2 & eth pin remap not working
* :github:`49803` - I/O APIC Driver in Zephyr makes incorrect register access.
* :github:`49792` - test-ci:  adc-dma :  frdm-k64f: dma dest addess assert
* :github:`49790` - Intel CAVS25: Failure in tests/boards/intel_adsp/smoke sporadically
* :github:`49789` - it8xxx2_evb: tests/crypto/tinycrypt/ test takes longer on sdk 0.15.0
* :github:`49786` - nsim_em: tests: fail to run tests/kernel/timer/timer_behavior
* :github:`49769` - STM32F1 CAN2 does not enable master can gating clock
* :github:`49766` - Document downstream module configuration recommendations
* :github:`49763` - nucleo_f767zi: sample.net.gptp build fails
* :github:`49762` - esp32: testing.ztest.error_hook.no_userspace build fails due to array-bounds warnings
* :github:`49760` - frdm_kl25z: sample.usb.dfu Kconfig issue causing build failure
* :github:`49747` - CAN2 interface on STM32F105 not working
* :github:`49738` - Bluetooth: Controller: Restarting periodic advertising causes crash when ADV_SYNC_PDU_BACK2BACK=y
* :github:`49733` - Error log "Could not lookup stream by iso 0xXXXXXXXX" from unicast server if client release the stream
* :github:`49717` - mcumgr: Bluetooth transport fix prevents passing GATT notify status back to SMP
* :github:`49716` - Intel CAVS15/18: Failure in tests/arch/common/timing
* :github:`49715` - The function ospi_read_sfdp in drivers/flash/flash_stm32_ospi.c can corrupt the stack
* :github:`49714` - tests:  tests/drivers/gpio/gpio_api_1pin failed on mec172xevb_assy6906 in daily test
* :github:`49713` - frdm_k64f: tests: failed to run tests/drivers/adc/adc_dma/drivers.adc-dma
* :github:`49711` - tests/arch/common/timing/arch.common.timing.smp fails for CAVS15, 18
* :github:`49703` - eSPI: Add platform specific Slave to Master Virtual Wires
* :github:`49696` - twister: testplan: toolchain_exclude filter is overridden by integration_platforms
* :github:`49687` - West: Allow having .west folder and west.yml in the same folder
* :github:`49678` - Zephyr 3.2 module updates overview
* :github:`49677` - STM32U5 consumes more current using power management
* :github:`49663` - Bluetooth seems to not work randomly on target device
* :github:`49662` - hello world+ mcuboot is not working
* :github:`49661` - mcumgr: bt transport runs in system workqueue thread and can cause resource deadlock
* :github:`49659` - logging: LOG_* appends 0x0D to 0x0A
* :github:`49648` - tests/subsys/logging/log_switch_format, log_syst build failures on CAVS
* :github:`49637` - CMSIS-DSP tests broken with SDK 0.15.0
* :github:`49631` - arch: arm: FP stack warning with GCC 12 and ``CONFIG_FPU=y``
* :github:`49629` - Bluetooth: ISO Broadcast sample fails to send data on nRF5340
* :github:`49628` - Compilation fails when ASAN is used with gcc
* :github:`49618` - &usart2_rx_pd6 no more available for STM32L073RZ
* :github:`49616` - acrn_ehl_crb: The testcase tests/kernel/common failed to run.
* :github:`49609` - sdk: failed to run tests/subsys/logging/log_core_additional
* :github:`49607` - ADC reading on E73-2G4M04S1B and nrf52dk
* :github:`49606` - BeagleBone Black / AM335x Support
* :github:`49605` - it8xxx2_evb: tests/kernel/timer/timer_api test failed after commit cb041d06
* :github:`49602` - Bluetooth: Audio: Build error when enable  CONFIG_LIBLC3
* :github:`49601` - mec15xxevb_assy6853: tests/drivers/adc/adc_api asynchronous test failed
* :github:`49599` - Bluetooth: Host: Unable to pair zephyr bluetooth peripheral with Secure connection and static passkey
* :github:`49590` - devicetree parsing does not error out on duplicate node names
* :github:`49587` - cross-compile toolchain variant doesn't working properly with multilib toolchain
* :github:`49586` - Json parser is incorrect with undefined parameter
* :github:`49578` - [RFC] Deprecate <zephyr/zephyr.h>
* :github:`49576` - tests: kernel: timer: timer_behavior: kernel.timer.timer fails
* :github:`49572` - Reproducable builds with MCUboot signing
* :github:`49542` - sdk: it8xxx2_evb cannot build the hello_world sample after zephyr SDK upgrade to 0.15.0
* :github:`49540` - Bluetooth: Host: sync termination callback parameters not populated correctly when using per. adv. list feature.
* :github:`49531` - LE Audio: Broadcast Sink not supporting general and specific BIS codec configurations in the BASE
* :github:`49523` - k_sleep in native_posix always sleeps one tick too much
* :github:`49498` - net: lib: coap: update method_from_code() to report success/failure
* :github:`49493` - Bluetooth: ISO: samples/bluetooth/broadcast_audio_source error -122
* :github:`49491` - arch.interrupt test fails on ARM64 QEMU targets when compiled with GCC 12
* :github:`49482` - stm32g0 interrupts for usart3,4,5,6 all set to 29
* :github:`49471` - stm32: dietemp node generates warning
* :github:`49465` - Bluetooth: Controller: Periodic adv. sync. degraded performance on latest main branch
* :github:`49463` - STM32G0B0 errors out on stm32g0_disable_dead_battery function in soc.c
* :github:`49462` - tests: tests/kernel/fatal/exception/ test case fail
* :github:`49444` - mcumgr: Outgoing packets that are larger than the transport MTU are wrongly split into different individual messages
* :github:`49442` - Intel CAVS25: Failure in tests/kernel/smp
* :github:`49440` - test-ci: mimxrt11xx: testing.ztest.base.verbose_x and crypto.tinycryp : run failure no console output
* :github:`49439` - test-ci: lpcxpresso54114_m4: libraries.devicetree.devices.requires test failure
* :github:`49410` - Bluetooth: Scan responses with info about periodic adv. sometimes stops being reported
* :github:`49406` - flash_stm32_ospi: OSPI wr in OPI/STR mode is for 32bit address only
* :github:`49360` - west boards doesn't print boards from modules
* :github:`49359` - nrf5*: crash when Bluetooth advertisements and flash write/erase are used simultaneously
* :github:`49350` - RFC: Add arch aligned memory Kconfig option
* :github:`49342` - Zephyr hci_usb sample cannot use LE coded phy
* :github:`49331` - device if got the ip is "10.4.239.xxx" when join in the switchboard, then the device can not visit the outer net.
* :github:`49329` - twister: frdm_k64f: test string mismatch
* :github:`49315` - loopback socket send from shell hangs
* :github:`49305` - Can't read and write to the Nor Flash at address 0x402a8000 on RT1060
* :github:`49268` - tests: samples/boards/stm32/power_mgmt/serial_wakeup failed on mec15xxevb_assy6853 (and several stm32 boards)
* :github:`49263` - ztest: tracing backend works incorrectly when new ZTEST enabled.
* :github:`49258` - MCUboot not loading properly due to missing ALIGN
* :github:`49251` - STM32 HW TIMER + DMA + DAC
* :github:`49203` - Intel CAVS15: Failure in tests/boards/intel_adsp/hda,hda_log
* :github:`49200` - Intel CAVS: Failure in tests/kernel/interrupt
* :github:`49195` - Integrate Zephyr SDK 0.15.0 to the Zephyr main CI
* :github:`49184` - DHCP client is not ``carrier`` aware
* :github:`49183` - Missing handling of UNKNOWN_RSP in peripheral PHY UPDATE procedure
* :github:`49178` - subsys: pm:  stats:  typo causes build failure
* :github:`49177` - usb: sam0: device driver is leaking memory when interface is reset
* :github:`49173` - Bluetooth: empty notification received by peer after unsubscribe
* :github:`49169` - v2m_musca_s1_ns fails to build several tfm related samples
* :github:`49166` - samples/drivers/flash_shell/sample.drivers.flash.shell fails to build on a few nxp platforms
* :github:`49164` - samples/arch/smp/pi/sample.smp.pi fails on a number esp32 platforms
* :github:`49162` - Calling cache maintenance APIs from user mode threads result in a bad syscall error.
* :github:`49154` - SDMMC driver with STM32 U575
* :github:`49145` - tests: kernel: fifo: fifo_timeout: kernel.fifo.timeout fails on nrf5340dk_nrf5340_cpuapp
* :github:`49142` - Bluetooth: Audio: MCC subscribe failure
* :github:`49136` - L2CAP ecred test cases failed.
* :github:`49134` - STM32G070RBT6 can not build with zephyr 3.1.99
* :github:`49119` - ARC: west: mdb runner: fix folder where MDB is run
* :github:`49116` - cmake cached BOARD_DIR variable does not get overwritten
* :github:`49106` - Add cherryusb as a module
* :github:`49105` - hda_host and hda_link registers block size are not equal
* :github:`49102` - hawkbit - dns name randomly not resolved
* :github:`49100` - STM b_u585i_iot02a  NOR flash and OSPI_SPI_MODE, erase failed
* :github:`49086` - twister: frdm_k64f: twister process blocks after the flash error occurs
* :github:`49074` - GD32: Use clocks instead rcu-periph-clock property
* :github:`49073` - SOC_FLASH_LPC vs SOC_FLASH_MCUX
* :github:`49066` - Mcumgr img_mgmt_impl_upload_inspect() can cause unaligned memory access hard fault.
* :github:`49057` - USB Mass Storage Sample crashes due to overflow of Mass Storage Stack
* :github:`49056` - STM b_u585i_iot02a MCUboot crash
* :github:`49054` - STM32H7 apps are broken in C++ mode due to HAL include craziness
* :github:`49051` - Nrf52832 ADC SAMPLE cannot compile
* :github:`49047` - LORAWAN Devicetree sx1262 setup on rak4631_nrf52840 board
* :github:`49046` - Cannot use devices behind I2C mux (TCA9548A)
* :github:`49044` - doc: boards: litex_vexrisc: update with common environment variables and arty-a7-100t support
* :github:`49036` - soc: telink_b91: ROM region section overlap
* :github:`49027` - Regulator support for gpio-leds
* :github:`49019` - Fix multiple issues with adxl372 driver
* :github:`49016` - intel_adsp smoke test fails with CONFIG_LOG_MIPI_SYST_USE_CATALOG=y
* :github:`49014` - Advertising address not updated after RPA Timeout with Extended Advertising enabled
* :github:`49012` - pm breaks intel dai ssp in cavs25
* :github:`49008` - ESP32:  net_buf_get() FAILED
* :github:`49006` - tests: subsys: portability: cmsis_rtos_v2: portability.cmsis_rtos_v2 -   test_timer - does not end within 60 sec
* :github:`49005` - samples: tfm_integration: tfm_regression_test: sample.tfm.regression_ipc_lvl2 no console output within 210 sec - timeout
* :github:`49004` - unexpected eof in qemu_cortex and mps2
* :github:`49002` - tests: subsys: settings: functional: fcb: system.settings.functional.fcb fails
* :github:`49000` - tests: arch: arm: arm_thread_swap: arch.arm.swap.common.no_optimizations USAGE FAULT
* :github:`48999` - tests: arch: arm: arm_interrupt: arch.interrupt.no_optimizations Wrong crash type got 2 expected 0
* :github:`48997` - tests: kernel: workq: work_queue: kernel.workqueue fails
* :github:`48991` - Receiving message from pc over PCAN-USB FD
* :github:`48977` - kernel: mem_protect: mimxrt11xx series build failure
* :github:`48967` - modem: hl7800 runtime log control API is broken
* :github:`48960` - coap_packet_parse() should return different values based on error type
* :github:`48951` - stm32wb55 BLE unable to connect / pair
* :github:`48950` - cmake: string quotes are removed from extra_kconfig_options.conf
* :github:`48937` - Compilation error when adding lwm2m client on CHIP/matter sample
* :github:`48921` - build system/west: Add a warning if any project repo does not match the manifest
* :github:`48918` - ztest: tests: add CONFIG_ZTEST_SHUFFLE=y to tests/subsys/logging/log_benchmark/prj.conf cause build failure
* :github:`48913` - net: Add pointer member to net_mgmt_event_callback struct to pass user data to the event handler.
* :github:`48912` - sample.drivers.flash.shell: Failed on NXP targets
* :github:`48911` - sample.drivers.flash.shell: Failed on atmel targets
* :github:`48907` - Does esp32 support BLE Mesh
* :github:`48897` - twister --sub-test never works
* :github:`48880` - BLE notifications on custom service not working anymore: <wrn> bt_gatt: Device is not subscribed to characteristic
* :github:`48877` - tests: kernel: mem_slab: mslab: kernel.memory_slabs fails
* :github:`48875` - tests: kernel: context: kernel.context fails at test_busy_wait and Kernel panic at test_k_sleep
* :github:`48863` - hawkbit subsystem - prints garbage if debug enabled and no update pending
* :github:`48829` - cbprintf is broken on multiple platforms with GCC 12
* :github:`48828` - Clicking a link leads to "Sorry, Page Not Found", where they ask to notify this GitHub
* :github:`48823` - Bluetooth: controller: llcp: limited nr. of simultaneous connections
* :github:`48813` - Bluetooth: bt_conn_disconnect randomly gives error "bt_conn: not connected!"
* :github:`48812` - Bluetooth controller extended advertisement crashes in lll layer
* :github:`48808` - Pinctl api breaks NXP imx6sx
* :github:`48806` - Bluetooth: controller: conformance test instability
* :github:`48804` - LE Audio: Add HAP sample to Zephyr footprint tracking
* :github:`48801` - test: driver: wdt: wdt cases fails in LPC platform randonly
* :github:`48799` - Why is the command input incomplete?
* :github:`48780` - boards: bus devices label names should include address on bus
* :github:`48779` - net.socket.select: failed (qemu/mps2_an385)
* :github:`48757` - Windows10 Installation: Failed to run ``west update``
* :github:`48742` - Linking fails during build when referencing functions in ``zephyr/bluetooth/crypto.h``
* :github:`48739` - net: tcp: Implicit MSS value is not correct
* :github:`48738` - dts: label: label defined in soc does not take effects in final zephyr.dts
* :github:`48731` - gen_handles script fails with pwmleds handle
* :github:`48725` - arm_thread_swap:  tests/arch/arm/arm_thread_swap/ failed on reel_board
* :github:`48724` - mpu9250 driver init function register setup using the same config parameter twice.
* :github:`48722` - flash_map: pointer dereferencing causes build to fail
* :github:`48718` - Completely disabling IP support leads to a build error when enabling IEEE 802.15.4 L2 support
* :github:`48715` - Enabling NET_L2_IEEE802154 and IEEE802154_RAW_MODE together breaks the build
* :github:`48699` - Is there a way to port the Bluetooth host stack to linux?
* :github:`48682` - ADC Support for STM32U575
* :github:`48671` - SAM V71B Initial USB Transfer Drops Data Bytes
* :github:`48665` - tests/usb/device: Add zassert to match zassume usage.
* :github:`48642` - nucleo_l011k4 does not build
* :github:`48630` - Process: maintainer involvement in triaging issues
* :github:`48626` - jlink flasher not working with recent versions of pylink dependency
* :github:`48620` - LC3 External Source Code
* :github:`48591` - Can't run zephyr application from SDRAM on RT1060-EVKB
* :github:`48585` - net: l2: ieee802154: decouple l2/l3 layers
* :github:`48584` - Remove netifaces Python package dependency
* :github:`48578` - NRF GPIO Toggle introduces race condition when multithreaded
* :github:`48567` - MIMXRT1060 custom board support for NXP HAL modules
* :github:`48547` - ztest: Incorrect display of test duration value.
* :github:`48541` - subsys/net/l2/ppp/fsm.c: BUS FAULT
* :github:`48537` - Can gpio output configuration flags be expressed in the devicetree?
* :github:`48534` - SMF missing events
* :github:`48531` - RFC: Changing the sys_clock interface to fix race conditions.
* :github:`48523` - Mathematical operations in Kconfig
* :github:`48518` - ``samples/sensor/*``: Build issue when board expose sensors defined on both I2C and SPI buses
* :github:`48516` - flash: sam: Build error for sam4s_xplained
* :github:`48514` - bsim mesh establish_multi.sh doesn't send data for one of devices
* :github:`48512` - frdm_k64f : failed to run tests/drivers/dma/scatter_gather
* :github:`48507` - error on console usb app.overlay
* :github:`48501` - Usage Fault :  Illegal use of EPSR , NRFSDK 2.0.0 and BLE DFU NRF52840 DK
* :github:`48492` - gdbstub for arc core
* :github:`48480` - ZTEST: duplicate symbol linker error
* :github:`48471` - net: tcp: Persistent timer for window probing does not implement exponential backoff
* :github:`48470` - Inconsistent return value of uart_mux_fifo_fill when called inside/outside of an ISR
* :github:`48469` - [bisected] 5a850a5d06e1 is breaking some tests on ARM64
* :github:`48465` - net: tcp: SYN flag received after connection is established should result in connection reset
* :github:`48463` - Grant Triage permission level to @aurel32
* :github:`48460` - Provide duration of each testsuite and testcase in ztest test summary.
* :github:`48459` - bluetooth: host: Dangling pointer in le_adv_recv
* :github:`48447` - ``hwinfo devid`` does not work correctly for NXP devices using ``nxp,lpc-uid`` device binding
* :github:`48444` - Problem upgrading ncs 1.5.1 upgrade to ncs 1.9.1 failed
* :github:`48424` - ZTEST Framework fails when ztest_run_all is called multiple times
* :github:`48416` - samples: samples/subsys/tracing is broken for UART tracing
* :github:`48392` - Compiling failure watchdog sample with nucleo_u575zi_q
* :github:`48386` - twister cannot take ``board@revision`` as platform filter
* :github:`48385` - Compilation failures on Cavs 18/20/25 GCC
* :github:`48380` - shell: Mixing mandatory arguments w/ SHELL_OPT_ARG_RAW causes crash
* :github:`48367` - Wrong clock assigned
* :github:`48343` - NVS nvs_recover_last_ate() does not align data length
* :github:`48328` - Add API to get the nvs_fs struct from the settings backend
* :github:`48321` - twister: bug in platform names verification
* :github:`48306` - Lwm2m_client sample broken on native_posix target
* :github:`48302` - West search for "compatible" device tree property does not expand C preprocessor macros
* :github:`48290` - ESP32 ble no work while enable CONFIG_SETTINGS
* :github:`48282` - BT_H4 overriding BT_SPI=y causing build to fail - HCI Host only build SPI bus
* :github:`48281` - Fix github permissions for user "alevkoy"
* :github:`48267` - No model in devicetree_unfixed.h :
* :github:`48253` - Only the first failing test is aborted and marked failed
* :github:`48223` - base64.c encode returns wrong count of output bytes
* :github:`48220` - adxl345: sensor value calculation should be wrong
* :github:`48216` - Running gPTP sample application on SAMe54 Xplained pro(Supports IEEE 802.1 AS gPTP clock) , PDelay Response Receipt Timeout
* :github:`48215` - docs: build the documentation failed due to "Could NOT find LATEX"
* :github:`48198` - NPCX Tachometer driver compiled despite status = "disabled"
* :github:`48194` - Support J-Link debugger for RaspberryPi Pico
* :github:`48185` - LV_Z_DISPLAY_DEV_NAME symbol has not got "parent" symbol with a type
* :github:`48175` - stm32 octospi flash driver
* :github:`48149` - Sensor Subsystem: client facing API: finding sensors
* :github:`48115` - tests: subsys: dfu: mcuboot_multi: dfu.mcuboot.multiimage hangs at first test case - test_request_upgrade_multi
* :github:`48113` - Zephyr support for STM32U5 series MCU
* :github:`48111` - LVGL: License agreement not found for the font arial.ttf
* :github:`48104` - [v 1.13 ] HID is not connecting to Linux based Master device
* :github:`48098` - Build error for samples/bluetooth/unicast_audio_server of nrf52dk-nrf52832 board
* :github:`48089` - AF_PACKET sockets not filling L2 header details in ``sockaddr_ll``
* :github:`48081` - tests/drivers/clock_control/stm32_clock_configuration/stm32u5_core not working with msis 48
* :github:`48071` - mec15xxevb_assy6853: test_i2c_pca95xx failed
* :github:`48060` - Have modbus RTU Client and modbus TCP Master on the same microcontroller
* :github:`48058` - Reading out a GPIO pin configured as output returns invalid value.
* :github:`48056` - Possible null pointer dereference after k_mutex_lock times out
* :github:`48055` - samples: subsys: usb: audio: headphones_microphone and headset - Can not get USB Device
* :github:`48051` - samples: logger: samples/subsys/logging/logger/sample.logger.basic failed on acrn_ehl_crb board
* :github:`48047` - Reference to obsolete files in cmake package docs
* :github:`48007` - tests: gpio driver fails in pin_get_config
* :github:`47991` - BLE functionality for STM32WB55 is limited with full version of BLE stack
* :github:`47987` - test: samples/boards/mec15xxevb_assy6853/power_management failed after commit 5f60164a0fc
* :github:`47986` - Rework of STM32 bxCAN driver filter handling required
* :github:`47985` - ARC wrong .debug_frame
* :github:`47970` - Flash: SFDP parameter address is not correct
* :github:`47966` - TCP: Zero window probe packet incorrect
* :github:`47948` - _kernel.threads' always points to NULL(0x0000'0000)
* :github:`47942` - Mutex priority inheritance when thread holds multiple mutexes
* :github:`47933` - tests: subsys: logging: log_switch_format: logging.log_switch_format - test_log_switch_format_success_case - Assertion failed
* :github:`47930` - tests: arch: arm: arm_interrupt: arch.interrupt.no_optimizations - Data Access Violation - MPU Fault
* :github:`47929` - tests: arch: arm: arm_thread_swap: arch.arm.swap.common.no_optimizations - Data Access Violation - MPU Fault
* :github:`47925` - Asynchronous UART API (DMA) not working like expected on nrf52840
* :github:`47921` - tests: pin_get_config failed on it8xxx2_evb
* :github:`47904` - drivers: can: loopback driver only compares loopback frames against CAN IDs in installed filters
* :github:`47902` - drivers: can: mcux: flexcan: failure to handle RTR frames correctly
* :github:`47895` - samples: smp_svr missing CONFIG_MULTITHREADING=y dependency
* :github:`47860` - Bluetooth: shell: bt init sync enables Bluetooth asynchronously
* :github:`47857` - Zephyr USB-RNDIS
* :github:`47855` - tests: arch: arm: fpu: arch.arm.swap.common.fpu_sharing.no_optimizations - Data Access Violation - MPU Fault
* :github:`47854` - Multiple blinking LED's cannot be turned off
* :github:`47852` - samples: boards: nrf: s2ram No valid output on console
* :github:`47847` - How to PM change pm_state
* :github:`47833` - Intel CAVS: cavstool.py fails to extract complete log from winstream buffer when logging is frequent
* :github:`47830` - Intel CAVS: Build failure due to #47713 PR
* :github:`47825` - qemu_cortex_a53_smp: tests/kernel/profiling/profiling_api failed
* :github:`47822` - Stack Overflow when calling spi at an interrupt on STM32l4
* :github:`47783` - warning: attempt to assign the value 'y' to the undefined symbol UART_0_NRF_FLOW_CONTROL
* :github:`47781` - MCUbootloader with b_u585i_iot02a (stm32u585) boot error
* :github:`47780` - WS2812 driver not work on nRF52833DK
* :github:`47762` - Some github users in the MAINTAINERS file are missing permissions
* :github:`47751` - soc/arm/common/cortex_m doesn't work for out-of-tree socs
* :github:`47742` - NXP LPC MCAN driver front-end lacks pinctrl support
* :github:`47734` - tests/posix/eventfd/ : failed on both nucleo_f103rb and nucleo_l073rz with 20K RAM only
* :github:`47731` - JESD216 fails to read SFDP on STM32 targets
* :github:`47725` - qemu_arc: tests/kernel/context/ failed when migrating to new ztest API
* :github:`47719` - Configure-time library dependency problem
* :github:`47714` - test: tests/lib/sprintf/ build fail
* :github:`47702` - twister: regression : Failures are counted as errors
* :github:`47696` - tests: arch: arm: arm_thread_swap: regression since use of new ztest API
* :github:`47682` - bt_gatt_unsubscribe creates write request to CCC and then cancels it
* :github:`47676` - bt_data_parse is destructive without warning
* :github:`47652` - The client-server based cavstool.py might be stuck when the ROM is not start
* :github:`47649` - ATT Notification buffer not released after reconnection
* :github:`47641` - Poor Ethernet Performance using NXP Enet MCUX Driver
* :github:`47640` - Zephyr and caches: a difficult love story.
* :github:`47613` - Samples / Tests without a testcase.yaml or sample.yaml
* :github:`47683` - TCP Connected Change the window size to 1/3/ff fail
* :github:`47609` - posix: pthread: descriptor leak with pthread_join
* :github:`47606` - nvs_read return value is not correct
* :github:`47592` - test: tests/drivers/gpio/gpio_basic_api failed after commit 2a8e3fe
* :github:`47588` - tests: sprintf: zero-length gnu_printf format string
* :github:`47580` - https connect failing with all the samples (qemu_x86 & mbedtls)
* :github:`47576` - undefined reference to ``__device_dts_ord_20`` When building with board hifive_unmatched on flash_shell samples
* :github:`47568` - uart_mcux_lpuart driver activates the noise error interrupt but does not clear the noise error flag
* :github:`47556` - sample: logging: Builds failing for samples/subsys/logging/syst
* :github:`47551` - Enabling CONFIG_OPENTHREAD_SRP_CLIENT on NRF52840 dongle board leads to MBED compilation errors.
* :github:`47546` - Revert https://github.com/zephyrproject-rtos/zephyr/pull/47511
* :github:`47520` - Support for sub-ghz channels in at86rf2xx radio driver
* :github:`47512` - up_squared: issues of EFI console feature
* :github:`47508` - tests: arch: the xtensa_asm2 test is broken
* :github:`47483` - PPP + GSM MUX doesn't work with Thales PLS83-W modem
* :github:`47476` - SX127x LoRaWAN - Failing on Boot - Missing Read/Write functions?
* :github:`47461` - Unable to build the flash_shell samples with board cc1352r1_launchxl
* :github:`47458` - BQ274XX Sensors Driver - Fails with CONFIG_BQ274XX_LAZY_CONFIGURE
* :github:`47445` - USB OTG FS controller support on STM32F413 broken
* :github:`47428` - SRAM increase in Bluetooth  [samples: bbc_microbit: pong fails to build]
* :github:`47426` - ZTEST_USER tests being skipped on systems without userspace support
* :github:`47420` - Tests: unittest with new ZTEST API
* :github:`47409` - LE Audio: Read PACS available contexts as unicast client
* :github:`47407` - stm32l5: tfm: Build error on tests/arch/arm/arm_thread_swap_tz
* :github:`47379` - Crypto sample fail to build with cryp node in .dts for STM32u5 (error: unknown type name 'CRYP_HandleTypeDef' etc.)
* :github:`47356` - cpp: global static object initialisation may fail for MMU and MPU platforms
* :github:`47330` - ARM Cortex-R52 doesn't have SPSR_hyp
* :github:`47326` - drivers: WINC1500: issues with buffer allocation when using sockets
* :github:`47323` - STM32WL LoRa SoC stuck at initialization due to SPI transmit buffer not emptying
* :github:`47307` - tests: kernel: fatal: exception: build failure on multiple platforms
* :github:`47301` - Module request: Lua
* :github:`47300` - Intel CAVS: Failure in tests/lib/spsc_pbuf
* :github:`47292` - it8xxx2_evb: many test cases failed probably due to the west update
* :github:`47288` - tests: posix: increase coverage for picolibc
* :github:`47275` - builds are broken with gnuarmemb toolchain, due to picolibc tests/configuration
* :github:`47273` - linker script: Vector table regression due to change in definition of _vector_end
* :github:`47272` - nrf51_ble400: onboard chip should be updated to nRF51822_QFAC in dts
* :github:`47248` - LE Audio: Crash on originating call.
* :github:`47240` - net: tcp: Correctly handle overlapped TCP retransmits
* :github:`47238` - SD Card init issue when CONFIG_SPEED_OPTIMIZATIONS=y
* :github:`47232` - Please add STM32F412RX
* :github:`47222` - zephyr doc: Unable to open pdf document version 3.1.0
* :github:`47220` - Twister: Skipping ``*.cpp`` files
* :github:`47204` - CAN filter with RTR mask causes infinite loop in MCAN driver on filtered message arrival
* :github:`47197` - BLE latency decreasing and increasing over time (possibly GPIO issue)
* :github:`47146` - STM32F103:  USB clock prescaler isn't set during USB initialisation
* :github:`47127` - twister : frdm_k64f ：Non-existent tests appear and fail on tests/lib/cmsis_dsp/transform
* :github:`47126` - New ztest API: build failure on qemu_cortex_m3 when CONFIG_CMAKE_LINKER_GENERATOR=y
* :github:`47119` - ADC_DT_SPEC_GET not working for channels >= 10
* :github:`47114` - ``check_compliance.py`` crash on Ubuntu 22.04
* :github:`47105` - drivers: clock_control: stm32 common: wrong PLLCLK rate returned
* :github:`47104` - Bluetooth: Controller: Errors in implementation of tx buffer queue mechanism
* :github:`47101` - drivers: clock_control: stm32 common: PLL_Q divider not converted to reg val
* :github:`47095` - ppp network interface -  MQTT/TCP communication
* :github:`47082` - drivers: modem: AT commands sent before OK from previous is received
* :github:`47081` - on x86, k_is_in_isr() returns false in execption context
* :github:`47077` - Intel CAVS: tests/subsys/logging/log_switch_format/ are skipped as no result captured
* :github:`47072` - ZTEST Docs Page
* :github:`47062` - dt-bindings: clock: STM32G4 device clk sources selection helper macros don't match the SOCs CCIPR register
* :github:`47061` - pipes: Usage between task and ISR results in corrupted pipe state
* :github:`47054` - it8xxx2_evb: flash fail in daily test
* :github:`47051` - drivers: usb: stm32: usb_write size on bulk transfer problematic
* :github:`47046` - samples/net/sockets/packet: Bus fault
* :github:`47030` - drivers: gpio: nrfx: return -ENOTSUP rather than -EIO for misconfigurations
* :github:`47025` - mimxrt1050_evk: reset cause
* :github:`47021` - Integrate Würth Elektronik Sensors SDK code for use in sensor drivers
* :github:`47010` - ACRN: failed to run the test case tests/drivers/coredump/coredump_api
* :github:`46988` - samples: net: openthread: coprocessor: RCP is missing required capabilities: tx-security tx-timing
* :github:`46985` - uOSCORE/uEDHOC integration as a Zephyr module
* :github:`46962` - Regression in apds9960 driver
* :github:`46954` - Binaries found in hal_nxp without conspicuous license information
* :github:`46935` - Not printk/log output working
* :github:`46931` - flash_stm32_ospi.c: Unable to erase flash partition using flash_map API
* :github:`46928` - drivers: modem: gsm_ppp: support hardware flow control
* :github:`46925` - Intel CAVS: tests/lib/mem_block/ failed, caused by too frequent log output.
* :github:`46917` - frdm_k64f : failed to run tests/drivers/gpio/gpio_get_direction
* :github:`46901` - RFC: I3C I2C API
* :github:`46887` - Automatically organize BLE EIR/AD data into a struct instead of providing it in a simple_network_buffer.
* :github:`46865` - Intel CAVS: Support for different ports for client / server
* :github:`46864` - Intel CAVS: cavstool_client.py sporadically fails
* :github:`46847` - STOP2 Mode on Nucleo-WL55JC1 not accessed
* :github:`46829` - LE Audio: Avoid multiple calls to ``bt_iso_chan_connect`` in parallel
* :github:`46822` - L2CAP disconnected packet timing in ecred reconf function
* :github:`46807` - lib: posix: semaphore: use consistent timebase in sem_timedwait
* :github:`46801` - Revisit modules and inclusion in the default manifest
* :github:`46799` - IRQ vector table: how to support different formats
* :github:`46798` - Zephyr does not store a new IRK when another device re-bonds with a Zephyr device
* :github:`46797` - UART Asynchronous API continuous data receiving weird behaviour
* :github:`46796` - IRQ vector table
* :github:`46793` - tests: posix: use new ztest api
* :github:`46765` - test-ci: kernel.timer: test_timer_remaining asserts
* :github:`46763` - LE Audio: Unicast Audio read PAC location
* :github:`46761` - logging: tagged arguments feature does not work with char arrays in C++
* :github:`46757` - Bluetooth: Controller: Missing validation of unsupported PHY when performing PHY update
* :github:`46749` - mbox: wrong syscall check
* :github:`46743` - samples: net: civetweb: websocket_server
* :github:`46740` - stm32 flash ospi fails on stm32l5 and stm32u5 disco
* :github:`46734` - drivers/disk: sdmmc: Doesn't compile for STM32F4
* :github:`46733` - ipc_rpmsg_static_vrings creates unaligned TX virtqueues
* :github:`46728` - mcumgr: rt1060: upload an image over the shell does not work
* :github:`46725` - stm32: QSPI flash driver have a broken priority configuration
* :github:`46721` - HAL module request: hal_renesas
* :github:`46706` - add missing checks for segment number
* :github:`46705` - Check buffer size in rx
* :github:`46698` - sm351 driver faults when using global thread
* :github:`46697` - Missed interrupts in NXP RT685 GPIO driver
* :github:`46694` - Bluetooth: controller: LLCP: missing release of tx nodes on disconnect when tx data paused
* :github:`46692` - Bluetooth: controller: LLCP: reduced throughput
* :github:`46689` - Missing handling of DISK_IOCTL_CTRL_SYNC in sdmmc_ioctl
* :github:`46684` - ethernet: w5500: driver will be stack overflowed when reading the invalid(corrupt) packet length
* :github:`46656` - Scheduling timing issue
* :github:`46650` - qemu_x86: shell does not work with tip of main
* :github:`46645` - NRFX samples use deprecated API
* :github:`46641` - tests : kernel: context test_kernel_cpu_idle fails on nucleo_f091 board
* :github:`46635` - tests: subsys: modbus: testcase hang up when running by twister
* :github:`46632` - Intel CAVS: Assertion failures in tests/boards/intel_adsp/hda
* :github:`46626` - USB CDC ACM Sample Application build fail with stm32_mini_dev_blue board
* :github:`46623` - Promote user "tari" to traige permission level
* :github:`46621` - drivers: i2c: Infinite recursion in driver unregister function
* :github:`46602` - BLE paring/connection issue on Windows (Zephyr 3.1.0)
* :github:`46594` - openthread net_mgmt_event_callback expects event info.
* :github:`46582` - LE Audio: PACS notify warns about failure when not connected
* :github:`46580` - Suggestion for additional configuration of ``twister --coverage`` gcovr formats
* :github:`46573` - raspberry pi pico always in mass storage mode
* :github:`46570` - Compiler warning when enabling userspace, sockets and speed optimization
* :github:`46558` - Bluetooth: Controller: Crash on bt_le_adv_start() when using CONFIG_BT_CTLR_ADVANCED_FEATURES
* :github:`46556` - Kconfig search webpage no longer shows all flags
* :github:`46555` - test: samples/drivers/adc twister result wrong
* :github:`46541` - Duplicate IDs used for different Systemview trace events
* :github:`46525` - PWM of it8xxx2
* :github:`46521` - '__device_dts_ord___BUS_ORD' undeclared here (not in a function); did you mean '__device_dts_ord_94'?
* :github:`46519` - STM32F4 CAN2 peripheral not working
* :github:`46510` - bluetooth: controller: llcp: set refactored LLCP as default
* :github:`46500` - Removal of logging v1
* :github:`46497` - Modbus: Add support for FC03 without floating-point extension as client
* :github:`46493` - Ethernet W5500 driver fails initialization with latest change - revert needed
* :github:`46483` - Update RISC-V ISA configs
* :github:`46478` - mimxrt1050_evk_qspi freeze when erasing flash
* :github:`46474` - LE Audio: Add seq_num and TS to bt_audio_send
* :github:`46470` - twister : retry failed parameter is not valid
* :github:`46464` - frdm_k64f : sudden failure to flash program
* :github:`46459` - Test framework incorrectly uses c++ keyword ``this``
* :github:`46453` - nRF52840 PWM with pinctrl - Unable to build samples/basic/blinky_pwm
* :github:`46446` - lvgl: Using sw_rotate with SSD1306 shield causes memory fault
* :github:`46444` - Proposal to integrate Cadence QSPI driver from Trusted Firmware-A
* :github:`46434` - ESP32-C3 UART1 broken since introduction of pinctrl
* :github:`46426` - Intel CAVS: Assertion failures on tests/boards/intel_adsp/smoke
* :github:`46422` - SDK version 14.2 increases image size significantly
* :github:`46414` - mcuboot: rt1060: confirmed image causes usage fault
* :github:`46413` - No multicast reception on IMX1064
* :github:`46410` - Add devicetree binding for ``zephyr,sdmmc-disk``
* :github:`46400` - STM32WB BLE HCI interface problem.
* :github:`46398` - ``mem_protect/mem_map`` is failing on ``qemu_x86_tiny`` when userspace is enabled
* :github:`46383` - fatal error: sys/cbprintf_enums.h: No such file or directory
* :github:`46382` - twister -x / --extra-args escaping quotes issue with CONFIG_COMPILER_OPTIONS
* :github:`46378` - CONFIG_SYS_CLOCK_TICKS_PER_SEC affects app code speed with tickless kernel
* :github:`46372` - Intel-ADSP: sporadic core boot
* :github:`46369` - LE Audio: Bidirectional stream is not created
* :github:`46368` - twister  : frdm_k64f ：the test case tests/subsys/logging/log_switch_format/logger.syst.v2.immediate/ blocks
* :github:`46366` - test_thread_stats_usage fail on arm64 fvp
* :github:`46363` - Initial Setup: Ubuntu 20.04: ensurepip is not available
* :github:`46355` - Sample wifi_station not building for esp32: No SOURCES given to Zephyr library: drivers__ethernet
* :github:`46350` - net: tcp: When the first FIN message is lost, the connection does not properly close
* :github:`46347` - MCUMGR_SMP_BT: system workqueue blocked during execution of shell commands
* :github:`46346` - LE Audio: Fatal crash when sending Audio data
* :github:`46345` - get_maintainer.py incorrectly invoked by Github?
* :github:`46341` - Zephyr scheduler lock: add selective locking up to a given priority ceiling
* :github:`46335` - For ESP32, initialization of static object during declaration with derived class type doesn't work.
* :github:`46326` - Async UART for STM32 U5 support
* :github:`46325` - ESP32 strcmp error while enable MCUBOOT and NEWLIB_LIBC
* :github:`46324` - it8xxx2_evb: tests/kernel/sched/schedule_api fail due to k_sleep(K_MSEC(100)) not correct
* :github:`46322` - Time units in shtcx sensor
* :github:`46312` - sample: bluetooth: ipsp - TCP not running over IPSP
* :github:`46286` - python-devicetree tox run fails
* :github:`46285` - nrf_qspi_nor: Inconsistent state of HOLD and WP for QSPI command execution causes hang on startup for some flash chips
* :github:`46284` - ring buffer in item mode crashes
* :github:`46277` - IMX8MM: Running fail a zephyr sample in the imx8mm
* :github:`46269` - docs: include/zephyr/net/socket.h is not documented anywhere
* :github:`46267` - docs: include/zephyr/net/http_client.h is not documented anywhere
* :github:`46266` - zephyr,sdmmc-disk compatible lacks a binding
* :github:`46263` - Regulator Control
* :github:`46255` - imxrt1010 wrong device tree addresses
* :github:`46235` - subsystem: Bluetooth LLL: ASSERTION FAIL [!link->next]
* :github:`46234` - samples: lsm6dso: prints incorrect anglular velocity units
* :github:`46208` - it8xxx2_evb: tests/kernel/sleep failed, elapsed_ms = 2125
* :github:`46206` - it8xxx2_evb: tests/kernel/fatal/exception/ assertion failed -- "thread was not aborted"
* :github:`46199` - LIS2DW12 I2C driver uses invalid write command
* :github:`46186` - ISO Broadcaster fails silently on unsupported RTN/SDU_Interval combination
* :github:`46183` - LE Audio: Broadcast sink stop sending syncable once synced
* :github:`46180` - Add GitHub app for Googler notifications
* :github:`46173` - nRF UART callback is not passed correct index via evt->data.rx.offset sometimes
* :github:`46170` - ipc_service: open-amp backend may never leave
* :github:`46167` - esp32: Unable to select GPIO for PWM LED driver channel
* :github:`46164` - scripts: release: ci checks for issues associated with backport prs
* :github:`46158` - frdm_k64f：failed to run test case tests/subsys/modbus/modbus.rtu/server_setup_low_none
* :github:`46157` - ACRN: some cases still failed because of the log missing
* :github:`46124` - stm32g071 ADC drivers apply errata during sampling config
* :github:`46117` - Kernel events can’t be manipulated without race conditions
* :github:`46100` - lib: posix: support for perror()
* :github:`46099` - libc: minimal: add strerror() function
* :github:`46075` - BT HCI Raw on STM32WB55RG
* :github:`46072` - subsys/hawkBit: Debug log error in hawkbit example "CONFIG_LOG_STRDUP_MAX_STRING"
* :github:`46066` - TF-M: Unable to trigger NMI interrupt from non-secure
* :github:`46065` - Bluetooth: controller: llcp: verify that refactored LLCP is used in EDTT
* :github:`46049` - Usage faults on semaphore usage in driver (stm32l1)
* :github:`46048` - Use dts memory-region property to retrieve memory region used by driver
* :github:`46008` - stm32h7: gptp sample does not work at all
* :github:`45993` - Matter(CHIP) support
* :github:`45955` - stm32h7 i2s support
* :github:`45953` - modem: simcom-sim7080: sendmsg() should result in single outgoing datagram
* :github:`45951` - modem: ublox-sara-r4: outgoing datagrams are truncated if they do not fit MTU
* :github:`45938` - Unable to combine USB CDC-ACM and Modbus Serial due to dependecy on uart_configure().
* :github:`45934` - ipc_service: nocopy tx buffer allocation works unexpectedly with RPMSG backend
* :github:`45933` - webusb sample code linking error for esp32 board
* :github:`45929` - up_squared：failed to run test case tests/posix/common
* :github:`45914` - test: tests/kernel/usage/thread_runtime_stats/ test fail
* :github:`45866` - drivers/entropy: stm32: non-compliant RNG configuration on some MCUs
* :github:`45848` - tests: console harness: inaccuracy testcases report
* :github:`45846` - New ZTEST API for noisily skipping a test based dependency failures
* :github:`45845` - tests: The failure test case number increase significantly in CMSIS DSP tests on ARM boards.
* :github:`45844` - Not all bytes are downloaded with HTTP request
* :github:`45842` - drivers: modem: uart_mux errors after second call to gsm_ppp_start
* :github:`45827` - bluetooth: bluetooth host: Adding the same device to resolving list
* :github:`45807` - CivetWeb doesn't build for CC3232SF
* :github:`45802` - Some tests reported as PASSED (device) but they were only build
* :github:`45774` - drivers: gpio: pca9555: Driver is writing to output port despite all pins been configured as input
* :github:`45760` - Running twister on new board files
* :github:`45741` - LE Audio: Allow unique ``bt_codec_qos`` for each unicast stream
* :github:`45678` - Lorawan: Devnonce has already been used
* :github:`45675` - testing.ztest.customized_output: mismatch twister results in json/xml file
* :github:`45666` - Building samples about BLE audio with nrf5340dk does not work
* :github:`45658` - Build failure: civetweb/http_server with target blackpill_f411ce and CONFIG_DEBUG=y
* :github:`45647` - test: drivers: counter: Test passes even when no instances are found
* :github:`45613` - LE Audio: Setting ISO chan path and CC from BAP
* :github:`45611` - GD32 build failure: CAN_MODE_NORMAL is redefined
* :github:`45596` - samples: Code relocation nocopy sample has some unusual failure on nrf5340dk
* :github:`45581` - samples: usb: mass: Sample.usb.mass_flash_fatfs fails on non-secure nrf5340dk
* :github:`45564` - Zephyr does not boot with CONFIG_PM=y
* :github:`45558` - LE Audio: Update MICP API with new naming scheme
* :github:`45532` - uart_msp432p4xx_poll_in() seems to be a blocking function
* :github:`45509` - ipc: ipc_icmsg: Can silently drop buffer if message is too big
* :github:`45441` - SPI NOR driver assume all SPI controller HW is implemnted in an identical way
* :github:`45374` - Creating the unicast group before both ISO connections have been configured might cause issue
* :github:`45349` - ESP32: fails to chain-load sample/board/esp32/wifi_station from MCUboot
* :github:`45315` - drivers: timer: nrf_rtc_timer: NRF boards take a long time to boot application in CONFIG_TICKLESS_KERNEL=n mode after OTA update
* :github:`45304` - drivers: can: CAN interfaces are brought up with default bitrate at boot, causing error frames if bus bitrate differs
* :github:`45270` - CMake - TEST_BIG_ENDIAN
* :github:`45234` - stm32: Allow multiple GPIOs to trigger an interrupt
* :github:`45222` - drivers: peci: user space handlers not building correctly
* :github:`45169` - rcar_h3ulcb: failed to run test case tests/drivers/can/api
* :github:`45168` - rcar_h3ulcb: failed to run test case tests/drivers/can/timing
* :github:`45157` - cmake: Use of -ffreestanding disables many useful optimizations and compiler warnings
* :github:`45130` - LE Audio: Allow CSIS set sizes of 1
* :github:`45117` - drivers: clock_control: clock_control_nrf
* :github:`45114` - Sample net/sockets/echo not working with disco_l475_iot1
* :github:`45105` - ACRN: failed to run testcase tests/kernel/fifo/fifo_timeout/
* :github:`45039` - Bluetooth: Controller: Broadcast multiple BIS (broadcast ISO streams)
* :github:`45021` - Configurable SDMMC bus width for STM32
* :github:`45012` - sam_e70b_xplained: failed to run test case tests/drivers/can/timing/drivers.can.timing
* :github:`45009` - twister: many tests failed with "mismatch error" after met a SerialException.
* :github:`45008` - esp32: i2c_read() error was returned successfully at the bus nack
* :github:`44998` - SMP shell exec command causes BLE stack breakdown if buffer size is too small to hold response
* :github:`44996` - logging: transient strings are no longer duplicated correctly
* :github:`44980` - ws2812_spi allow setting CPHA in overlay
* :github:`44944` - LE Audio: Add ISO part to broadcast audio bsim tests
* :github:`44925` - intel_adsp_cavs25: multiple tests failed after running tests/boards/intel_adsp
* :github:`44898` - mgmt/mcumgr: Fragmentation of responses may cause mcumgr to drop successfully processed response
* :github:`44861` - WiFi support for STM32 boards
* :github:`44830` - Unable to set compiler warnings on app exclusively
* :github:`44824` - mgmt/mcumgr/lib: Use slist in group registration to unify with Zephyr code
* :github:`44725` - drivers: can: stm32: can_add_rx_filter() does not respect CONFIG_CAN_MAX_FILTER
* :github:`44622` - Microbit v2 board dts file for lsm303agr int line
* :github:`44579` - MCC: Discovery cannot complete with success
* :github:`44573` - Do we have complete RNDIS stack available for STM32 controller in zephyr ?
* :github:`44466` - Zephyr misses strict aliasing disabling
* :github:`44455` - LE Audio: Remove ``struct bt_codec *codec`` parameter from ``bt_audio_broadcast_sink_sync``
* :github:`44403` - MPU fault and ``CONFIG_CMAKE_LINKER_GENERATOR``
* :github:`44400` - LE Audio: Unicast server stream control
* :github:`44340` - Bluetooth: controller: Handle parallel (across connections) CU/CPRs in refactored LLCP
* :github:`44338` - Intel CAVS: tests/subsys/logging/log_immediate/ failed due to non-intact log
* :github:`44324` - Compile error in byteorder.h
* :github:`44228` - drivers: modem: bg9x: bug on cmd AT+QICSGP
* :github:`44219` - mgmt/mcumgr/lib: Incorrect processing of img_mgmt_impl_write_image_data leaves mcumgr in broken state in case of error
* :github:`44214` - mgmt/mcumgr/lib: Parasitic use of CONFIG_HEAP_MEM_POOL_SIZE in image management
* :github:`44143` - Adding picolibc as a module
* :github:`44071` - LE Audio: Upstream remaining parts of topic branch
* :github:`44070` - west spdx TypeError: 'NoneType' object is not iterable
* :github:`44059` - Hearing Aid Role
* :github:`44058` - Hearing Access Service API
* :github:`44005` - add strtoll and strtoull to libc minimal
* :github:`43940` - Support for CH32V307 devices
* :github:`43933` - llvm: twister: multiple errors with set but unused variables
* :github:`43928` - pm: going to PM_STATE_SOFT_OFF in pm_policy_next_state causes assert in some cases
* :github:`43913` - LE Audio: Callbacks as singletons or lists?
* :github:`43910` - civetweb/http_server - DEBUG_OPTIMIZATIONS enabled
* :github:`43887` - SystemView tracing with STM32L0x fails to compile
* :github:`43859` - Bluetooth: ISO: Add sequence number and timestamp to bt_iso_chan_send
* :github:`43828` - Intel CAVS: multiple tests under tests/boards/intel_adsp/smoke are failing
* :github:`43811` - ble: gatt: db_hash_work runs for too long and makes serial communication fail
* :github:`43788` - LE Audio: Broadcast Sink shall instantiate PACS
* :github:`43767` - LE Audio: Broadcast sink/source use list of streams instead of array
* :github:`43718` - Bluetooth: bt_conn: Unable to allocate buffer within timeout
* :github:`43655` - esp32c3: Connection fail loop
* :github:`43646` - mgmt/mcumgr/lib: OS taskstat may give shorter list than expected
* :github:`43515` - reel_board: failed to run tests/kernel/workq/work randomly
* :github:`43450` - twister: platform names from quarantine file are not verified
* :github:`43435` - Bluetooth: controller: llcp: failing EBQ and Harmony tests
* :github:`43335` - Automatic Automated Backports?
* :github:`43246` - Bluetooth: Host: Deadlock with Mesh and Ext Adv on native_posix
* :github:`43245` - GitHub settings: Update topics
* :github:`43202` - LE Audio: Avoid hardcoding context type for LC3 macros
* :github:`43135` - stm32: uart: Support for wakeup from stop
* :github:`43130` - STM32WL ADC idles / doesn't work
* :github:`43124` - twister: Create pytest-based PoC for twister v2
* :github:`43115` - Data corruption in STM32 SPI driver in Slave Mode
* :github:`43103` - LwM2M library should use JSON library for parsing
* :github:`42890` - Bluetooth: Controller: Periodic Advertising: AD data fragmentation
* :github:`42889` - Bluetooth: Controller: Extended Advertising: AD data fragmentation
* :github:`42885` - Bluetooth: Controller: Group auxiliary PDU transmissions
* :github:`42842` - BBRAM API is missing a documentation reference page
* :github:`42700` - Support module.yml in zephyr repo
* :github:`42684` - New LLCP handling of Preferred PHY (default tx/rx phy) needs a review
* :github:`42649` - bt_ots_client_unregister()
* :github:`42629` - stm32g0: Device hang/hard fault with AT45 + ``CONFIG_PM_DEVICE``
* :github:`42574` - i2c: No support for bus recovery imx.rt and or timeout on bus busy
* :github:`42522` - LE Audio: Immediate alert service
* :github:`42472` - ztest: add support for assumptions
* :github:`42450` - cmake: dts.cmake: Add Board overlays before shields
* :github:`42420` - mgmt/mcumgr/lib: Async image erase command with status check
* :github:`42356` - Repo size - board documentation - large PNGs
* :github:`42341` - LE Audio: CSIS Ordered Access procedure use rank
* :github:`42324` - mgmt/mcumgr/lib: Move to direct use of net_buf
* :github:`42277` - Zephyr Docs on West need to be updated to include SBOM generation
* :github:`42208` - tests/subsys/logging/log_api/ fails qemu_leon3 if ptr_in_rodata() is enabled for SPARC
* :github:`42197` - Bluetooth: Controller: llcp: No disconnect if remote does not response for initiated control procedure
* :github:`42134` - TLS handshake error using DTLS on updatehub
* :github:`42102` - doc: searches for sys_reboot() are inconsistent
* :github:`41954` - Bluetooth: Controller: BIS: Event timing calculations
* :github:`41922` - Bluetooth: Controller: ISOAL: TX: Implement SDU Fragmentation into Unframed PDUs
* :github:`41880` - Strict test ordering in new ztest API
* :github:`41776` - LLVM: support -fuse-ld=lld linker on qemu_x86.
* :github:`41772` - stm32: G0: adc: Add support for VBAT internal channel
* :github:`41711` - LE Audio: CAP Acceptor Implementation
* :github:`41355` - Bluetooth: API to determine if notification over EATT is possible
* :github:`41286` - Bluetooth SDP: When the SDP attribute length is greater than SDP_MTU, the attribute is discarded
* :github:`41281` - Style Requirements Seem to Be Inconsistent with Uncrustify Configuration
* :github:`41224` - LE Audio: Telephony and Media Audio Profile (TMAP)
* :github:`41217` - LE Audio: Support for a minimum CCP client
* :github:`41214` - LE Audio: Add public API to CCP/TBS
* :github:`41211` - LE Audio: BASS support for multiple connection
* :github:`41208` - LE Audio: BASS use multi-characteristic macro for receive states
* :github:`41205` - OTS: Debug metadata output
* :github:`41204` - LE Audio: BASS read long
* :github:`41203` - LE Audio: BASS write long
* :github:`41199` - LE Audio: Media API with one call per command, rather than sending opcodes
* :github:`41197` - LE Audio: Use BT_MEDIA_PROXY values instead of BT_MCS
* :github:`41193` - LE Audio: Couple IN audio stream with an OUT audio stream
* :github:`40933` - mgmt/mcumgr/lib: Divide the lib Kconfig into sub-Kconfigs dedicated to specific mgmt cmd group
* :github:`40893` - mgmt/mcumg/lib: Encode shell command execution result in additional field of response
* :github:`40855` - mgmt/mcumgr/lib: Add optional image/slot parameter to "image erase" mcumgr request command
* :github:`40854` - mgmt/mcumgr/lib: Extend taskstat response with "runtime" statistics
* :github:`40827` - Tensorflow example not working in zephyr v2.6
* :github:`40664` - Bluetooth: GATT: EATT:  Multiple notify feature not utilize new PDU fully
* :github:`40444` - Late C++ constructor initialization on native posix boards
* :github:`40389` - Inconsistent use of CMake / environment variables
* :github:`40309` - Multi-image support for MCUboot
* :github:`40146` - On the status of DT-defined regions and MPU
* :github:`39888` - STM32L4: usb-hid: regression in hal 1.17.0
* :github:`39491` - Add a hal module for Nuclei RISC-V core (NMSIS)
* :github:`39486` - Improve emulator APIs for testing
* :github:`39347` - Static object constructors do not execute on the NATIVE_POSIX_64 target
* :github:`39153` - Improve ztest test suites (setup/teardown/before/after + OOD)
* :github:`39037` - CivetWeb samples fail to build with CONFIG_NEWLIB_LIBC
* :github:`38727` - [RFC] Add hal_gigadevice to support GigaDevice SoC Vendor
* :github:`38654` - drivers: modem: bg9x: Has no means to update size of received packet.
* :github:`38613` - BLE connection parameters updated with inconsistent values
* :github:`38544` - drivers: wifi: esWIFI: Regression due to 35815
* :github:`38494` - Flooded logs when using CDC_ACM as back-end
* :github:`38336` - Bluetooth: Host: separate authentication callbacks for each identity
* :github:`37883` - Mesh Bluetooth Sample not working with P-NUCLEO-WB55RG
* :github:`37704` - hello world doesn't work on qemu_arc_em when CONFIG_ISR_STACK_SIZE=1048510
* :github:`37212` - improve docs with diagram for boot flow of ACRN on x86 ehl_crb
* :github:`36819` - qemu_leon3 samples/subsys/portability/cmsis_rtos_v2 samples failing
* :github:`36644` - Toolchain C++ headers can be included when libstdc++ is not selected
* :github:`36476` - Add intel HAL support
* :github:`36084` - Arduino Nano 33 BLE: USB gets disconnected after flashing
* :github:`36026` - wolfssl / wolfcrypt
* :github:`35931` - Bluetooth: controller: Assertion in ull_master.c
* :github:`35816` - timer: STM32: using hw timer for counting and interrupt callback
* :github:`35778` - pwm : STM32: Timer handling interrupt callback handling
* :github:`35762` - SAMPLES: shell_module gives no console output on qemu_leon3
* :github:`35719` - WiFi Management expects networking to be offloaded
* :github:`35512` - OpenThread can't find TRNG driver on nRF5340
* :github:`34927` - Add error check to twister if set of platforms between platform_allow and integration_platforms is empty
* :github:`34600` - Bluetooth: L2CAP: Deadlock when there are no free buffers while transmitting on multiple channels
* :github:`34571` - Twister mark successfully passed tests as failed
* :github:`34438` - CivetWeb sample only supports HTTP, Zephyr lacks TLS support
* :github:`34413` - Improve __used attribute to actually keep requested function/variable
* :github:`34227` - Use compile time resolved device bindings in flash map, when possible
* :github:`34226` - Compile error when building civetweb samples for posix_native
* :github:`34190` - Newbie: Simple C++ List App Builds for QEMU but not Native Posix Emulation
* :github:`33876` - Lora sender sample build error for esp32
* :github:`33865` - Bluetooth: iso_server security is not applied
* :github:`33725` - Modularisation and Restructuring of Documentation
* :github:`33627` - Provide alternative nvs_init that will take const struct ``*device`` instead of device name
* :github:`33520` - Update module civetweb (bug fixes and increased stack size requirement)
* :github:`33339` - API/functions to get remaining free heap size
* :github:`33185` - TCP traffic with IPSP sample not working on 96Boards Nitrogen
* :github:`33015` - spi_nor driver: SPI_NOR_IDLE_IN_DPD breaks SPI_NOR_SFDP_RUNTIME
* :github:`32665` - Bluetooth: controller: inclusion of vendor data type and function overrides provided by vendor LLL
* :github:`32608` - Revert practice of removing devicetree labels
* :github:`32516` - RFC: 1-Wire driver
* :github:`32197` - arch_switch() on SPARC isn't quite right
* :github:`31290` - dts: arm: st: standardize pwm default property st,prescaler to 0
* :github:`31208` - Bluetooth Mesh CCM Hardware Acceleration
* :github:`31175` - STM32F1 RTC
* :github:`30694` - Some boards enable non-minimal peripherals by default
* :github:`30505` - Rework pipe_api test for coverage
* :github:`30365` - TCP2 does not implement Nagle algorithm
* :github:`29866` - Drivers/PCIE: read/write 8/16/32 bit word to an endpoint's configuration space
* :github:`28145` - nRF52840 Dongle cannot scan LE Coded PHY devices
* :github:`27997` - Errors in copy paste lengthy script into Shell Console
* :github:`27975` - [Thingy52_nrf52832 board] - Working with other led than led0
* :github:`27735` - Enable DT-based sanity-check test filtering
* :github:`27585` - investigate using the interrupt stack for the idle thread
* :github:`27511` - coverage: qemu platforms: sanitycheck generates many ``unexpected eof`` failures when enable coverage
* :github:`27033` - Update terminology related to I2C
* :github:`26938` - gpio: api to query pin configuration
* :github:`26179` - devicetree: Missing support of unquoted strings
* :github:`25442` - Does Zephyr support USB host mode ?
* :github:`25382` - devicetree: Add ranges property support for PCIe node
* :github:`24457` - Common Trace Format - Failed to produce correct trace output
* :github:`24373` - NULL-pointer dereferencing in GATT when master connection fails
* :github:`23893` - server to client ble coms: two characteristics with notifications failing to notify the right characteristics at the client
* :github:`23302` - Poor TCP performance
* :github:`23165` - macOS setup fails to build for lack of "elftools" Python package
* :github:`23111` - drivers:usb:device:sam0:  Descriptor tables are filled with zeros in attach()
* :github:`23032` - Need help to enable Sub-GHz for ieee802154_cc13xx_cc26xx
* :github:`22208` - gpio: clean up debounce configuration
* :github:`22079` - Add reception channel information to advertise_report
* :github:`21980` - Doesn't Install on Raspberry Pi
* :github:`21234` - drivers: usb_dc_sam0: usb detach and reattach does not work
* :github:`19979` - Implement Cortex-R floating-point support
* :github:`19244` - BLE throughput of DFU by Mcumgr is too slow
* :github:`18551` - address-of-temporary idiom not allowed in C++
* :github:`16683` - [RFC] Missing parts of libc required for CivetWeb
* :github:`16674` - Checkpatch generates incorrect warning for __DEPRECATED_MACRO
* :github:`15591` - Add STM32 LCD-TFT Display Controller (LTDC) Driver
* :github:`15429` - shields: improve cmake to define/extract pinmux and defconfig info
* :github:`15256` - Link Layer Control Procedure overhaul
* :github:`15214` - Enforce correct compilers in boilerplate.cmake
* :github:`14527` - [wip] Generic support for out-of-tree drivers
* :github:`14068` - Allow better control on SPI pin settings
* :github:`13662` - samples/subsys/usb/cdc_acm: Stuck at "Wait for DTR"
* :github:`13639` - Use dirsync for doxygen directory syncing
* :github:`13519` - BLE Split Link Layer Improvements
* :github:`13196` - LwM2M: support Access Control objects (object id 2)
* :github:`12272` - SD/MMC interface support
* :github:`12191` - Nested interrupt test has very poor coverage
* :github:`11975` - Logging subsystem doesn't work with prink char_out functions
* :github:`11918` - Runtime pin configuration
* :github:`11636` - Generic GPIO reset driver
* :github:`10938` - Standardize labels (string device names) used for device binding
* :github:`10516` - Migrate drivers to Devicetree
* :github:`10512` - Console, logger, shell architecure
* :github:`8945` - Explore baselibc as a replacement for minimal libc
* :github:`8497` - Need a "monitor" spin-for-ISR API
* :github:`8496` - Need a "lock" wrapper around k_sem
* :github:`8139` - Driver for BMA400 accelerometer
* :github:`7876` - net: tcp: Zero Window Probes are not supported/handled properly
* :github:`7516` - Support binary blobs / libraries and glue code in vanilla upstream Zephyr
* :github:`6498` - Kernel high-resolution timer support
* :github:`5408` - Improve docs & samples on device tree overlay
* :github:`1392` - No module named 'elftools'
* :github:`2170` - I2C fail to read GY2561 sensor when GY2561 & GY271 sensor are attached to I2C bus.
