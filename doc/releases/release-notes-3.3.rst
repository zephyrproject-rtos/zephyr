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

* Mesh

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

Libraries / Subsystems
**********************

* Management

  * MCUMGR functionality deprecated in 3.1 has been removed:
    CONFIG_FS_MGMT_UL_CHUNK_SIZE, CONFIG_IMG_MGMT_UL_CHUNK_SIZE,
    CONFIG_OS_MGMT_ECHO_LENGTH

HALs
****

MCUboot
*******

Trusted Firmware-M
******************

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
