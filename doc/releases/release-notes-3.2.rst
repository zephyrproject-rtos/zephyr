:orphan:

.. _zephyr_3.2:

Zephyr 3.2.0 (Working Draft)
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

Removed APIs in this release
============================

* The following functions, macros, and structures related to the
  deprecated kernel work queue API have been removed:

  * ``k_work_pending()``
  * ``k_work_q_start()``
  * ``k_delayed_work``
  * ``k_delayed_work_init()``
  * ``k_delayed_work_submit_to_queue()``
  * ``k_delayed_work_submit()``
  * ``k_delayed_work_pending()``
  * ``k_delayed_work_cancel()``
  * ``k_delayed_work_remaining_get()``
  * ``k_delayed_work_expires_ticks()``
  * ``k_delayed_work_remaining_ticks()``
  * ``K_DELAYED_WORK_DEFINE``

Deprecated in this release
==========================

Stable API changes in this release
==================================

New APIs in this release
========================

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

These GitHub issues were addressed since the previous 3.1.0 tagged
release:
