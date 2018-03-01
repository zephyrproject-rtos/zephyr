:orphan:

.. _zephyr_1.11:

Zephyr Kernel 1.11.0
#####################

We are pleased to announce the release of Zephyr kernel version 1.11.0.

Major enhancements with this release include:

* Thread-level memory protection on x86, ARC and ARM, userspace and memory
  domains
* Windows build compatibility.
* Integration with OpenThread, currently only on Nordic chipsets.
* Additional SoC, platform and driver support for many of the already supported
  platforms.

The following sections provide detailed lists of changes by component.

Kernel
******


Architectures
*************

* User space and system call related changes:
  * Added ARC user space implementation
  * Added ARM user space implementation
  * Fixed a few MPU related issues with stack guards

Boards
******

Drivers and Sensors
*******************


Networking
**********

* Generic OpenThread support added
* OpenThread support to nrf5 IEEE 802.15.4 driver added
* NXP MCUX ethernet driver IPv6 multicast join/leave enhancements
* Ethernet stm32 fixes
* IEEE 802.15.4 Sub-GHz TI CC1200 chip support added
* IEEE 802.15.4 test driver (upipe) hw filtering support added
* IEEE 802.15.4 radio API enhancements
* Net loopback driver fixes
* Net management API event enhancements
* IPv6 neighbor addition and removal can be monitored
* Static IPv4 and DHCPv4 configuration enhancements
* Bluetooth IPSP disconnect fix
* Network buffer enhancements
* ICMPv4 and ICMPv6 error checking fixes
* Network interface address handling enhancements
* Add routing support between network interfaces
* LWM2M fixes and enhancements
* Old legacy HTTP API removed
* Old legacy ZoAP API removed
* CoAP fixes
* TCP fixes
* HTTP fixes
* RPL fixes
* Net-app API fixes
* Net-shell fixes
* BSD socket API fixes

Bluetooth
*********


Build and Infrastructure
************************


Libraries / Subsystems
***********************

HALs
****

Documentation
*************

* Added MPU specific stack and userspace documentation

Tests and Samples
*****************

* Added additional tests and test improvements for user space testing

Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.10.0 tagged
release:

.. comment  List derived from Jira/GitHub Issue query: ...


