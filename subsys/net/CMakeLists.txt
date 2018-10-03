zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NET_BUF             buf.c)
zephyr_library_sources_ifdef(CONFIG_NET_HOSTNAME_ENABLE hostname.c)

if(CONFIG_NETWORKING)
  add_subdirectory(l2)

  if(CONFIG_NET_RAW_MODE)
    zephyr_library_sources(ip/net_pkt.c)
  else()
    add_subdirectory(ip)
  endif()
endif()

add_subdirectory(lib)
