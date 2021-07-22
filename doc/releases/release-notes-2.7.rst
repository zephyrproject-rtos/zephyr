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

Changes in this release

==========================

Removed APIs in this release

* Removed support for the deprecated ``DEVICE_INIT`` and ``DEVICE_AND_API_INIT`` macros.
* Removed support for the deprecated ``BUILD_ASSERT_MSG`` macro.
* Removed support for the deprecated ``GET_ARG1``, ``GET_ARG2`` and ``GET_ARGS_LESS_1`` macros.
* Removed support for the deprecated Kconfig ``PRINTK64`` option.
* Removed support for the deprecated ``bt_set_id_addr`` function.

============================

Stable API changes in this release
==================================

Kernel
******


Architectures
*************

* ARC


* ARM

  * AARCH32

     * Added support for FPU in QEMU for Cortex-M, allowing to build and execute
       tests in CI with FPU and FPU_SHARING options enabled.


  * AARCH64


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


* Removed support for these SoC series:


* Made these changes in other SoC series:


* Changes for ARC boards:


* Added support for these ARM boards:


* Added support for these ARM64 boards:


* Removed support for these ARM boards:


* Removed support for these X86 boards:


* Made these changes in other boards:


* Added support for these following shields:


Drivers and Sensors
*******************

* ADC


* Bluetooth


* CAN


* Clock Control


* Console


* Counter


* DAC

   * Added support for Microchip MCP4725

* Disk

  * Added SDMMC support on STM32L4+

* Display

  * Added support for ST7735R

* Disk


* DMA

  * Added support on STM32G0 and STM32H7

* EEPROM

  * Added support for EEPROM emulated in flash.

* ESPI

  * Added support for Microchip eSPI SAF

* Ethernet


* Flash


* GPIO


* Hardware Info


* I2C


* I2S


* IEEE 802.15.4


* Interrupt Controller


* LED


* LoRa


* Modem


* PWM


* Sensor


* Serial


* SPI


* Timer


* USB


* Watchdog


* WiFi


Networking
**********

* CoAP:


* DHCPv4:


* DNS:


* HTTP:


* IPv4:


* LwM2M:


* Misc:


* OpenThread:


* Socket:


* TCP:


* TLS:


USB
***


Build and Infrastructure
************************


* Devicetree


* West (extensions)


Libraries / Subsystems
**********************

* Disk


* Management


* CMSIS subsystem


* Power management


* Logging


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
