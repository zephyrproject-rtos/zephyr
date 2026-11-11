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
be disabled. Buffer management keeps copies to a minimum: the stack avoids
copying application data on the transmit path.

Link Layer
==========

* :ref:`Ethernet <ethernet_interface>`, IEEE 802.3
* :ref:`Wi-Fi <wifi_mgmt>`, IEEE 802.11
* :ref:`IEEE 802.15.4 <ieee802154_interface>` and
  :ref:`Thread <thread_protocol_interface>`
  (samples: :zephyr:code-sample-category:`openthread`)
* Cellular / :ref:`PPP <ppp>` (:rfc:`1661`)
* CAN bus for SocketCAN
* SLIP (IP over serial line), used to connect QEMU targets to the host as an
  Ethernet interface so that host applications can exchange data with Zephyr.
* Offloaded network devices, such as Wi-Fi and cellular modems that run their
  own IP stack, through :ref:`network offloading <net_offload_interface>` or
  :ref:`socket offloading <net_socket_offloading>`.

Several technologies can be enabled at the same time. Each packet is sent on
the interface that matches its destination, or on the default interface if
none does, unless the socket is bound to an interface. Packets are forwarded
between interfaces only when IPv4 or IPv6 forwarding or Ethernet bridging is
enabled.

Ethernet and TSN
================

* :ref:`Virtual LANs <vlan_interface>` partition physical Ethernet networks
  into logical networks.
* :ref:`Bridging <ethernet_bridge_interface>` of Ethernet interfaces, with a
  forwarding database that learns addresses and the Rapid Spanning Tree
  Protocol (RSTP). Sample: :zephyr:code-sample:`bridge`.
* :ref:`Distributed Switch Architecture <dsa>` (DSA) for Ethernet switches.
  Sample: :zephyr:code-sample:`dsa`.
* :ref:`LLDP <lldp_interface>` (Link Layer Discovery Protocol).
  Sample: :zephyr:code-sample:`lldp`.
* :ref:`Traffic classification <traffic-class-support>` prioritizes sent and
  received packets.
* Time Sensitive Networking with :ref:`gPTP <gptp_interface>` (generalized
  Precision Time Protocol), :ref:`PTP <ptp_interface>` (IEEE 1588) and
  configuration of :ref:`IEEE 802.1Qav <8021Qav>` credit-based shapers.

Network Layer
=============

**IPv6** (:rfc:`8200`)

* Static addresses or SLAAC (Stateless Address Autoconfiguration,
  :rfc:`4862`), with a configurable number of addresses and prefixes.
* Neighbor Discovery (:rfc:`4861`) with an optional neighbor cache, and
  Multicast Listener Discovery v2 (:rfc:`3810`).
* 6LoWPAN (:rfc:`4944`) for IEEE 802.15.4 networks, with IPHC header
  compression (:rfc:`6282`) and fragmentation.
* DHCPv6 (:rfc:`8415`) client and server, including prefix delegation.
  See :ref:`DHCPv6 <dhcpv6_interface>`.
* DNS server addresses from Router Advertisements (RDNSS, :rfc:`8106`).
* Privacy extensions (:rfc:`8981`).
* Fragmentation and Path MTU Discovery (:rfc:`8201`).
* Route table, forwarding between interfaces with
  :kconfig:option:`CONFIG_NET_IPV6_FORWARDING`, and multicast routing and
  forwarding.

**IPv4** (:rfc:`791`). Not available on IEEE 802.15.4, which is IPv6 only.

* Static addresses, link-local address autoconfiguration (:rfc:`3927`) and
  address conflict detection (:rfc:`5227`).
  Sample: :zephyr:code-sample:`ipv4-autoconf`.
* DHCPv4 (:rfc:`2131`) client and server. See :ref:`DHCPv4 <dhcpv4_interface>`.
* IGMPv2 (:rfc:`2236`) and IGMPv3 (:rfc:`3376`).
* Fragmentation and Path MTU Discovery (:rfc:`1191`).
* Route table, and forwarding between interfaces with
  :kconfig:option:`CONFIG_NET_IPV4_FORWARDING`.
* NAT (Network Address Translation): source NAT (masquerading) of forwarded
  packets that match iptables-like rules, with connection tracking that
  translates the replies back.

IPv6 and IPv4 can be used at the same time (dual-stack).

**Filtering and tunneling**

* :ref:`Packet filtering <net_pkt_filter_interface>` of sent and received
  packets with rules.
* WireGuard VPN (`WireGuard protocol`_).
  Sample: :zephyr:code-sample:`wireguard-vpn`.
* IP-in-IP tunnels on virtual network interfaces.
  Sample: :zephyr:code-sample:`virtual-network-interface`.

Transport and Sockets
=====================

* **UDP** (:rfc:`768`) and UDP options (:rfc:`9868`), with Datagram
  Packetization Layer Path MTU Discovery (DPLPMTUD, :rfc:`8899`) for UDP-based
  protocols, and for plain UDP sockets through UDP options (:rfc:`9869`).
  See :ref:`DPLPMTUD <net_dplpmtud>`.
* **TCP** (:rfc:`9293`), client and server, with a build-time configurable
  number of sockets. Selective acknowledgment (:rfc:`2018`) with
  :kconfig:option:`CONFIG_NET_TCP_SACK`: of received data
  (:kconfig:option:`CONFIG_NET_TCP_SACK_RECV`), and SACK-based loss recovery
  (:rfc:`6675`) when sending (:kconfig:option:`CONFIG_NET_TCP_SACK_SEND`).
  Window scaling (:rfc:`7323`) with
  :kconfig:option:`CONFIG_NET_TCP_WINDOW_SCALE`.
* **QUIC** (:rfc:`9000`) with integrated TLS 1.3 (:rfc:`9001`).
  See :ref:`QUIC <quic_transport_interface>`. Samples:
  :zephyr:code-sample:`quic-client-echo`, :zephyr:code-sample:`quic-service-echo`.
* **BSD sockets**, a subset of the :ref:`BSD sockets API <bsd_sockets_interface>`:
  blocking and non-blocking datagram (UDP), stream (TCP) and packet
  (``AF_PACKET``) sockets.
* **TLS and DTLS** sockets backed by Mbed TLS.
  See :ref:`secure sockets <secure_sockets_interface>`. A device can generate
  its own private key and certificate signing request, see
  :ref:`TLS credentials <sockets_tls_credentials_subsys>`.
  Sample: :zephyr:code-sample:`tls-keygen`.

Application Protocols
=====================

The protocol name links to its documentation, when there is one.

.. list-table::
   :header-rows: 1
   :widths: 20 30 25 25

   * - Protocol
     - Standard
     - Role
     - Samples
   * - :ref:`CoAP <coap_sock_interface>` [#coap]_
     - :rfc:`7252`
     - client, server
     - :zephyr:code-sample:`coap-client`, :zephyr:code-sample:`coap-server`
   * - :ref:`DNS <dns_resolve_interface>`
     - :rfc:`1035`
     - resolver
     - :zephyr:code-sample:`dns-resolve`
   * - :ref:`mDNS, DNS-SD <dns_resolve_interface>`
     - :rfc:`6762`, :rfc:`6763`
     - resolver, responder
     - :zephyr:code-sample:`mdns-responder`
   * - :ref:`LLMNR <dns_resolve_interface>` (deprecated)
     - :rfc:`4795`
     - resolver, responder
     -
   * - :ref:`FTP <ftp_client_interface>`
     - :rfc:`959`
     - client
     - :zephyr:code-sample:`ftp-client`
   * - HTTP
     - HTTP/1.1 (:rfc:`2616`), HTTP/2 (:rfc:`9113`), HTTP/3 (:rfc:`9114`)
     - :ref:`client <http_client_interface>` (HTTP/1.1),
       :ref:`server <http_server_interface>` (HTTP/1.1, HTTP/2, HTTP/3)
     - :zephyr:code-sample:`sockets-http-client`,
       :zephyr:code-sample:`sockets-http-server`
   * - :ref:`LwM2M <lwm2m_interface>` [#lwm2m]_
     - `LwM2M specification 1.0.2`_, `LwM2M specification 1.1.1`_
     - client
     - :zephyr:code-sample:`lwm2m-client`
   * - :ref:`MCP <mcp_server_interface>` (Model Context Protocol)
     - 2025-11-25
     - server
     - :zephyr:code-sample:`mcp-server-hello-world`
   * - MIDI 2.0
     - Network UDP transport
     - endpoint host
     - :zephyr:code-sample:`netmidi2`
   * - :ref:`MQTT <mqtt_socket_interface>`
     - 3.1.1, 5.0
     - client
     - :zephyr:code-sample:`mqtt-publisher`
   * - :ref:`MQTT-SN <mqtt_sn_socket_interface>`
     - 1.2
     - client
     - :zephyr:code-sample:`mqtt-sn-publisher`
   * - :ref:`OCPP <ocpp_interface>` (Open Charge Point Protocol)
     - 1.6
     - charge point
     - :zephyr:code-sample:`ocpp`
   * - Prometheus
     -
     - metrics server
     - :zephyr:code-sample:`prometheus`
   * - :ref:`RTP <rtp_interface>` (Real-time Transport Protocol), SRTP
     - :rfc:`3550`, :rfc:`3711`
     - source, sink
     - :zephyr:code-sample:`net-rtp`
   * - :ref:`SNTP <sntp_interface>`
     - :rfc:`5905`
     - client, server
     - :zephyr:code-sample:`sntp-client`, :zephyr:code-sample:`sntp-server`
   * - :ref:`SOCKS5 <socks5_interface>`
     - :rfc:`1928`
     - client
     -
   * - SSH
     - SSH-2 (:rfc:`4251` to :rfc:`4254`)
     - client, server (shell, exec and subsystem channels), shell backend
     - :zephyr:code-sample:`ssh-server-client`
   * - :ref:`STUN <stun_interface>`
     - :rfc:`8489`
     - message codec
     -
   * - :ref:`TFTP <tftp_interface>`
     - :rfc:`1350`
     - client
     - :zephyr:code-sample:`tftp-client`
   * - :ref:`WebSocket <websocket_interface>`
     - :rfc:`6455`
     - client, HTTP server upgrade
     - :zephyr:code-sample:`sockets-websocket-client`

.. [#coap] Block-wise transfers (:rfc:`7959`), resource observation
   (:rfc:`7641`), OSCORE (:rfc:`8613`), and CoAP over TCP and TLS (:rfc:`8323`)
   for the client.

.. [#lwm2m] Bootstrap, Client Registration, Device Management & Service
   Enablement and Information Reporting interfaces, the required core objects
   and several IPSO Smart Objects. Version 1.1.1 is enabled with a Kconfig
   option.

Management
==========

* :ref:`Network management API <net_mgmt_interface>` to receive events from the
  network stack, for example when an IP address is added or an interface comes
  up.
* :ref:`Connection Manager <conn_mgr_overview>` to monitor IP connectivity and
  control network association through one API, whatever the network technology.
* :ref:`Network configuration library <net_config_interface>` to set up network
  interfaces at boot from Kconfig options.
* :ref:`Wi-Fi management API <wifi_mgmt>` to scan for and connect to networks,
  and Wi-Fi network managers that register with the Wi-Fi stack to implement it.

Diagnostics
===========

* :ref:`Network shell <net_shell>` to inspect network status, enable or disable
  features and run commands such as ping or DNS queries.
* :ref:`zperf <zperf>` network performance and bandwidth measurement, client and
  server, compatible with iPerf v2 and iperf3. Sample: :zephyr:code-sample:`zperf`.
* :ref:`Packet capture <net_capture_interface>` of network traffic to a remote
  host for analysis.
* :ref:`Network statistics <net_stats_interface>`, collected for the whole
  stack and per interface. Sample: :zephyr:code-sample:`net-stats`.

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

.. _WireGuard protocol:
   https://www.wireguard.com/
