# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/net/ethernet.h
)

zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/l2)

zephyr_library()
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources_ifdef(CONFIG_NET_L2_ETHERNET      ethernet.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_ETHERNET_MGMT ethernet_mgmt.c)

if(CONFIG_NET_NATIVE)
zephyr_library_sources_ifdef(CONFIG_NET_ARP              arp.c)
zephyr_library_sources_ifdef(CONFIG_NET_VLAN             vlan.c)
zephyr_library_sources_ifdef(CONFIG_NET_STATISTICS_ETHERNET ethernet_stats.c)

if(CONFIG_NET_GPTP)
  add_subdirectory(gptp)
endif()

if(CONFIG_NET_LLDP)
  add_subdirectory(lldp)
endif()

if(CONFIG_NET_DSA)
  add_subdirectory(dsa)
endif()

if(CONFIG_NET_ETHERNET_BRIDGE)
  add_subdirectory(bridge)
endif()

endif()
