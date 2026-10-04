.. _ip_stack_overview:

Overview
########

.. contents::
    :local:
    :depth: 2

Supported Features
******************

The networking IP stack is modular and highly configurable via build-time
configuration options. You can minimize system memory consumption by enabling
only those network features required by your application. Almost all features
can be disabled if not needed.

* **IPv6** (:rfc:`8200`) is supported. Various IPv6 sub-options
  can be enabled or disabled depending on networking needs.

  * Developer can set the number of unicast and multicast IPv6 addresses that
    are active at the same time.
  * The IPv6 address for the device can be set either statically or
    dynamically using SLAAC (Stateless Address Auto Configuration, :rfc:`4862`).
  * The system also supports multiple IPv6 prefixes and the maximum
    IPv6 prefix count can be configured at build time.
  * The IPv6 neighbor cache can be disabled if not needed, and its size can be
    configured at build time.
  * The IPv6 neighbor discovery support (:rfc:`4861`) is enabled by default.
  * Multicast Listener Discovery v2 support (:rfc:`3810`) is enabled by default.
  * IPv6 header compression (6lo) is available for IPv6 connectivity for
    IEEE 802.15.4 networks (:rfc:`4944`).
  * DHCPv6 (Dynamic Host Configuration Protocol for IPv6) (:rfc:`8415`) client
    and server are supported, including prefix delegation.
    See :ref:`DHCPv6 <dhcpv6_interface>` for more details.
  * DNS server addresses can be learned from Router Advertisements (RDNSS,
    :rfc:`8106`).
  * The IPv6 privacy extension (:rfc:`8981`) is supported.
  * IPv6 fragmentation and Path MTU Discovery (:rfc:`8201`) are supported.
  * Multicast routing and forwarding is supported.

* **IPv4** (:rfc:`791`) is supported. It cannot be used by IEEE 802.15.4 as
  this network technology supports only IPv6. IPv4 can be used for example
  in Ethernet, Wi-Fi and Cellular based networks.

  * DHCP (Dynamic Host Configuration Protocol) client and server is supported
    (:rfc:`2131`). See :ref:`DHCPv4 <dhcpv4_interface>` for more details.
  * The IPv4 address can also be configured manually. Static IPv4 addresses
    are supported by default.
  * IPv4 link-local address autoconfiguration (:rfc:`3927`) and address
    conflict detection (:rfc:`5227`) are supported.
    :zephyr:code-sample:`ipv4-autoconf` sample is provided.
  * IGMPv2 (:rfc:`2236`) and IGMPv3 (:rfc:`3376`) multicast group management
    is supported.
  * IPv4 fragmentation and Path MTU Discovery (:rfc:`1191`) are supported.
  * IPv4 NAT (Network Address Translation) is supported. Packets can
    hop between interfaces by performing SNAT and DNAT. Connection tracking
    and iptable rules are used to filter and forward packets across subnets.

* **Dual stack support.** The networking stack allows a developer to configure
  the system to use both IPv6 and IPv4 at the same time.

* **UDP** User Datagram Protocol (:rfc:`768`) is supported.
  The developer can send UDP datagrams (client side support) or create a
  listener to receive UDP packets destined to certain port (server side
  support).
  Datagram Packetization Layer Path MTU Discovery (DPLPMTUD, :rfc:`8899`) is
  available for UDP-based protocols.
  See :ref:`DPLPMTUD <net_dplpmtud>` for more details.

* **TCP** Transmission Control Protocol (:rfc:`9293`) is supported. Both server
  and client roles can be used the application. The amount of TCP sockets
  that are available to applications can be configured at build time.
  Selective acknowledgment (:rfc:`2018`) is supported for received data
  (:kconfig:option:`CONFIG_NET_TCP_SACK`).

* **QUIC** (:rfc:`9000`) transport with integrated TLS 1.3 (:rfc:`9001`) is
  supported. See :ref:`QUIC <quic_transport_interface>` for more details.
  :zephyr:code-sample:`quic-client-echo` and :zephyr:code-sample:`quic-service-echo`
  samples are provided.

* **BSD Sockets API** Support for a subset of a
  :ref:`BSD sockets compatible API <bsd_sockets_interface>` is
  implemented. Both blocking and non-blocking datagram (UDP) and stream (TCP)
  sockets are supported. Packet sockets (``AF_PACKET``) are also supported.

* **Secure Sockets API** TLS and DTLS support with configuration options for
  the sockets API. Secure functions for the implementation are provided by the
  Mbed TLS library. See :ref:`secure sockets <secure_sockets_interface>` for
  more details.

* **MQTT** Message Queue Telemetry Transport (ISO/IEC PRF 20922) versions 3.1.1 and 5.0
  are supported.
  A sample :zephyr:code-sample:`mqtt-publisher` client application for MQTT v3.1.1 and v5.0
  is provided.

* **MQTT-SN** MQTT for Sensor Networks version 1.2 is supported.
  A sample :zephyr:code-sample:`mqtt-sn-publisher` client application is provided.

* **CoAP** Constrained Application Protocol (:rfc:`7252`) is supported,
  including block-wise transfers (:rfc:`7959`), resource observation
  (:rfc:`7641`), CoAP over TCP and TLS (:rfc:`8323`) for the client, and
  OSCORE (:rfc:`8613`).
  Both :zephyr:code-sample:`coap-client` and :zephyr:code-sample:`coap-server` sample
  applications are provided.

* **LwM2M** OMA Lightweight Machine-to-Machine Protocol
  (`LwM2M specification 1.0.2`_) is supported via the "Bootstrap", "Client
  Registration", "Device Management & Service Enablement" and "Information
  Reporting" interfaces.  The required core LwM2M objects are implemented as
  well as several IPSO Smart Objects. (`LwM2M specification 1.1.1`_) is
  supported in similar manner when enabled with a Kconfig option.
  :zephyr:code-sample:`lwm2m-client` sample implements the library as an example.

* **HTTP** Hypertext Transfer Protocol client and server are supported.
  :ref:`http_client_interface` library supports HTTP/1.1 (:rfc:`2616`).
  :ref:`http_server_interface` library supports HTTP/1.1 (:rfc:`2616`),
  HTTP/2 (:rfc:`9113`) and HTTP/3 (:rfc:`9114`).
  :zephyr:code-sample:`sockets-http-client` and
  :zephyr:code-sample:`sockets-http-server` samples are provided.

* **Websocket** (:rfc:`6455`) client is supported, and the HTTP server can
  upgrade connections to Websocket.
  :zephyr:code-sample:`sockets-websocket-client` sample is provided.

* **DNS** Domain Name Service (:rfc:`1035`) client functionality is supported.
  Applications can use the DNS API to query domain name information or IP
  addresses from the DNS server. Both IPv4 (A) and IPv6 (AAAA) records can
  be queried.
  Multicast DNS (mDNS) (:rfc:`6762`) is supported, as well as the deprecated
  link-local multicast name resolution (LLMNR, :rfc:`4795`).
  The DNS Service Discovery (:rfc:`6763`) is also supported.
  An mDNS responder can answer queries and advertise DNS-SD services;
  :zephyr:code-sample:`mdns-responder` sample is provided.

* **Network Management API.** Applications can use network management API to
  listen management events generated by core network stack when for example IP address
  is added to the device, or network interface is coming up etc.

* **Wi-Fi Management API.** Applications can use Wi-Fi management API to
  manage the interface, in example to connect to Wi-Fi network and to scan
  available Wi-Fi networks.

* **Wi-Fi Network Manager API.** Wi-Fi Network Managers can now register
  themselves to the Wi-Fi stack. The Network Managers can then implement
  the Wi-Fi Management API and manage the Wi-Fi interface.

* **Multiple Network Technologies.** The Zephyr OS can be configured to
  support multiple network technologies at the same time simply by enabling
  them in Kconfig: for example, Ethernet, Wi-Fi and 802.15.4 support. Note
  that no automatic IP routing functionality is provided between these
  technologies. Applications can send data according to their needs to desired
  network interface.

* **Minimal Copy Network Buffer Management.** It is possible to have minimal
  copy network data path. This means that the system tries to avoid copying
  application data when it is sent to the network.

* **Virtual LAN support.** Virtual LANs (VLANs) allow partitioning of physical
  ethernet networks into logical networks.
  See :ref:`VLAN support <vlan_interface>` for more details.

* **Network traffic classification.** The sent and received network packets can
  be prioritized depending on application needs.
  See :ref:`traffic classification <traffic-class-support>` for more details.

* **Time Sensitive Networking.** Both the gPTP (generalized Precision Time Protocol)
  and PTP (Precision Time Protocol, IEEE 1588) are supported.
  See :ref:`gPTP support <gptp_interface>` and :ref:`PTP support <ptp_interface>`
  for more details.

* **SNTP** Simple Network Time Protocol (:rfc:`5905`) client and server are
  supported. See :ref:`SNTP <sntp_interface>` for more details.
  :zephyr:code-sample:`sntp-client` and :zephyr:code-sample:`sntp-server`
  samples are provided.

* **SOCKS5** proxy version 5 (:rfc:`1928`) is supported.

* **TFTP** Trivial File Transfer Protocol (:rfc:`1350`) client is supported.
  :zephyr:code-sample:`tftp-client` sample is provided.

* **FTP** File Transfer Protocol (:rfc:`959`) client is supported.
  See :ref:`FTP client <ftp_client_interface>` for more details.
  :zephyr:code-sample:`ftp-client` sample is provided.

* **SSH** SSH-2 server and client are supported, and the shell can be
  accessed over SSH. :zephyr:code-sample:`ssh-server-client` sample is provided.

* **WireGuard** VPN is supported.
  :zephyr:code-sample:`wireguard-vpn` sample is provided.

* **RTP** Real-time Transport Protocol (:rfc:`3550`) is supported.
  :zephyr:code-sample:`net-rtp` sample is provided.

* **MIDI2** MIDI 2.0 network UDP transport is supported.
  :zephyr:code-sample:`netmidi2` sample is provided.

* **OCPP** Open Charge Point Protocol is supported.
  :zephyr:code-sample:`ocpp` sample is provided.

* **Prometheus** Metric Server functionality is supported.
  :zephyr:code-sample:`prometheus` is provided.

* **MCP** Model Context Protocol server is supported, letting AI agents
  discover and invoke tools exposed by the device over HTTP.
  See :ref:`MCP server <mcp_server_interface>` for more details.
  :zephyr:code-sample:`mcp-server-hello-world` sample is provided.

* **Packet filtering and capture.** Received and sent packets can be filtered
  with user-defined rules, and network traffic can be captured and sent to a
  remote host for analysis. See :ref:`packet filtering <net_pkt_filter_interface>`
  and :ref:`network packet capture <net_capture_interface>` for more details.

* **Network shell.** The network shell provides helpers for figuring out
  network status, enabling/disabling features, and issuing commands like ping
  or DNS resolving. The net-shell is useful when developing network software.
  See :ref:`network shell <net_shell>` for more details.

* **zperf** is a network performance and bandwidth measurement tool compatible
  with iPerf v2 and iperf3. Both client and server functionality is supported.
  :zephyr:code-sample:`zperf` sample is provided.

Additionally these network technologies (link layers) are supported in Zephyr OS:

* IEEE 802.15.4
* Ethernet, IEEE 802.3
* Wi-Fi, IEEE 802.11
* Cellular / PPP (:rfc:`1661`)
* Thread (:zephyr:code-sample-category:`openthread` samples are provided)
* CAN bus for SocketCAN
* SLIP (IP over serial line). Used for testing with QEMU. It provides
  ethernet interface to host system (like Linux) and test applications
  can be run in Linux host and send network data to Zephyr OS device.

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
