:orphan:

.. _zephyr_3.3:

Zephyr 3.3.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.3.0.

Major enhancements with this release include:

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

API Changes
***********

Changes in this release
=======================
* Bluetooth: :kconfig:option:`CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER`
  and :kconfig:option:`CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER` have been
  added to enable the PAST implementation rather than
  :kconfig:option:`CONFIG_BT_CONN`.
* Flashdisk: :kconfig:option:`CONFIG_DISK_FLASH_VOLUME_NAME`,
  :kconfig:option:`CONFIG_DISK_FLASH_DEV_NAME`,
  :kconfig:option:`CONFIG_DISK_FLASH_START`,
  :kconfig:option:`CONFIG_DISK_FLASH_MAX_RW_SIZE`,
  :kconfig:option:`CONFIG_DISK_ERASE_BLOCK_SIZE`,
  :kconfig:option:`CONFIG_DISK_FLASH_ERASE_ALIGNMENT`,
  :kconfig:option:`CONFIG_DISK_VOLUME_SIZE` and
  :kconfig:option:`CONFIG_DISK_FLASH_SECTOR_SIZE` Kconfig options have been
  removed in favor of new :dtcompatible:`zephyr,flash-disk` devicetree binding.

- Starting from this release ``zephyr-`` prefixed tags won't be created
  anymore. The project will continue using ``v`` tags, for example ``v3.3.0``.

Removed APIs in this release
============================

Deprecated in this release
==========================

* MCUmgr subsystem, specifically the SMP transport API, is dropping `zephyr_`
  prefix, deprecating prefixed functions and callback type definitions with the
  prefix and replacing them with with prefix-less variants.
  The :c:struct:`zephyr_smp_transport` type, representing transport object,
  is now replaced with :c:struct:`smp_transport`, and the later one is used,
  instead of the former one, by all prefix-less functions.

  Deprecated functions and their replacements:

  .. table::
     :align: center

     +-------------------------------------+---------------------------------------+
     | Deprecated                          | Drop in replacement                   |
     +=====================================+=======================================+
     | :c:func:`zephyr_smp_transport_init` | :c:func:`smp_transport_init`          |
     +-------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_rx_req`         | :c:func:`smp_rx_req`                  |
     +-------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_alloc_rsp`      | :c:func:`smp_alloc_rsp`               |
     +-------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_free_buf`       | :c:func:`smp_free_buf`                |
     +-------------------------------------+---------------------------------------+

  Deprecated callback types and their replacements:

  .. table::
     :align: center

     +---------------------------------------------+---------------------------------------+
     | Deprecated                                  | Drop in replacement                   |
     +=============================================+=======================================+
     | :c:func:`zephyr_smp_transport_out_fn`       | :c:func:`smp_transport_out_fn`        |
     +---------------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_transport_get_mtu_fn`   | :c:func:`smp_transport_get_mtu_fn`    |
     +---------------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_transport_ud_copy_fn`   | :c:func:`smp_transport_ud_copy_fn`    |
     +---------------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_transport_ud_free_fn`   | :c:func:`smp_transport_ud_free_fn`    |
     +---------------------------------------------+---------------------------------------+

  NOTE: Only functions are marked as ``__deprecated``, type definitions are not.

Stable API changes in this release
==================================

New APIs in this release
========================

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

  * Fixed missing calls to bt_le_per_adv_sync_cb.term when deleting a periodic
    advertising sync object.

* Mesh

  * Change default advertiser to be extended advertiser.

* Controller

* HCI Driver

Boards & SoC Support
********************

* Added support for these SoC series:

* Removed support for these SoC series:

* Made these changes in other SoC series:

* Changes for ARC boards:

* Added support for these ARM boards:

* Added support for these ARM64 boards:

* Removed support for these ARM boards:

* Removed support for these X86 boards:

* Added support for these RISC-V boards:

* Added support for these Xtensa boards:

* Removed support for these Xtensa boards:

* Made these changes in other boards:

* Added support for these following shields:

Build system and infrastructure
*******************************

Drivers and Sensors
*******************

* ADC

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

  * NRF: Added CONFIG_SOC_FLASH_NRF_TIMEOUT_MULTIPLIER to allow tweaking the timeout of flash operations.

* GPIO

* I2C

* I2S

* I3C

* IEEE 802.15.4

* Interrupt Controller

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

* Reset

* SDHC

* Sensor

* Serial

* SPI

* Timer

* USB

* W1

* Watchdog

* WiFi

Networking
**********

USB
***

Devicetree
**********

* Bindings

  * New:

    * :dtcompatible:`zephyr,flash-disk`

Libraries / Subsystems
**********************

* Management

  * MCUmgr functionality deprecated in 3.1 has been removed:
    CONFIG_FS_MGMT_UL_CHUNK_SIZE, CONFIG_IMG_MGMT_UL_CHUNK_SIZE,
    CONFIG_OS_MGMT_ECHO_LENGTH
  * MCUmgr fs_mgmt issue with erasing a file prior to writing the first block
    of data has been worked around by only truncating/deleting the file data
    if the file exists. This can help work around an issue whereby logging is
    enabled and the command is sent on the same UART as the logging system, in
    which a filesystem error was emitted.
  * A MCUmgr bug when using the smp_svr sample with Bluetooth transport that
    could have caused a stack overflow has been fixed.
  * A MCUmgr issue with Bluetooth transport that could cause a deadlock of the
    mcumgr thread if the remote device disconnected before the output message
    was sent has been fixed.

* LwM2M

  * The ``lwm2m_senml_cbor_*`` files have been regenerated using zcbor 0.6.0.

HALs
****

MCUboot
*******

Trusted Firmware-M
******************

zcbor
*****

Upgraded zcbor to 0.6.0. Among other things, this brings in a few convenient
changes for Zephyr:

* In the zcbor codebase, the ``ARRAY_SIZE`` macro has been renamed to
  ``ZCBOR_ARRAY_SIZE`` to not collide with Zephyr's :c:macro:`ARRAY_SIZE` macro.
* The zcbor codebase now better supports being used in C++ code.

The entire release notes can be found at
https://github.com/zephyrproject-rtos/zcbor/blob/0.6.0/RELEASE_NOTES.md

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
