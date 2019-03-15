:orphan:

.. _zephyr_1.14:

Zephyr Kernel 1.14.0 (Working Draft)
####################################

We are pleased to announce the release of Zephyr kernel version 1.14.0.

Major enhancements with this release include:

* TBD

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

* The :ref:`BSD socket API <bsd_sockets_interface>` should be used by
  applications for any network connectivity needs.
* Majority of the network sample applications are converted to use
  the BSD socket API.
* New BSD socket based APIs created for these components and protocols:

  - :ref:`MQTT <mqtt_socket_interface>`
  - :ref:`CoAP <coap_sock_interface>`
  - :ref:`LWM2M <lwm2m_interface>`
  - :ref:`SNTP <sntp_interface>`
* net-app client and server APIs are removed. This also requires removal of
  following net-app based legacy APIs:

  - MQTT
  - CoAP
  - SNTP
  - LWM2M
  - HTTP client & server
  - Websocket
* Network packet (:ref:`net-pkt <net_pkt_interface>`) API overhaul. The new
  net-pkt API uses less memory and is more streamlined than the old one.
* Implement following BSD socket APIs: ``freeaddrinfo()``, ``gethostname()``,
  ``getnameinfo()``, ``getsockopt()``, ``select()``, ``setsockopt()``,
  ``shutdown()``
* Convert BSD socket code to use global file descriptor numbers.
* Network subsystem converted to use new :ref:`logging system <logger>`.
* Add support for disabling IPv4, IPv6, UDP, and TCP simultaneously.
* Add support for :ref:`BSD socket offloading <net_socket_offloading>`.
* Add support for long lifetime IPv6 prefixes.
* Add enhancements to IPv6 multicast address checking.
* Add support for IPv6 Destination Options Header extension.
* Add support for packet socket (AF_PACKET).
* Add support for socket CAN (AF_CAN).
* Add support for SOCKS5 proxy in MQTT client.
* Add support for IPSO Timer object in LWM2M.
* Add support for receiving gratuitous ARP request.
* Add :ref:`sample application <google-iot-mqtt-sample>` for Google IoT Cloud.
* :ref:`Network interface <net_if_interface>` numbering starts now from 1 for
  POSIX compatibility.
* :ref:`OpenThread <thread_protocol_interface>` enhancements.
* :ref:`zperf <zperf-sample>` sample application fixes.
* :ref:`LLDP <lldp_interface>` (Link Layer Discovery Protocol) enhancements.
* ARP cache update fix.
* gPTP link delay calculation fixes.
* Change how network data is passed from
  :ref:`L2 to network device driver <network_stack_architecture>`.
* Remove RPL (Ripple) IPv6 mesh routing support.
* Network device driver additions and enhancements:

  - Add Intel PRO/1000 Ethernet driver (e1000).
  - Add SMSC9118/LAN9118 Ethernet driver (smsc911x).
  - Add Inventek es-WiFi driver for disco_l475_iot1 board.
  - Add support for automatically enabling QEMU based Ethernet drivers.
  - SAM-E70 gmac Ethernet driver Qav fixes.
  - enc28j60 Ethernet driver fixes and enhancements.

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

These GitHub issues were addressed since the previous 1.13.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`99999` - issue title
