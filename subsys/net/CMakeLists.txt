zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NET_BUF buf.c)

if(CONFIG_NETWORKING)
  if(CONFIG_NET_L2_RAW_CHANNEL)
    zephyr_library_sources(ip/net_pkt.c)
  else()
    add_subdirectory(ip)
  endif()
endif()

add_subdirectory(lib)
