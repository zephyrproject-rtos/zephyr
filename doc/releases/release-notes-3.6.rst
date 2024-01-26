:orphan:

.. _zephyr_3.6:

Zephyr 3.6.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.6.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v3.5.0 to Zephyr v3.6.0 can be found in the separate :ref:`migration guide<migration_3.6>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Kernel
******

Architectures
*************

* ARC

* ARM

* ARM64

* RISC-V

* Xtensa

  * Removed the unused Kconfig option ``CONFIG_XTENSA_NO_IPC``.

* x86

* POSIX

Bluetooth
*********

* Audio

* Direction Finding

* Host

* Mesh

  * Added the delayable messages functionality to apply random delays for
    the transmitted responses on the Access layer.
    The functionality is enabled by the :kconfig:option:`CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG`
    Kconfig option.
  * The Bluetooth Mesh Protocol 1.1 is now supported by default.

* Controller

Boards & SoC Support
********************

* Added support for these SoC series:

  * Added support for Renesas R-Car Gen4 series

* Removed support for these SoC series:

* Made these changes in other SoC series:

  * Nordic SoCs now imply :kconfig:option:`CONFIG_XIP` instead of selecting it, this allows for
    creating RAM-based applications by disabling it.

* Added support for these ARC boards:

* Added support for these ARM boards:

  * Added support for Renesas R-Car Spider board CR52: ``rcar_spider_cr52``

* Added support for these ARM64 boards:

* Added support for these RISC-V boards:

* Added support for these X86 boards:

* Added support for these Xtensa boards:

* Added support for these POSIX boards:

* Made these changes for ARC boards:

* Made these changes for ARM boards:

* Made these changes for ARM64 boards:

* Made these changes for RISC-V boards:

* Made these changes for X86 boards:

* Made these changes for Xtensa boards:

* Made these changes for POSIX boards:

* Removed support for these ARC boards:

* Removed support for these ARM boards:

* Removed support for these ARM64 boards:

* Removed support for these RISC-V boards:

* Removed support for these X86 boards:

* Removed support for these Xtensa boards:

* Made these changes in other boards:

* Added support for these following shields:

Build system and infrastructure
*******************************

* Dropped the ``COMPAT_INCLUDES`` option, it was unused since 3.0.

* Fixed an issue whereby board revision ``0`` did not include overlay files for that revision.

* Added ``PRE_IMAGE_CMAKE`` and ``POST_IMAGE_CMAKE`` hooks to sysbuild modules, which allows for
  modules to run code after and before each image's cmake invocation.

* Added :kconfig:option:`CONFIG_ROM_END_OFFSET` option which allows reducing the size of an image,
  this is intended for use with firmware signing scripts which add additional data to the end of
  images outside of the build itself.

* Added MCUboot image size reduction to sysbuild images which include MCUboot which prevents
  issues with building firmware images that are too large for MCUboot to swap.

* Deprecated :kconfig:option:`CONFIG_BOOTLOADER_SRAM_SIZE`, users of this should transition to
  having RAM set up properly in their board devicetree files.

* Fixed an issue whereby shields were processed in order of the root they resided in rather than
  the order they were supplied to cmake in.

* Fixed an issue whereby using some shields with sysbuild would cause a cmake Kconfig error.

Drivers and Sensors
*******************

* ADC

* CAN

  * Added system call :c:func:`can_get_mode()` for getting the current operation mode of a CAN
    controller.

  * Add system call :c:func:`can_get_transceiver()` for getting the CAN transceiver associated with
    a CAN controller.

* Clock control

  * Renesas R-Car clock control driver now supports Gen4 SoCs
  * Renamed ``CONFIG_CLOCK_CONTROL_RA`` to :kconfig:option:`CONFIG_CLOCK_CONTROL_RENESAS_RA`

* Counter

* DAC

* Disk

* Display

* DMA

* EEPROM

* Entropy

* Ethernet

* Flash

  * ``spi_nor`` driver now sleeps between polls in ``spi_nor_wait_until_ready``. If this is not
    desired (For example due to ROM constraints in a bootloader),
    :kconfig:option:`CONFIG_SPI_NOR_SLEEP_WHILE_WAITING_UNTIL_READY` can be disabled.

* GPIO

  * Renesas R-Car GPIO driver now supports Gen4 SoCs
  * Renamed ``CONFIG_GPIO_RA`` to :kconfig:option:`CONFIG_GPIO_RENESAS_RA`

* I2C

* I2S

* I3C

  * The Legacy Virtual Register defines have been renamed from ``I3C_DCR_I2C_*``
    to ``I3C_LVR_I2C_*``.

* IEEE 802.15.4

  * Removed :kconfig:option:`CONFIG_IEEE802154_SELECTIVE_TXPOWER` Kconfig option.

* Interrupt Controller

* Input

* PCIE

* ACPI

* Pin control

  * Renesas R-Car pinctrl driver now supports Gen4 SoCs
  * Renamed ``CONFIG_PINCTRL_RA`` to :kconfig:option:`CONFIG_PINCTRL_RENESAS_RA`

* PWM

* Regulators

* Reset

* Retained memory

  * Retained memory driver backend for registers has been added.

  * Retained memory API status changed from experimental to unstable.

* RTC

* SDHC

* Sensor

* Serial

  * Renamed ``CONFIG_UART_RA`` to :kconfig:option:`CONFIG_UART_RENESAS_RA`

* SPI

* Timer

* USB

* WiFi

Networking
**********

* CoAP:

  * Emit observer/service network events using the Network Event subsystem.

  * Added new API functions:

    * :c:func:`coap_get_transmission_parameters`
    * :c:func:`coap_set_transmission_parameters`

* Connection Manager:

* DHCP:

* Ethernet:

* gPTP:

* ICMP:

* IPv6:

* LwM2M:

* Misc:

  * It is now possible to have separate IPv4 TTL value and IPv6 hop limit value for
    unicast and multicast packets. This can be controlled in each socket via
    :c:func:`setsockopt` API.

  * Added support for compile time network event handlers using the macro
    :c:macro:`NET_MGMT_REGISTER_EVENT_HANDLER`.

  * The :kconfig:option:`CONFIG_NET_MGMT_EVENT_WORKER` choice is added to
    allow emitting network events using the system work queue or synchronously.

* MQTT-SN:

* OpenThread:

* PPP:

* Sockets:

  * Added support for IPv4 multicast ``IP_ADD_MEMBERSHIP`` and ``IP_DROP_MEMBERSHIP`` socket options.
  * Added support for IPv6 multicast ``IPV6_ADD_MEMBERSHIP`` and ``IPV6_DROP_MEMBERSHIP`` socket options.

* TCP:

* TFTP:

* WebSocket

* Wi-Fi:


USB
***

Devicetree
**********

API
===

Bindings
========

Libraries / Subsystems
**********************

* Management

  * Fixed an issue in MCUmgr image management whereby erasing an already erased slot would return
    an unknown error, it now returns success.

  * Fixed MCUmgr UDP transport structs being statically initialised, this results in about a
    ~5KiB flash saving.

  * Fixed an issue in MCUmgr which would cause a user data buffer overflow if the UDP transport was
    enabled on IPv4 only but IPv6 support was enabled in the kernel.

  * Implemented datetime functionality in MCUmgr OS management group, this makes use of the RTC
    driver API.

  * Fixed an issue in MCUmgr console UART input whereby the FIFO would be read outside of an ISR,
    which is not supported in the next USB stack.

  * Fixed an issue whereby the ``mcuboot erase`` DFU shell command could be used to erase the
    MCUboot or currently running application slot.

  * Fixed an issue whereby messages that were too large to be sent over the UDP transport would
    wrongly return :c:enum:`MGMT_ERR_EINVAL` instead of :c:enum:`MGMT_ERR_EMSGSIZE`.

* File systems

* Modem modules

* Power management

* Random

* Retention

  * Fixed issue whereby :kconfig:option:`CONFIG_RETENTION_BUFFER_SIZE` values over 256 would cause
    an infinite loop due to use of 8-bit variables.

* Storage

  * File systems: LittleFS module has been updated to version 2.8.1.

* Binary descriptors

* POSIX API

* LoRa/LoRaWAN

* CAN ISO-TP

* RTIO

* ZBus

  * Renamed :kconfig:option:`ZBUS_MSG_SUBSCRIBER_NET_BUF_DYNAMIC` and
    :kconfig:option:`ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC`
    with :kconfig:option:`ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC` and
    :kconfig:option:`ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_STATIC`

HALs
****

MCUboot
*******

Nanopb
******

zcbor
*****

zcbor has been updated from 0.7.0 to 0.8.0.
Full release notes can be found at:
https://github.com/zephyrproject-rtos/zcbor/blob/0.8.0/RELEASE_NOTES.md

Highlights:

* Add support for unordered maps
* Performance improvements
* Naming improvements for generated code
* Bugfixes

LVGL
****

Trusted Firmware-A
******************

Documentation
*************

Tests and Samples
*****************

* :ref:`native_sim<native_sim>` has replaced :ref:`native_posix<native_posix>` as the default
  test platform.
  :ref:`native_posix<native_posix>` remains supported and used in testing but will be deprecated
  in a future release.

* Fixed an issue in :zephyr:code-sample:`smp-svr` sample whereby if USB was already initialised,
  application would fail to boot properly.
