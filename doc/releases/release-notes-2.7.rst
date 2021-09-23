:orphan:

.. _zephyr_2.7:

Zephyr 2.7.0 (Working draft)
############################

We are pleased to announce the release of Zephyr RTOS version 2.7.0.



The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:


Known issues
************

You can check all currently known issues by listing them using the GitHub
interface and listing all issues with the `bug label
<https://github.com/zephyrproject-rtos/zephyr/issues?q=is%3Aissue+is%3Aopen+label%3Abug>`_.

API Changes
***********

Deprecated in this release

* :c:macro:`DT_ENUM_TOKEN` and :c:macro:`DT_ENUM_UPPER_TOKEN`,
  were deprecated in favor of utilizing
  :c:macro:`DT_STRING_TOKEN` and :c:macro:`DT_STRING_UPPER_TOKEN`

* :c:macro:`BT_CONN_ROLE_MASTER` and :c:macro:`BT_CONN_ROLE_SLAVE`
  have been deprecated in favor of
  :c:macro:`BT_CONN_ROLE_CENTRAL` and :c:macro:`BT_CONN_ROLE_PERIPHERAL`

* :c:macro:`BT_LE_SCAN_OPT_FILTER_WHITELIST`
  has been deprecated in favor of
  :c:macro:`BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST`

* The following whitelist functions have been deprecated:
  :c:func:`bt_le_whitelist_add`
  :c:func:`bt_le_whitelist_rem`
  :c:func:`bt_le_whitelist_clear`
  in favor of
  :c:func:`bt_le_filter_accept_list_add`
  :c:func:`bt_le_filter_accept_list_remove`
  :c:func:`bt_le_filter_accept_list_clear`

Modified in this release

* The following Bluetooth macros and structures in :file:`hci.h` have been
  modified to align with the inclusive naming in the v5.3 specification:

  * ``BT_LE_FEAT_BIT_SLAVE_FEAT_REQ`` is now ``BT_LE_FEAT_BIT_PER_INIT_FEAT_XCHG``
  * ``BT_LE_FEAT_BIT_CIS_MASTER`` is now ``BT_LE_FEAT_BIT_CIS_CENTRAL``
  * ``BT_LE_FEAT_BIT_CIS_SLAVE`` is now ``BT_LE_FEAT_BIT_CIS_PERIPHERAL``
  * ``BT_FEAT_LE_SLAVE_FEATURE_XCHG`` is now ``BT_FEAT_LE_PER_INIT_FEAT_XCHG``
  * ``BT_FEAT_LE_CIS_MASTER`` is now ``BT_FEAT_LE_CIS_CENTRAL``
  * ``BT_FEAT_LE_CIS_SLAVE`` is now ``BT_FEAT_LE_CIS_PERIPHERAL``
  * ``BT_LE_STATES_SLAVE_CONN_ADV`` is now ``BT_LE_STATES_PER_CONN_ADV``
  * ``BT_HCI_OP_LE_READ_WL_SIZE`` is now ``BT_HCI_OP_LE_READ_FAL_SIZE``
  * ``bt_hci_rp_le_read_wl_size`` is now ``bt_hci_rp_le_read_fal_size``
  * ``bt_hci_rp_le_read_wl_size::wl_size`` is now ``bt_hci_rp_le_read_fal_size::fal_size``
  * ``BT_HCI_OP_LE_CLEAR_WL`` is now ``BT_HCI_OP_LE_CLEAR_FAL``
  * ``BT_HCI_OP_LE_ADD_DEV_TO_WL`` is now ``BT_HCI_OP_LE_REM_DEV_FROM_FAL``
  * ``bt_hci_cp_le_add_dev_to_wl`` is now ``bt_hci_cp_le_add_dev_to_fal``
  * ``BT_HCI_OP_LE_REM_DEV_FROM_WL`` is now ``BT_HCI_OP_LE_REM_DEV_FROM_FAL``
  * ``bt_hci_cp_le_rem_dev_from_wl`` is now ``bt_hci_cp_le_rem_dev_from_fal``
  * ``BT_HCI_ROLE_MASTER`` is now ``BT_HCI_ROLE_CENTRAL``
  * ``BT_HCI_ROLE_SLAVE`` is now ``BT_HCI_ROLE_PERIPHERAL``
  * ``BT_EVT_MASK_CL_SLAVE_BC_RX`` is now ``BT_EVT_MASK_CL_PER_BC_RX``
  * ``BT_EVT_MASK_CL_SLAVE_BC_TIMEOUT`` is now ``BT_EVT_MASK_CL_PER_BC_TIMEOUT``
  * ``BT_EVT_MASK_SLAVE_PAGE_RSP_TIMEOUT`` is now ``BT_EVT_MASK_PER_PAGE_RSP_TIMEOUT``
  * ``BT_EVT_MASK_CL_SLAVE_BC_CH_MAP_CHANGE`` is now ``BT_EVT_MASK_CL_PER_BC_CH_MAP_CHANGE``
  * ``m_*`` structure members are now ``c_*``
  * ``s_*`` structure members are now ``p_*``

* The ``CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY`` Kconfig option is now
  :kconfig:`CONFIG_BT_PERIPHERAL_PREF_LATENCY`
* The ``CONFIG_BT_CTLR_SLAVE_FEAT_REQ_SUPPORT`` Kconfig option is now
  :kconfig:`CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG_SUPPORT`
* The ``CONFIG_BT_CTLR_SLAVE_FEAT_REQ`` Kconfig option is now
  :kconfig:`CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG`

Changes in this release
==========================

Removed APIs in this release

* Removed support for the deprecated ``DEVICE_INIT`` and ``DEVICE_AND_API_INIT`` macros.
* Removed support for the deprecated ``BUILD_ASSERT_MSG`` macro.
* Removed support for the deprecated ``GET_ARG1``, ``GET_ARG2`` and ``GET_ARGS_LESS_1`` macros.
* Removed support for the deprecated Kconfig ``PRINTK64`` option.
* Removed support for the deprecated ``bt_set_id_addr`` function.
* Removed support for the Kconfig ``USB`` option. Option ``USB_DEVICE_STACK``
  is sufficient to enable USB device support.

* Removed ``CONFIG_OPENTHREAD_COPROCESSOR_SPINEL_ON_UART_ACM`` and
  ``CONFIG_OPENTHREAD_COPROCESSOR_SPINEL_ON_UART_DEV_NAME`` Kconfig options
  in favor of chosen node ``zephyr,ot-uart``.
* Removed ``CONFIG_BT_UART_ON_DEV_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,bt-uart``.
* Removed ``CONFIG_BT_MONITOR_ON_DEV_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,bt-mon-uart``.
* Removed ``CONFIG_MODEM_GSM_UART_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,gsm-ppp``.
* Removed ``CONFIG_UART_MCUMGR_ON_DEV_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,uart-mcumgr``.
* Removed ``CONFIG_UART_CONSOLE_ON_DEV_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,console``.
* Removed ``CONFIG_UART_SHELL_ON_DEV_NAME`` Kconfig option
  in favor of direct use of chosen node ``zephyr,shell-uart``.

============================

Stable API changes in this release
==================================

* Bluetooth

  * Added :c:struct:`multiple` to the :c:struct:`bt_gatt_read_params` - this
    structure contains two members: ``handles``, which was moved from
    :c:struct:`bt_gatt_read_params`, and ``variable``.

* Networking

  * Added IPv4 address support to the multicast group join/leave monitor. The
    address parameter passed to the callback function was therefore changed from
    ``in6_addr`` to ``net_addr`` type.

Kernel
******


Architectures
*************

* ARC


* ARM

  * AARCH32

     * Updated CMSIS version to 5.8.0
     * Added support for FPU in QEMU for Cortex-M, allowing to build and execute
       tests in CI with FPU and FPU_SHARING options enabled.


  * AARCH64


* RISC-V

  * Added support to RISC-V CPU devicetree compatible bindings
  * Added support to link with ITCM & DTCM sections


* x86


Bluetooth
*********

* Audio

* Host

* Mesh

  * Added return value for opcode callback

* Bluetooth LE split software Controller

* HCI Driver

Boards & SoC Support
********************

* Added support for these SoC series:


* Removed support for these SoC series:


* Made these changes in other SoC series:

  * Added Atmel SAM0 pinctrl support
  * Added Atmel SAM4L USBC device controller
  * Added Atmel GMAC support for MDIO driver
  * Added Atmel GMAC support to use generic PHY driver
  * Added Atmel SAM counter (TC) Driver
  * Added Atmel SAM DAC (DACC) driver
  * Enabled Atmel SAM ``clock-frequency`` support from devicetree
  * Free Atmel SAM TRACESWO pin when unused
  * Enabled Cypress PSoC-6 Cortex-M4 support


* Changes for ARC boards:


* Added support for these ARM boards:


* Added support for these ARM64 boards:


* Removed support for these ARM boards:


* Removed support for these X86 boards:


* Made these changes in other boards:

  * arduino_due: Added support for TC driver
  * atsame54_xpro: Added support for PHY driver
  * sam4l_ek: Added support for TC driver
  * sam4e_xpro: Added support for PHY driver
  * sam4e_xpro: Added support for TC driver
  * sam4s_xplained: Added support for TC driver
  * sam_e70_xplained: Added support for DACC driver
  * sam_e70_xplained: Added support for PHY driver
  * sam_e70_xplained: Added support for TC driver
  * sam_v71_xult: Added support for DACC driver
  * sam_v71_xult: Added support for PHY driver
  * sam_v71_xult: Added support for TC driver
  * sam_v71_xult: Enable pwm on LED0
  * cy8ckit_062_ble: Added arduino's nexus map


* Added support for these following shields:


Drivers and Sensors
*******************

* ADC


* Bluetooth


* CAN


* Clock Control


* Console


* Counter

  * Add Atmel SAM counter (TC) Driver


* DAC

  * Added Atmel SAM DAC (DACC) driver
  * Added support for Microchip MCP4725


* Disk

  * Added SDMMC support on STM32L4+

* Display

  * Added support for ST7735R

* Disk


* DMA

  * Added Atmel SAM XDMAC reload support
  * Added support on STM32G0 and STM32H7


* EEPROM

  * Added support for EEPROM emulated in flash.

* ESPI

  * Added support for Microchip eSPI SAF

* Ethernet

  * Added Atmel SAM/SAM0 GMAC devicetree support
  * Added Atmel SAM/SAM0 MDIO driver
  * Added MDIO driver
  * Added generic PHY driver


* Flash


* GPIO


* Hardware Info


* I2C


* I2S

  * Added Atmel SAM I2S driver support to XDMAC reload


* IEEE 802.15.4


* Interrupt Controller


* LED


* LoRa

  * lora_send now blocks until the transmission is complete. lora_send_async
    can be used for the previous, non-blocking behaviour.

* Modem

  * Added gsm_ppp devicetree support


* Pinmux

  * Added Atmel SAM0 pinctrl support


* PWM


* Sensor


* Serial


* SPI


* Timer


* USB

  * Add Atmel SAM4L USBC device controller


* Watchdog


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


* Devicetree

  * Various compatibles had incorrect vendor prefixes in their :ref:`compatible
    <dt-important-props>` properties; the following changes were made to fix
    these.

    * ``nios,i2c`` is now ``altr,nios2-i2c``
    * ``colorway,lpd8803`` is now ``greeled,lpd8803``
    * ``colorway,lpd8806`` is now ``greeled,lpd8806``
    * ``grove,light`` is now ``seeed,grove-light``
    * ``grove,temperature`` is now ``seeed,grove-temperature``
    * ``max,max30101`` is now ``maxim,max30101``
    * ``ublox,sara-r4`` is now ``u-blox,sara-r4``
    * ``xtensa,core-intc`` is now ``cdns,xtensa-core-intc``

    Out of tree users of these compatibles will need to update their
    devicetrees.

    You can support multiple versions of Zephyr with one devicetree by
    including both the old and new values in your nodes' compatible properties,
    like this example for the LPD8803::

        my-led-strip@0 {
                compatible = "colorway,lpd8803", "greeled,lpd8803";
                ...
        };

* West (extensions)

    * openocd runner: Zephyr thread awareness is now available in GDB by default
      for application builds with :kconfig:`CONFIG_DEBUG_THREAD_INFO` set to ``y``
      in :ref:`kconfig`. This applies to ``west debug``, ``west debugserver``,
      and ``west attach``. OpenOCD version later than 0.11.0 must be installed
      on the host system.


Libraries / Subsystems
**********************

* Disk


* Management


* CMSIS subsystem


* Power management

  * The APIs to set/clear/check if devices are busy from a power management
    perspective have been moved to the PM subsystem. Their naming and signature
    has also been adjusted to follow common conventions. Below you can find the
    equivalence list.

    * ``device_busy_set`` -> ``pm_device_busy_set``
    * ``device_busy_clear`` -> ``pm_device_busy_clear``
    * ``device_busy_check`` -> ``pm_device_is_busy``
    * ``device_any_busy_check`` -> ``pm_device_is_any_busy``

  * The device power management callback (``pm_device_control_callback_t``) has
    been largely simplified to work based on *actions*, resulting in simpler and
    more natural implementations. This principle is also used by other OSes like
    the Linux Kernel. As a result, the callback argument list has been reduced
    to the device instance and an action (e.g. ``PM_DEVICE_ACTION_RESUME``).
    Other improvements include specification of error codes, removal of some
    unused/unclear states, or guarantees such as avoid calling a device for
    suspend/resume if it is already at the right state. All these changes
    together have allowed simplifying multiple device power management callback
    implementations.

* Logging


* Random

  * xoroshiro128+ PRNG deprecated in favor of xoshiro128++

* Shell


* Storage


* Task Watchdog


* Tracing


* Debug

* OS


HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.


Trusted Firmware-m
******************

* Renamed psa_level_1 sample to psa_crypto. Extended the use of the PSA Cryptography
  1.0 API in the sample code to demonstrate additional crypto functionality.
* Added a new sample to showcase the PSA Protecter Storage service.

Documentation
*************

* Kconfig options need to be referenced using the ``:kconfig:`` Sphinx role.
  Previous to this change, ``:option:`` was used for this purpose.
* Doxygen alias ``@config{}`` has been deprecated in favor of ``@kconfig{}``.

Tests and Samples
*****************


Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.6.0 tagged
release:
