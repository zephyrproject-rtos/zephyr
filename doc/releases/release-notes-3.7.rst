:orphan:

.. _zephyr_3.7:

Zephyr 3.7.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.7.0.

Major enhancements with this release include:

* A new, completely overhauled hardware model has been introduced. This changes
  the way both SoCs and boards are named, defined and constructed in Zephyr.
  Additional information can be found in the :ref:`board_porting_guide`.

An overview of the changes required or recommended when migrating your application from Zephyr
v3.6.0 to Zephyr v3.7.0 can be found in the separate :ref:`migration guide<migration_3.7>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Architectures
*************

* ARC

* ARM

* Xtensa

Bluetooth
*********

  * Added Nordic UART Service (NUS), enabled by the :kconfig:option:`CONFIG_BT_NUS`.
    This Service exposes the ability to declare multiple instances of the GATT service,
    allowing multiple serial endpoints to be used for different purposes.

Boards & SoC Support
********************

* Added support for these SoC series:

* Made these changes in other SoC series:

* Added support for these ARM boards:

* Added support for these Xtensa boards:

* Made these changes for ARM boards:

* Made these changes for RISC-V boards:

* Made these changes for native/POSIX boards:

* Added support for these following shields:

Build system and Infrastructure
*******************************

Drivers and Sensors
*******************

* ADC

* Auxiliary Display

* Audio

* Battery

  * Added ``re-charge-voltage-microvolt`` property to the ``battery`` binding. This allows to set
    limit to automatically start charging again.

* Battery backed up RAM

* CAN

  * Deprecated the :c:func:`can_calc_prescaler` API function, as it allows for bitrate
    errors. Bitrate errors between nodes on the same network leads to them drifting apart after the
    start-of-frame (SOF) synchronization has taken place, leading to bus errors.
  * Extended support for automatic sample point location to also cover :c:func:`can_calc_timing` and
    :c:func:`can_calc_timing_data`.
  * Added optional ``min-bitrate`` devicetree property for CAN transceivers.
  * Added devicetree macros :c:macro:`DT_CAN_TRANSCEIVER_MIN_BITRATE` and
    :c:macro:`DT_INST_CAN_TRANSCEIVER_MIN_BITRATE` for getting the minimum supported bitrate of a CAN
    transceiver.
  * Added support for specifying the minimum bitrate supported by a CAN controller in the internal
    ``CAN_DT_DRIVER_CONFIG_GET`` and ``CAN_DT_DRIVER_CONFIG_INST_GET`` macros.
  * Added a new CAN controller API function :c:func:`can_get_min_bitrate` for getting the minimum
    supported bitrate of a CAN controller/transceiver combination.
  * Updated the CAN timing functions to take the minimum supported bitrate into consideration when
    validating the bitrate.
  * Made the ``sample-point`` and ``sample-point-data`` devicetree properties optional.

* Charger

  * Added ``chgin-to-sys-current-limit-microamp`` property to ``maxim,max20335-charger``.
  * Added ``system-voltage-min-threshold-microvolt`` property to ``maxim,max20335-charger``.
  * Added ``re-charge-threshold-microvolt`` property to ``maxim,max20335-charger``.
  * Added ``thermistor-monitoring-mode`` property to ``maxim,max20335-charger``.

* Clock control

* Counter

* Crypto

* Display

* DMA

* Entropy

* Ethernet

* Flash

* GNSS

* GPIO

* I2C

* I2S

* I3C

* IEEE 802.15.4

* Input

* MDIO

* MFD

* PCIE

* MEMC

* MIPI-DBI

* Pin control

* PWM

* Regulators

* Retained memory

* RTC

* SMBUS:

* SDHC

* Sensor

* Serial

  * Added driver to support UART over Bluetooth LE using NUS (Nordic UART Service). This driver
    enables using Bluetooth as a transport to all the subsystems that are currently supported by
    UART (e.g: Console, Shell, Logging).

* SPI

* USB

* W1

* Wi-Fi

Networking
**********

* DHCPv4:

  * Added support for encapsulated vendor specific options. By enabling
    :kconfig:option:`CONFIG_NET_DHCPV4_OPTION_CALLBACKS_VENDOR_SPECIFIC` callbacks can be
    registered with :c:func:`net_dhcpv4_add_option_vendor_callback` to handle these options after
    being initialised with :c:func:`net_dhcpv4_init_option_vendor_callback`.

  * Added support for the "Vendor class identifier" option. Use the
    :kconfig:option:`CONFIG_NET_DHCPV4_VENDOR_CLASS_IDENTIFIER` to enable it and
    :kconfig:option:`CONFIG_NET_DHCPV4_VENDOR_CLASS_IDENTIFIER_STRING` to set it.

  * The NTP server from the DHCPv4 option can now be used to set the system time. This is done by
    default, if :kconfig:option:`CONFIG_NET_CONFIG_CLOCK_SNTP_INIT` is enabled.

* LwM2M:

  * Added new API function:

    * :c:func:`lwm2m_set_bulk`

USB
***

Devicetree
**********

Libraries / Subsystems
**********************

* Management

* Logging

  * By enabling :kconfig:option:`CONFIG_LOG_BACKEND_NET_USE_DHCPV4_OPTION`, the IP address of the
    syslog server for the networking backend is set by the DHCPv4 Log Server Option (7).

* Modem modules

* Picolibc

* Power management

* Crypto

* Retention

* SD

* Storage

* POSIX API

* LoRa/LoRaWAN

* ZBus

HALs
****

* STM32

MCUboot
*******

zcbor
*****

LVGL
****

Tests and Samples
*****************

  * Added snippet for easily enabling UART over Bluetooth LE by passing ``-S nus-console`` during
    ``west build``. This snippet sets the :kconfig:option:`CONFIG_BT_NUS_AUTO_START_BLUETOOTH`
    which allows non-Bluetooth samples that use the UART APIs to run without modifications
    (e.g: Console and Logging examples).
