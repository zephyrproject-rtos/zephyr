zephyr_library()
zephyr_library_include_directories(.)
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources(
  net_context.c
  net_core.c
  net_if.c
  net_pkt.c
  net_tc.c
  utils.c
  )

zephyr_library_sources_ifdef(CONFIG_NET_6LO          6lo.c)
zephyr_library_sources_ifdef(CONFIG_NET_DHCPV4       dhcpv4.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV4_AUTO    ipv4_autoconf.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV4         icmpv4.c       ipv4.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV6         icmpv6.c nbr.c ipv6.c ipv6_nbr.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV6_MLD     ipv6_mld.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV6_FRAGMENT     ipv6_fragment.c)
zephyr_library_sources_ifdef(CONFIG_NET_MGMT_EVENT   net_mgmt.c)
zephyr_library_sources_ifdef(CONFIG_NET_ROUTE        route.c)
zephyr_library_sources_ifdef(CONFIG_NET_RPL          rpl.c)
zephyr_library_sources_ifdef(CONFIG_NET_RPL_MRHOF    rpl-mrhof.c)
zephyr_library_sources_ifdef(CONFIG_NET_RPL_OF0      rpl-of0.c)
zephyr_library_sources_ifdef(CONFIG_NET_SHELL        net_shell.c)
zephyr_library_sources_ifdef(CONFIG_NET_STATISTICS   net_stats.c)
zephyr_library_sources_ifdef(CONFIG_NET_TCP          connection.c tcp.c)
zephyr_library_sources_ifdef(CONFIG_NET_TRICKLE      trickle.c)
zephyr_library_sources_ifdef(CONFIG_NET_UDP          connection.c udp.c)
zephyr_library_sources_ifdef(CONFIG_NET_PROMISCUOUS_MODE promiscuous.c)

if(CONFIG_NET_SHELL)
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/l2)
zephyr_link_interface_ifdef(CONFIG_MBEDTLS mbedTLS)
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
endif()
