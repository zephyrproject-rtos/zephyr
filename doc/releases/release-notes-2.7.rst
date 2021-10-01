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

  * Added STM32U5 basic SoC support

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
  * Added low power support to STM32L0, STM32G0 and STM32WL series
  * STM32: Enabled ART Flash accelerator by default when available (F2, F4, F7, H7, L5)
  * STM32: Added Kconfig option to enable STM32Cube asserts (CONFIG_USE_STM32_ASSERT)


* Changes for ARC boards:


* Changes for ARM boards:

  * Added SPI support on Arduino standard SPI when possible

* Added support for these ARM boards:

  * Dragino NBSN95 NB-IoT Sensor Node
  * Seeedstudio LoRa-E5 Dev Board
  * ST B_U585I_IOT02A Discovery kit
  * ST Nucleo F446ZE
  * ST Nucleo U575ZI Q
  * ST STM32H735G Discovery

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

  * 4.2inch epaper display (GDEW042T2)
  * X-NUCLEO-EEPRMA2 EEPROM memory expansion board

Drivers and Sensors
*******************

* ADC

  * Added STM32WL ADC driver
  * STM32: Added support for oversampling

* Bluetooth


* CAN


* Clock Control


* Console


* Counter

  * Add Atmel SAM counter (TC) Driver
  * Added STM32WL RTC counter driver

* Crypto

  * STM23: Add support for SOCs with AES hardware block (STM32G0, STM32L5 and STM32WL)

* DAC

  * Added Atmel SAM DAC (DACC) driver
  * Added support for Microchip MCP4725
  * Added support for STM32F3 series

* Disk

  * Added SDMMC support on STM32L4+

* Display

  * Added support for ST7735R

* Disk

  * STM32 SDMMC now supports SDIO enabled devices

* DMA

  * Added Atmel SAM XDMAC reload support
  * Added support on STM32F3, STM32G0, STM32H7 and STM32L5
  * STM32: Reviewed bindings definitions, "st,stm32-dma-v2bis" introduced.


* EEPROM

  * Added support for EEPROM emulated in flash.

* Entropy

  * Added support for STM32F2, STM32G0, STM32WB and STM32WL

* ESPI

  * Added support for Microchip eSPI SAF

* Ethernet

  * Added Atmel SAM/SAM0 GMAC devicetree support
  * Added Atmel SAM/SAM0 MDIO driver
  * Added MDIO driver
  * Added generic PHY driver


* Flash

  * Added STM32F2, STM32L5 and STM32WL Flash driver support
  * STM32: Max erase time parameter was moved to device tree
  * Added quad SPI support for STM32F4

* GPIO


* Hardware Info


* I2C


* I2S

  * Added Atmel SAM I2S driver support to XDMAC reload


* IEEE 802.15.4

* IPM

  * Added STM32 HSEM IPM driver.

* Interrupt Controller


* IPM

  * STM32: Add HSEM based IPM driver for STM32H7 series

* LED


* LoRa

  * lora_send now blocks until the transmission is complete. lora_send_async
    can be used for the previous, non-blocking behaviour.
  * Enabled support for STM32WL series

* MEMC

  * Added STM32F4 support


* Modem

  * Added gsm_ppp devicetree support

* PCI/PCIe

  * Fixed an issue that MSI-X was used even though it is not available.
  * Improved MBAR retrieval for MSI-X.
  * Added ability to retrieve extended PCI/PCIe capabilities.

* Pinmux

  * Added Atmel SAM0 pinctrl support
  * STM32: Deprecated definitions like 'STM32F2_PINMUX_FUNC_PA0_UART4_TX'
    are now removed.


* PWM

  * Property "st,prescaler" of binding "st,stm32-pwm" now defaults to "0".


* Sensor


* Serial

  * Added kconfig to disable runtime re-configuration of UART
    to reduce footprint if so desired.
  * Added ESP32-C3 polling only UART driver.
  * Added ESP32-S2 polling only UART driver.
  * Added Microchip XEC UART driver.

* SPI


* Timer


* USB

  * Add Atmel SAM4L USBC device controller


* Watchdog

  * Added STM32L5 watchdog support


* WiFi


Networking
**********

* 802.15.4 L2:

  * Fixed a bug, where the net_pkt structure contained invalid LL address
    pointers after being processed by 802.15.4 L2.
  * Added an optional destination address filtering in the 802.15.4 L2.

* CoAP:

  * Added ``user_data`` field to the :c:struct:`coap_packet` structure.
  * Fixed processing of out-of-order notifications.
  * Fixed :c:func:`coap_packet_get_payload` function.
  * Converted CoAP test suite to ztest API.
  * Improved :c:func:`coap_packet_get_payload` function to minimize number
    of RNG calls.
  * Fixed retransmissions in the ``coap_server`` sample.
  * Fixed observer removal in the ``coap_server`` sample (on notification
    timeout).

* DHCPv4:

  * Fixed a bug, where DHPCv4 library removed statically configured gateway
    before obtaining a new one from the server.

* DNS:

  * Fixed a bug, where the same IP address was used to populate the result
    address info entries, when multiple IP addresses were obtained from the
    server.

* HTTP:

  * Switched the library to use ``zsock_*`` API, to improve compatibility with
    various POSIX configurations.
  * Fixed a bug, where ``HTTP_DATA_FINAL`` notification was triggered even for
    intermediate response fragments.

* IPv6:

  * Multiple IPv6 fixes, addressing failures in IPv6Ready compliance tests.

* LwM2M:

  * Added support for notification timeout reporting to the application.
  * Fixed a bug, where a multi instance resource with only one active instance
    was incorrectly encoded on reads.
  * Fixed a bug, where notifications were generated on changes to non-readable
    resources.
  * Added mutex protection  for the state variable of the ``lwm2m_rd_client``
    module.
  * Removed LWM2M_RES_TYPE_U64 type, as it's not possible to encode it properly
    for large values.
  * Fixed a bug, where large unsigned integers were incorrectly encoded in TLV.
  * Multiple fixes for FLOAT type processing in the LwM2M engine and encoders.
  * Fix a bug, where IPSO Push Button counter resource was not triggering
    notification on incrementation.
  * Fixed a bug, where Register failures were reported as success to the
    application.

* Misc:

  * Added RX/TX timeout on a socket in ``big_http_download`` sample.
  * Introduced :c:func:`net_pkt_remove_tail` function.
    Added IEEE 802.15.4 security-related flags to the :c:struct:`net_pkt`
    structure.
  * Added bridging support to the Ethernet L2.
  * Fixed a bug in mDNS, where an incorrect address type could be set as a
    response destination.
  * Added an option to suppress ICMP destination unreachable errors.
  * Fixed possible assertion in ``net nbr`` shell command.
  * Major refactoring of the TFTP library.

* MQTT:

  * Added an option to register a custom transport type.
  * Fixed a bug in :c:func:`mqtt_abort`, where the function could return without
    releasing a lock.

* OpenThread:

  * Update OpenThread module up to commit ``9ea34d1e2053b6b2a80e1d46b65a6aee99fc504a``.
    Added several new Kconfig options to align with new OpenThread
    configurations.
  * Added OpenThread API mutex protection during initialization.
  * Converted OpenThread thread to a dedicated work queue.
  * Implemented missing :c:func:`otPlatAssertFail` platform function.
  * Fixed a bug, where NONE level OpenThread logs were not processed.
  * Added possibility to disable CSL sampling, when used.
  * Fixed a potential bug, where invalid error code could be returned by the
    platform radio layer to OpenThread.
  * Reworked UART configuration in the OpenThread Coprocessor sample.

* Socket:

  * Added microsecond accuracy in :c:func:`zsock_select` function.
  * Reworked :c:func:`zsock_select` into a syscall.
  * Fixed a bug, where :c:func:`poll` events were not signalled correctly
    for socketpair sockets.
  * Fixed a bug, where socket mutex could be used after being initialized by a
    new owner after being deallocated in :c:func:`zsock_close`.
  * Fixed a possible assert after enabling CAN sockets.
  * Fixed IPPROTO_RAW usage in packet socket implementation.

* TCP:

  * Fixed a bug, where ``unacked_len`` could be set to a negative value.
  * Fixed possible assertion failure in :c:func:`tcp_send_data`.
  * Fixed a bug, where [FIN, PSH, ACK] was not handled properly in
    TCP_FIN_WAIT_2 state.

* TLS:

  * Reworked TLS sockets to use secure random generator from Zephyr.
  * Fixed busy looping during DTLS handshake with offloaded sockets.
  * Fixed busy looping during TLS/DTLS handshake on non blocking sockets.
  * Reset mbed TLS session on timed out DTLS handshake, to allow a retry without
    closing a socket.
  * Fixed TLS/DTLS :c:func:`sendmsg` implementation for larger payloads.
  * Fixed TLS/DTLS sockets ``POLLHUP`` notification.

* WebSocket:

  * Fixed :c:func:`poll` implementation for WebSocket, which did not work
    correctly with offloaded sockets.
  * Fixed :c:func:`ioctl` implementation for WebSocket, which did not work
    correctly with offloaded sockets.

USB
***


Build and Infrastructure
************************

* Devicetree API

  * New "for-each" macros which work like existing APIs, but take variable
    numbers of arguments: :c:macro:`DT_FOREACH_CHILD_VARGS`,
    :c:macro:`DT_FOREACH_CHILD_STATUS_OKAY_VARGS`,
    :c:macro:`DT_FOREACH_PROP_ELEM_VARGS`,
    :c:macro:`DT_INST_FOREACH_CHILD_VARGS`,
    :c:macro:`DT_INST_FOREACH_STATUS_OKAY_VARGS`,
    :c:macro:`DT_INST_FOREACH_PROP_ELEM_VARGS`

  * Other new "for-each" macros: :c:macro:`DT_FOREACH_STATUS_OKAY`,
    :c:macro:`DT_FOREACH_STATUS_OKAY_VARGS`

  * New macros for converting strings to C tokens: :c:macro:`DT_STRING_TOKEN`,
    :c:macro:`DT_STRING_UPPER_TOKEN`

  * New :ref:`devicetree-pinctrl-api` helper macros

* Devicetree tooling

  * Errors are now generated when invalid YAML files are discovered while
    searching for bindings. See :ref:`dt-where-bindings-are-located` for
    information on the search path.

  * File names ending in ``.yml`` are now considered YAML files when searching
    for bindings.

  * Errors are now generated if invalid node names are used. For example, the
    node name ``node?`` now generates an error message ending in ``node?: Bad
    character '?' in node name``. The valid node names are documented in
    "2.2.2 Node Names" of the Devicetree specification v0.3.

  * Warnings are now generated if a :ref:`compatible property
    <dt-important-props>` in the ``vendor,device`` format uses an unknown
    vendor prefix. This warning does not apply to the root node.

    Known vendor prefixes are defined in
    :file:`dts/bindings/vendor-prefixes.txt` files, which may appear in any
    directory in :ref:`DTS_ROOT <dts_root>`.

    These may be upgraded to errors using the edtlib Python APIs; Zephyr's CI
    now generates such errors.

* Devicetree bindings

  * Various bindings had incorrect vendor prefixes in their :ref:`compatible
    <dt-important-props>` properties; the following changes were made to fix
    these.

    .. list-table::
       :header-rows: 1

       - * Old compatible
         * New compatible
       - * ``nios,i2c``
         * :dtcompatible:`altr,nios2-i2c`
       - * ``cadence,tensilica-xtensa-lx4``
         * :dtcompatible:`cdns,tensilica-xtensa-lx4`
       - * ``cadence,tensilica-xtensa-lx6``
         * :dtcompatible:`cdns,tensilica-xtensa-lx6`
       - * ``colorway,lpd8803``
         * :dtcompatible:`greeled,lpd8803`
       - * ``colorway,lpd8806``
         * :dtcompatible:`greeled,lpd8806`
       - * ``grove,light``
         * :dtcompatible:`seeed,grove-light`
       - * ``grove,temperature``
         * :dtcompatible:`seeed,grove-temperature`
       - * ``max,max30101``
         * :dtcompatible:`maxim,max30101`
       - * ``ublox,sara-r4``
         * :dtcompatible:`u-blox,sara-r4`
       - * ``xtensa,core-intc``
         * :dtcompatible:`cdns,xtensa-core-intc`
       - * ``vexriscv,intc0``
         * :dtcompatible:`vexriscv-intc0`

    Out of tree users of these bindings will need to update their
    devicetrees.

    You can support multiple versions of Zephyr with one devicetree by
    including both the old and new values in your nodes' compatible properties,
    like this example for the LPD8803::

        my-led-strip@0 {
                compatible = "colorway,lpd8803", "greeled,lpd8803";
                ...
        };

  * Other new bindings in alphabetical order: :dtcompatible:`andestech,atcgpio100`,
    :dtcompatible:`arm,gic-v3-its`, :dtcompatible:`atmel,sam0-gmac`,
    :dtcompatible:`atmel,sam0-pinctrl`, :dtcompatible:`atmel,sam-dac`,
    :dtcompatible:`atmel,sam-mdio`, :dtcompatible:`atmel,sam-usbc`,
    :dtcompatible:`cdns,tensilica-xtensa-lx7`,
    :dtcompatible:`espressif,esp32c3-uart`,
    :dtcompatible:`espressif,esp32-intc`,
    :dtcompatible:`espressif,esp32s2-uart`, :dtcompatible:`ethernet-phy`,
    :dtcompatible:`fcs,fxl6408`, :dtcompatible:`ilitek,ili9341`,
    :dtcompatible:`ite,it8xxx2-bbram`, :dtcompatible:`ite,it8xxx2-kscan`,
    :dtcompatible:`ite,it8xxx2-pinctrl-conf`, :dtcompatible:`ite,it8xxx2-pwm`,
    :dtcompatible:`ite,it8xxx2-pwmprs`, :dtcompatible:`ite,it8xxx2-watchdog`,
    :dtcompatible:`lm75`, :dtcompatible:`lm77`, :dtcompatible:`meas,ms5607`,
    :dtcompatible:`microchip,ksz8863`, :dtcompatible:`microchip,mcp7940n`,
    :dtcompatible:`microchip,xec-adc-v2`, :dtcompatible:`microchip,xec-ecia`,
    :dtcompatible:`microchip,xec-ecia-girq`,
    :dtcompatible:`microchip,xec-gpio-v2`,
    :dtcompatible:`microchip,xec-i2c-v2`, :dtcompatible:`microchip,xec-pcr`,
    :dtcompatible:`microchip,xec-uart`, :dtcompatible:`nuvoton,npcx-bbram`,
    :dtcompatible:`nuvoton,npcx-booter-variant`,
    :dtcompatible:`nuvoton,npcx-ps2-channel`,
    :dtcompatible:`nuvoton,npcx-ps2-ctrl`, :dtcompatible:`nuvoton,npcx-soc-id`,
    :dtcompatible:`nxp,imx-ccm-rev2`, :dtcompatible:`nxp,lpc-ctimer`,
    :dtcompatible:`nxp,lpc-uid`, :dtcompatible:`nxp,mcux-usbd`,
    :dtcompatible:`nxp,sctimer-pwm`, :dtcompatible:`ovti,ov2640`,
    :dtcompatible:`renesas,rcar-can`, :dtcompatible:`renesas,rcar-i2c`,
    :dtcompatible:`reserved-memory`, :dtcompatible:`riscv,sifive-e24`,
    :dtcompatible:`sensirion,sgp40`, :dtcompatible:`sensirion,sht4x`,
    :dtcompatible:`sensirion,shtcx`, :dtcompatible:`silabs,si7055`,
    :dtcompatible:`silabs,si7210`, :dtcompatible:`snps,creg-gpio`,
    :dtcompatible:`st,i3g4250d`, :dtcompatible:`st,stm32-aes`,
    :dtcompatible:`st,stm32-dma`, :dtcompatible:`st,stm32-dma-v2bis`,
    :dtcompatible:`st,stm32-hsem-mailbox`, :dtcompatible:`st,stm32-nv-flash`,
    :dtcompatible:`st,stm32-spi-subghz`,
    :dtcompatible:`st,stm32u5-flash-controller`,
    :dtcompatible:`st,stm32u5-msi-clock`, :dtcompatible:`st,stm32u5-pll-clock`,
    :dtcompatible:`st,stm32u5-rcc`, :dtcompatible:`st,stm32wl-hse-clock`,
    :dtcompatible:`st,stm32wl-subghz-radio`, :dtcompatible:`st,stmpe1600`,
    :dtcompatible:`syscon`, :dtcompatible:`telink,b91`,
    :dtcompatible:`telink,b91-flash-controller`,
    :dtcompatible:`telink,b91-gpio`, :dtcompatible:`telink,b91-i2c`,
    :dtcompatible:`telink,b91-pinmux`, :dtcompatible:`telink,b91-power`,
    :dtcompatible:`telink,b91-pwm`, :dtcompatible:`telink,b91-spi`,
    :dtcompatible:`telink,b91-trng`, :dtcompatible:`telink,b91-uart`,
    :dtcompatible:`telink,b91-zb`, :dtcompatible:`ti,hdc2010`,
    :dtcompatible:`ti,hdc2021`, :dtcompatible:`ti,hdc2022`,
    :dtcompatible:`ti,hdc2080`, :dtcompatible:`ti,hdc20xx`,
    :dtcompatible:`ti,ina219`, :dtcompatible:`ti,ina23x`,
    :dtcompatible:`ti,tca9538`, :dtcompatible:`ti,tca9546a`,
    :dtcompatible:`ti,tlc59108`,
    :dtcompatible:`xlnx,gem`, :dtcompatible:`zephyr,bbram-emul`,
    :dtcompatible:`zephyr,cdc-acm-uart`, :dtcompatible:`zephyr,gsm-ppp`,
    :dtcompatible:`zephyr,native-posix-udc`

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
