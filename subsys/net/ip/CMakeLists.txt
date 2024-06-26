# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/net/net_if.h
  ${ZEPHYR_BASE}/include/zephyr/net/net_ip.h
)

zephyr_library()
zephyr_library_include_directories(.)
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources(
  net_core.c
  net_if.c
  net_timeout.c
  utils.c
  )

if(CONFIG_NET_OFFLOAD)
zephyr_library_sources(net_context.c net_pkt.c net_tc.c)
endif()

zephyr_library_sources_ifdef(CONFIG_NET_MGMT_EVENT   net_mgmt.c)

if(CONFIG_NET_NATIVE)
zephyr_library_sources(net_context.c)
zephyr_library_sources(net_pkt.c)
zephyr_library_sources(net_tc.c)
zephyr_library_sources(icmp.c)
zephyr_library_sources_ifdef(CONFIG_NET_IP           connection.c)
zephyr_library_sources_ifdef(CONFIG_NET_6LO          6lo.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV4_AUTO    ipv4_autoconf.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV4         icmpv4.c ipv4.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV4_ACD     ipv4_acd.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV4_IGMP    igmp.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV6         icmpv6.c nbr.c
                                                     ipv6.c ipv6_nbr.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV6_MLD     ipv6_mld.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV6_PE      ipv6_pe.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV6_FRAGMENT     ipv6_fragment.c)
zephyr_library_sources_ifdef(CONFIG_NET_IPV4_FRAGMENT     ipv4_fragment.c)
zephyr_library_sources_ifdef(CONFIG_NET_MGMT_EVENT   net_mgmt.c)
zephyr_library_sources_ifdef(CONFIG_NET_ROUTE        route.c)
zephyr_library_sources_ifdef(CONFIG_NET_STATISTICS   net_stats.c)
zephyr_library_sources_ifdef(CONFIG_NET_TCP          tcp.c)
zephyr_library_sources_ifdef(CONFIG_NET_TEST_PROTOCOL           tp.c)
zephyr_library_sources_ifdef(CONFIG_NET_UDP          udp.c)
zephyr_library_sources_ifdef(CONFIG_NET_PROMISCUOUS_MODE promiscuous.c)

# Net Connection Socket Adapters
zephyr_library_sources_ifdef(CONFIG_NET_CONNECTION_SOCKETS  connection.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_PACKET      packet_socket.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_CAN         canbus_socket.c)

if(CONFIG_NET_TCP_ISN_RFC6528 OR CONFIG_NET_IPV6_PE)
  zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
endif()
endif()

# To get private includes like net_shell.h
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/lib)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )
