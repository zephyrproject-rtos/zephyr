:orphan:

.. _zephyr_3.2:

Zephyr 3.2.0 (Working Draft)
############################

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2022-2993: Under embargo until 2022-11-03

* CVE-2022-2741: Under embargo until 2022-10-14


Known issues
************

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

  * Added support of non-multithreading mode for all UP ARC targets
  * Added extra compile-time checks of :kconfig:option:`CONFIG_ISR_STACK_SIZE`
    and :kconfig:option:`CONFIG_ARC_EXCEPTION_STACK_SIZE` value
  * Added support of generation symbol file for ARC MWDT toolchain variant
  * Added ARC MWDT toolchain version check
  * Added support for GCC mcpu option tuning for ARC targets on SoC level
  * Switched ARCv3 64bit targets for usage of new linker output format value
  * Added ARCv3 64bit accumulator reg save / restore, cleanup it for ARCv3
    32bit targets
  * Fixed SMP race in ASM ARC interrupt handling code

* ARM

  * Improved HardFault handling on Cortex-M.
  * Enabled automatic placement of the IRQ vector table.
  * Enabled S2RAM for Cortex-M, hooking up the provided API functions.
  * Added icache and dcache maintenance functions, and switched to the new
    Kconfig symbols (:kconfig:option:`CPU_HAS_DCACHE` and
    :kconfig:option:`CPU_HAS_ICACHE`).
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
  * TF-A (TrustedFirmware-A) is now shipped as module

* Posix

* RISC-V

* x86

* Xtensa

  * Macros ``RSR`` and ``WSR`` have been renamed to :c:macro:`XTENSA_RSR`
    and :c:macro:`XTENSA_WSR` to give them proper namespace.
  * Fixed a rounding error in timing function when coverting from cycles
    to nanoseconds.
  * Fixed the calculation of average "cycles to nanoseconds" to actually
    return nanoseconds instead of cycles.

Bluetooth
*********

* Audio

  * Implemented central security establishment when required
  * Added additional security level options to the connection call
  * Switched the unicast client and server to bidirectional CIS if available
  * Added a new RSI advertising callback for CSIS
  * Added multiple improvements to context handling, including public functions
    to get contexts
  * Added ordered access procedure for the CSIS client, as well as storing
    active members by rank
  * Added support for Write Preset Name in HAS
  * Added support for using PACS for the broadcast sink role
  * Cleaned up the MICP implementation, including renaming several structures
    and functions
  * Implemented the CAP Acceptor role
  * Added ASCS Metadata verification support
  * Started exposing broadcast sink advertising data to the application
  * Added support for unicast server start, reconfigure, release, disable and
    metadata
  * Added support for multi-CIS
  * Implemented HAS client support for preset switching
  * Added support for setting vendor-specific non-HCI data paths for audio
    streams

* Direction Finding

  * Added support for selectable IQ samples conversion to 8-bit
  * Added support for VS IQ sample reports in ``int16_t`` format

* Host

  * Added support for LE Secure Connections permission checking
  * Added support for Multiple Variable Length Read procedure without EATT
  * Added a new callback :c:func:`rpa_expired` in the struct
    :c:struct:`bt_le_ext_adv_cb` to enable synchronization of the advertising
    payload updates with the Resolvable Private Address (RPA) rotations when
    the :kconfig:option:`CONFIG_BT_PRIVACY` is enabled
  * Added a new :c:func:`bt_le_set_rpa_timeout()` API call to dynamically change
    the the Resolvable Private Address (RPA) timeout when the
    :kconfig:option:`CONFIG_BT_RPA_TIMEOUT_DYNAMIC` is enabled
  * Added :c:func:`bt_conn_auth_cb_overlay` to overlay authentication callbacks
    for a Bluetooth LE connection
  * Removed ``CONFIG_BT_HCI_ECC_STACK_SIZE``. A new Bluetooth long workqueue
    (:kconfig:option:`CONFIG_BT_LONG_WQ`) is used for processing ECC commands
    instead of the former dedicated thread
  * :c:func:`bt_conn_get_security` and :c:func:`bt_conn_enc_key_size` now take
    a ``const struct bt_conn*`` argument
  * The handling of GATT multiple notifications has been rewritten, and is now
    only to be used as a low-level API
  * Added support for GATT CCCs in arbitrary locations as a client
  * Extended the ``bt_conn_info`` structure with security information
  * Added a new :kconfig:option:`CONFIG_BT_PRIVACY_RANDOMIZE_IR` that prevents
    the Host from using Controller-provided identity roots
  * Added support for GATT over EATT
  * Implemented the Immediate Alert Client

* Mesh

  * Added support for selectable RPL backends
  * Changed the way segmented messages are sent, avoiding bulk transmission
  * Added an async config client API
  * Added model publish support to the Health Client
  * Moved relayed messages to a separate buffer pool
  * Reduced delay of sending segment acknowledge message. Set
    :kconfig:option:`CONFIG_BT_MESH_SEG_ACK_PER_SEGMENT_TIMEOUT` to 100 to get
    the previous timing.
  * Restructured shell commands

* Controller

  * Made the new LLCP implementation the default one. Enable
    :kconfig:option:`CONFIG_BT_LL_SW_LLCP_LEGACY` to revert back to the legacy
    implementation. :kconfig:option:`CONFIG_BT_LL_SW_LLCP_LEGACY` is marked
    deprecated in favor of the new :kconfig:option:`CONFIG_BT_LL_SW_LLCP`, which
    is the default now
  * Marked Extended Advertising as stable, no longer experimental
  * Added deinit() infrastructure in order to properly support disabling
    Bluetooth support, including the controller
  * Implemented the Peripheral CIS Create procedure
  * Implemented the CIS Terminate procedure
  * Added support for Periodic Advertising ADI
  * Implemented support for Extended Scan Response Data fragment operations
  * Enable back-to-back PDU chaining for AD data
  * Added a new :kconfig:option:`CONFIG_BT_CTLR_SYNC_PERIODIC_SKIP_ON_SCAN_AUX`
    for allowing periodic sync event skipping
  * Added a new :kconfig:option:`CONFIG_BT_CTLR_SCAN_AUX_SYNC_RESERVE_MIN` for
    minimal time resevation
  * Implemented ISO Test Mode HCI commands
  * Added support for multiple BIS sync selection within a BIG
  * Implement flushing pending ISO TX PDUs when a BIG event is terminated
  * Added a new :kconfig:option:`CONFIG_BT_CTLR_ADV_DATA_CHAIN` to enable
    experimental Advertising Data chaining support

* HCI Driver

  * Added a new Telink B91 HCI driver

Boards & SoC Support
********************

* Added support for these SoC series:

  * Atmel SAML21, SAMR34, SAMR35.
  * GigaDevice GD32E50X
  * GigaDevice GD32F470
  * NXP i.MX8MN, LPC55S36, LPC51U68
  * renesas_smartbond da1469x SoC series

* Made these changes in other SoC series:

  * gigadevice: Enable SEGGER RTT

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

* Added support for these following shields:

  * ARCELI W5500 ETH
  * MAX7219 LED display driver shield
  * Panasonic Grid-EYE (AMG88xx)

Build system and infrastructure
*******************************

* Introduced sysbuild, a new higher-level build system layer that enables
  combining multiple build systems together. It can be used to generate multiple
  images from a single build system invocation while maintaining a link between
  those different applications/images via a shared Kconfig layer
* Introduced support for binary blobs in west, via a new ``west blobs`` command
  that allows users to list, fetch and delete binary blobs from their
  filesystem. Vendors can thus now integrate binary blobs, be it images or
  libraries, with upstream Zephyr
* Removed deprecated ``GCCARMEMB_TOOLCHAIN_PATH`` setting

Drivers and Sensors
*******************

* ADC

  * STM32: Now supports Vbat monitoring channel and STM32U5 series.
  * Added driver for GigaDevice GD32 SoCs

* Audio

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
  * Added driver for GigaDevice GD32 SoCs (peripheral clocks configuration only)

* Coredump

* Counter

  * STM32: RTC : Now supports STM32U5 and STM32F1 series.
  * STM32: Timer : Now supports STM32L4 series.
  * Added counter support using CTimer for NXP MIMXRT595.
  * ESP32: Added support to Pulse Counter Mode (PCNT)

* Crypto

* DAC

* DAI

* Display

* Disk

  * Added support for DMA transfers when using STM32 SD host controller
  * Added support for SD host controller present on STM32L5X family

* DMA

  * STM32: Now supports stm32u5 series.
  * cAVS drivers renamed with the broader Intel ADSP naming
  * Kconfig depends on improvements with device tree statuses
  * Added driver for GigaDevice GD32 SoCs
  * Added DMA support for NXP MIMXRT595

* EEPROM

  * Added Microchip XEC (MEC172x) on-chip EEPROM driver. See the
    :dtcompatible:`microchip,xec-eeprom` devicetree binding for more information.

* Entropy

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

  * Added GPIO driver for Renesas Smartbond platform

* HWINFO

* I2C

  * Terminology updated to latest i2c specification removing master/slave
    terminology and replacing with controller/target terminology.
  * Asynchronous APIs added for requesting i2c transactions without
    waiting for the completion of them.
  * Added NXP LPI2C driver asynchronous i2c implementation with sample
    showing usage with a FRDM-K64F board.
  * STM32: support for second target address was added.
  * Kconfig depends on improvements with device tree statuses
  * Improved ITE I2C support with FIFO and command queue mode
  * Improve gd32 driver stability (remove repeated START, use STOP + START conditions instead)
  * Fixed gd32 driver incorrect Fast-mode config
  * Add bus recovery support to the NXP MCUX LPI2C driver.
  * Enable I2C support on NXP MIMXRT595 EVK.

* I2S

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

  * Enable the touch panel on the NXP MIMXRT1170 EVK.

* LED

  * Updated LED PWM support for ESP32 boards

* LoRa

* MBOX

* MEMC

  * Added support for Atmel SAM SMC/EBI.

* MM

* Modem

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

  * Added driver for Infineon XMC4XXX
  * Added driver for Renesas Smartbond platform
  * Added driver for Xilinx Zynq-7000
  * Added support for PSL pads in NPCX driver
  * MEC15XX driver now supports both MEC15XX and MEC17XX
  * nRF driver now supports disconnecting a pin by using ``NRF_PSEL_DISCONNECT``
  * nRF driver will use S0D1 drive mode for TWI/TWIM pins by default

* PWM

  * Added PWM driver for Renesas R-Car platform
  * Added PWM support for NXP LPC55S36
  * Added MCPWM support for ESP32 boards

* Power Domain

  * Enabled access to the PMIC on NXP MXRT595 EVK.
  * Added soft off mode to the RT10xx Power Management.
  * Added support for power gating for Intel ADSP devices.

* Reset

  * Added driver for GigaDevice GD32 SoCs

* SDHC

  * Added SDHC driver for NXP LPCXpresso platform
  * Added support for card busy signal in SDHC SPI driver, to support
    the :ref:`File System API <file_system_api>`

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

  * Added serial driver for Renesas Smartbond platform
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

  * Add interrupt-driven mode support for gd32 driver
  * Enable SPI support on NXP MIMXRT595 EVK.

* Timer

  * STM32 LPTIM based timer should now be configured using device tree.

* USB

  * Restructured the NXP MCUX USB driver.
  * Added USB support for NXP MXRT595.

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
  * Refactored SenML-JSON and JSON econder/decoder to use Zephyr's JSON library
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
     * ``DT_INST_LABEL(inst)``: use ``DT_INST_PROP(inst, label)`` instead
     * ``DT_BUS_LABEL(node_id)``: use ``DT_PROP(DT_BUS(node_id), label))`` instead
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
    * :dtcompatible:`arm,itm`
    * :dtcompatible:`arm,pl022`
    * :dtcompatible:`aspeed,ast10x0-clock`
    * :dtcompatible:`atmel,24mac402`
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
    * :dtcompatible:`gd,gd32-adac`
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
    * :dtcompatible:`sifive,e24.yaml` (formerly ``riscv,sifive-e24``)
    * :dtcompatible:`sifive,e31.yaml` (formerly ``riscv,sifive-e31``)
    * :dtcompatible:`sifive,e51.yaml` (formerly ``riscv,sifive-e51``)
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
        of the associated upstream driver to support multiple instances
      * :dtcompatible:`adi,adxl362`: new ``wakeup-mode``, ``autosleep`` properties

    * Atmel SoCs:

      * :dtcompatible:`atmel,rf2xx`: new ``channel-page``, ``tx-pwr-table``,
        ``tx-pwr-min``, ``tx-pwr-max`` properties
      * GMAC: new ``mac-eeprom`` property

    * Espressif SoCs:

      * :dtcompatible:`espressif,esp32-i2c`: the ``sda-pin`` and ``scl-pin``
        properties are now ``scl-gpios`` and ``sda-gpios``
      * :dtcompatible:`espressif,esp32-ledc`: device configuration moved to
        devicetree via a new child binding
      * :dtcompatible:`espressif,esp32-pinctrl`: this now uses pin groups
      * :dtcompatible:`espressif,esp32-spi`: new ``use-iomux`` property
      * :dtcompatible:`espressif,esp32-usb-serial`: removed ``peripheral``
        property

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
        new ``paddr-size``, ``exec-bit-idx``, ``write-bit-idx`` properties
      * :dtcompatible:`intel,cavs-shim-clkctl`: new ``wovcro-supported`` property
      * Removed ``intel,dmic`` binding
      * Removed ``intel,s1000-pinmux`` binding

    * Nordic SoCs:

      * :dtcompatible:`nordic,nrf-pinctrl`: ``NRF_PSEL_DISCONNECTED`` can be used
        to disconnect a pin
      * :dtcompatible:`nordic,nrf-spim`: new ``rx-delay-supported``,
        ``rx-delay`` properties
      * :dtcompatible:`nordic,nrf-spim`, :dtcompatible:`nordic,nrf-spi`: new
         ``overrun-character``, ``max-frequency``, ``memory-region``,
         ``memory-region-names`` properties
      * :dtcompatible:`nordic,nrf-uarte`: new ``memory-region``,
        ``memory-region-names`` properties
      * Various bindings have had ``foo-pin`` properties deprecated. For
        example, :dtcompatible:`nordic,nrf-qspi` has a deprecated ``sck-pin``
        property. Uses of such properties should be replaced with pinctrl
        equivalents; see :dtcompatible:`nordic,nrfpinctrl`.

    * Nuvoton SoCs:

      * :dtcompatible:`nuvoton,npcx-leakage-io`: new ``lvol-maps`` property
      * :dtcompatible:`nuvoton,npcx-scfg`: removed ``io_port``, ``io_bit``
        cells in ``lvol_cells`` specifiers
      * Removed: ``nuvoton,npcx-lvolctrl-def``, ``nuvoton,npcx-psl-out``,
        ``nuvoton,npcx-pslctrl-conf``, ``nuvoton,npcx-pslctrl-def``
      * Added pinctrl support for PSL (Power Switch Logic) pads

    * NXP SoCs:

      * :dtcompatible:`nxp,imx-pwm`: new ``run-in-wait``, ``run-in-debug`` properties
      * :dtcompatible:`nxp,lpc-spi`: new ``def-char`` property
      * :dtcompatible:`nxp,lpc-iocon-pinctrl`: new ``nxp,analog-alt-mode`` property
      * removed deprecated ``nxp,lpc-iap`` binding
      * :dtcompatible:`nxp,imx-csi`: new ``sensor`` property replacing the
        ``sensor-label`` property
      * :dtcompatible:`nxp,imx-lpi2c`: new ``scl-gpios``, ``sda-gpios`` properties

    * STM32 SoCs:

      * :dtcompatible:`st,stm32-adc`: new ``has-vbat-channel`` property
      * :dtcompatible:`st,stm32-can`: removed ``one-shot`` property
      * :dtcompatible:`st,stm32-fdcan`: new ``clocks``, ``clk-divider`` properties
      * :dtcompatible:`st,stm32-ospi`: new ``dmas``, ``dma-names`` properties
      * :dtcompatible:`st,stm32-ospi-nor`: new ``four-byte-opcodes``,
        ``writeoc`` properties; new enum values ``2`` and ``4`` in
        ``spi-bus-width`` property
      * :dtcompatible:`st,stm32-pwm`: removed deprecated ``st,prescaler`` property
      * :dtcompatible:`st,stm32-rng`: new ``nist-config`` property
      * :dtcompatible:`st,stm32-sdmmc`: new ``dmas``, ``dma-names``,
        ``bus-width`` properties
      * :dtcompatible:`st,stm32-temp-cal`: new ``ts-cal-resolution`` property;
        removed ``ts-cal-offset`` property
      * :dtcompatible:`st,stm32u5-pll-clock`: new ``div-p`` property
      * temperature sensor bindings no longer have a ``ts-voltage-mv`` property
      * UART bindings: new ``wakeup-line`` properties

    * Texas Instruments parts:

      * :dtcompatible:`ti,ina237`: new ``alert-config``, ``irq-gpios`` properties
      * :dtcompatible:`ti,bq274xx`: new ``zephyr,lazy-load`` property

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

      * :dtcompatible:`zephyr,bt-hci-spi`: new ``reset-assert-duration-ms`` property
      * removed ``zephyr,ipm-console`` binding
      * :dtcompatible:`zephyr,ipc-openamp-static-vrings`: new
        ``zephyr,buffer-size`` property
      * :dtcompatible:`zephyr,memory-region`: new ``PPB`` and ``IO`` region support

    * :dtcompatible:`infineon,xmc4xxx-uart`: new ``input-src`` property
    * WSEN-HIDS sensors: new ``drdy-gpios``, ``odr`` properties
    * :dtcompatible:`sitronix,st7789v`: ``cmd-data-gpios`` is now optional
    * :dtcompatible:`solomon,ssd16xxfb`: new ``dummy-line``,
      ``gate-line-width`` properties. The ``gdv``, ``sdv``, ``vcom``, and
      ``border-waveform`` properties are now optional.
    * ``riscv,clint0`` removed; all in-tree users were converted to
      ``sifive,clint0`` or derived bindings
    * :dtcompatible:`worldsemi,ws2812`: SPI bindings have new ``spi-cpol``,
      ``spi-cpha`` properties
    * :dtcompatible:`ns16550`: ``reg-shift`` is now required
    * Removed ``reserved-memory`` binding

* Implementation details

  * The generated devicetree header file placed in the build directory was
    renamed from ``devicetree_unfixed.h`` to ``devicetree_generated.h``

  * The generated ``device_extern.h`` has been replaced using
    ``DT_FOREACH_STATUS_OKAY_NODE``. See `commit
    0224f2c508df154ffc9c1ecffaf0b06608d6b623
    <https://github.com/zephyrproject-rtos/zephyr/commit/0224f2c508df154ffc9c1ecffaf0b06608d6b623>`_

Libraries / Subsystems
**********************

* Console

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

* Emul

* Filesystem

* IPC

* Management

  * MCUMGR race condition when using the task status function whereby if a
    thread state changed it could give a falsely short process list has been
    fixed.
  * MCUMGR shell (group 9) CBOR structure has changed, the ``rc``
    response is now only used for mcumgr errors, shell command
    execution result codes are instead returned in the ``ret``
    variable instead, see :ref:`mcumgr_smp_group_9` for updated
    information. Legacy bahaviour can be restored by enabling
    :kconfig:option:`CONFIG_MCUMGR_CMD_SHELL_MGMT_LEGACY_RC_RETURN_CODE`
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
  * MCUMgr transport API drops ``zst`` parameter, of :c:struct:`zephyr_smp_transport`
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

* Cbprintf and logging

  * Updated cbprintf static packaging to interpret ``unsigned char *`` as a pointer
    to a string. See :ref:`cbprintf_packaging_limitations` for more details about
    how to efficienty use strings. Change mainly applies to the ``logging`` subsystem
    since it uses this feature.
  * Added :c:macro:`LOG_RAW` for logging strings without additional formatting.
    It is similar to :c:macro:`LOG_PRINTK` but do not append ``<cr>`` when new line is found.

* IPC

  * Introduced a 'zephyr,buffer-size' DT property to set the sizes for TX and
    RX buffers per created instance.
  * Set WQ priority back to PRIO_PREEMPT to fix an issue that was starving the scheduler.
  * ``icmsg_buf`` library was renamed to ``spsc_pbuf``.
  * Added cache handling support to ``spsc_pbuf``.
  * Fixed an issue where the TX virtqueue was misaligned by 2 bytes due to the
    way the virtqueue start address is calculated
  * Added :c:func:`ipc_service_deregister_endpoint` function to deregister endpoints.

* LoRaWAN

* Modbus

* Power management

* POSIX

  * Make ``tz`` non-const in ``gettimeofday()`` for conformance to spec.
  * Fix pthread descriptor resource leak. Previously only pthreads with state
    ``PTHREAD_TERMINATED`` could be reused. However, ``pthread_join()`` sets
    the state to ``PTHREAD_EXITED``. Consider both states as candidates in
    ``pthread_create()``.
  * Add ``perror()`` implementation
  * Use consistent timebase in ``sem_timedwait()``

* RTIO

  * Initial version of an asynchronous task and executor API for I/O similar inspired
    by Linux's very successful io_uring.
  * Provides a simple linear and limited concurrency executor, simple task queuing,
    and the ability to poll for task completions.

* SD Subsystem

  * SDMMC STM32: Now compatible with STM32L5 series. Added DMA support for DMA-V1
    compatible devices.
  * Added support for handling the :c:macro:`DISK_IOCTL_CTRL_SYNC` ioctl call.
    this enables the filesystem api :c:func:`fs_sync`.

* Settings

* Shell

* Storage

* Testsuite

* Tracing

HALs
****

* Atmel

  * sam: Fix incorrect CIDR values for revision b silicon of SAMV71 devices.

* Espressif

  * Updated Espressif HAL with esp-idf 4.4.1 updates
  * Added support to binary blobs implementation
  * Fixed ESP32-C3 wifi issues

* GigaDevice

  * Added support for gd32e50x
  * gd32e10x: upgraded to v1.3.0
  * gd32f4xx: upgraded to v3.0.0

* NXP

  * Updated the NXP MCUX SDK to version 2.12
  * Updated the USB middleware to version 2.12
  * Removed all binary Blobs for power management libraries
  * Removed all binary archive files

* Nordic

* RPi Pico

* Renesas

* ST

* STM32

  * stm32cube: update stm32f7 to cube version V1.17.0
  * stm32cube: update stm32g0 to cube version V1.6.1
  * stm32cube: update stm32g4 to cube version V1.5.1
  * stm32cube: update stm32l4 to cube version V1.17.2
  * stm32cube: update stm32u5 to cube version V1.1.1
  * stm32cube: update stm32wb to cube version V1.14.0
  * pinctrl: some pin definitions did not contain the "_c" suffix, used by pins
    with analog switch on certain H7 devices

* Silabs

* TI

  * simplelink: cc13x2_cc26x2: include ADC driverlib sources
  * simplelink: cc13x2_cc26x2: include flash driverlib sources
  * cc13x2: kconfig conditions for P variant support & custom RF hwattrs
  * cc13x2_cc26x2: update to TI SimpleLink SDK 4.40.04.04

* Telink

* Wurth Elektronik

* Xtensa

MCUboot
*******

Trusted Firmware-M
******************

* Allow enabling FPU in the application when TF-M is enabled.
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

Issue Related Items
*******************

These GitHub issues were addressed since the previous 3.1.0 tagged
release:
