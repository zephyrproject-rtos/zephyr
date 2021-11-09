:orphan:

.. _zephyr_3.0:

Zephyr 3.0.0 (Working draft)
############################

We are pleased to announce the release of Zephyr RTOS version 3.0.0.



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

Changes in this release

* Following functions in UART Asynchronous API are using microseconds to represent
  timeout instead of milliseconds:
  * :c:func:`uart_tx`
  * :c:func:`uart_rx_enable`

* Replaced custom LwM2M :c:struct:`float32_value` type with a native double type.

* Added function for getting status of USB device remote wakeup feature.

* Added ``ranges`` and ``dma-ranges`` as invalid property to be used with DT_PROP_LEN()
  along ``reg`` and ``interrupts``.

Changes in this release
=======================

Removed APIs in this release:

* The following Kconfig options related to radio front-end modules (FEMs) were
  removed:

  * ``CONFIG_BT_CTLR_GPIO_PA``
  * ``CONFIG_BT_CTLR_GPIO_PA_PIN``
  * ``CONFIG_BT_CTLR_GPIO_PA_POL_INV``
  * ``CONFIG_BT_CTLR_GPIO_PA_OFFSET``
  * ``CONFIG_BT_CTLR_GPIO_LNA``
  * ``CONFIG_BT_CTLR_GPIO_LNA_PIN``
  * ``CONFIG_BT_CTLR_GPIO_LNA_POL_INV``
  * ``CONFIG_BT_CTLR_GPIO_LNA_OFFSET``
  * ``CONFIG_BT_CTLR_FEM_NRF21540``
  * ``CONFIG_BT_CTLR_GPIO_PDN_PIN``
  * ``CONFIG_BT_CTLR_GPIO_PDN_POL_INV``
  * ``CONFIG_BT_CTLR_GPIO_CSN_PIN``
  * ``CONFIG_BT_CTLR_GPIO_CSN_POL_INV``
  * ``CONFIG_BT_CTLR_GPIO_PDN_CSN_OFFSET``

  This FEM configuration is hardware description, and was therefore moved to
  :ref:`devicetree <dt-guide>`. See the :dtcompatible:`nordic,nrf-radio`
  devicetree binding's ``fem`` property for information on what to do instead
  on the Nordic open source controller.

* Removed Kconfig option ``CONFIG_USB_UART_CONSOLE``.
  Option ``CONFIG_USB_UART_CONSOLE`` was only relevant for console driver
  when CDC ACM UART is used as backend. Since the behavior of the CDC ACM UART
  is changed so that it more closely mimics the real UART controller,
  option is no longer necessary.

Stable API changes in this release
==================================

Kernel
******


Architectures
*************

* ARC


* ARM

  * AARCH32


  * AARCH64


* x86


Bluetooth
*********

* Audio

* Host

* Mesh

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


* Disk


* Display


* Disk


* DMA


* EEPROM


* ESPI


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

  * Fixed the mcumgr SMP protocol over serial not adding the length of the CRC16 to packet length.

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


Trusted Firmware-m
******************


Documentation
*************


Tests and Samples
*****************


Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.7.0 tagged
release:
