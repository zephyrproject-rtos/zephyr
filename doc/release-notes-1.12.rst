:orphan:

.. _zephyr_1.12:

Zephyr Kernel 1.12.0 (DRAFT)
############################

We are pleased to announce the release of Zephyr kernel version 1.12.0.

Major enhancements with this release include:

- 802.1Q - Virtual Local Area Network (VLAN) traffic on an Ethernet network
- Support multiple concurrent filesystem devices, partitions, and FS types
- Support for TAP net device on the the native POSIX port
- SPI slave support
- Runtime non-volatile configuration data storage system


The following sections provide detailed lists of changes by component.

Kernel
******

* Detail

Architectures
*************

* nxp_imx/mcimx7_m4: Added support for i.MX7 Cortex M4 core

Boards
******

* nios2: Added device tree support for qemu_nios2 and altera_max10
* colibri_imx7d_m4: Added support for Toradex Colibri i.MX7 board

Drivers and Sensors
*******************

* serial: Added support for i.MX UART interface
* gpio: Added support for i.MX GPIO

Networking
**********


Bluetooth
*********


Build and Infrastructure
************************


Libraries / Subsystems
***********************


HALs
****

* nxp/imx: imported i.MX7 FreeRTOS HAL

Documentation
*************


Tests and Samples
*****************


Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.11.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title
