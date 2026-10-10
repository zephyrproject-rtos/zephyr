.. _ip_stack_overview:

Overview
########

.. contents::
    :local:
    :depth: 2

Supported Features
******************

The networking stack is modular and configured at build time. Enable only the
features your application needs to minimize memory use; almost all of them can
be disabled.

Link Layers
===========

* Ethernet, IEEE 802.3
* Wi-Fi, IEEE 802.11
* IEEE 802.15.4 and Thread (samples: :zephyr:code-sample-category:`openthread`)
* Cellular / PPP (:rfc:`1661`)
* CAN bus for SocketCAN
* SLIP (IP over serial line), used to connect QEMU targets to the host as an
  Ethernet interface so that host applications can exchange data with Zephyr.

Several technologies can be enabled at the same time. No automatic routing is
done between them; applications send data to the interface they choose.

IP
==

**IPv6** (:rfc:`8200`)

* Static addresses or SLAAC (Stateless Address Autoconfiguration,
  :rfc:`4862`), with a configurable number of addresses and prefixes.
* Neighbor Discovery (:rfc:`4861`) with an optional neighbor cache, and
  Multicast Listener Discovery v2 (:rfc:`3810`).
* 6LoWPAN header compression for IEEE 802.15.4 networks (:rfc:`4944`).
* DHCPv6 (:rfc:`8415`) client and server, including prefix delegation.
  See :ref:`DHCPv6 <dhcpv6_interface>`.
* DNS server addresses from Router Advertisements (RDNSS, :rfc:`8106`).
* Privacy extensions (:rfc:`8981`).
* Fragmentation and Path MTU Discovery (:rfc:`8201`).
* Multicast routing and forwarding.

**IPv4** (:rfc:`791`), for example on Ethernet, Wi-Fi and cellular networks.
IEEE 802.15.4 is IPv6 only.

* Static addresses, link-local address autoconfiguration (:rfc:`3927`) and
  address conflict detection (:rfc:`5227`).
  Sample: :zephyr:code-sample:`ipv4-autoconf`.
* DHCPv4 (:rfc:`2131`) client and server. See :ref:`DHCPv4 <dhcpv4_interface>`.
* IGMPv2 (:rfc:`2236`) and IGMPv3 (:rfc:`3376`).
* Fragmentation and Path MTU Discovery (:rfc:`1191`).
* NAT (Network Address Translation): SNAT and DNAT between interfaces, with
  connection tracking and iptables-like rules to filter and forward packets
  across subnets.

IPv6 and IPv4 can be used at the same time.

Transport and Sockets
=====================

* **UDP** (:rfc:`768`), with Datagram Packetization Layer Path MTU Discovery
  (DPLPMTUD, :rfc:`8899`) for UDP-based protocols.
  See :ref:`DPLPMTUD <net_dplpmtud>`.
* **TCP** (:rfc:`9293`), client and server, with a build-time configurable
  number of sockets. Selective acknowledgment (:rfc:`2018`) of received data
  with :kconfig:option:`CONFIG_NET_TCP_SACK`.
* **QUIC** (:rfc:`9000`) with integrated TLS 1.3 (:rfc:`9001`).
  See :ref:`QUIC <quic_transport_interface>`. Samples:
  :zephyr:code-sample:`quic-client-echo`, :zephyr:code-sample:`quic-service-echo`.
* **BSD sockets**, a subset of the :ref:`BSD sockets API <bsd_sockets_interface>`:
  blocking and non-blocking datagram (UDP), stream (TCP) and packet
  (``AF_PACKET``) sockets.
* **TLS and DTLS** sockets backed by Mbed TLS.
  See :ref:`secure sockets <secure_sockets_interface>`.

Application Protocols
=====================

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Protocol
     - Standard
     - Role
     - Documentation
     - Samples
   * - CoAP [#coap]_
     - :rfc:`7252`
     - client, server
     - :ref:`CoAP <coap_sock_interface>`
     - :zephyr:code-sample:`coap-client`, :zephyr:code-sample:`coap-server`
   * - DNS
     - :rfc:`1035`
     - resolver
     - :ref:`DNS <dns_resolve_interface>`
     -
   * - mDNS, DNS-SD
     - :rfc:`6762`, :rfc:`6763`
     - resolver, responder
     - :ref:`DNS <dns_resolve_interface>`
     - :zephyr:code-sample:`mdns-responder`
   * - LLMNR (deprecated)
     - :rfc:`4795`
     - resolver
     - :ref:`DNS <dns_resolve_interface>`
     -
   * - FTP
     - :rfc:`959`
     - client
     - :ref:`FTP <ftp_client_interface>`
     - :zephyr:code-sample:`ftp-client`
   * - HTTP
     - HTTP/1.1 (:rfc:`2616`), HTTP/2 (:rfc:`9113`), HTTP/3 (:rfc:`9114`)
     - client (HTTP/1.1), server
     - :ref:`client <http_client_interface>`, :ref:`server <http_server_interface>`
     - :zephyr:code-sample:`sockets-http-client`,
       :zephyr:code-sample:`sockets-http-server`
   * - LwM2M [#lwm2m]_
     - `LwM2M specification 1.0.2`_, `LwM2M specification 1.1.1`_
     - client
     - :ref:`LwM2M <lwm2m_interface>`
     - :zephyr:code-sample:`lwm2m-client`
   * - MCP (Model Context Protocol)
     - 2025-11-25
     - server
     - :ref:`MCP <mcp_server_interface>`
     - :zephyr:code-sample:`mcp-server-hello-world`
   * - MIDI 2.0
     - Network UDP transport
     - endpoint host
     -
     - :zephyr:code-sample:`netmidi2`
   * - MQTT
     - 3.1.1, 5.0
     - client
     - :ref:`MQTT <mqtt_socket_interface>`
     - :zephyr:code-sample:`mqtt-publisher`
   * - MQTT-SN
     - 1.2
     - client
     - :ref:`MQTT-SN <mqtt_sn_socket_interface>`
     - :zephyr:code-sample:`mqtt-sn-publisher`
   * - OCPP (Open Charge Point Protocol)
     - 1.6
     - charge point
     - :ref:`OCPP <ocpp_interface>`
     - :zephyr:code-sample:`ocpp`
   * - Prometheus
     -
     - metrics server
     -
     - :zephyr:code-sample:`prometheus`
   * - RTP (Real-time Transport Protocol)
     - :rfc:`3550`
     -
     -
     - :zephyr:code-sample:`net-rtp`
   * - SNTP
     - :rfc:`5905`
     - client, server
     - :ref:`SNTP <sntp_interface>`
     - :zephyr:code-sample:`sntp-client`, :zephyr:code-sample:`sntp-server`
   * - SOCKS5
     - :rfc:`1928`
     - client
     - :ref:`SOCKS5 <socks5_interface>`
     -
   * - SSH
     - SSH-2
     - client, server, shell backend
     -
     - :zephyr:code-sample:`ssh-server-client`
   * - TFTP
     - :rfc:`1350`
     - client
     - :ref:`TFTP <tftp_interface>`
     - :zephyr:code-sample:`tftp-client`
   * - WebSocket
     - :rfc:`6455`
     - client, HTTP server upgrade
     - :ref:`WebSocket <websocket_interface>`
     - :zephyr:code-sample:`sockets-websocket-client`
   * - WireGuard
     -
     - VPN peer
     -
     - :zephyr:code-sample:`wireguard-vpn`

.. [#coap] Block-wise transfers (:rfc:`7959`), resource observation
   (:rfc:`7641`), OSCORE (:rfc:`8613`), and CoAP over TCP and TLS (:rfc:`8323`)
   for the client.

.. [#lwm2m] Bootstrap, Client Registration, Device Management & Service
   Enablement and Information Reporting interfaces, the required core objects
   and several IPSO Smart Objects. Version 1.1.1 is enabled with a Kconfig
   option.

Management and Diagnostics
==========================

* :ref:`Network management API <net_mgmt_interface>` to receive events from the
  network stack, for example when an IP address is added or an interface comes
  up.
* :ref:`Wi-Fi management API <wifi_mgmt>` to scan for and connect to networks,
  and Wi-Fi network managers that register with the Wi-Fi stack to implement it.
* :ref:`Virtual LANs <vlan_interface>`, :ref:`traffic classification
  <traffic-class-support>`, and Time Sensitive Networking with
  :ref:`gPTP <gptp_interface>` and :ref:`PTP <ptp_interface>` (IEEE 1588).
* :ref:`Packet filtering <net_pkt_filter_interface>` of sent and received
  packets, and :ref:`packet capture <net_capture_interface>` to a remote host.
* :ref:`Network shell <net_shell>` to inspect network status, enable or disable
  features and run commands such as ping or DNS queries.
* :ref:`zperf <zperf>` network performance and bandwidth measurement, client and
  server, compatible with iPerf v2 and iperf3. Sample: :zephyr:code-sample:`zperf`.
* Minimal copy buffer management: the stack avoids copying application data on
  the transmit path.

Source Tree Layout
******************

The networking stack source code tree is organized as follows:

:zephyr_file:`subsys/net/`
  Various optional network stack components like connection manager,
  packet filter code and hostname handling are located here.

:zephyr_file:`subsys/net/ip/`
  This is where the core network stack code is located.

:zephyr_file:`subsys/net/l2`
  This is where the IP stack layer 2 code is located. This includes generic
  support for Ethernet, IEEE 802.15.4 and Wi-Fi.

:zephyr_file:`subsys/net/lib/`
  Application-level protocols (DNS, MQTT, etc.) and additional stack
  components (BSD Sockets, etc.).

:zephyr_file:`include/zephyr/net/`
  Public API header files. These are the header files applications need
  to include to use IP networking functionality.

:zephyr_file:`samples/net/`
  Sample networking code. This is a good reference to get started with
  network application development.

:zephyr_file:`tests/net/`
  Test applications. These applications are used to verify the
  functionality of the IP stack, but are not the best
  source for sample code (see :zephyr_file:`samples/net/` instead).

.. _LwM2M specification 1.0.2:
   https://www.openmobilealliance.org/release/LightweightM2M/V1_0_2-20180209-A/OMA-TS-LightweightM2M-V1_0_2-20180209-A.pdf

.. _LwM2M specification 1.1.1:
   https://www.openmobilealliance.org/release/LightweightM2M/V1_1_1-20190617-A/
