# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__ZEPHYR_SUPERVISOR__)

if(CONFIG_NET_PKT_FILTER)
  zephyr_library()
  zephyr_library_sources(base.c)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_ETHERNET ethernet.c)
  zephyr_library_sources_ifdef(CONFIG_NET_PKT_FILTER_LOCAL_IN_HOOK l4.c)
  zephyr_library_include_directories("${ZEPHYR_BASE}/subsys/net/ip")
endif()
