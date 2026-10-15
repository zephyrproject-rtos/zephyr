.. _ip_stack_overview:

Overview
########

.. contents::
    :local:
    :depth: 2

Supported Features
******************

The networking IP stack is modular and configured at build time. Enable only
the features your application needs to minimize memory use; almost all of them
can be disabled.

* **IPv6** (:rfc:`8200`), with build-time configurable sub-features:

  * Number of unicast and multicast addresses active at the same time.
  * Static addresses or SLAAC (Stateless Address Autoconfiguration,
    :rfc:`4862`).
  * Multiple prefixes, up to a configurable maximum.
  * Optional neighbor cache of configurable size.
  * Neighbor Discovery (:rfc:`4861`) and Multicast Listener Discovery v2
    (:rfc:`3810`), enabled by default.
  * 6LoWPAN header compression for IEEE 802.15.4 networks (:rfc:`4944`).
  * DHCPv6 (:rfc:`8415`) client and server, including prefix delegation.
    See :ref:`DHCPv6 <dhcpv6_interface>`.
  * DNS server addresses from Router Advertisements (RDNSS, :rfc:`8106`).
  * Privacy extensions (:rfc:`8981`).
  * Fragmentation and Path MTU Discovery (:rfc:`8201`).
  * Multicast routing and forwarding.

* **IPv4** (:rfc:`791`), for example on Ethernet, Wi-Fi and cellular networks.
  IEEE 802.15.4 is IPv6 only.

  * DHCPv4 (:rfc:`2131`) client and server. See :ref:`DHCPv4 <dhcpv4_interface>`.
  * Static addresses, enabled by default.
  * Link-local address autoconfiguration (:rfc:`3927`) and address conflict
    detection (:rfc:`5227`). Sample: :zephyr:code-sample:`ipv4-autoconf`.
  * IGMPv2 (:rfc:`2236`) and IGMPv3 (:rfc:`3376`).
  * Fragmentation and Path MTU Discovery (:rfc:`1191`).
  * NAT (Network Address Translation): SNAT and DNAT between interfaces, with
    connection tracking and iptables-like rules to filter and forward packets
    across subnets.

* **Dual stack.** IPv6 and IPv4 can be used at the same time.

* **UDP** (:rfc:`768`), for sending datagrams and listening on a port, and
  UDP options (:rfc:`9868`). Datagram Packetization Layer Path MTU Discovery
  (DPLPMTUD, :rfc:`8899`) for UDP-based protocols, and for plain UDP sockets
  through UDP options (:rfc:`9869`). See :ref:`DPLPMTUD <net_dplpmtud>`.

* **TCP** (:rfc:`9293`), client and server, with a build-time configurable
  number of sockets. Selective acknowledgment (:rfc:`2018`) of received data
  with :kconfig:option:`CONFIG_NET_TCP_SACK`.

* **QUIC** (:rfc:`9000`) with integrated TLS 1.3 (:rfc:`9001`).
  See :ref:`QUIC <quic_transport_interface>`. Samples:
  :zephyr:code-sample:`quic-client-echo`, :zephyr:code-sample:`quic-service-echo`.

* **BSD Sockets API**, a subset of the :ref:`BSD sockets API <bsd_sockets_interface>`:
  blocking and non-blocking datagram (UDP) and stream (TCP) sockets, and packet
  sockets (``AF_PACKET``).

* **Secure Sockets API**, TLS and DTLS for the sockets API, backed by Mbed TLS.
  See :ref:`secure sockets <secure_sockets_interface>`.

* **MQTT** (ISO/IEC PRF 20922) versions 3.1.1 and 5.0.
  Sample: :zephyr:code-sample:`mqtt-publisher`.

* **MQTT-SN** version 1.2. Sample: :zephyr:code-sample:`mqtt-sn-publisher`.

* **CoAP** (:rfc:`7252`), with block-wise transfers (:rfc:`7959`), resource
  observation (:rfc:`7641`), OSCORE (:rfc:`8613`), and CoAP over TCP and TLS
  (:rfc:`8323`) for the client. Samples: :zephyr:code-sample:`coap-client`,
  :zephyr:code-sample:`coap-server`.

* **LwM2M** OMA Lightweight Machine-to-Machine Protocol
  (`LwM2M specification 1.0.2`_, and `LwM2M specification 1.1.1`_ through a
  Kconfig option): Bootstrap, Client Registration, Device Management & Service
  Enablement and Information Reporting interfaces, the required core objects
  and several IPSO Smart Objects. Sample: :zephyr:code-sample:`lwm2m-client`.

* **HTTP** :ref:`client <http_client_interface>` for HTTP/1.1 (:rfc:`2616`) and
  :ref:`server <http_server_interface>` for HTTP/1.1, HTTP/2 (:rfc:`9113`) and
  HTTP/3 (:rfc:`9114`). Samples: :zephyr:code-sample:`sockets-http-client`,
  :zephyr:code-sample:`sockets-http-server`.

* **Websocket** (:rfc:`6455`) client, and Websocket upgrade in the HTTP server.
  Sample: :zephyr:code-sample:`sockets-websocket-client`.

* **DNS** (:rfc:`1035`) resolver for IPv4 (A) and IPv6 (AAAA) records, multicast
  DNS (mDNS, :rfc:`6762`), DNS Service Discovery (DNS-SD, :rfc:`6763`) and the
  deprecated LLMNR (:rfc:`4795`). An mDNS responder answers queries and
  advertises DNS-SD services. Sample: :zephyr:code-sample:`mdns-responder`.

* **Network Management API** to receive events from the network stack, for
  example when an IP address is added or an interface comes up.

* **Wi-Fi Management API** to manage Wi-Fi interfaces, for example to scan for
  and connect to networks.

* **Wi-Fi Network Manager API** for network managers that register with the
  Wi-Fi stack and implement the Wi-Fi Management API.

* **Multiple network technologies**, such as Ethernet, Wi-Fi and IEEE 802.15.4,
  enabled at the same time in Kconfig. No automatic routing is done between
  them; applications send data to the interface they choose.

* **Minimal copy buffer management.** The stack avoids copying application data
  on the transmit path.

* **Virtual LANs** partition physical Ethernet networks into logical networks.
  See :ref:`VLAN <vlan_interface>`.

* **Traffic classification** prioritizes sent and received packets.
  See :ref:`traffic classification <traffic-class-support>`.

* **Time Sensitive Networking** with gPTP (generalized Precision Time Protocol)
  and PTP (IEEE 1588). See :ref:`gPTP <gptp_interface>` and
  :ref:`PTP <ptp_interface>`.

* **SNTP** (:rfc:`5905`) client and server. See :ref:`SNTP <sntp_interface>`.
  Samples: :zephyr:code-sample:`sntp-client`, :zephyr:code-sample:`sntp-server`.

* **SOCKS5** proxy (:rfc:`1928`).

* **TFTP** (:rfc:`1350`) client. Sample: :zephyr:code-sample:`tftp-client`.

* **FTP** (:rfc:`959`) client. See :ref:`FTP client <ftp_client_interface>`.
  Sample: :zephyr:code-sample:`ftp-client`.

* **SSH** SSH-2 (:rfc:`4251` to :rfc:`4254`) server and client. Session
  channels carry shell, exec and subsystem requests, and the Zephyr shell is
  available over SSH. Sample: :zephyr:code-sample:`ssh-server-client`.

* **WireGuard** VPN (`WireGuard protocol`_).
  Sample: :zephyr:code-sample:`wireguard-vpn`.

* **RTP** Real-time Transport Protocol (:rfc:`3550`).
  Sample: :zephyr:code-sample:`net-rtp`.

* **MIDI2** MIDI 2.0 UDP network transport. Sample: :zephyr:code-sample:`netmidi2`.

* **OCPP** Open Charge Point Protocol. Sample: :zephyr:code-sample:`ocpp`.

* **Prometheus** metrics server. Sample: :zephyr:code-sample:`prometheus`.

* **MCP** Model Context Protocol server, exposing device tools that AI agents
  discover and invoke over HTTP. See :ref:`MCP server <mcp_server_interface>`.
  Sample: :zephyr:code-sample:`mcp-server-hello-world`.

* **Packet filtering and capture.** Filter sent and received packets with
  rules, and capture traffic to a remote host for analysis.
  See :ref:`packet filtering <net_pkt_filter_interface>` and
  :ref:`network packet capture <net_capture_interface>`.

* **Network shell** to inspect network status, enable or disable features and
  run commands such as ping or DNS queries. See :ref:`network shell <net_shell>`.

* **zperf** network performance and bandwidth measurement tool, client and
  server, compatible with iPerf v2 and iperf3. Sample: :zephyr:code-sample:`zperf`.

Supported network technologies (link layers):

* IEEE 802.15.4
* Ethernet, IEEE 802.3
* Wi-Fi, IEEE 802.11
* Cellular / PPP (:rfc:`1661`)
* Thread (samples: :zephyr:code-sample-category:`openthread`)
* CAN bus for SocketCAN
* SLIP (IP over serial line), used to connect QEMU targets to the host as an
  Ethernet interface so that host applications can exchange data with Zephyr.

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
