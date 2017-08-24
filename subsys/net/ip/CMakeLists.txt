zephyr_library()
zephyr_library_include_directories(PRIVATE .)
zephyr_library_compile_definitions_ifdef(CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__)

zephyr_library_sources(
  net_context.c
  net_core.c
  net_if.c
  net_pkt.c
  utils.c
  )

zephyr_library_sources_ifdef(CONFIG_NET_6LO         6lo.c)
zephyr_library_sources_ifdef(CONFIG_NET_DHCPV4      dhcpv4.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV4        icmpv4.c       ipv4.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV6        icmpv6.c nbr.c ipv6.c)
zephyr_library_sources_ifdef(CONFIG_NET_MGMT_EVENT  net_mgmt.c)
zephyr_library_sources_ifdef(CONFIG_NET_ROUTE       route.c)
zephyr_library_sources_ifdef(CONFIG_NET_RPL         rpl.c)
zephyr_library_sources_ifdef(CONFIG_NET_RPL_MRHOF   rpl-mrhof.c)
zephyr_library_sources_ifdef(CONFIG_NET_RPL_OF0     rpl-of0.c)
zephyr_library_sources_ifdef(CONFIG_NET_SHELL       net_shell.c)
zephyr_library_sources_ifdef(CONFIG_NET_STATISTICS  net_stats.c)
zephyr_library_sources_ifdef(CONFIG_NET_TCP         connection.c tcp.c)
zephyr_library_sources_ifdef(CONFIG_NET_TRICKLE     trickle.c)
zephyr_library_sources_ifdef(CONFIG_NET_UDP         connection.c udp.c)

zephyr_library_sources_ifdef(CONFIG_NET_ARP         l2/arp.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_BT       l2/bluetooth.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_BT_SHELL l2/bluetooth_shell.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_DUMMY    l2/dummy.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_ETHERNET l2/ethernet.c)

if(CONFIG_NET_L2_IEEE802154)
  zephyr_library_sources(l2/ieee802154/ieee802154.c)
  zephyr_library_sources(l2/ieee802154/ieee802154_frame.c)

  zephyr_library_sources_ifdef(CONFIG_NET_L2_IEEE802154_FRAGMENT       l2/ieee802154/ieee802154_fragment.c)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_IEEE802154_MGMT           l2/ieee802154/ieee802154_mgmt.c)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_IEEE802154_RADIO_ALOHA    l2/ieee802154/ieee802154_radio_aloha.c)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA  l2/ieee802154/ieee802154_radio_csma_ca.c)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_IEEE802154_SECURITY       l2/ieee802154/ieee802154_security.c)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_IEEE802154_SHELL          l2/ieee802154/ieee802154_shell.c)
endif()
