.. _net_shell:

Network Shell
#############

Network shell provides helpers for figuring out network status,
enabling/disabling features, and issuing commands like ping or DNS resolving.
Note that ``net-shell`` should probably not be used in production code
as it will require extra memory. See also :ref:`generic shell <shell_api>`
for detailed shell information.

The following net-shell commands are implemented:

.. csv-table:: net-shell commands
   :header: "Command", "Description"
   :widths: 15 85

   "net allocs", "Print network memory allocations. Only available if
   :kconfig:option:`CONFIG_NET_DEBUG_NET_PKT_ALLOC` is set."
   "net arp", "Print information about IPv4 ARP cache. Only available if
   :kconfig:option:`CONFIG_NET_ARP` is set in IPv4 enabled networks."
   "net capture", "Monitor network traffic See :ref:`network_monitoring`
   for details."
   "net conn", "Print information about network connections."
   "net dns", "Show how DNS is configured. The command can also be used to
   resolve a DNS name. Only available if :kconfig:option:`CONFIG_DNS_RESOLVER` is set."
   "net events", "Enable network event monitoring. Only available if
   :kconfig:option:`CONFIG_NET_MGMT_EVENT_MONITOR` is set."
   "net gptp", "Print information about gPTP support. Only available if
   :kconfig:option:`CONFIG_NET_GPTP` is set."
   "net iface", "Print information about network interfaces."
   "net ipv6", "Print IPv6 specific information and configuration.
   Only available if :kconfig:option:`CONFIG_NET_IPV6` is set."
   "net mem", "Print information about network memory usage. The command will
   print more information if :kconfig:option:`CONFIG_NET_BUF_POOL_USAGE` is set."
   "net nbr", "Print neighbor information. Only available if
   :kconfig:option:`CONFIG_NET_IPV6` is set."
   "net ping", "Ping a network host."
   "net route", "Show IPv6 network routes. Only available if
   :kconfig:option:`CONFIG_NET_ROUTE` is set."
   "net sockets", "Show network socket information and statistics. Only available if
   :kconfig:option:`CONFIG_NET_SOCKETS_OBJ_CORE` and :kconfig:option:`CONFIG_OBJ_CORE`
   are set."
   "net stats", "Show network statistics."
   "net tcp", "Connect/send data/close TCP connection. Only available if
   :kconfig:option:`CONFIG_NET_TCP` is set."
   "net vlan", "Show Ethernet virtual LAN information. Only available if
   :kconfig:option:`CONFIG_NET_VLAN` is set."
