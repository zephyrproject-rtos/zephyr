# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources_ifdef(CONFIG_NET_L2_PPP
			     ppp_l2.c
			     fsm.c
			     lcp.c
			     auth.c
			     options.c
			     link.c
			     network.c
			     misc.c)

zephyr_library_sources_ifdef(CONFIG_NET_STATISTICS_PPP ppp_stats.c)

if(CONFIG_NET_IPV4)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_PPP ipcp.c)
endif()

if(CONFIG_NET_IPV6)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_PPP ipv6cp.c)
endif()
