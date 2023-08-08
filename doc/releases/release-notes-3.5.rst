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

New APIs in this release
========================

Kernel
******

Architectures
*************

* ARC

* ARM

  * Fixed the Cortex-A/-R linker command file:

    * The sections for zero-initialized (.bss) and uninitialized (.noinit) data
      are now the last sections within the binary. This allows the linker to just
      account for the required memory, but not having to actually include large
      empty spaces within the binary. With the .bss and .noinit sections placed
      somewhere in the middle of the resulting binary, as was the case with
      previous releases, the linker had to pad the space for zero-/uninitialized
      data due to subsequent sections containing initialized data. The inclusion
      of large zero-initialized arrays or statically defined heaps reflected
      directly in the size of the resulting binary, resulting in unnecessarily
      large binaries, even when stripped.
    * Fixed the location of the z_mapped_start address marker to point to the
      base of RAM instead of to the start of the .text section. Therefore, the
      single 4k page .vectors section, which is located right at the base of RAM
      before the .text section and which was previously not included in the
      mapped memory range, is now considered mapped and unavailable for dynamic
      memory mapping via the MMU at run-time. This prevents the 4k page containing
      the exception vectors data being mapped as regular memory at run-time, with
      any subsequently mapped pages being located beyond the permanently mapped
      memory regions (beyond z_mapped_end), resulting in non-contiguous memory
      allocation for any first memory request greater than 4k.

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

* Removed support for these SoC series:

* Made these changes in other SoC series:

* Added support for these ARC boards:

* Added support for these ARM boards:

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

Drivers and Sensors
*******************

* ADC

* Battery-backed RAM

* CAN

* Clock control

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

* Flash

* FPGA

* Fuel Gauge

* GPIO

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

* PWM

* Power domain

* Regulators

* Reset

* SDHC

* Sensor

* Serial

* SPI

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

* Wi-Fi
  * Added Passive scan support.
  * The Wi-Fi scan API updated with Wi-Fi scan parameter to allow scan mode selection.

USB
***

Devicetree
**********

Libraries / Subsystems
**********************

* Management

  * Added response checking to MCUmgr's :c:enumerator:`MGMT_EVT_OP_CMD_RECV`
    notification callback to allow applications to reject MCUmgr commands.

HALs
****

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

Issue Related Items
*******************

Known Issues
============

Addressed issues
================
