:orphan:

.. _zephyr_1.13:

Zephyr Kernel 1.13.0 (DRAFT)
############################

We are pleased to announce the release of Zephyr kernel version 1.13.0.

Major enhancements with this release include:

-

The following sections provide detailed lists of changes by component.

Kernel
******

* Detail

Architectures
*************


Boards
******


Drivers and Sensors
*******************


Networking
**********

* Introduce system calls to zsock socket APIs.
* Add IPv4 autoconf support. This adds support for IPv4 link-local addresses (169.254.*.*)
* Add TLS support to BSD socket API. The TLS is configured via setsockopt() API.
* Add DTLS support to setsockopt() API.
* Add TLS and DTLS support to socket echo-client/server sample applications.
* Add support for IEEE 802.1AS-2011 generalized Precision Time Protocol (gPTP) for ethernet networks. A sample application is created to show how to interact with gPTP code.
* Add support for PTP clock driver. This driver will be used by gPTP supported ethernet drivers.
* Add Link Layer Discovery Protocol (LLDP) TX support.
* Add support for managing Qav credit-based shaper algorithm.
* Add generic TX timestamping support.
* Add carrier detection support to ethernet L2 driver.
* Add support for having vendor specific ethernet statistics.
* Add getter support to ethernet management interface.
* Add promiscuous mode support to network interface. A sample application is created that shows how to use the user API for getting all network packets. The native_posix ethernet driver supports promiscuous mode at this point.
* Add support for Link Layer Multicast Name Resolution (LLMNR). LLMNR is used in Microsoft Windows networks for local name resolution.
* Add BSD socket based echo-server and echo-client sample applications.
* Add API to net_pkt to prefill a network packet to a pre-defined value.
* Add IEEE 802.1Qav support to Atmel GMAC ethernet driver.
* Add hardware TX timestamping support to Atmel GMAC ethernet driver.
* Add multiple hardware queue support to Atmel GMAC ethernet driver.
* Add gPTP support to Atmel GMAC ethernet driver.
* Add support for TI SimpleLink WiFI offload driver.
* Add support for randomized but stable MAC address in NXP MCUX ethernet driver.
* Add extra prints to net-shell for ethernet based network interfaces. The supported features and priority queue information is printed.
* Add and fix string to integer conversions in net-shell.
* Allow user to configure MAC address filters into ethernet devices.
* Catch network interface ON and OFF events in DHCPv4 and renew address lease if we re-connected.
* Remove forever timeouts when waiting a new network buffer to be available.
* Kconfig option naming changes in network config API.
* Relay network interface up/down command from net-shell to Linux host for native_posix ethernet driver.
* No need to join IPv6 solicited node multicast group for Bluetooth IPSP supported nodes.
* Allow external program to be started for native_posix ethernet driver. This allows for example startup of wireshark when zeth is created.
* Network packet priority and traffic class fixes and clarifications.
* Lower memory consumption in net by using packed enums when applicable.
* Correctly notify net_app server when TCP is disconnected.
* Register OpenThread used unicast and multicast IPv6 addresses for network interface.
* Enable Fast Connect policy for TI SimpleLink ethernet driver.
* Fix ieee802154 simulator driver channel/tx power settings.
* Handle large IPv6 packets properly.
* Enable gPTP support in native_posix, NXP mcux and Atmel GMAC ethernet drivers. The native_posix ethernet driver gPTP support is only for testing purposes.
* Split net_app API to a separate network configuration library and to existing net_app API.
* Moving all layer 2 (L2) network code into subsys/net/l2 directory.
* Fix cache support in Atmel GMAC ethernet driver.
* Add MSS option on sending TCP SYN request.
* Fix TCP by processing zero window probes when our receive window is 0.
* IPv6 code refactoring.
* IPv4 code refactoring.
* ICMPv6 code refactoring.
* IPv6 address lifetime fixes.
* IPv6 fragmentation fixes.
* ARP code refactoring.
* ARP fixes when using VLAN.
* IP address print function enhancements.
* HTTP fix when sending the last chunck.
* DHCPv4 fixes and timeout management refactoring.
* MQTT fixes.
* LWM2M cleanups and fixes.
* Timeout too long lasting ARP requests.
* TCP retry, RST packet handling, and memory leak fixes.
* Socket echo-client sample fixes.
* Handle out-of-buf situations gracefully in echo-client and echo-server sample applications.
* Fix NXP MCUX ethernet driver to detect carrier lost event.

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


Tests and Samples
*****************


Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.12.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title
