# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NET_BUF             buf.c)
zephyr_library_sources_ifdef(CONFIG_NET_HOSTNAME_ENABLE hostname.c)

if(CONFIG_NET_CODE_RELOCATE)
  zephyr_library_relocate("${CONFIG_NET_CODE_RAM_NAME}_TEXT")
endif()

if(CONFIG_NETWORKING)
  add_subdirectory(l2)
  add_subdirectory(pkt_filter)

  if(CONFIG_NET_RAW_MODE)
    zephyr_library_sources(ip/net_pkt.c)
  else()
    add_subdirectory(ip)
  endif()
endif()

add_subdirectory(lib)
