:orphan:

.. _zephyr_3.1:

Zephyr 3.1.0 (Working Draft)
############################

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

Known issues
************

API Changes
***********

Changes in this release
=======================

* LoRaWAN: The message type parameter in :c:func:`lorawan_send` was changed
  from ``uint8_t`` to ``enum lorawan_message_type``. If ``0`` was passed for
  unconfirmed message, this has to be changed to ``LORAWAN_MSG_UNCONFIRMED``.

Removed APIs in this release
============================

Deprecated in this release
==========================

* :c:func:`nvs_init` is deprecated in favor of utilizing :c:func:`nvs_mount`.

Stable API changes in this release
==================================

New APIs in this release
========================

* Util

  * Added :c:macro:`IN_RANGE` for checking if a value is in the range of two
    other values.

Kernel
******

Architectures
*************

* ARM

  * AARCH32

  * AARCH64

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

* Made these changes in other boards:

* Added support for these following shields:


Drivers and Sensors
*******************

* ADC

* CAN

* Counter

* DAC

* Disk

* DMA

* EEPROM

* Entropy

* Ethernet

* Flash

* GPIO

* I2C

* I2S

* Interrupt Controller

* MBOX

* MEMC

* Pin control

* PWM

* Sensor

* Serial

* SPI

* Timer

* USB

* Watchdog

Networking
**********

USB
***

Build and Infrastructure
************************

Libraries / Subsystems
**********************

HALs
****

MCUboot
*******

Trusted Firmware-m
******************

Documentation
*************

Tests and Samples
*****************

Issue Related Items
*******************

These GitHub issues were addressed since the previous 3.0.0 tagged
release:
