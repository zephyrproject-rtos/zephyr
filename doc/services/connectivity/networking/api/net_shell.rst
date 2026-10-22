.. _net_shell:

Network Shell
#############

Network shell, and its companion shells provide helpers for figuring out network status,
enabling/disabling features, and issuing commands like ping or DNS resolving.
Note that ``net-shell`` should probably not be used in production code
as it will require extra memory. See also :ref:`generic shell <shell_api>`
for detailed shell information.

Note that by default both the enabled or disabled net-shell commands are
available to the user. This helps user to discover what commands are available
and how to enable them. This extra help can be turned off by disabling
:kconfig:option:`CONFIG_NET_SHELL_SHOW_DISABLED_COMMANDS` option.

The following net-shell commands are implemented:

.. csv-table:: net-shell commands
   :header: "Command", "Description"
   :widths: 15 85

   "net allocs", "Print network memory allocations. Only available if
   :kconfig:option:`CONFIG_NET_DEBUG_NET_PKT_ALLOC` is set."
   "net arp", "Print information about IPv4 ARP cache. Only available if
   :kconfig:option:`CONFIG_NET_ARP` is set in IPv4 enabled networks."
   "net bridge", "Print information and manipulate Ethernet bridges. Only available if
   :kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_SHELL` is set."
   "net capture", "Monitor network traffic See :ref:`network_monitoring`
   for details."
   "net cm", "Connection manager shell. Only available if
   :kconfig:option:`CONFIG_NET_CONNECTION_MANAGER` is set."
   "net conn", "Print information about network connections."
   "net dhcpv4", "Enable / disable DHCPv4 client or server support. Only available if
   :kconfig:option:`CONFIG_NET_DHCPV4_SERVER` or :kconfig:option:`CONFIG_NET_DHCPV4`
   is set."
   "net dhcpv6", "Enable / disable DHCPv6 client support. Only available if
   :kconfig:option:`CONFIG_NET_DHCPV6` is set."
   "net dns", "Show how DNS is configured. The command can also be used to
   resolve a DNS name. Only available if :kconfig:option:`CONFIG_DNS_RESOLVER` is set."
   "net events", "Enable network event monitoring. Only available if
   :kconfig:option:`CONFIG_NET_MGMT_EVENT_MONITOR` is set."
   "net filter", "View network packet filter rules. Only available if
   :kconfig:option:`CONFIG_NET_PKT_FILTER` is set."
   "net ftp", "Connect to FTP server to transfer files. Only available if
   :kconfig:option:`CONFIG_FTP_CLIENT` is set."
   "net gptp", "Print information about gPTP support. Only available if
   :kconfig:option:`CONFIG_NET_GPTP` is set."
   "net http", "Show HTTP server information, or send HTTP GET/POST/PUT/DELETE requests.
   Only available if :kconfig:option:`CONFIG_HTTP_SERVER` or
   :kconfig:option:`CONFIG_HTTP_CLIENT` is set."
   "net iface", "Print information about network interfaces."
   "net ipv4", "Print IPv4 specific information and configuration.
   Only available if :kconfig:option:`CONFIG_NET_IPV4` is set."
   "net ipv6", "Print IPv6 specific information and configuration.
   Only available if :kconfig:option:`CONFIG_NET_IPV6` is set."
   "net mem", "Print information about network memory usage. The command will
   print more information if :kconfig:option:`CONFIG_NET_BUF_POOL_USAGE` is set."
   "net nbr", "Print neighbor information. Only available if
   :kconfig:option:`CONFIG_NET_IPV6` is set."
   "net ping", "Ping a network host."
   "net pkt", "Print low level network packet information for debugging purposes. "
   "net pmtu", "Print MTU path discovery information. Only available if
   :kconfig:option:`CONFIG_NET_IPV6_PMTU` or :kconfig:option:`CONFIG_NET_IPV4_PMTU` is set."
   "net ppp", "Print Point-to-Point protocol information. Only available if
   :kconfig:option:`CONFIG_NET_L2_PPP` and :kconfig:option:`CONFIG_NET_PPP` are set."
   "net ptp", "Print information about PTP support. Use ``net ptp <port>`` for detailed
   per-port view. Only available if :kconfig:option:`CONFIG_PTP` is set."
   "net qbv", "Show and configure IEEE 802.1Qbv Time-Aware Shaper (TAS) information.
   Only available if :kconfig:option:`CONFIG_NET_QBV` is set."
   "net quic", "Show and configure QUIC transport.
   Only available if :kconfig:option:`CONFIG_QUIC` is set."
   "net resume", "Resume a network interface if network power management is enabled."
   "net route", "Show IPv6 or IPv4 network routes. Only available if
   :kconfig:option:`CONFIG_NET_IPV6_ROUTING` or :kconfig:option:`CONFIG_NET_IPV4_ROUTING` is set."
   "net sockets", "Show network socket information and statistics. Only available if
   :kconfig:option:`CONFIG_NET_SOCKETS_OBJ_CORE` and :kconfig:option:`CONFIG_OBJ_CORE`
   are set."
   "net ssh", "SSH client support. Only available if
   :kconfig:option:`CONFIG_SSH_CLIENT` is set."
   "net sshd", "SSH server support. Only available if
   :kconfig:option:`CONFIG_SSH_SERVER` is set."
   "net ssh_key", "SSH key generation/removing/saving/loading support. Only available if
   :kconfig:option:`CONFIG_SSH_CLIENT` or :kconfig:option:`CONFIG_SSH_SERVER` is set."
   "net stats", "Show network statistics."
   "net suspend", "Suspend a network interface if network power management is enabled."
   "net tcp", "Connect/send data/close TCP connection. Only available if
   :kconfig:option:`CONFIG_NET_TCP` is set."
   "net udp", "Send UDP data directly from shell. Only available if
   :kconfig:option:`CONFIG_NET_UDP` is set."
   "net virtual", "Show and manipulate network virtual interfaces. Only available if
   :kconfig:option:`CONFIG_NET_L2_VIRTUAL` is set."
   "net vlan", "Show Ethernet virtual LAN information. Only available if
   :kconfig:option:`CONFIG_NET_VLAN` is set."
   "net websocket", "Print websocket information. Only available if
   :kconfig:option:`CONFIG_WEBSOCKET_CLIENT` is set."
   "net wg", "Show WireGuard VPN information and setup VPNs. Only available if
   :kconfig:option:`CONFIG_WIREGUARD` is set."

The Wi-Fi shell provides commands to scan, connect, disconnect and configure
Wi-Fi networks. Following Wi-Fi shell commands are implemented:

.. csv-table:: wifi-shell commands
   :header: "Command", "Description"
   :widths: 15 85

   "wifi <cmd>", "Multiple commands for Wi-Fi network connection, disconnection,
   scanning and configuration. Only available if
   :kconfig:option:`CONFIG_NET_L2_WIFI_SHELL` is set."
   "wifi cred", "Show/add/delete Wi-Fi network credentials. Only available if
   :kconfig:option:`CONFIG_WIFI_CREDENTIALS_SHELL` is set.
   See :ref:`lib_wifi_credentials` for details."

The TLS credentials shell provides commands to list, add, delete and retrieve
TLS credential information either from a volatile or protected backend storage.
Following TLS credentials shell commands are implemented. These commands are
only available if :kconfig:option:`CONFIG_TLS_CREDENTIALS_SHELL` is set.
See :ref:`tls_credentials_shell` for details.

.. csv-table:: tls-credentials-shell commands
   :header: "Command", "Description"
   :widths: 15 85

   "cred buf", "Buffer in credential data so it can be added."
   "cred add", "Add a TLS credential."
   "cred del", "Delete a TLS credential."
   "cred get", "Retrieve the contents of a TLS credential."
   "cred list", "List stored TLS credentials."
