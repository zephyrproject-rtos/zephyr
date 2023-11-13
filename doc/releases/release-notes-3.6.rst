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

* x86

* POSIX

Bluetooth
*********

* Audio

* Direction Finding

* Host

* Mesh

* Controller

Boards & SoC Support
********************

* Added support for these SoC series:

* Removed support for these SoC series:

* Made these changes in other SoC series:

  * Nordic SoCs now imply :kconfig:option:`CONFIG_XIP` instead of selecting it, this allows for
    creating RAM-based applicatins by disabling it.

* Added support for these ARC boards:

* Added support for these ARM boards:

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

Drivers and Sensors
*******************

* ADC

* CAN

* Clock control

* Counter

* DAC

* Disk

* Display

* DMA

* EEPROM

* Entropy

* Ethernet

* Flash

* GPIO

* I2C

* I2S

* I3C

  * The Legacy Virtual Register defines have been renamed from ``I3C_DCR_I2C_*``
    to ``I3C_LVR_I2C_*``.

* IEEE 802.15.4

* Interrupt Controller

* Input

* PCIE

* ACPI

* Pin control

* PWM

* Regulators

* Reset

* Retained memory

* RTC

* SDHC

* Sensor

* Serial

* SPI

* Timer

* USB

* WiFi

Networking
**********

* CoAP:

* Connection Manager:

* DHCP:

* Ethernet:

* gPTP:

* ICMP:

* IPv6:

* LwM2M:

* Misc:

* MQTT-SN:

* OpenThread:

* PPP:

* Sockets:

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

  * Fixes an issue in MCUmgr console UART input whereby the FIFO would be read outside of an ISR,
    which is not supported in the next USB stack.

* File systems

* Modem modules

* Power management

* Random

* Retention

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
