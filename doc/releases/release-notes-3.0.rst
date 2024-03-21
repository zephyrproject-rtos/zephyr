:orphan:

.. _zephyr_3.0:

Zephyr 3.0.0
############

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

* STM32 clock_control driver configuration was moved from Kconfig to :ref:`devicetree <dt-guide>`.
  See the :dtcompatible:`st,stm32-rcc` devicetree binding for more information.
  As a consequence, following Kconfig symbols were removed:

  * ``CONFIG_CLOCK_STM32_SYSCLK_SRC_HSE``
  * ``CONFIG_CLOCK_STM32_SYSCLK_SRC_HSI``
  * ``CONFIG_CLOCK_STM32_SYSCLK_SRC_MSI``
  * ``CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL``
  * ``CONFIG_CLOCK_STM32_SYSCLK_SRC_CSI``
  * ``CONFIG_CLOCK_STM32_HSE_BYPASS``
  * ``CONFIG_CLOCK_STM32_MSI_RANGE``
  * ``CONFIG_CLOCK_STM32_PLL_SRC_MSI``
  * ``CONFIG_CLOCK_STM32_PLL_SRC_HSI``
  * ``CONFIG_CLOCK_STM32_PLL_SRC_HSE``
  * ``CONFIG_CLOCK_STM32_PLL_SRC_PLL2``
  * ``CONFIG_CLOCK_STM32_PLL_SRC_CSI``
  * ``CONFIG_CLOCK_STM32_AHB_PRESCALER``
  * ``CONFIG_CLOCK_STM32_APB1_PRESCALER``
  * ``CONFIG_CLOCK_STM32_APB2_PRESCALER``
  * ``CONFIG_CLOCK_STM32_CPU1_PRESCALER``
  * ``CONFIG_CLOCK_STM32_CPU2_PRESCALER``
  * ``CONFIG_CLOCK_STM32_AHB3_PRESCALER``
  * ``CONFIG_CLOCK_STM32_AHB4_PRESCALER``
  * ``CONFIG_CLOCK_STM32_PLL_PREDIV``
  * ``CONFIG_CLOCK_STM32_PLL_PREDIV1``
  * ``CONFIG_CLOCK_STM32_PLL_MULTIPLIER``
  * ``CONFIG_CLOCK_STM32_PLL_XTPRE``
  * ``CONFIG_CLOCK_STM32_PLL_M_DIVISOR``
  * ``CONFIG_CLOCK_STM32_PLL_N_MULTIPLIER``
  * ``CONFIG_CLOCK_STM32_PLL_P_DIVISOR``
  * ``CONFIG_CLOCK_STM32_PLL_Q_DIVISOR``
  * ``CONFIG_CLOCK_STM32_PLL_R_DIVISOR``
  * ``CONFIG_CLOCK_STM32_LSE``
  * ``CONFIG_CLOCK_STM32_HSI_DIVISOR``
  * ``CONFIG_CLOCK_STM32_D1CPRE``
  * ``CONFIG_CLOCK_STM32_HPRE``
  * ``CONFIG_CLOCK_STM32_D2PPRE1``
  * ``CONFIG_CLOCK_STM32_D2PPRE2``
  * ``CONFIG_CLOCK_STM32_D1PPRE``
  * ``CONFIG_CLOCK_STM32_D3PPRE``
  * ``CONFIG_CLOCK_STM32_PLL3_ENABLE``
  * ``CONFIG_CLOCK_STM32_PLL3_M_DIVISOR``
  * ``CONFIG_CLOCK_STM32_PLL3_N_MULTIPLIER``
  * ``CONFIG_CLOCK_STM32_PLL3_P_ENABLE``
  * ``CONFIG_CLOCK_STM32_PLL3_P_DIVISOR``
  * ``CONFIG_CLOCK_STM32_PLL3_Q_ENABLE``
  * ``CONFIG_CLOCK_STM32_PLL3_Q_DIVISOR``
  * ``CONFIG_CLOCK_STM32_PLL3_R_ENABLE``
  * ``CONFIG_CLOCK_STM32_PLL3_R_DIVISOR``
  * ``CONFIG_CLOCK_STM32_PLL_DIVISOR``
  * ``CONFIG_CLOCK_STM32_MSI_PLL_MODE``

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

    * :kconfig:option:`CONFIG_UART_WIDE_DATA` is added to enable this new APIs.

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

  * New :ref:`devicetree-ranges-property` APIs

  * Removed: ``DT_CHOSEN_ZEPHYR_CANBUS_LABEL``; use
    ``DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))`` to get the device instead, and
    read the name from the device structure if needed.

  * Removed deprecated macros:

    * ``DT_CLOCKS_LABEL_BY_IDX``
    * ``DT_CLOCKS_LABEL``
    * ``DT_INST_CLOCKS_LABEL_BY_IDX``
    * ``DT_INST_CLOCKS_LABEL_BY_NAME``
    * ``DT_INST_CLOCKS_LABEL``
    * ``DT_PWMS_LABEL_BY_IDX``
    * ``DT_PWMS_LABEL_BY_NAME``
    * ``DT_PWMS_LABEL``
    * ``DT_INST_PWMS_LABEL_BY_IDX``
    * ``DT_INST_PWMS_LABEL_BY_NAME``
    * ``DT_INST_PWMS_LABEL``
    * ``DT_IO_CHANNELS_LABEL_BY_IDX``
    * ``DT_IO_CHANNELS_LABEL_BY_NAME``
    * ``DT_IO_CHANNELS_LABEL``
    * ``DT_INST_IO_CHANNELS_LABEL_BY_IDX``
    * ``DT_INST_IO_CHANNELS_LABEL_BY_NAME``
    * ``DT_INST_IO_CHANNELS_LABEL``
    * ``DT_DMAS_LABEL_BY_IDX``
    * ``DT_INST_DMAS_LABEL_BY_IDX``
    * ``DT_DMAS_LABEL_BY_NAME``
    * ``DT_INST_DMAS_LABEL_BY_NAME``
    * ``DT_ENUM_TOKEN``
    * ``DT_ENUM_UPPER_TOKEN``


* CAN

  * Added :c:func:`can_get_max_filters` for retrieving the maximum number of RX
    filters support by a CAN controller device.

Kernel
******

  * Added support for event objects.  Threads may wait on an event object such
    that any events posted to that event object may wake a waiting thread if the
    posting satisfies the waiting threads' event conditions.
  * Extended CPU runtime stats to track current, total, peak and average usage
    (as bounded by the scheduling of the idle thread).  This permits a developer
    to obtain more system information if desired to tune the system.
  * Added "thread_usage" API for thread runtime cycle monitoring.
  * Fixed timeout issues when SYSTEM_CLOCK_SLOPPY_IDLE is configured.

Architectures
*************

* ARM

  * AARCH32

    * Converted inline assembler calls to using CMSIS provided functions for
      :c:func:`arm_core_mpu_enable` and :c:func:`arm_core_mpu_disable`.
    * Replaced Kconfig `CONFIG_CPU_CORTEX_R` with `CONFIG_ARMV7_R` to enable
      differentiation between v7 and v8 Cortex-R.
    * Updated the Cortex-R syscall behavior to match that of the Cortex-M.

  * AARCH64

    * Fixed out-of-bounds error when large number of IRQs are enabled and ignore
      special INTDs between 1020 and 1023
    * Added MPU code for ARMv8R
    * Various MMU fixes
    * Added nocache memory segment support
    * Added Xen hypercall interface for ARM64
    * Fixed race condition on SMP scheduling code.

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

  * The :kconfig:option:`CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE` is now enabled
    by default. Storing CCC right after it's written reduces risk of
    inconsistency of CCC values between bonded peers
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
  * Added support for filter access list filtering for extended scanning
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
  * Raspberry Pi RP2040
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
  * Raspberry Pi Pico
  * ST Nucleo G031K8
  * ST Nucleo H7A3ZI Q
  * ST STM32G081B Evaluation

* Added support for these ARM64 boards:

  * Intel SoC FPGA Agilex development kit

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

* CAN

  * Renamed ``zephyr,can-primary`` chosen property to ``zephyr,canbus``.
  * Added :c:macro:`CAN_STATE_ERROR_WARNING` CAN controller state.
  * Added Atmel SAM Bosch M_CAN CAN-FD driver.
  * Added NXP LPCXpresso Bosch M_CAN CAN-FD driver.
  * Added ST STM32H7 Bosch M_CAN CAN-FD driver.
  * Rework transmission error handling the NXP FlexCAN driver to automatically
    retry transmission in case or arbitration lost or missing acknowledge and
    to fail early in :c:func:`can_send` if in :c:macro:`CAN_STATE_BUS_OFF`.
  * Added support for disabling automatic retransmissions ("one-shot" mode") to
    the ST STM32 bxCAN driver.
  * Converted the emulated CAN loopback driver to be configured through
    devicetree instead of Kconfig.

* Counter

  * stm32: Added timer based counter driver (stm32f4 only for now).

* DAC

  * Added support for GigaDevice GD32 SoCs
  * Added support for stm32u5 series

* Disk

  * stm32 sdmmc: Converted from polling to IT driven mode and added Hardware
    Flow Control option

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

* I2C

  * Added driver for GigaDevice GD32 SoCs
  * Added stats functionality to all drivers
  * Added I2C driver for Renesas R-Car platform
  * Added support for TCA9548A I2C switch

* I2S

  * mimxrt10xx: Added support for I2S
  * mimxrt11xx: Added support for I2S

* Interrupt Controller

  * Added ECLIC driver for GigaDevice RISC-V GD32 SoCs
  * Added EXTI driver for GigaDevice GD32 SoCs

* MBOX

  * Added MBOX NRFX IPC driver

* MEMC

  *  Added support for stm32f7 series

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


Networking
**********

* CoAP:

  * Refactored ``coap_client``/``coap_server`` samples to make better use of
    observe APIs.
  * Added PATCH, iPATCH and FETCH methods.
  * A few fixes for the block transfer handling.

* DNS:

  * Make mdns and llmnr responders join their multicast groups.
  * Added support for mdns/dns_sd service type enumeration.

* ICMPv6:

  * Added support for Route Information option processing.

* IPv4:

  *  Add IPv4 support to multicast monitor.

* LwM2M:

  * Added a parameter to forcefully close the LwM2M session to
    :c:func:`lwm2m_rd_client_stop` function.
  * Replaced custom ``float32_value_t`` type with double.
  * Added :kconfig:option:`LWM2M_FIRMWARE_PORT_NONSECURE`/
    :kconfig:option:`LWM2M_FIRMWARE_PORT_SECURE` options, which allow to
    specify a custom port or firmware update.
  * Added :c:func:`lwm2m_update_device_service_period` API function.
  * Added observe callback for observe and notification events.
  * Added support for multiple LwM2M Firmware Update object instances.
  * Improved error handling in LwM2M content writers.
  * Added unit tests for LwM2M content writers.
  * Implemented LwM2M Security, Server, Connection Monitor objects in version 1.1.
  * Multiple minor bugfixes in the LwM2M stack.
  * Added support for the following objects:

    * LWM2M Software Management (ID 9)
    * LwM2M Gateway (ID 25)
    * IPSO Current (ID 3317)
    * uCIFI Battery (ID 3411)
    * IPSO Filling level (ID 3435)

* Misc:

  * gptp: clock sync ratio as double, not float
  * Added support for route lifetime and preference.
  * Refactored various packed structures across the networking stack, to avoid
    unaliged access warnings from gcc.
  * Added automatic loopback addresses registration to loopback interface.
  * Fixed source address selection for ARP.
  * Allow to implement a custom IEEE802154 L2 on top of existing drivers.
  * Introduced a network packet filtering framework.

* MQTT:

  * Fixed incomplete :c:func:`zsock_sendmsg` writes handling.
  * Fixed :c:func:`zsock_setsockopt` error handling in SOCKS5 transport.

* OpenThread:

  * Updated OpenThread revision up to commit ``ce77ab3c1d7ad91b284615112ae38c08527bf73e``.
  * Fixed an overflow bug in the alarm implementation for Zephyr.
  * Added crypto backend based on PSA API.
  * Allow to store OpenThread settings in RAM.

* Socket:

  * Fixed :c:func:`zsock_sendmsg` when payload size exceeded network MTU.
  * Added socket processing priority.
  * Fixed possible crash in :c:func:`zsock_getaddrinfo` when DNS callback is
    delayed.

* Telnet:

  * Fixed handling of multiple commands in a single packet.
  * Enabled command handling by default.

* TCP:

  * Added support for sending our MSS to peer.
  * Fixed packet sending to local addresses.
  * Fixed possible deadlock between TCP and socket layer, when connection close
    is initiated from both sides.
  * Multiple other minor bugfixes and improvements in the TCP implementation.

* TLS:

  * Added support for ``TLS_CERT_NOCOPY`` socket option, which allows to
    optimise mbed TLS heap usage.
  * Fixed ``POLLHUP`` detection when underlying TCP connection is closed.
  * Fixed mbedtls session reset on handshake errors.

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

* Management

  * Fixed the mcumgr SMP protocol over serial not adding the length of the CRC16 to packet length.
  * Kconfig option OS_MGMT_TASKSTAT is now disabled by default.

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

* Tracing

  * Support all syscalls being traced using the python syscall generator to
    introduce a tracing hook call.

* IPC

  * Added IPC service support and RPMsg with static VRINGs backend

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
* Fixed issue which caused that progressive's erase feature was off although was
  selected by Kconfig (introduced by #42c985cead).
* Added check of reset address in incoming image validation phase, see
  ``CONFIG_MCUBOOT_VERIFY_IMG_ADDRESS``.
* Allow image header bigger than 1 KB for encrypted images.
* Support Mbed TLS 3.0.
* stm32: watchdog support.
* many documentation improvements.
* Fixed deadlock on cryptolib selectors in Kconfig.
* Fixed support for single application slot with serial recovery.
* Added various hooks to be able to change how image data is accessed, see
  ``CONFIG_BOOT_IMAGE_ACCESS_HOOKS``.
* Added custom commands support in serial recovery (PERUSER_MGMT_GROUP): storage
  erase ``CONFIG_BOOT_MGMT_CUSTOM_STORAGE_ERASE``, custom image status
  ``CONFIG_BOOT_MGMT_CUSTOM_IMG_LIST``.
* Added support for direct image upload, see
  ``CONFIG_MCUBOOT_SERIAL_DIRECT_IMAGE_UPLOAD`` in serial recovery.

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

* :github:`42973` - Zephyr-sdkConfig.cmake file not found
* :github:`42961` - Bluetooth: periodic_sync sample never executes .recv callback
* :github:`42942` - sizeof(struct sockaddr_storage) is smaller than sizeof(struct sockaddr_in6)
* :github:`42862` - Bluetooth: L2CAP: Security check on l2cap request is wrong
* :github:`42816` - samples: Bluetooth: df: DF samples build fail
* :github:`42794` - samples: Bluetooth: df: Wrong periodic sync termination handling in direction_finding_connectionless_rx sample
* :github:`42793` - net_socket: mimxrt1170_evk_cm7: build failure
* :github:`42778` - bluetooth: autopts: can't start on the board
* :github:`42759` - armv8 qemu_cortex_a53 bug(gdb) on official sample
* :github:`42756` - mec15xxevb_assy6853: ringbuffer testsuite failing once due to a timeout randomly when run multiple times.
* :github:`42746` - echo_server and echo_client sample code builds fail for native_posix_64
* :github:`42735` - Bluetooth: Host: df: Uninitialized variable used to assign length of antenna identifiers
* :github:`42693` - Bluetooth: DF connectionless TX sample fails to build if CONFIG_BT_CTLR_DF_SCAN_CTE_RX  is disabled
* :github:`42690` - sample.bootloader.mcuboot.serial_recovery fails to compile
* :github:`42687` - [v 1.13 ] HID is not connecting to intel 7265 Bluetooth Module
* :github:`42665` - tests: kernel.common.context: test failure on imxrt series platform
* :github:`42648` - Setting long advertising data does not work
* :github:`42627` - Hardfault regression on 90 tests on CM0+ STM32 boards introduced by #39963 Cortex-R mpu fix  on 90 tests
* :github:`42615` - [v2.7.2] Bluetooth: Controller: Missing ticks slot offset calculation in Periodic Advertising event scheduling
* :github:`42608` - bsim_test_mesh: pb_adv_reprovision.sh fails after commit to prevent multiple arguments in logging
* :github:`42604` - doc: broken CONFIG_GPIO link in https://docs.zephyrproject.org/latest/reference/peripherals/gpio.html
* :github:`42602` - I2C scan writes 0 bytes
* :github:`42588` - lsm6dso
* :github:`42586` - Net buffer macros rely on GCC extension
* :github:`42585' - 3.0.0-rc1: warning: LOG_STRDUP_MAX_STRING was assigned the value '100` but got the value ''
* :github:`42581` - include: drivers: clock_control: stm32 incorrect DT_PROP is used for 'xtpre'
* :github:`42573` - docs: sphinx-build issue, zephyr conf.py issue or something else?
* :github:`42556` - frdm_k64f: samples/subsys/modbus are failing with a timeout.
* :github:`42555` - mimxrt1050_evk: samples/subsys/task_wdt is failing with control thread getting stuck
* :github:`42502` - Unable to add a specific syscon driver out-of-tree
* :github:`42499` - mec15xxevb_assy6853: boards.mec15xxevb_assy6853.i2c.i2c_pca95xx test failed.
* :github:`42477` - Linker scripts not working properly on xtensa
* :github:`42462` - logging: syst/v2: hang or crash if log contains string arguments
* :github:`42435` - NXP RT1170/1160 base address error for SAI4 in devicetree
* :github:`42417` - tests drivers flash on stm32 qspi controller
* :github:`42414` - twister: testcases skipped by ztest_test_skip() have reason "Unknown" in report
* :github:`42411` - CLion CMake error while opening nRF-Connect-SDK project
* :github:`42403` - 'crc16_ansi()' isn't CRC-16-ANSI
* :github:`42397` - Direction finding nrf5340: uninitialized memory is passed to the callback
* :github:`42396` - ztest: weak test_main() is promoted over given testsuite's test_main() if the testsuite uses own library
* :github:`42392` - Openocd Thread awareness broken on 3.0
* :github:`42385` - STM32: Entropy : health test config & magic never used
* :github:`42380` - USDHC driver encounters usage fault during frequency setting
* :github:`42373` - add k_spin_lock() to doxygen prior to v3.0 release
* :github:`42367` - stm32wb: BLE connections not working
* :github:`42361` - OpenOCD flashing not working on cc1352r1_launchxl/cc26x2r1_launchxl
* :github:`42358` - net: lwm2m: client context accessed after being invalidated in lwm2m_rd_client_stop()
* :github:`42353` - LwM2M not pass official LightweightM2M-1.1-int-256 and stack enter dead lock
* :github:`42323` - lwm2m_engine: Error when enabling debug log because of uninitialized variable 'from_addr'
* :github:`42308` - pm: Force shutdown has no effect
* :github:`42299` - spi: nRF HAL driver asserts when PM is used
* :github:`42292` - Compilation failed: Driver MPU6050
* :github:`42279` - The pthreads are not working on user space. ARM64 cortex_a53 but generic requirement.
* :github:`42278` - USB CDC-ACM non-functional after host reboot
* :github:`42272` - doc: "Building on Linux without the Zephyr SDK" does not describe how to actually do it
* :github:`42269` - impossible to run west flash. NoneType error
* :github:`42228` - hal_stm32: Wrong symbol definition
* :github:`42227` - Teensy41 support SDHC - Storage init Error
* :github:`42218` - stm32wl: Issue when disabling gpio interrupt
* :github:`42214` - drivers: uart_nrfx_uarte: Cannot start another reception after reception is complete
* :github:`42208` - tests/subsys/logging/log_api/ fails qemu_leon3 if ptr_in_rodata() is enabled for SPARC
* :github:`42205` - drivers: i2s_sam_ssc: data received via I2S bus are partially corrupted
* :github:`42199` - drivers: qdec_sam: position measurement unstable if adc_sam_afec driver is enabled
* :github:`42187` - Settings tests are not correctly run
* :github:`42184` - Incremental build with config changes can produce an invalid binary when userspace is enabled
* :github:`42179` - driver: i2s: i2s_mcux_sai build failure on mixmrt1170_evk_cm7
* :github:`42177` - PM_STATE_INFO_DT_ITEMS_LIST macro does not fill the pm_min_residency array
* :github:`42176` - mec15xxevb_assy6853: can not be flashed due to "chip not identified"
* :github:`42171` - v3.0.0-rc1: mimxrt685_evk_cm33: undefined reference to 'SystemCoreClock' for latency_measure benchmark
* :github:`42170` - v3.0.0-rc1: mimxrt685_evk_cm33: dma driver build failure
* :github:`42168` - v3.0.0-rc1: mimxrt685_evk_cm33: i2s driver build failure
* :github:`42164` - tests/bluetooth/tester broken after switch to logging v2
* :github:`42163` - BIT_MASK(32) generate warning on 32 bits processor
* :github:`42161` - samples/compression/l4z: Expected RAM size for correct execution is too low
* :github:`42159` - samples: lora: Miss twister harness
* :github:`42157` - tests/lib/ringbuffer/libraries.ring_buffer: Miss a timeout
* :github:`42151` - eth_sam_gmac: unable to change MAC address
* :github:`42149` - DT_SPI_DEV_CS_GPIOS_DT_SPEC_GET is a layering violation that shouldn't exist
* :github:`42147` - hts221 driver fails to build
* :github:`42125` - Bluetooth: controller: llcp: lll_scan_aux does not compile with new LLCP
* :github:`42120` - HTS221 missed header hts221_reg.h
* :github:`42118` - mimxrt685_evk_cm33: Build failed on tests/drivers/spi/spi_loopback/drivers.spi.loopback
* :github:`42117` - efr32mg_sltb004a: Build issue on 'tests/drivers/spi/spi_loopback/drivers.spi.loopback'
* :github:`42112` - OTS: L2CAP: Unable to find channel of LE Credits packet
* :github:`42106` - AARCH64 stack initialisation fails with newlib for qemu_cortex_a53
* :github:`42098` - intel_adsp_cavs25: west sign command output some unrecognized ASCII char.
* :github:`42092` - stm32l0: Voltage regulator is not restored after leaving STOP mode
* :github:`42070` - west: spdx: Missing field for certain build results
* :github:`42065` - Bluetooth Controller: scan aux setup not checking extended header length of received packet
* :github:`42061` - obj_tracking hangs system on intel_adsp_cavs25
* :github:`42031` - Ringbuffer used in CDC_ACM seems to corrupt data if completely filled during transfer
* :github:`42024` - unrecognized argument in option '-mabi=lp64'
* :github:`42010` - intel_adsp_cavs18: Test cases failed on SMP related test cases (when CONFIG_MP_NUM_CPUS > 1)
* :github:`41996` - LWM2M writing too long strings trigger post_write_cb with previously written value
* :github:`41993` - Intel_adsp_cavs18: test cases can not get complete log
* :github:`41992` - Intel_adsp_cavs18: tests/kernel/smp_boot_delay: failed to run case
* :github:`41991` - Intel_adsp_cavs18: some test cases can not get any log
* :github:`41989` - tests: kernel: tickless: ADSP stalls after firmware downloaded on Up Xtreme
* :github:`41982` - twister: Test not aborted after board was timed out
* :github:`41976` - Extra closing bracket in function lsm6dso_handle_interrupt
* :github:`41963` - Kernel usage fault when using semaphore with multi-threading
* :github:`41953` - drivers: counter: mcux_ctimer: config used as non-const
* :github:`41952` - Log timestamp overflows when using LOGv2
* :github:`41951` - drivers: regulator: pmic: config used as non-const
* :github:`41945` - nxp_hal module: Seconds GPIO interrupt does never fire
* :github:`41943` - Intel_adsp_cavs15:   all the test cases run failed when running them by twister
* :github:`41942` - k_delayable_work being used as k_work in work's handler
* :github:`41938` - esp_wrover_kit: hello_world build failure
* :github:`41933` - updatehub  metadata size 0
* :github:`41915` - regression: Build fails after switching logging to V2
* :github:`41911` - pm_power_state_force returns false after first call
* :github:`41894` - ISOAL sink handle value checked incorrectly
* :github:`41887` - Documentation setup page missing packages for arch linux
* :github:`41879` - new ztest api fails when user space is enabled
* :github:`41877` - tests: kernel: fatal: ADSP stalls after firmware downloaded on Up Xtreme
* :github:`41873` - STM32H735 Power Supply Config incorrect
* :github:`41862` - tests: kernel: fail to download firmware to ADSP on Up Xtreme
* :github:`41861` - tests: kernel: There are no log output after flashing image to intel_adsp_cavs25
* :github:`41860` -  tests: kernel: queue: test case kernel.queue failed on ADSP of Up Xtreme
* :github:`41839` - BLE causes system sleep before main
* :github:`41835` - UP squared and acrn_ehl_crb:  test cases which have config SMP config failed
* :github:`41826` - MQTT connection failed
* :github:`41821` - ESP32 mcuboot bootloader failed
* :github:`41818` - In uart.h uart_irq_rx_ready() function not working properly for STM32F429 controller
* :github:`41816` - nrf_802154 radio driver takes random numbers directly from entropy source
* :github:`41806` - tests: driver: clock: nrf: Several failures on nrf52dk_nrf52832
* :github:`41794` - Bluetooth: ATT calls GATT callbacks with NULL conn pointer during disconnect
* :github:`41792` - CPU load halfed after PR #40784
* :github:`41745` - Power Management blinky example does not work on STM32H735G-DK
* :github:`41736` - Xtensa xt-xc++ Failed to build C++ code
* :github:`41734` - Can't enable pull-up resistors in ESP32 gpio 25,26,27
* :github:`41722` - mcuboot image not confirmed on nrf5340dk
* :github:`41707` - esp32 newlib
* :github:`41698` - What does one have to do to activate BT_DBG?
* :github:`41694` - undefined reference to '_open'
* :github:`41691` - Tickless Kernel on STM32H7 fails with Exception
* :github:`41686` - SPI CS signal not used in SSD1306 driver
* :github:`41683` - http_client: Unreliable rsp->body_start pointer
* :github:`41682` - ESP32 mcuboot
* :github:`41653` - Bluetooth: Controller: Extended Advertising Scan: Implement Scan Data length maximum
* :github:`41637` - Modbus Gateway: Transaction ID Error!
* :github:`41635` - Samples: iso_broadcast can not work properly unless some extra configuration flags
* :github:`41627` - PPP_L2 does not properly terminate the modem state machine when going down.
* :github:`41624` - ESP32 Uart uart_esp32_irq_tx_ready
* :github:`41623` - esp32: fail to build sample/hello_world with west
* :github:`41608` - LwM2M: Cannot set pmin/pmax on observable object
* :github:`41582` - stm32h7: CSI as PLL source is broken
* :github:`41581` - STM32 subghzspi fails pinctrl setup
* :github:`41557` - ESP32 Uart 2-bit Stop Register Setting
* :github:`41526` - ESP32 UART driver tx_complete fires before last byte sent
* :github:`41525` - tests: drivers: : ethernet: fails to link for sam_v71_xult and sam_v71b_xult
* :github:`41524` - drivers: dma: dma_mcux_edma: unused variables cause daily build failures
* :github:`41523` - drivers: i2c: i2c_mcux: unused variables cause daily build failures
* :github:`41509` - OpenThread's timer processing enters infinite loop in 49th day of system uptime
* :github:`41503` - including <net/socket.h> fails with redefinition of 'struct zsock_timeval' (sometimes :-) )
* :github:`41499` - drivers: iwdg: stm32: 'WDT_OPT_PAUSE_HALTED_BY_DBG' might not work
* :github:`41488` - Stall logging on nrf52840
* :github:`41486` - Zephyr project installation
* :github:`41482` - kernel: Dummy thread should not have an uninitialized resource pool
* :github:`41471` - qemu_cortex_r5: failed to enable debug
* :github:`41465` - Periodic advertising sync failure, when "DONT_SYNC_AOA" or "DONT_SYNC_AOD" options is used
* :github:`41442` - power_init for STM32L4 and STM32G0 in POST_KERNEL
* :github:`41440` - twister: skip marked as pass
* :github:`41426` - ARMCLANG build fail
* :github:`41422` - The option CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE blocks k_sleep when CONFIG_PM is enabled
* :github:`41418` - tests/lib/devicetree/devices fails to build for thingy52_nrf52832
* :github:`41413` - NRF52832 - PWM not working after zephyr update
* :github:`41404` - if zsock_connect() fails, tls_context does not get released automatically
* :github:`41399` - samples: userspace: syscall_perf test cannot be run
* :github:`41395` - littlefs(external spi flash) + mcuboot can't get right mount area
* :github:`41392` - arm ：arm-none-eabi  Unable to complete compilation, an error occurred during linking
* :github:`41385` - SHT3xD example does not work on ESP32
* :github:`41359` - Bluetooth: connection times out when trying to connect from certain centrals
* :github:`41352` - uart_esp32_poll_in returns incorrect value
* :github:`41347` - tests: kernel: RT1170 fails test_kernel_cpu_idle
* :github:`41339` - stm32, Unable to read UART while checking from Framing error.
* :github:`41331` - tests: drivers: disk: fail to handle no SD card situation
* :github:`41317` - ADSP: Many kernel test cases which have CONFIG_MP_NUM_CPUS=1 failed in daily testing
* :github:`41299` - IS25LP016D SPI NOR FLASH PROBLEM
* :github:`41291` - LVGL touch event "LV_EVENT_LONG_PRESSED" can not be generated if I press the screen without lift up my finger
* :github:`41289` - shell: infinite error loop upon LOG_ERR in ISR context
* :github:`41284` - pthread_cond_wait return value incorrect
* :github:`41272` - ci: twister: mcuboot: MCUboot tests are no longer executed in the CI
* :github:`41268` - 'bt_gatt_cancel' type mismatch
* :github:`41256` - Zero Latency Interrupts conflicts
* :github:`41255` - drivers/can/can_mcan.c: address-of-packed-member warnings
* :github:`41251` - RT1170 EVK Can does not send data
* :github:`41244` - subsys: pm: Low power mode transition broken
* :github:`41240` - logging can get messed up when messages are dropped
* :github:`41237` - [v2.7] drivers: ieee802154_dw1000: use dedicated workqueue
* :github:`41222` - tests: remove not existing platforms from platform allow or integration_platform
* :github:`41153` - rt i2s build fail
* :github:`41127` - OpenAMP Sample does not work on LPCXpresso55S69
* :github:`41117` - Incorrect setting of gyro sensitivity in LSM6DSO driver
* :github:`41111` - uint64 overflow in z_tmcvt() function
* :github:`41100` - Non-standard RISC-V assembly is used
* :github:`41097` - west init issue
* :github:`41095` - libc: newlib: 'gettimeofday' causes stack overflow on non-POSIX builds
* :github:`41093` - Kconfig.defconfig:11: error: couldn't parse 'default $(dt_node_int_prop_int,/cpus/cpu@0,clock-frequency)'
* :github:`41077` - console: gsm_mux: could not send more than 128 bytes of data on dlci
* :github:`41074` - can_mcan_send sends corrupted CAN frames with a byte-by-byte memcpy implementation
* :github:`41066` - twister --generate-map is broken
* :github:`41062` - kernel: userspace: Potential misaligned access
* :github:`41058` - stm32h723 : application gets hung during spi_transceive() operation
* :github:`41052` - tests-ci : portability: posix: fs.tls.newlib test Build failure
* :github:`41050` - MCUMgr Sample Fails to build
* :github:`41043` - Sporadic Bus Fault when using I2C on a nrf52840
* :github:`41026` - LoRa: sx126x: DIO1 interrupt left enabled in sleep mode
* :github:`41024` - SPI Loopback test fails to build on iMX RT EVKs
* :github:`41017` - USB string descriptors can be re-ordered causing corruption and out-of-bounds-write
* :github:`41016` - i2c_sam0.c i2c_sam0_transfer operations do not execute a STOP
* :github:`41012` - irq_enable() doesn’t support enabling NVIC IRQ number more than 127
* :github:`40999` - Unable to boot smp_svr sample image as documentation suggests, or sign
* :github:`40974` - Xtensa High priority interrupts cannot be masked during initialization
* :github:`40965` - Halt on receipt of Google Cloud IoT Core MQTT message sized 648+ bytes
* :github:`40946` - Xtensa Interrupt nesting issue
* :github:`40942` - Xtensa debug bug
* :github:`40936` - STM32 ADC gets stuck in Calibration
* :github:`40925` - mesh_badge not working reel_board_v2
* :github:`40917` - twister --export-tests export all cases even this case can not run on given platform
* :github:`40916` - Assertion in nordic's BLE controller lll.c:352
* :github:`40903` - documentation generation fails on function typedefs
* :github:`40889` - samples: samples/kernel/metairq_dispatch failed on acrn_ehl_crb
* :github:`40888` - samples:    samples/subsys/portability/cmsis_rtos_v1/philosophers failed on ehl crb
* :github:`40887` - tests: debug:  test case subsys/debug/coredump failed on acrn_ehl_crb
* :github:`40883` - Limitation on logging module
* :github:`40881` - Bluetooth: shell: fatal error because ctx_shell is NULL
* :github:`40873` - qemu_cortex_r5: fail to handle user_string_alloc_copy() with null parameter
* :github:`40870` - tests: syscall: failed to build on fvp_baser_aemv8r_smp
* :github:`40866` - Undefined behavior in lib/os/cbprintf_packaged.c: subtraction involving definitely null pointer
* :github:`40838` - Nordic UART driver (UARTE) fail to transfer buffers from read only memory
* :github:`40827` - Tensorflow example not working in zephyr v2.6
* :github:`40825` - STM32WB55RGV6: No output after west flash
* :github:`40820` - coap: blockwise: context current does not match total size after transfer is completed
* :github:`40808` - Invalid CMake warning related to rimage
* :github:`40795` - Timer signal thread execution loop break SMP on ARM64
* :github:`40783` - samples/subsys/usb/dfu  should filter on FLASH driver
* :github:`40776` - HCI_USB with nRF52840 dongle disconnect after 30 s
* :github:`40775` - stm32: multi-threading broken after #40173
* :github:`40770` - tests/subsys/cpp/libcxx/cpp.libcxx.newlib fails on m2gl025_miv and qemu_cortex_m0
* :github:`40761` - Bluetooth: host: Wait for the response callback before clearing Service Changed data
* :github:`40759` - Bluetooth: host: Improper restore of CCC values and handling Service Change indication when bonded peer reconnects
* :github:`40758` - Bluetooth: host: CCC values are not immediately stored on GATT Server by default (risk of inconsistency)
* :github:`40744` - RT600 LittleFS Sample produces build warning in default configuration
* :github:`40740` - tests: logging: test case log_msg2.logging.log_msg2_64b_timestamp failed on qemu_cortex_a9
* :github:`40724` - tests: logging: logging test cases failed in multiple boards
* :github:`40717` - twister: failure in parsing code coverage file
* :github:`40714` - west flash, Invalid DFU suffix signature
* :github:`40688` - in "pinmux_stm32.c"  function "stm32_dt_pinctrl_remap" not work
* :github:`40672` - EDTT: buffer overflow in edtt_hci_app
* :github:`40668` - Issue with twister code coverage tests not working with minimal C library (nRF52840)
* :github:`40663` - WWDG not supported on STM32H7 family
* :github:`40658` - shtcx not reporting correct humidity value
* :github:`40646` - Can't read more than one OUTPUT|INPUT gpio pin in gpio_emul
* :github:`40643` - intel_adsp_cavs15:  the zephyr_pre0.elf  is quite large (530MB) on ADSP for some test cases
* :github:`40640` - drivers: usb_dc_native_posix: segfault when using composite USB device
* :github:`40638` - drivers: usb_dc_mcux: processing endpoint callbacks in ISR context causes assertion
* :github:`40633` - CI documentation build hangs when there is a broken reference
* :github:`40624` - twister: coverage: Using --coverage flag for on-target test make tests last until time limit
* :github:`40622` - Dark mode readability problem in Unit Test Documentation
* :github:`40621` - npcx uart driver uses device PM callback to block suspension
* :github:`40614` - poll: the code judgment condition is always true
* :github:`40590` - gen_app_partitions scans object files unrelated to current image
* :github:`40586` - tests: logging: Logging.add.user scenario fails on all nrf boards
* :github:`40578` - MODBUS RS-485 transceiver support broken on several platforms due to DE race condition
* :github:`40569` - bisected: kernel.common.stack_protection_arm_fpu_sharing fails on mps3_an547
* :github:`40546` - Bluetooh:host: GATT notify multiple feature not working properly
* :github:`40538` - mcuboot build fails with nrf52 internal RC oscillator
* :github:`40517` - msgq: NULL handler assertion with data cache enabled
* :github:`40483` - ESP32: display sample over i2c not working
* :github:`40464` - Dereferencing NULL with getsockname() on TI Simplelink Platform
* :github:`40456` - Bluetooth: L2CAP tester application is missing preprocessor flags for ECFC function call
* :github:`40453` - LittleFS fails when block count is greater than block size
* :github:`40450` - Twister map file shows baud in quotes but should not be in quotes
* :github:`40449` - Twister tests fail when running on actual hardware due to deprecated command warning
* :github:`40439` - Undefined escape sequence: ill-formed for the C standard
* :github:`40438` - Ill-formed sources due to external linkage inline functions calling static functions
* :github:`40433` - RTT fails to work in program with large global variable
* :github:`40420` - Lower-case characters in Kconfig symbol names cause obscure errors
* :github:`40411` - Xtensa xcc compile build fails with SOF application on latest Zephyr main
* :github:`40376` - HiFIve1 failed to run tests/kernel/workq/work/
* :github:`40374` - up_squared: isr_dynamic test is failing
* :github:`40369` - tests/subsys/logging/log_core/ and tests/subsys/shell/shell/ hang on qemu_cortex_a53 and qemu_riscv64
* :github:`40367` - sample: cycle_64 is failing out due to a timeout on 64-bit versions of qemu_x86 and ehl_crb
* :github:`40348` - STM32L496 Uart rx interrupt callback fails to work with LVGL
* :github:`40329` - nucleo_g0b1re: FDCAN message RAM write fails on byte-oriented write
* :github:`40317` - Crash in ull.c when stressing periodic advertising sync (scanner side)
* :github:`40316` - Error undefined reference to '__aeabi_uldivmod' when build with Zephyr 2.7.0 for STM32
* :github:`40298` - Bluetooth assertions in lll_conn.c
* :github:`40290` - CAN_STM32: Build error with CONFIG_CAN_AUTO_BUS_OFF_RECOVERY=n
* :github:`40256` - websocket: the size of a websocket payload is limited
* :github:`40254` - TF-M: BL2 signing is broken due to incompatible MCUboot version
* :github:`40244` - [v2.7-branch] hci_spi sample cannot be built for nrf51dk_nrf51422 and 96b_carbon_nrf51
* :github:`40236` - Unsigned int can't be used in condition compare with int
* :github:`40215` - RSSI in periodic adv. callbacks always -127 (sync_recv and cte_report_cb)
* :github:`40209` - Bluetooth: First AUX_SYNC_IND never received, missing event send to host
* :github:`40202` - Bluetooth: Periodic advertising synchronization not re-established after advertiser reset without scan disable
* :github:`40198` - Shell module doesn't work on main branch for esp32 board
* :github:`40189` - k_poll infrastructure can miss "signals" in a heavily contended SMP system
* :github:`40169` - drivers: can: net: compilation broken and no test cases in CI
* :github:`40159` - Bluetooth Mesh branch incorrect return value
* :github:`40153` - mimxrt1050_evk: failed to run samples/subsys/task_wdt
* :github:`40152` - task_wdt  can get stuck in a loop where hardware reset is never fired
* :github:`40133` - mimxrt1060-evk flash shell command causes shell deadlock
* :github:`40129` - 'tests/net/socket/tls/net.socket.tls.preempt' fails with 'qemu_cortex_a9'
* :github:`40124` - Build fails with 'CONFIG_SHELL_VT100_COMMANDS=n'
* :github:`40119` - OBJECT_TRACING for kernel objects
* :github:`40115` - logging: int-uint comparsion causes false assert & epic hang
* :github:`40107` - lwm2m: if network drops during firmware update, lock occurs
* :github:`40077` - driver: wdt: twrke18f: test_wdt fails
* :github:`40076` - Driver led pca9633 does only use first device in devicetree
* :github:`40074` - sara-r4: socket call fails due to regression
* :github:`40070` - canbus: isotp: Violations of k_fifo and net_buf API usage
* :github:`40069` - Bluetooth CCM encryption bug in MIC generation
* :github:`40068` - Test suite subsys.pm.device_runtime_api fail on qemu_x86_64
* :github:`40030` - STM32 SD hardware flow control gets disabled if disk_access_init is used
* :github:`40021` - mimxrt1060_evk_hyperflash board definition is broken
* :github:`40020` - tests: kernel: mem_slab: mslab_api: undefined reference to z_impl_k_sem_give and z_impl_k_sem_take
* :github:`40007` - twister: cannot build samples/tests on Windows
* :github:`40003` - Bluetooth: host: zephyr writes to disconnected device and triggers a bus fault
* :github:`40000` - k_timer timeout handler is called with interrupts locked
* :github:`39989` - Zephyr does not persist CCC data written before bonding when bonding has completed which leads to loss of subscriptions on device reset
* :github:`39985` - Telnet shell breaks upon sending Ctrl+C character
* :github:`39978` - logging.log2_api_deferred and logging.msg2 tests fail on qemu_cortex_a9
* :github:`39973` - Bluetooth: hci_usb example returning "Unknown HCI Command" after reset.
* :github:`39969` - USB not automatically enabled when USB_UART_CONSOLE is set
* :github:`39968` - samples: tfm_integration: tfm_psa_test broken on OS X (Windows?)
* :github:`39947` - open-amp problem with dcache
* :github:`39942` - usdhc disk_usdhc_access_write busy fail
* :github:`39923` - qspi_sfdp_read fails errata work around
* :github:`39919` - CONFIG_ISM330DHCX cannot compile due to missing file
* :github:`39904` - bl654_usb does not work with hci_usb sample application
* :github:`39900` - usb bug :USB device descriptor could not be obtained   on windows10
* :github:`39893` - Bluetooth: hci usb: scan duplicate filter not working
* :github:`39883` - BLE stack overlow due to the default option value when compiling with no optimization
* :github:`39874` - [Coverity CID: 240214] Dereference before null check in drivers/dma/dma_mcux_edma.c
* :github:`39872` - [Coverity CID: 240218] Dereference after null check in subsys/bluetooth/controller/ll_sw/ull_scan_aux.c
* :github:`39870` - [Coverity CID: 240220] Argument cannot be negative in tests/net/socket/af_packet_ipproto_raw/src/main.c
* :github:`39869` - [Coverity CID: 240221] Unchecked return value from library in drivers/usb/device/usb_dc_native_posix.c
* :github:`39868` - [Coverity CID: 240222] Dereference before null check in drivers/dma/dma_mcux_edma.c
* :github:`39857` - [Coverity CID: 240234] Uninitialized scalar variable in subsys/bluetooth/shell/iso.c
* :github:`39856` - [Coverity CID: 240235] Explicit null dereferenced in subsys/bluetooth/controller/ll_sw/ull_scan_aux.c
* :github:`39852` - [Coverity CID: 240241] Out-of-bounds access in subsys/bluetooth/host/adv.c
* :github:`39851` - [Coverity CID: 240242] Dereference after null check in tests/bluetooth/tester/src/l2cap.c
* :github:`39849` - [Coverity CID: 240244] Untrusted value as argument in drivers/usb/device/usb_dc_native_posix.c
* :github:`39844` - [Coverity CID: 240658] Argument cannot be negative in tests/net/lib/dns_sd/src/main.c
* :github:`39843` - [Coverity CID: 240659] Out-of-bounds read in /zephyr/include/generated/syscalls/kernel.h (Generated Code)
* :github:`39841` - [Coverity CID: 240661] Unchecked return value in tests/net/net_pkt/src/main.c
* :github:`39840` - [Coverity CID: 240662] Improper use of negative value in subsys/mgmt/osdp/src/osdp.c
* :github:`39839` - [Coverity CID: 240663] Out-of-bounds access in tests/benchmarks/mbedtls/src/benchmark.c
* :github:`39835` - [Coverity CID: 240667] Improper use of negative value in samples/subsys/usb/cdc_acm_composite/src/main.c
* :github:`39833` - [Coverity CID: 240670] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`39832` - [Coverity CID: 240671] Out-of-bounds access in drivers/flash/flash_mcux_flexspi_hyperflash.c
* :github:`39830` - [Coverity CID: 240673] Out-of-bounds read in /zephyr/include/generated/syscalls/kernel.h (Generated Code)
* :github:`39827` - [Coverity CID: 240676] Out-of-bounds access in drivers/ieee802154/ieee802154_dw1000.c
* :github:`39825` - [Coverity CID: 240678] Unchecked return value in drivers/ieee802154/ieee802154_cc1200.c
* :github:`39824` - [Coverity CID: 240679] Out-of-bounds access in samples/subsys/usb/cdc_acm_composite/src/main.c
* :github:`39823` - [Coverity CID: 240681] Improper use of negative value in drivers/bluetooth/hci/h4.c
* :github:`39817` - drivers: pwm: nxp: (potentially) Incorrect return value on API function
* :github:`39815` - [Coverity CID: 240688] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`39813` - [Coverity CID: 240691] Out-of-bounds access in tests/benchmarks/mbedtls/src/benchmark.c
* :github:`39812` - [Coverity CID: 240692] Unintended sign extension in subsys/stats/stats.c
* :github:`39810` - [Coverity CID: 240696] Operands don't affect result in subsys/net/lib/lwm2m/lwm2m_util.c
* :github:`39809` - [Coverity CID: 240697] Out-of-bounds access in samples/subsys/usb/cdc_acm/src/main.c
* :github:`39807` - [Coverity CID: 240699] Out-of-bounds access in tests/bluetooth/tester/src/l2cap.c
* :github:`39806` - [Coverity CID: 240700] Unchecked return value in drivers/ieee802154/ieee802154_cc2520.c
* :github:`39805` - [Coverity CID: 240703] Improper use of negative value in drivers/bluetooth/hci/h4.c
* :github:`39797` - STM32 G4 series compile error when both ADC1 and ADC2 are opened
* :github:`39780` - On ESP32S2 platform zsock_getaddrinfo() call causes RTOS to crash
* :github:`39774` - modem: uart mux reading optimization never used
* :github:`39758` - Build is broken if LWM2M_CANCEL_OBSERVE_BY_PATH config is set
* :github:`39756` - kconfig: choice default is not set if hidden under invisible menu
* :github:`39726` - How to use PWM LED driver for ESP32?
* :github:`39721` - bq274xx sensor - Fails to compile when CONFIG_PM_DEVICE enabled
* :github:`39720` -  XCC BUILD FAIL :K_MEM_SLAB_DEFINE && K_HEAP_DEFINE
* :github:`39718` - STM32L496G_DISCO uart testing fails on single buffer read
* :github:`39712` - bq274xx sensor - Fails to compile when CONFIG_PM_DEVICE enabled
* :github:`39707` - Can't enable CONFIG_SHELL_LOG_BACKEND Log Shell Menus with pure Telnet Shell Backend
* :github:`39705` - Canot use POSIX_API and NET_SOCKETS together
* :github:`39704` - Using OpenThread makes the system unresponsive after 49.7 days
* :github:`39703` - stm32 uart testing fails on test_read_abort
* :github:`39687` - sensor: qdec_nrfx: PM callback has incorrect signature
* :github:`39675` - list_boards.py script doesn't properly traverse external board roots
* :github:`39672` - net_config_init count calculation appears incorrect.
* :github:`39660` - poll() not notified when a TLS/TCP connection is closed without TLS close_notify
* :github:`39655` - Linker error with CONFIG_NET_TCP=y
* :github:`39645` - STM32L496 Zephyr using LVGL disp_drv.flush_cb can not work
* :github:`39629` - Small Compiler warning in subsys/fs/shell.c:381:23 in latest release, need argument change only
* :github:`39627` - samples: http_get: cannot run on QEMU
* :github:`39624` - Bluetooth: Submitting more GATT writes than available buffers blocks for 30s and then errors out
* :github:`39619` - twister: integration_platforms getting unnoticeably skipped when --subset is used
* :github:`39609` - spi: slave: division by zero in timeout calculation
* :github:`39601` - On ESP32S2 platform GPIO interrupt causes RTOS to hang when configured to GPIO_INT_EDGE_BOTH
* :github:`39594` - Possible bug or undocumented behaviour of spi_write
* :github:`39588` - drivers: i2c: nrf: i2c error with burst write
* :github:`39575` - k_mutex_lock and k_sem_take with K_FOREVER return -EAGAIN value
* :github:`39569` - [ESP32] crash when trying to set a low cpu clock frequency
* :github:`39549` - Bluetooth: Incomplete Delayed Initialization of acl_mtu Allows Controller to Crash Host Layer
* :github:`39546` - mcumgr over serial does not add CRC to length of packet len
* :github:`39541` - can: mcux_flexcan: wrong timing calculation
* :github:`39538` - logging: rtt: Compilation fails when CONFIG_LOG_BACKEND_RTT_MODE_OVERWRITE=y and CONFIG_MULTITHREADING=n
* :github:`39523` - task watchdog crash/asset on NRF52840 - need to reorder task_wdt_feed() in task_wdt_add()
* :github:`39516` - function net_eth_vlan_enable does not properly validate vlan tag value
* :github:`39506` - Bluetooth: crash in att.c when repeatedly scanning/connecting/disconnecting
* :github:`39505` - question: ethernet: carrier_on_off
* :github:`39503` - Zephyr boot banner not updated on rebuild with opdated SHA
* :github:`39497` - doc: kernel: event object static initialization mismatch
* :github:`39487` - esp32 IRQ01 stack utilisation is 100%
* :github:`39483` - LSM6DS0 Gyroscope rad/s Calculation Error
* :github:`39463` - ESP32 GPIO intterupt
* :github:`39461` - Bluetooth: hci acl flow control: bugs of bluetooth hci ACL flow control
* :github:`39457` - mec15xxevb_assy6853: metairq_dispatch sample is failing due to timeout while monitoring serial output
* :github:`39438` - Scanning for devices sending periodic advertisements stops working after a while, but keeps reporting none periodic.
* :github:`39423` - mcuboot not upgrade  for stm32l1 series
* :github:`39418` - test: run testcase failed on platform mps2_an521_ns
* :github:`39416` - west debug throws error
* :github:`39405` - CTE report callback have the wrong pointer to bt_le_per_adv_sync
* :github:`39400` - stm32f103 example servo_motor don't work
* :github:`39399` - linker: Missing align __itcm_load_start / __dtcm_data_load_start linker symbols
* :github:`39392` - ARC nsim_sem fail on tests/crypto/tinycrypt_hmac_prng test when use ARCMWDT toolchain
* :github:`39340` - Shell FS sample halts with a usage fault error
* :github:`39311` - SPDX --init fails on windows systems
* :github:`39300` - Library globals in .sdata/.sbss sections doesn't put into memory partition in userspace
* :github:`39293` - Can not run normally on MIMXRT1061CVL5A SOC
* :github:`39269` - Fail to initialize BLE stack with optimization level zero
* :github:`39253` - modem: hl7800: IPv6 socket not created properly
* :github:`39242` - net: sockets: Zephyr Fatal in dns_resolve_cb if dns request was attempted in offline state
* :github:`39221` - Errors when debuging application in Eclipse using STM32L496G-DISCO
* :github:`39216` - Twister: Broken on NRF52840 with pyocd option timeout error
* :github:`39179` - twister: --generate-hardware-map ends up in RuntimeError
* :github:`39144` - gsm_ppp: stop & starting not working as expected with nullpointer dereference & no full modem init
* :github:`39136` - SD disk access runs into TXUNDERRUN and RXOVERRUN of SDMMC driver
* :github:`39131` - GATT DB hash calculation is wrong on characteristic declarations using 128-bit UUIDs.
* :github:`39096` - DNS responders assume interfaces are up at initialization
* :github:`39024` - drivers: sensors: FXOS8700: Interrupt pin routing configuration must be changed in standby power mode
* :github:`38988` - MCP2515 driver CS gpio active high support issue
* :github:`38987` - Unable to build ESP32 example code using west tool - zephyr
* :github:`38954` - Can't get FlexPWM working for imxrt1060
* :github:`38631` - printk to console fails for freescale kinetis 8.2.0 (Zephyr 2.6.0) on FRDM-K64F
* :github:`38624` - mcuboot gets the wrong value of DT_FIXED_PARTITION_ID
* :github:`38606` - drivers: adc: stm32h7: Oversampling Ratio set incorrectly
* :github:`38598` - net_context_put will not properly close TCP connection (might lead to tcp connection leak)
* :github:`38576` - net shell: self-connecting to TCP might lead to a crash
* :github:`38502` - Update mcumgr library to fix wrong callback state
* :github:`38446` - intel_adsp_cavs15: Fail to get testcases output on ADSP
* :github:`38376` - Raw Socket Failure when using 2 Raw Sockets and zsock_select() statement - improper mapping from sock to handlers
* :github:`38303` - The current BabbleSim tests build system based on bash scripts hides warnings
* :github:`38128` - [Coverity CID: 239574] Out-of-bounds access in subsys/storage/flash_map/flash_map.c
* :github:`38047` - twister: The --board-root parameter doesn't appear to work
* :github:`37893` - mcumgr_serial_tx_pkt with len==91 fails to transmit CRC
* :github:`37389` - nucleo_g0b1re: Swapping image in mcuboot results in hard fault and softbricks the device
* :github:`36986` - LittleFS mount fails (error -22)
* :github:`36962` - littlefs: Too small heap for file cache (again).
* :github:`36852` - acrn_ehl_crb:  the test of tests/subsys/cpp/libcxx/ failed
* :github:`36808` - xtensa xcc build  Fail ,   CONFIG_NO_OPTIMIZATIONS=y
* :github:`36766` - tests-ci :kernel.tickless.concept.tickless_slice : test failed
* :github:`34732` - stm32h747i_disco: Wrong Power supply setting LDO
* :github:`34375` - drivers: can: CAN configure fails when  CONFIG_CAN_FD_MODE is enabled
* :github:`31748` - boards:lpcxpresso55s69: Manual toggling of CS required with ETH Click shield
* :github:`23052` - nrf52840_pca10056: Spurious RTS pulse and incorrect line level with hardware flow control disabled
* :github:`16587` - build failures with gcc 9.x
* :github:`8924` - Get rid of -fno-strict-overflow
