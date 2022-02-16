:orphan:

.. _zephyr_3.0:

Zephyr 3.0.0 (Working draft)
############################

We are pleased to announce the release of Zephyr RTOS version 3.0.0.



The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2021-3835: `Zephyr project bug tracker GHSA-fm6v-8625-99jf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fm6v-8625-99jf>`_

* CVE-2021-3861: `Zephyr project bug tracker GHSA-hvfp-w4h8-gxvj
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hvfp-w4h8-gxvj>`_

* CVE-2021-3966: `Zephyr project bug tracker GHSA-hfxq-3w6x-fv2m
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hfxq-3w6x-fv2m>`_

Known issues
************

You can check all currently known issues by listing them using the GitHub
interface and listing all issues with the `bug label
<https://github.com/zephyrproject-rtos/zephyr/issues?q=is%3Aissue+is%3Aopen+label%3Abug>`_.

API Changes
***********

Changes in this release
=======================

* Following functions in UART Asynchronous API are using microseconds to represent
  timeout instead of milliseconds:
  * :c:func:`uart_tx`
  * :c:func:`uart_rx_enable`

* Replaced custom LwM2M :c:struct:`float32_value` type with a native double type.

* Added function for getting status of USB device remote wakeup feature.

* Added ``ranges`` and ``dma-ranges`` as invalid property to be used with DT_PROP_LEN()
  along ``reg`` and ``interrupts``.

* The existing :c:func:`crc16` and :c:func:`crc16_ansi` functions have been
  modified. The former has a new signature. The latter now properly calculates the
  CRC-16-ANSI checksum. A new function, :c:func:`crc16_reflect`, has been
  introduced to calculated reflected CRCs.

* GATT callbacks ``bt_gatt_..._func_t`` would previously be called with argument
  ``conn = NULL`` in the event of a disconnect. This was not documented as part
  of the API. This behavior is changed so the ``conn`` argument is provided as
  normal when a disconnect occurs.

Removed APIs in this release
============================

* The following Kconfig options related to radio front-end modules (FEMs) were
  removed:

  * ``CONFIG_BT_CTLR_GPIO_PA``
  * ``CONFIG_BT_CTLR_GPIO_PA_PIN``
  * ``CONFIG_BT_CTLR_GPIO_PA_POL_INV``
  * ``CONFIG_BT_CTLR_GPIO_PA_OFFSET``
  * ``CONFIG_BT_CTLR_GPIO_LNA``
  * ``CONFIG_BT_CTLR_GPIO_LNA_PIN``
  * ``CONFIG_BT_CTLR_GPIO_LNA_POL_INV``
  * ``CONFIG_BT_CTLR_GPIO_LNA_OFFSET``
  * ``CONFIG_BT_CTLR_FEM_NRF21540``
  * ``CONFIG_BT_CTLR_GPIO_PDN_PIN``
  * ``CONFIG_BT_CTLR_GPIO_PDN_POL_INV``
  * ``CONFIG_BT_CTLR_GPIO_CSN_PIN``
  * ``CONFIG_BT_CTLR_GPIO_CSN_POL_INV``
  * ``CONFIG_BT_CTLR_GPIO_PDN_CSN_OFFSET``

  This FEM configuration is hardware description, and was therefore moved to
  :ref:`devicetree <dt-guide>`. See the :dtcompatible:`nordic,nrf-radio`
  devicetree binding's ``fem`` property for information on what to do instead
  on the Nordic open source controller.

* Removed Kconfig option ``CONFIG_USB_UART_CONSOLE``.
  Option ``CONFIG_USB_UART_CONSOLE`` was only relevant for console driver
  when CDC ACM UART is used as backend. Since the behavior of the CDC ACM UART
  is changed so that it more closely mimics the real UART controller,
  option is no longer necessary.

* Removed Kconfig option ``CONFIG_OPENOCD_SUPPORT`` in favor of
  ``CONFIG_DEBUG_THREAD_INFO``.

* Removed ``flash_write_protection_set()`` along with the flash write protection
  implementation handler.

* Removed ``CAN_BUS_UNKNOWN`` and changed the signature of
  :c:func:`can_get_state` to return an error code instead.

* Removed ``DT_CHOSEN_ZEPHYR_CANBUS_LABEL`` in favor of utilizing
  :c:macro:`DEVICE_DT_GET`.

* Removed ``CONFIG_LOG_MINIMAL``. Use ``CONFIG_LOG_MODE_MINIMAL`` instead.

Deprecated in this release
==========================

* Removed ``<power/reboot.h>`` and ``<power/power.h>`` deprecated headers.
  ``<sys/reboot.h>`` and ``<pm/pm.h>`` should be used instead.
* :c:macro:`USBD_CFG_DATA_DEFINE` is deprecated in favor of utilizing
  :c:macro:`USBD_DEFINE_CFG_DATA`
* :c:macro:`SYS_DEVICE_DEFINE` is deprecated in favor of utilizing
  :c:macro:`SYS_INIT`.
* :c:func:`device_usable_check` is deprecated in favor of utilizing
  :c:func:`device_is_ready`.
* Custom CAN return codes (:c:macro:`CAN_TX_OK`, :c:macro:`CAN_TX_ERR`,
  :c:macro:`CAN_TX_ARB_LOST`, :c:macro:`CAN_TX_BUS_OFF`,
  :c:macro:`CAN_TX_UNKNOWN`, :c:macro:`CAN_TX_EINVAL`,
  :c:macro:`CAN_NO_FREE_FILTER`, and :c:macro:`CAN_TIMEOUT`) are deprecated in
  favor of utilizing standard errno error codes.
* :c:func:`can_configure` is deprecated in favor of utilizing
  :c:func:`can_set_bitrate` and :c:func:`can_set_mode`.
* :c:func:`can_attach_workq` is deprecated in favor of utilizing
  :c:func:`can_add_rx_filter_msgq` and :c:func:`k_work_poll_submit`.
* :c:func:`can_attach_isr` is is deprecated and replaced by
  :c:func:`can_add_rx_filter`.
* :c:macro:`CAN_DEFINE_MSGQ` is deprecated and replaced by
  :c:macro:`CAN_MSGQ_DEFINE`.
* :c:func:`can_attach_msgq` is deprecated and replaced by
  :c:func:`can_add_rx_filter_msgq`.
* :c:func:`can_detach` is deprecated and replaced by
  :c:func:`can_remove_rx_filter`.
* :c:func:`can_register_state_change_isr` is deprecated and replaced by
  :c:func:`can_set_state_change_callback`.
* :c:func:`can_write` is deprecated in favor of utilizing :c:func:`can_send`.

Stable API changes in this release
==================================

New APIs in this release
========================

* Serial

  * Added new APIs to support datum wider than 8-bit.

    * :kconfig:`CONFIG_UART_WIDE_DATA` is added to enable this new APIs.

    * Following functions, mirroring similar functions for 8-bit datum,
      are added:

      * :c:func:`uart_tx_u16` to send a given number of datum from buffer.

      * :c:func:`uart_rx_enable_u16` to start receiving data.

      * :c:func:`uart_rx_buf_rsp_u16` to set buffer for receiving data
        in response to ``UART_RX_BUF_REQUEST`` event.

      * :c:func:`uart_poll_in_u16` to poll for input.

      * :c:func:`uart_poll_out_u16` to output datum in polling mode.

      * :c:func:`uart_fifo_fill_u16` to fill FIFO with data.

      * :c:func:`uart_fifo_read_u16` to read data from FIFO.

* Devicetree

  * Added new Devicetree helpers:

    * :c:macro:`DT_INST_ENUM_IDX`
    * :c:macro:`DT_INST_ENUM_IDX_OR`
    * :c:macro:`DT_INST_PARENT`

* CAN

  * Added :c:func:`can_get_max_filters` for retrieving the maximum number of RX
    filters support by a CAN controller device.

Kernel
******


Architectures
*************

* ARC


* ARM

  * AARCH32


  * AARCH64


* x86

* Xtensa

  * Introduced a mechanism to automatically figure out which scratch registers
    are used for internal code, instead of hard-coding. This is to accommodate
    the configurability of the architecture where some registers may exist in
    one SoC but not on another one.

  * Added coredump support for Xtensa.

  * Added GDB stub support for Xtensa.

Bluetooth
*********

* Updated all experimental features in Bluetooth to use the new ``EXPERIMENTAL``
  selectable Kconfig option
* Bluetooth now uses logging v2 as with the rest of the tree

* Audio

  * Implemented the Content Control ID module (CCID)
  * Added support for the Coordinated Set Identification Service (CSIS)
  * Added a Temporary Object Transfer client implementation
  * Added a Media Control client implementation
  * Added a Media Control Server implementation
  * Implemented the Media Proxy API
  * Implemented CIG reconfiguration and state handling
  * Updated the CSIS API for both server and client
  * Added Basic Audio Profile (BAP) unicast and broadcast server support

* Direction Finding

  * Added support for filtering of Periodic Advertising Sync by CTE type
  * Added additional handling logic for Periodic Advertising Sync Establishemnt
  * Added CTE RX, sampling and IQ report handling in DF connected mode
  * Added support for CTE configuration in connected mode
  * Direction Finding connection mode now uses the newly refactored LLCP
    implementation

* Host

  * The :kconfig:`CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE` is now enabled by default.
    Storing CCC right after it's written reduces risk of inconsistency of CCC values between bonded peers
  * Added support for L2CAP channel reconfiguration.
  * Added support for SMP error code 0xF, where the peer rejects a distributed
    key
  * Added ``bt_gatt_service_is_registered()`` to verify sevice registration
  * Added create an delete procedures to the Object Transfer Service
    implementation
  * Added support for reassembling extended advertising reports
  * Added support for reassembling periodic advertising reports
  * Added support for setting long periodic advertising data
  * Implemented GATT Long Writes reassembly before forwarding them up to the
    application
  * The GATT Server DB hash calculation logic has been corrected
  * Added storing of the CCC data upon paring complete

* Mesh

  * Split out the Proxy services, which can now be compiled out
  * Added an option to call back on every retransmission
  * Added support for multiple Advertising Sets
  * Refactored he Config Client and Health Client API to allow async use

* Controller

  * Added support for a brand new implementation of LL Control Procedures
    (LLCP), currently disabled by default, can be enabled using the
    ``CONFIG_BT_LL_SW_LLCP_IMPL`` Kconfig choice
  * Added initial support for Broadcast Isochronous Groups (BIG)
  * Integrated ISO Sync RX datapath
  * Transitioned FEM configurations (PA/LNA) from Kconfig to Devicetree
  * Updated the supported Bluetooth HCI version to 5.3
  * Added support for Periodic Advertiser List
  * Added support for Periodic Advertising Synchronization Receive Enable
  * Added support for filter access list filtering for exended scanning
  * Added support for Advertising Extensions dynamic TX power control
  * Added handling of direct address type in extended adv reports
  * Implemented auxiliary PDU device address matching
  * Implemented fragmentation of extended advertising reports over HCI
  * Implemented Extended Advertising and Scan report back-to-back chaining
  * Implemented Periodic Advertising ADI support,including duplicate filtering
  * Introduced a new preferred central connection spacing feature


* HCI Driver

  * Added support for a new optional ``setup()`` function for vendor-specific
    setup code required to bring up the controller
  * Fixed DTM mode not being reset correctly with the HCI Reset command
  * Limited the maximum ACL TX buffer size to 251 bytes

Boards & SoC Support
********************

* Added support for these SoC series:

  * GigaDevice GD32VF103, GD32F3X0, GD32F403 and GD32F450.
  * NXP i.MXRT595, i.MX8MQ, i.MX8MP

* Removed support for these SoC series:


* Made these changes in other SoC series:

  * stm32h7: Added SMPS support
  * stm32u5: Enabled TF-M

* Changes for ARC boards:


* Added support for these ARM boards:

  * GigaDevice GD32F350R-EVAL
  * GigaDevice GD32F403Z-EVAL
  * GigaDevice GD32F450I-EVAL
  * OLIMEX-STM32-H405
  * NXP MIMXRT595-EVK
  * NXP MIMX8MQ-EVK
  * NXP MIMX8MP-EVK
  * ST Nucleo G031K8
  * ST Nucleo H7A3ZI Q
  * ST STM32G081B Evaluation

* Added support for these ARM64 boards:


* Removed support for these ARM boards:


* Removed support for these X86 boards:

* Added support for these RISC-V boards:

  * GigaDevice GD32VF103V-EVAL
  * Sipeed Longan Nano and Nano Lite

* Made these changes in other boards:

  * sam_e70_xplained: Added support for CAN-FD driver
  * mimxrt11xx: Added SOC level power management
  * mimxrt11xx: Added support for GPT timer as OS timer


* Added support for these following shields:


Drivers and Sensors
*******************

* ADC

  * Added support for stm32u5 series
  * stm32: Added shared IRQ support

* Bluetooth


* CAN

  * Renamed ``zephyr,can-primary`` chosen property to ``zephyr,canbus``.
  * Added :c:macro:`CAN_ERROR_WARNING` CAN controller state.
  * Added Atmel SAM Bosch M_CAN CAN-FD driver.
  * Added NXP LPCXpresso Bosch M_CAN CAN-FD driver.
  * Added ST STM32H7 Bosch M_CAN CAN-FD driver.
  * Rework transmission error handling the NXP FlexCAN driver to automatically
    retry transmission in case or arbitration lost or missing acknowledge and
    to fail early in :c:func:`can_send` if in :c:macro:`CAN_BUS_OFF` state.
  * Added support for disabling automatic retransmissions ("one-shot" mode") to
    the ST STM32 bxCAN driver.
  * Converted the emulated CAN loopback driver to be configured through
    devicetree instead of Kconfig.

* Clock Control


* Console


* Counter

  * stm32: Added timer based counter driver (stm32f4 only for now).

* DAC

  * Added support for GigaDevice GD32 SoCs
  * Added support for stm32u5 series

* Disk

  * stm32 sdmmc: Converted from polling to IT driven mode and added Hardware
    Flow Control option

* Display


* Disk


* DMA

  * Added support for suspending and resuming transfers
  * Added support for SoCs with DMA between application and embedded
    processors, allows for transfer directions to be identified as such.
  * mimxrt11xx: Added support for DMA

* EEPROM

  * Added support for the EEPROM present in the TMP116 digital temperature
    sensor.

* Entropy

  * Added support for stm32u5 series

* ESPI


* Ethernet

  * Added support for Synopsys DesignWare MAC driver with implementation
    on stm32h7 series.
  * stm32 (hal based): Added promiscuous mode support
  * stm32 (hal based): Added PTP L2 timestamping support
  * mimxrt11xx: Added support for 10/100M ENET

* Flash

  * stm32g0: Added Dual Bank support
  * stm32_qspi: General enhancement (Generation of the reset pulse for SPI-NOR memory,
    Usage of 4IO for read / write (4READ/4PP), Support for different QSPI banks,
    Support for 4B addressing on spi-nor)

  * ite_i8xxx2: The driver has been reworked so the write/erase protection
    management has been moved to implementations of the flash_write()
    and the flash_erase() calls. The driver was keeping the write protection API
    which was designed to be removed since 2.6 release.


* GPIO

  * Added driver for GigaDevice GD32 SoCs

* Hardware Info


* I2C

  * Added driver for GigaDevice GD32 SoCs
  * Added stats functionality to all drivers
  * Added I2C driver for Renesas R-Car platform
  * Added support for TCA9548A I2C switch

* I2S

  * mimxrt10xx: Added support for I2S
  * mimxrt11xx: Added support for I2S

* IEEE 802.15.4


* Interrupt Controller

  * Added ECLIC driver for GigaDevice RISC-V GD32 SoCs
  * Added EXTI driver for GigaDevice GD32 SoCs

* LED


* LoRa


* MEMC

  *  Added support for stm32f7 series

* Modem

* Pin control

  * Introduced a new state-based pin control (``pinctrl``) API inspired by the
    Linux design principles. The ``pinctrl`` API will replace the existing
    pinmux API, so all platforms using pinmux are encouraged to migrate. A
    detailed guide with design principles and implementation guidelines can be
    found in :ref:`pinctrl-guide`.
  * Platforms already supporting the ``pinctrl`` API:

    * GigaDevice GD32
    * Nordic (preliminary support)
    * Renesas R-Car
    * STM32

* PWM

  * stm32: DT bindings: `st,prescaler` property was moved from pwm
    to parent timer node.
  * stm32: Implemented PWM capture API
  * Added driver for GigaDevice GD32 SoCs. Only PWM output is supported.
  * mimxrt1021: Added support for PWM

* Sensor

  * Added Invensense MPU9250 9-axis IMU driver.
  * Added ITE IT8XX2 tachometer driver.
  * Added STM L5 die temperature driver.
  * Added STM I3G4250D gyroscope driver.
  * Added TI TMP108 driver.
  * Added Winsen MH-Z19B CO2 driver.
  * Constified device config access in sbs_gauge and LM75 drivers.
  * Dropped DEV_DATA/DEV_CFG usage from various drivers.
  * Moved ODR and range properties from Kconfig to devicetree in various STM
    drivers.
  * Refactored INA230 driver to add support for INA237 variant.
  * Refactored various drivers to use I2C/SPI/GPIO DT APIs.
  * Enabled level triggered interrupts in LIS2DH driver.
  * Fixed TMP112 driver to avoid I2C burst write portability issues.
  * Fixed SENSOR_DEG2RAD_DOUBLE macro in LSM6DS0 driver.
  * Fixed gain factor in LSM303DLHC magnetometer driver.

* Serial

  * stm32: Implemented half-duplex option.
  * Added driver for GigaDevice GD32 SoCs. Polling and interrupt driven modes
    are supported.

* SPI

  * stm32: Implemented Frame format option (TI vs Motorola).
  * mimxrt11xx: Added support for Flexspi

* Timer

  * stm32 lptim: Added support for stm32h7

* USB

  * Added support for stm32u5 series (OTG full speed)

* Watchdog

  * Added support for stm32u5 series (Independent and Window)
  * mimxrt1170: Added support for watchdog on CM7

* WiFi


Networking
**********

* CoAP:


* DHCPv4:


* DNS:


* HTTP:


* IPv4:


* LwM2M:


* Misc:


* OpenThread:


* Socket:


* TCP:


* TLS:


USB
***


Build and Infrastructure
************************

* Build system

  * New CMake extension functions:

    * ``dt_alias()``
    * ``target_sources_if_dt_node()``

  * The following CMake extension functions now handle devicetree aliases:

    * ``dt_node_exists()``
    * ``dt_node_has_status()``
    * ``dt_prop()``
    * ``dt_num_regs()``
    * ``dt_reg_addr()``
    * ``dt_reg_size()``

* Devicetree

  * Support for the devicetree compatible ``ti,ina23x`` has been removed.
    Instead, use :dtcompatible:`ti,ina230` or :dtcompatible:`ti,ina237`.

* West (extensions)

  * Added support for gd32isp runner


Libraries / Subsystems
**********************

* Disk


* Management

  * Fixed the mcumgr SMP protocol over serial not adding the length of the CRC16 to packet length.
  * Kconfig option OS_MGMT_TASKSTAT is now disabled by default.

* CMSIS subsystem


* Power management

  * Power management resources are now manually allocated by devices using
    :c:macro:`PM_DEVICE_DEFINE`, :c:macro:`PM_DEVICE_DT_DEFINE` or
    :c:macro:`PM_DEVICE_DT_INST_DEFINE`. Device instantiation macros take now
    a reference to the allocated resources. The reference can be obtained using
    :c:macro:`PM_DEVICE_GET`, :c:macro:`PM_DEVICE_DT_GET` or
    :c:macro:`PM_DEVICE_DT_INST_GET`. Thanks to this change, devices not
    implementing support for device power management will not use unnecessary
    memory.
  * Device runtime power management API error handling has been simplified.
  * :c:func:`pm_device_runtime_enable` suspends the target device if not already
    suspended. This change makes sure device state is always kept in a
    consistent state.
  * Improved PM states Devicetree macros naming
  * Added a new API call :c:func:`pm_state_cpu_get_all` to obtain information
    about CPU power states.
  * ``pm/device.h`` is no longer included by ``device.h``, since the device API
    no longer depends on the PM API.
  * Added support for power domains. Power domains are implemented as
    simple devices and use the existent PM API for resume and suspend, devices
    under a power domain are notified when it becomes active or suspended.
  * Added a new action :c:enum:`PM_DEVICE_ACTION_TURN_ON`. This action
    is used by power domains to notify devices when it becomes active.
  * Added new API (:c:func:`pm_device_state_lock`,
    :c:func:`pm_device_state_unlock` and
    :c:func:`pm_device_state_is_locked`) to lock a device pm
    state. When the device has its state locked, the kernel will no
    longer suspend and resume devices when the system goes to sleep
    and device runtime power management operations will fail.
  * :c:func:`pm_device_state_set` is deprecated in favor of utilizing
    :c:func:`pm_device_action_run`.
  * Proper multicore support. Devices are suspended only when the last
    active CPU. A cpu parameter was added to Policy and SoC interfaces.

* Logging


* Shell


* Storage


* Task Watchdog


* Tracing

  * Support all syscalls being traced using the python syscall generator to
    introduce a tracing hook call.

* Debug

* OS


HALs
****

* STM32

  * stm32cube/stm32wb and its lib: Upgraded to version V1.12.1
  * stm32cube/stm32mp1: Upgraded to version V1.5.0
  * stm32cube/stm32u5: Upgraded to version V1.0.2

* Added `GigaDevice HAL module
  <https://github.com/zephyrproject-rtos/hal_gigadevice>`_

MCUboot
*******

* Fixed serial recovery skipping on nrf5340.
* Fixed issue which caused that progressive's erase feature was off although was selected by Kconfig (introduced by #42c985cead).
* Added check of reset address in incoming image validation phase, see ``CONFIG_MCUBOOT_VERIFY_IMG_ADDRESS``.
* Allow image header bigger than 1 KB for encrypted images.
* Support Mbed TLS 3.0.
* stm32: watchdog support.
* many documentation improvements.
* Fixed deadlock on cryptolib selectors in Kconfig.
* Fixed support for single application slot with serial recovery.
* Added various hooks to be able to change how image data is accessed, see ``CONFIG_BOOT_IMAGE_ACCESS_HOOKS``.
* Added custom commands support in serila recovery (PERUSER_MGMT_GROUP): storage erase ``CONFIG_BOOT_MGMT_CUSTOM_STORAGE_ERASE``, custo image status ``CONFIG_BOOT_MGMT_CUSTOM_IMG_LIST``.
* Added support for direct image upload, see ``CONFIG_MCUBOOT_SERIAL_DIRECT_IMAGE_UPLOAD`` in serial recovery.

Trusted Firmware-m
******************

* Updated TF-M to 1.5.0 release, with a handful of additional cherry-picked
  commits.

Documentation
*************

* A new theme is used by the Doxygen HTML pages. It is based on
  `doxygen-awesome-css <https://github.com/jothepro/doxygen-awesome-css>`_
  theme.

Tests and Samples
*****************

* Drivers: clock_control: Added test suite for stm32 (u5, h7).

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.7.0 tagged
release:
