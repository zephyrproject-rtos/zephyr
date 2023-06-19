:orphan:

.. _zephyr_3.5:

Zephyr 3.5.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.5.0.

Major enhancements with this release include:

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

API Changes
***********

Changes in this release
=======================

* Set :kconfig:option:`CONFIG_BOOTLOADER_SRAM_SIZE` default value to ``0`` (was
  ``16``). Bootloaders that use a part of the SRAM should set this value to an
  appropriate size. :github:`60371`

* Time and timestamps in the network subsystem, PTP and IEEE 802.15.4
  were more precisely specified and all in-tree call sites updated accordingly.
  Fields for timed TX and TX/RX timestamps have been consolidated. See
  :c:type:`net_time_t`, :c:struct:`net_ptp_time`, :c:struct:`ieee802154_config`,
  :c:struct:`ieee802154_radio_api` and :c:struct:`net_pkt` for extensive
  documentation. As this is largely an internal API, existing applications will
  most probably continue to work unchanged.

* The Kconfig option CONFIG_GPIO_NCT38XX_INTERRUPT has been renamed to
  :kconfig:option:`CONFIG_GPIO_NCT38XX_ALERT`.

Removed APIs in this release
============================

Deprecated in this release
==========================

* Setting the GIC architecture version by selecting
  :kconfig:option:`CONFIG_GIC_V1`, :kconfig:option:`CONFIG_GIC_V2` and
  :kconfig:option:`CONFIG_GIC_V3` directly in Kconfig has been deprecated.
  The GIC version should now be specified by adding the appropriate compatible, for
  example :dtcompatible:`arm,gic-v2`, to the GIC node in the device tree.

Stable API changes in this release
==================================

* MCUmgr SMP version 2 error codes entry has changed due to a collision with an
  existing response in shell_mgmt. Previously, these errors had the entry ``ret``
  but now have the entry ``err``. ``smp_add_cmd_ret()`` is now deprecated and
  :c:func:`smp_add_cmd_err` should be used instead, ``MGMT_CB_ERROR_RET`` is
  now deprecated and :c:enumerator:`MGMT_CB_ERROR_ERR` should be used instead.
  SMP version 2 error code defines for in-tree modules have been updated to
  replace the ``*_RET_RC_*`` parts with ``*_ERR_*``.

New APIs in this release
========================

* Introduced MCUmgr client support with handlers for img_mgmt and os_mgmt.

Kernel
******

Architectures
*************

* ARM

* ARM

* ARM64

* RISC-V

* Xtensa

Bluetooth
*********

* Audio

* Direction Finding

* Host

* Mesh

* Controller

* HCI Driver

Boards & SoC Support
********************

* Added support for these SoC series:

  * Nuvoton NuMaker M46x series

* Removed support for these SoC series:

* Made these changes in other SoC series:

  * i.MX RT SOCs no longer enable CONFIG_DEVICE_CONFIGURATION_DATA by default.
    boards using external SDRAM should set CONFIG_DEVICE_CONFIGURATION_DATA
    and CONFIG_NXP_IMX_EXTERNAL_SDRAM to enabled.
  * i.MX RT SOCs no longer support CONFIG_OCRAM_NOCACHE, as this functionality
    can be achieved using devicetree memory regions

* Added support for these ARC boards:

* Added support for these ARM boards:

  * Nuvoton NuMaker Platform M467

* Added support for these ARM64 boards:

* Added support for these RISC-V boards:

* Added support for these X86 boards:

* Added support for these Xtensa boards:

* Made these changes for ARC boards:

* Made these changes for ARM boards:

* Made these changes for ARM64 boards:

* Made these changes for RISC-V boards:

* Made these changes for X86 boards:

* Made these changes for Xtensa boards:

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

* SCA (Static Code Analysis)

  * Added support for CodeChecker

Drivers and Sensors
*******************

* ADC

* Battery-backed RAM

* CAN

* Clock control

  * Added support for Nuvoton NuMaker M46x

* Counter

* Crypto

* DAC

* DFU

* Disk

* Display

* DMA

* EEPROM

* Entropy

* ESPI

* Ethernet

  * Added :kconfig:option:`CONFIG_ETH_NATIVE_POSIX_RX_TIMEOUT` to set rx timeout for native posix.

* Flash

  * Introduce npcx flash driver that supports two or more spi nor flashes via a
    single Flash Interface Unit (FIU) module and Direct Read Access (DRA) mode
    for better performance.
  * Added support for Nuvoton NuMaker M46x embedded flash

* FPGA

* Fuel Gauge

* GPIO

  * Added support for Nuvoton NuMaker M46x

* hwinfo

* I2C

* I2S

* I3C

* IEEE 802.15.4

* Interrupt Controller

  * GIC: Architecture version selection is now based on the device tree

* IPM

* KSCAN

* LED

* MBOX

* MEMC

* PCIE

* PECI

* Pin control

  * Added support for Nuvoton NuMaker M46x

* PWM

* Power domain

* Regulators

* Reset

  * Added support for Nuvoton NuMaker M46x

* SDHC

* Sensor

* Serial

  * Added support for Nuvoton NuMaker M46x

* SPI

  * Remove npcx spi driver implemented by Flash Interface Unit (FIU) module.

* Timer

  * The TI CC13xx/26xx system clock timer compatible was changed from
    :dtcompatible:`ti,cc13xx-cc26xx-rtc` to :dtcompatible:`ti,cc13xx-cc26xx-rtc-timer`
    and the corresponding Kconfig option from :kconfig:option:`CC13X2_CC26X2_RTC_TIMER`
    to :kconfig:option:`CC13XX_CC26XX_RTC_TIMER` for improved consistency and
    extensibility. No action is required unless the internal timer was modified.

* USB

* W1

* Watchdog

* WiFi

Networking
**********

* CoAP:

  * Use 64 bit timer values for calculating transmission timeouts. This fixes potential problems for
    devices that stay on for more than 49 days when the 32 bit uptime counter might roll over and
    cause CoAP packets to not timeout at all on this event.

* LwM2M:

  * Added support for tickless mode. This removes the 500 ms timeout from the socket loop
    so the engine does not constantly wake up the CPU. This can be enabled by
    :kconfig:option:`CONFIG_LWM2M_TICKLESS`.

* Wi-Fi
  * Added Passive scan support.
  * The Wi-Fi scan API updated with Wi-Fi scan parameter to allow scan mode selection.

USB
***

Devicetree
**********

* ``zephyr,memory-region-mpu`` was renamed ``zephyr,memory-attr``

* The following macros were added:
  :c:macro:`DT_FOREACH_NODE_VARGS`,
  :c:macro:`DT_FOREACH_STATUS_OKAY_NODE_VARGS`
  :c:macro:`DT_MEMORY_ATTR_FOREACH_NODE`
  :c:macro:`DT_MEMORY_ATTR_APPLY`
  :c:macro:`DT_MEM_FROM_FIXED_PARTITION`
  :c:macro:`DT_FIXED_PARTITION_ADDR`

Libraries / Subsystems
**********************

* Management

  * Added response checking to MCUmgr's :c:enumerator:`MGMT_EVT_OP_CMD_RECV`
    notification callback to allow applications to reject MCUmgr commands.

  * MCUmgr SMP version 2 error translation (to legacy MCUmgr error code) is now
    supported in function handlers by setting ``mg_translate_error`` of
    :c:struct:`mgmt_group` when registering a transport. See
    :c:type:`smp_translate_error_fn` for function details.

  * Fixed an issue with MCUmgr img_mgmt group whereby the size of the upload in
    the initial packet was not checked.

  * Fixed an issue with MCUmgr fs_mgmt group whereby some status codes were not
    checked properly, this meant that the error returned might not be the
    correct error, but would only occur in situations where an error was
    already present.

  * Fixed an issue whereby the SMP response function did not check to see if
    the initial zcbor map was created successfully.

  * Fixes an issue with MCUmgr shell_mgmt group whereby the length of a
    received command was not properly checked.

  * Added optional mutex locking support to MCUmgr img_mgmt group, which can
    be enabled with :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_MUTEX`.

  * Added MCUmgr settings management group, which allows for manipulation of
    zephyr settings from a remote device, see :ref:`mcumgr_smp_group_3` for
    details.

* File systems

  * Added support for ext2 file system.

HALs
****

* Nuvoton

  * Added Nuvoton NuMaker M46x

MCUboot
*******

Storage
*******

Trusted Firmware-M
******************

Trusted Firmware-A
******************

* Updated to TF-A 2.9.0.

zcbor
*****

Documentation
*************

Tests and Samples
*****************

* Created common sample for file systems (`fs_sample`). It originates from sample for FAT
  (`fat_fs`) and supports both FAT and ext2 file systems.

Known Issues
************
