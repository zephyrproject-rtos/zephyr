:orphan:

.. _zephyr_1.10:

Zephyr Kernel 1.10.0
#####################

DRAFT: We are pleased to announce the release of Zephyr kernel version 1.10.0.

Major enhancements with this release include:

* Integration with MCUBOOT Bootloader
* Additional implementation of MMU/MPU support
* Build and Configuration System (CMake)
* Newtron Flash Filesystem (NFFS) Support
* Increased testsuite coverage

The following sections provide detailed lists of changes by component.

Kernel
******

* details ...

Architectures
*************

* nrf52: Add support for LOW_POWER state and SYSTEM_OFF

Boards
******

* details ...

Drivers and Sensors
*******************

* timer: Add Support for TICKLESS KERNEL in xtensa_sys_timer

Networking
**********

* HTTP API changed to use net-app API. Old HTTP API is deprecated.
* Loopback network interface support added. This is used in testing only.
* LWM2M multi-fragment network packet support added.
* New CoAP library implementation which supports longer network packets.
* Old ZoAP library deprecated.
* mDNS (multicast DNS) support added.
* SNTP (Simple Network Time Protocol) client library added.
* TCP fixes.
* RPL fixes.
* ARP fixes.
* DNS fixes.
* LWM2M fixes.
* Ethernet fixes.
* net-app API fixes.
* Network shell fixes.
* BSD socket API fixes.
* Network management API fixes.
* Networking sample application fixes.
* 6lo IPv6 header compression fixes.
* IEEE 802.15.4 generic fixes.
* IEEE 802.15.4 mcr20a driver fixes.
* IEEE 802.15.4 kw41z driver fixes.
* IEEE 802.15.4 nrf5 driver fixes.

Bluetooth
*********

* details ...

Build and Infrastructure
************************

* details ...

Libraries
*********

* details ...

HALs
****

* details ...

Documentation
*************

* details ...

Tests and Samples
*****************

* details ...

Issue Related Items
*******************

.. comment  List derived from Jira/GitHub Issue query: ...

* :jira:`ZEP-248` - reference a (public) Jira issue
* :github:`1234` - reference a GitHub issue
