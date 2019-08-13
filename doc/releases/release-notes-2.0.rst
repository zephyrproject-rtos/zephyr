:orphan:

.. _zephyr_2.0:

Zephyr Kernel 2.0.0 (Working Draft)
####################################

We are pleased to announce the release of Zephyr kernel version 2.0.0.

Major enhancements with this release include:

* TBD
* We added support for SOCKS5 proxy. SOCKS5 is an Internet protocol that
  exchanges network packets between a client and server through a proxy server.
* Introduced support for 6LoCAN, a 6Lo adaption layer for Controller Area
  Networks.
* We added support for :ref:`Point-to-Point Protocol (PPP) <ppp>`. PPP is a
  data link layer (layer 2) communications protocol used to establish a direct
  connection between two nodes.
* We added support for UpdateHub which is an end-to-end solution for large scale
  over-the-air update of devices.

The following sections provide detailed lists of changes by component.

Kernel
******

* TBD

Architectures
*************

* TBD

Boards & SoC Support
********************

* TBD

Drivers and Sensors
*******************

* TBD

Networking
**********

* Add support for `SOCKS5 proxy <https://en.wikipedia.org/wiki/SOCKS>`__.
  See also `RFC1928 <https://tools.ietf.org/html/rfc1928>`__ for details.
* Add support for 6LoCAN, a 6Lo adaption layer for Controller Area Networks.
* Add support for :ref:`Point-to-Point Protocol (PPP) <ppp>`.
* Add support for UpdateHub which is an end-to-end solution for large scale
  over-the-air update of devices.
  See `UpdateHub.io <https://updatehub.io/>`__ for details.
* Add support to automatically register network socket family.
* Add support for ``getsockname()`` function.
* Add SO_PRIORITY support to ``setsockopt()``
* Add support for VLAN tag stripping.
* Add IEEE 802.15.4 API for ACK configuration.
* Add dispatching support to SocketCAN sockets.
* Add user mode support to PTP clock API.
* Add user mode support to network interface address functions.
* Add AF_NET_MGMT socket address family support. This is for receiving network
  event information in user mode application.
* Add user mode support to ``net_addr_ntop()`` and ``net_addr_pton()``
* Add support for sending network management events when DNS server is added
  or deleted.
* Add LiteEth Ethernet driver.
* Add support for ``sendmsg()`` API.
* Add `civetweb <https://civetweb.github.io/civetweb/>`__ HTTP API support.
* Add LWM2M IPSO Accelerometer, Push Button, On/Off Switch and Buzzer object
  support.
* Add LWM2M Location and Connection Monitoring object support.
* Add network management L4 layer. The L4 management events are used
  when monitoring network connectivity.
* Allow net-mgmt API to pass information length to application.
* Remove network management L1 layer as it was never used.
* By default a network interface is set to UP when the device starts.
  If this is not desired, then it is possible to disable automatic start.
* Allow collecting network packet TX throughput times in the network stack.
  This information can be seen in net-shell.
* net-shell Ping command overhaul.
* Accept UDP packet with missing checksum.
* 6lo compression rework.
* Incoming connection handling refactoring.
* Network interface refactoring.
* IPv6 fragmentation fixes.
* TCP data length fixes if TCP options are present.
* SNTP client updates.
* Trickle timer re-init fixes.
* ``getaddrinfo()`` fixes.
* DHCPv4 fixes.
* LWM2M fixes.
* gPTP fixes.
* MQTT fixes.
* DNS fixes for non-compressed answers.
* mDNS resolver fixes.
* LLMNR resolver fixes.
* Ethernet ARP fixes.
* OpenThread updates and fixes.
* Network device driver enhancements:

  - Ethernet e1000 fixes.
  - Ethernet enc28j60 fixes.
  - Ethernet mcux fixes.
  - Ethernet stellaris fixes.
  - Ethernet gmac fixes.
  - Ethernet stm32 fixes.
  - WiFi eswifi fixes.
  - IEEE 802.15.4 kw41z fixes.
  - IEEE 802.15.4 nrf5 fixes.

Bluetooth
*********

* TBD

Build and Infrastructure
************************

* TBD

Libraries / Subsystems
***********************

* TBD

HALs
****

* TBD

Documentation
*************

* TBD

Tests and Samples
*****************

* TBD

Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.14.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`99999` - issue title
